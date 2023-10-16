#include "attention.h"

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <torch/torch.h>

DEFINE_string(varlen_masked_self_attention,
              "",
              "type of attention to use for varlen_masked_self_attention, "
              "slow, cuda, or empty for auto");

DEFINE_string(
    single_query_masked_self_attention,
    "",
    "type of attention to use for single_query_masked_self_attention, slow, "
    "cuda, or empty for auto");

// ref to flash_attn in third_party/flash_attn
extern std::vector<at::Tensor> mha_varlen_fwd(
    const at::Tensor& q,
    const at::Tensor& k,
    const at::Tensor& v,
    c10::optional<at::Tensor>& out_,
    const at::Tensor& cu_seqlens_q,
    const at::Tensor& cu_seqlens_k,
    const at::Tensor& alibi_slopes,
    int max_seqlen_q,
    int max_seqlen_k,
    float p_dropout,
    float softmax_scale,
    bool zero_tensors,
    bool is_causal,
    int window_size_left,
    int window_size_right,
    bool return_softmax,
    c10::optional<at::Generator> gen_);

// ref to single_query_cached_kv_attention in third_party/vllm
extern void single_query_cached_kv_attention(
    torch::Tensor& out,
    torch::Tensor& query,
    torch::Tensor& key_cache,
    torch::Tensor& value_cache,
    torch::Tensor& head_mapping,
    float scale,
    torch::Tensor& block_tables,
    torch::Tensor& context_lens,
    int block_size,
    int max_context_len,
    const c10::optional<torch::Tensor>& alibi_slopes);

namespace llm {
AttentionImpl::AttentionImpl(int64_t n_heads,
                             int64_t n_kv_heads,
                             int64_t head_dim,
                             float scale,
                             torch::ScalarType /*dtype*/,
                             const torch::Device& device)
    : n_heads_(n_heads),
      n_kv_heads_(n_kv_heads),
      head_dim_(head_dim),
      scale_(scale) {
  CHECK(n_heads % n_kv_heads == 0)
      << "n_heads " << n_heads << " not divisible by n_kv_heads " << n_kv_heads;

  // prepare kv_head_mapping
  auto kv_head_mapping = torch::arange(
      0, n_kv_heads, torch::TensorOptions().dtype(torch::kInt).device(device));
  const auto num_group = n_heads / n_kv_heads;
  if (num_group > 1) {
    kv_head_mapping = kv_head_mapping.repeat_interleave(/*repeats=*/num_group);
  }
  kv_head_mapping_ = register_buffer("kv_head_mapping", kv_head_mapping);
}

torch::Tensor AttentionImpl::forward(const torch::Tensor& query,
                                     const torch::Tensor& key,
                                     const torch::Tensor& value,
                                     KVCache& kv_cache,
                                     const InputParameters& input_params) {
  const int64_t num_tokens = query.size(0);
  // (num_tokens, n_heads, head_dim)
  auto q = query.view({num_tokens, n_heads_, head_dim_});
  auto k = key.view({num_tokens, n_kv_heads_, head_dim_});
  auto v = value.view({num_tokens, n_kv_heads_, head_dim_});

  // store k/v into cache based on slots
  kv_cache.set_kv_cache(input_params.slot_ids, k, v);

  auto output = torch::empty_like(q);
  const auto num_prompt_tokens = input_params.num_prompt_tokens;
  if (num_prompt_tokens > 0) {
    // process sequences with prompt tokens (prefill)
    auto sliced_output =
        output.slice(/*dim=*/0, /*start=*/0, /*end=*/num_prompt_tokens);
    auto sliced_query =
        q.slice(/*dim=*/0, /*start=*/0, /*end=*/num_prompt_tokens);
    auto sliced_key =
        k.slice(/*dim=*/0, /*start=*/0, /*end=*/num_prompt_tokens);
    auto sliced_value =
        v.slice(/*dim=*/0, /*start=*/0, /*end=*/num_prompt_tokens);
    torch::Tensor alibi_slopes;
    detail::varlen_masked_self_attention(sliced_query,
                                         sliced_key,
                                         sliced_value,
                                         alibi_slopes,
                                         input_params.cu_seq_lens,
                                         input_params.max_seq_len,
                                         scale_,
                                         sliced_output);
  }

  if (num_prompt_tokens < num_tokens) {
    // process sequences without prompt tokens (decode)
    auto sliced_output = output.slice(/*dim=*/0, /*start=*/num_prompt_tokens);
    auto sliced_query = q.slice(/*dim=*/0, /*start=*/num_prompt_tokens);
    detail::single_query_masked_self_attention(kv_cache,
                                               kv_head_mapping_,
                                               sliced_query,
                                               input_params.block_tables,
                                               input_params.context_lens,
                                               input_params.max_context_len,
                                               scale_,
                                               sliced_output);
  }
  return output.view({num_tokens, -1});
}

namespace detail {
using torch::indexing::Slice;
constexpr float negative_infinity = -std::numeric_limits<float>::infinity();

torch::Tensor masked_self_attention(
    const torch::Tensor& query,  // [num_tokens/seq_len, n_heads, head_dim]
    const torch::Tensor& key,    // [num_tokens/seq_len, n_kv_heads, head_dim]
    const torch::Tensor& value,  // [num_tokens/seq_len, n_kv_heads, head_dim]
    const torch::Tensor& mask,   // [1, seq_len, seq_len]
    float scale) {
  auto scores = torch::einsum("qhd,khd->hqk", {query * scale, key});
  scores = scores.to(torch::kFloat);
  if (mask.defined()) {
    scores += mask;
  }
  // (n_heads, seq_len, seq_len)
  scores = torch::softmax(scores, /*dim=*/-1).type_as(query);
  return torch::einsum("hqk,khd->qhd", {scores, value});
}

void varlen_masked_self_attention(
    const torch::Tensor& query,         // [num_tokens, n_heads, head_dim]
    const torch::Tensor& key,           // [num_tokens, n_kv_heads, head_dim]
    const torch::Tensor& value,         // [num_tokens, n_kv_heads, head_dim]
    const torch::Tensor& alibi_slopes,  // [n_heads]
    const torch::Tensor& cu_seq_lens,   // [num_seq + 1]
    int32_t max_seq_len,                // maximum sequence length
    float scale,                        // scale for softmax
    const torch::Tensor& output) {
  if (query.is_cuda()) {
    // use cuda kernel
    if (FLAGS_varlen_masked_self_attention.empty() ||
        FLAGS_varlen_masked_self_attention == "cuda") {
      return varlen_masked_self_attention_cuda(query,
                                               key,
                                               value,
                                               alibi_slopes,
                                               cu_seq_lens,
                                               max_seq_len,
                                               scale,
                                               output);
    }
  }
  return varlen_masked_self_attention_generic(
      query, key, value, alibi_slopes, cu_seq_lens, max_seq_len, scale, output);
}

void single_query_masked_self_attention(
    const KVCache& kv_cache,  // where to get key and value
    const torch::Tensor& kv_head_mapping,
    const torch::Tensor& query,  // [num_tokens/num_seq, n_heads, head_dim]
    const torch::Tensor& block_tables,  // [num_tokens, num_blocks]
    const torch::Tensor&
        context_lens,         // [num_tokens] the length of each sequence
    int32_t max_context_len,  // maximum context length
    float scale,              // scale for softmax
    const torch::Tensor& output) {
  if (query.is_cuda()) {
    // use cuda kernel
    if (FLAGS_single_query_masked_self_attention.empty() ||
        FLAGS_single_query_masked_self_attention == "cuda") {
      return single_query_masked_self_attention_cuda(kv_cache,
                                                     kv_head_mapping,
                                                     query,
                                                     block_tables,
                                                     context_lens,
                                                     max_context_len,
                                                     scale,
                                                     output);
    }
  }
  return single_query_masked_self_attention_generic(kv_cache,
                                                    query,
                                                    block_tables,
                                                    context_lens,
                                                    max_context_len,
                                                    scale,
                                                    output);
}

void varlen_masked_self_attention_generic(
    const torch::Tensor& query,         // [num_tokens, n_heads, head_dim]
    const torch::Tensor& key,           // [num_tokens, n_kv_heads, head_dim]
    const torch::Tensor& value,         // [num_tokens, n_kv_heads, head_dim]
    const torch::Tensor& alibi_slopes,  // [n_heads]
    const torch::Tensor& cu_seq_lens,   // [num_seq + 1]
    int32_t /*max_seq_len*/,            // maximum sequence length
    float scale,                        // scale for softmax
    torch::Tensor output) {
  DCHECK(query.size(0) == key.size(0));
  DCHECK(query.size(0) == value.size(0));

  const auto head_dim = query.size(-1);
  torch::Tensor cu_seq_lens_cpu = cu_seq_lens.cpu();
  const size_t num_seqs = cu_seq_lens_cpu.numel() - 1;
  const int32_t* cu_lens = cu_seq_lens_cpu.data_ptr<int32_t>();

  // repeat keys/values if num_heads != num_kv_heads
  torch::Tensor _key = key;
  torch::Tensor _value = value;
  const auto n_heads = query.size(1);
  const auto n_kv_heads = key.size(1);
  if (n_heads != n_kv_heads) {
    CHECK(n_heads % n_kv_heads == 0);
    const auto num_goups = n_heads / n_kv_heads;
    _key = _key.repeat_interleave(/*repeats=*/num_goups, /*dim=*/1);
    _value = _value.repeat_interleave(/*repeats=*/num_goups, /*dim=*/1);
  }

  for (int64_t i = 0; i < num_seqs; ++i) {
    // calaculate attention for each sequence
    const int32_t start_idx = cu_lens[i];
    const int32_t end_idx = cu_lens[i + 1];
    const int32_t seq_len = end_idx - start_idx;

    // create attention mask based on sequence length
    torch::Tensor mask;
    if (seq_len > 1) {
      mask = torch::full({1, seq_len, seq_len}, negative_infinity);
      mask = torch::triu(mask, /*diagonal=*/1).type_as(query);

      if (alibi_slopes.defined()) {
        CHECK(alibi_slopes.size(0) == n_heads);

        // calculate alibi attention mask
        auto bias = torch::arange(0, seq_len, query.options());
        bias = bias.unsqueeze(/*dim=*/0) - bias.unsqueeze(/*dim=*/1);
        bias = bias.expand({n_heads, seq_len, seq_len});
        bias = bias * alibi_slopes.view({n_heads, 1, 1});

        mask = mask + bias;
      }
    }

    const auto attn = masked_self_attention(
        query.slice(/*dim=*/0, /*start=*/start_idx, /*end=*/end_idx),
        _key.slice(/*dim=*/0, /*start=*/start_idx, /*end=*/end_idx),
        _value.slice(/*dim=*/0, /*start=*/start_idx, /*end=*/end_idx),
        mask,
        scale);
    output.index_put_({Slice(start_idx, end_idx), Slice(), Slice()}, attn);
  }
}

void varlen_masked_self_attention_cuda(
    const torch::Tensor& query,         // [num_tokens, n_heads, head_dim]
    const torch::Tensor& key,           // [num_tokens, n_kv_heads, head_dim]
    const torch::Tensor& value,         // [num_tokens, n_kv_heads, head_dim]
    const torch::Tensor& alibi_slopes,  // [n_heads]
    const torch::Tensor& cu_seq_lens,   // [num_seq + 1]
    int32_t max_seq_len,                // maximum sequence length
    float scale,                        // scale for softmax
    torch::Tensor output) {
  const auto head_dim = query.size(-1);

  torch::optional<at::Tensor> out = output;
  mha_varlen_fwd(query,
                 key,
                 value,
                 out,
                 cu_seq_lens,
                 cu_seq_lens,
                 alibi_slopes,
                 max_seq_len,
                 max_seq_len,
                 /*p_dropout=*/0.0f,
                 /*softmax_scale=*/scale,
                 /*zero_tensors=*/false,
                 /*is_causal=*/true,
                 /*window_size_left=*/-1,
                 /*window_size_right=*/0,
                 /*return_softmax=*/false,
                 /*gen_=*/torch::nullopt);
}

void single_query_masked_self_attention_generic(
    const KVCache& kv_cache,     // where to get key and value
    const torch::Tensor& query,  // [num_tokens/num_seq, n_heads, head_dim]
    const torch::Tensor& block_tables,  // [num_tokens, num_blocks]
    const torch::Tensor&
        context_lens,             // [num_tokens] the length of each sequence
    int32_t /*max_context_len*/,  // maximum context length
    float scale,                  // scale for softmax
    torch::Tensor output) {
  const auto num_seq = query.size(0);
  // process each sequence
  // don't need attention mask for single token
  torch::Tensor mask;
  for (int64_t i = 0; i < num_seq; ++i) {
    // [1, n_heads, head_dim]
    const auto q = query[i].unsqueeze(0);
    const auto block_table = block_tables[i];
    const auto context_len = context_lens[i].item<int>();
    // fetch keys/values from cache
    auto [k, v] = kv_cache.get_kv_cache(block_table, context_len);

    // repeat keys/values if num_heads != num_kv_heads
    const auto num_heads = query.size(1);
    const auto num_kv_heads = k.size(1);
    if (num_heads != num_kv_heads) {
      CHECK(num_heads % num_kv_heads == 0);
      const auto num_goups = num_heads / num_kv_heads;
      k = k.repeat_interleave(/*repeats=*/num_goups, /*dim=*/1);
      v = v.repeat_interleave(/*repeats=*/num_goups, /*dim=*/1);
    }

    const auto attn = masked_self_attention(q, k, v, mask, scale);
    output.index_put_({i, Slice(), Slice()}, attn);
  }
}

void single_query_masked_self_attention_cuda(
    const KVCache& kv_cache,  // where to get key and value
    const torch::Tensor& kv_head_mapping,
    const torch::Tensor& query,  // [num_tokens/num_seq, n_heads, head_dim]
    const torch::Tensor& block_tables,  // [num_tokens, num_blocks]
    const torch::Tensor&
        context_lens,         // [num_tokens] the length of each sequence
    int32_t max_context_len,  // maximum context length
    float scale,              // scale for softmax
    torch::Tensor output) {
  auto [key_cache, value_cache] = kv_cache.get_kv_cache();
  const auto block_size = key_cache.size(3);

  // make a 'copy' of variable since the api is using non-const reference
  torch::Tensor _query = query;
  torch::Tensor _block_tables = block_tables;
  torch::Tensor _context_lens = context_lens;
  torch::Tensor _kv_head_mapping = kv_head_mapping;
  single_query_cached_kv_attention(output,
                                   _query,
                                   key_cache,
                                   value_cache,
                                   _kv_head_mapping,
                                   scale,
                                   _block_tables,
                                   _context_lens,
                                   block_size,
                                   max_context_len,
                                   /*alibi_slopes=*/torch::nullopt);
}

}  // namespace detail

}  // namespace llm
