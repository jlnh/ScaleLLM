#pragma once

#include <string>
#include <vector>

namespace llm {

// "stop" - the model hit a natural stop point or a provided stop sequence.
// "length" - the maximum number of tokens specified in the request was reached.
// "function_call" - the model called a function.
enum class FinishReason {
  NONE = 0,
  STOP = 1,
  LENGTH,
  FUNCTION_CALL,
};

struct Statistics {
  // the number of tokens in the prompt.
  size_t num_prompt_tokens = 0;

  // the number of tokens in the generated completion.
  size_t num_generated_tokens = 0;

  // the total number of tokens used in the request (prompt + completion).
  size_t num_total_tokens = 0;
};

struct SequenceOutput {
  // the index of the sequence in the request.
  size_t index;

  // the generated/delta text.
  // delta text is the text generated since the last response for streaming.
  std::string text;

  // the reason the sequence finished.
  FinishReason finish_reason;
};

struct RequestOutput {
  // the output for each sequence in the request.
  std::vector<SequenceOutput> outputs;

  // the statistics for the request.
  Statistics stats;

  // whether the request is finished.
  bool finished = false;
};

}  // namespace llm