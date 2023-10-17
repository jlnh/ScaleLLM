#pragma once
#include <glog/logging.h>

#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "models/args.h"
#include "models/causal_lm.h"

namespace llm {

using CausalLMFactory =
    std::function<std::unique_ptr<CausalLM>(const ModelArgs& args,
                                            const QuantizationArgs& quant_args,
                                            const ParallelArgs& parallel_args,
                                            torch::ScalarType dtype,
                                            const torch::Device& device)>;
using ModelArgsLoader =
    std::function<bool(const nlohmann::json& data, ModelArgs* args)>;

using QuantizationArgsLoader =
    std::function<bool(const nlohmann::json& data, QuantizationArgs* args)>;

// TODO: add default args loader.
struct ModelMeta {
  CausalLMFactory causal_lm_factory;
  ModelArgsLoader model_args_loader;
  QuantizationArgsLoader quant_args_loader;
};

// Model registry is a singleton class that registers all models with the
// ModelFactory, ModelArgParser to facilitate model loading.
class ModelRegistry {
 public:
  static ModelRegistry* get();

  void register_causallm_factory(const std::string& name,
                                 CausalLMFactory factory) {
    CHECK(model_registry_[name].causal_lm_factory == nullptr)
        << "causal lm factor for " << name << " already registered";
    model_registry_[name].causal_lm_factory = factory;
  }

  void register_model_args_loader(const std::string& name,
                                  ModelArgsLoader loader) {
    CHECK(model_registry_[name].model_args_loader == nullptr)
        << "model args loader for " << name << " already registered";
    model_registry_[name].model_args_loader = loader;
  }

  void register_quant_args_loader(const std::string& name,
                                  QuantizationArgsLoader loader) {
    CHECK(model_registry_[name].quant_args_loader == nullptr)
        << "quant args loader for " << name << " already registered";
    model_registry_[name].quant_args_loader = loader;
  }

  CausalLMFactory get_causallm_factory(const std::string& name) {
    return model_registry_[name].causal_lm_factory;
  }

  ModelArgsLoader get_model_args_loader(const std::string& name) {
    return model_registry_[name].model_args_loader;
  }

  QuantizationArgsLoader get_quant_args_loader(const std::string& name) {
    return model_registry_[name].quant_args_loader;
  }

 private:
  std::map<std::string, ModelMeta> model_registry_;
};

// Macro to register a model with the ModelRegistry
#define REGISTER_CAUSAL_MODEL(ModelType, ModelClass)                        \
  const bool ModelType##_registered = []() {                                \
    ModelRegistry::get()->register_causallm_factory(                        \
        #ModelType,                                                         \
        [](const ModelArgs& args,                                           \
           const QuantizationArgs& quant_args,                              \
           const ParallelArgs& parallel_args,                               \
           torch::ScalarType dtype,                                         \
           const torch::Device& device) {                                   \
          ModelClass model(args, quant_args, parallel_args, dtype, device); \
          model->eval();                                                    \
          return std::make_unique<llm::CausalLMImpl<ModelClass>>(           \
              std::move(model));                                            \
        });                                                                 \
    return true;                                                            \
  }();

// Macro to register a model args loader with the ModelRegistry
#define REGISTER_MODEL_ARGS_LOADER(ModelType, Loader)                     \
  const bool ModelType##_args_loader_registered = []() {                  \
    ModelRegistry::get()->register_model_args_loader(#ModelType, Loader); \
    return true;                                                          \
  }();

// Macro to register a quantization args loader with the ModelRegistry
#define REGISTER_QUANT_ARGS_LOADER(ModelType, Loader)                     \
  const bool ModelType##_quant_args_loader_registered = []() {            \
    ModelRegistry::get()->register_quant_args_loader(#ModelType, Loader); \
    return true;                                                          \
  }();

#define REGISTER_MODEL_ARGS(ModelType, ...)                                    \
  REGISTER_MODEL_ARGS_LOADER(ModelType,                                        \
                             [](const nlohmann::json& data, ModelArgs* args) { \
                               __VA_ARGS__();                                  \
                               return true;                                    \
                             });

#define LOAD_ARG_OR(arg_name, json_name, default_value)         \
  if (data.contains(json_name) && !data[json_name].is_null()) { \
    auto value = args->arg_name();                              \
    args->arg_name() = data[json_name].get<decltype(value)>();  \
  } else {                                                      \
    args->arg_name() = default_value;                           \
  }

#define LOAD_OPTIONAL_ARG(arg_name, json_name)                             \
  if (data.contains(json_name) && !data[json_name].is_null()) {            \
    auto value = args->arg_name();                                         \
    args->arg_name() = data[json_name].get<decltype(value)::value_type>(); \
  }

#define LOAD_ARG_WITH_FUNC(arg_name, json_name, ...)            \
  if (data.contains(json_name) && !data[json_name].is_null()) { \
    auto value = args->arg_name();                              \
    args->arg_name() = data[json_name].get<decltype(value)>();  \
  } else {                                                      \
    args->arg_name() = __VA_ARGS__();                           \
  }

}  // namespace llm