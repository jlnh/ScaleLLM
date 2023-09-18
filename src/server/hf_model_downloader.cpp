#include "hf_model_downloader.h"

#include <glog/logging.h>
#include <pybind11/embed.h>

#include <string>

namespace llm {
std::string download_hf_model(const std::string& model_name) {
  namespace py = pybind11;
  py::scoped_interpreter guard{};  // Start the interpreter
  py::module_ hub;
  try {
    hub = py::module_::import("huggingface_hub");
  } catch (const std::exception& e) {
    LOG(ERROR) << "Please install huggingface_hub by running: "
               << "pip3 install huggingface_hub";
    throw;
  }

  return hub.attr("snapshot_download")(model_name).cast<std::string>();
}

}  // namespace llm
