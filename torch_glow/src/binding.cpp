/**
 * Copyright (c) Glow Contributors. See CONTRIBUTORS file.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <pybind11/pybind11.h>

#include "FuseKnownPatterns.h"
#include "GlowCompileSpec.h"
#include "GlowFuser.h"
#include "PyTorchCommon.h"
#include "Registration.h"
#include "TorchGlowBackend.h"
#include "glow/Flags/Flags.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <torch/csrc/jit/python/pybind_utils.h>

#include "glow/Graph/Graph.h"

namespace py = pybind11;

using namespace glow;

/// The torch_glow pybind11 module.
#ifdef TORCH_GLOW_MODULE_NAME
PYBIND11_MODULE(TORCH_GLOW_MODULE_NAME, m) {
#else
PYBIND11_MODULE(_torch_glow, m) {
#endif
  /// Register Glow op and FusionPass, enable the fusion pass if
  /// fusionPassEnabled is set in PyTorchLoaderSettings.
  registerGlowFusionOpAndPass([]() {
    return getGlobalPyTorchLoaderSettingsMutable().fusionPassEnabled;
  });

  /// 1) Registers TorchGlowBackend as a PyTorch backend.
  /// 2) Registers custom classes used by TorchGlowBackend.
  /// 3) Registers JIT IR ops that are used in preprocessing.
  registerTorchGlowBackendAndDeps();

  /// Enable overriding signal handlers for torch_glow to make interruping long
  /// running processes possible. This should only be used when running
  /// torch_glow with Python.
  enableSignalHandlerOverrides();

  /// Enable compiling PyTorch subgraphs to Glow Functions.
  m.def("enableFusionPass", []() {
    getGlobalPyTorchLoaderSettingsMutable().fusionPassEnabled = true;
  });

  /// Disable compiling PyTorch subgraphs to Glow Functions.
  m.def("disableFusionPass", []() {
    getGlobalPyTorchLoaderSettingsMutable().fusionPassEnabled = false;
  });

  /// Get status of converting PyTorch subgraphs to Glow Functions.
  m.def("getFusionPassEnabled", []() {
    return getGlobalPyTorchLoaderSettingsMutable().fusionPassEnabled;
  });

  /// Enable dumping Glow DAG to file after model loading finishes.
  m.def("enableDumpGlowDag",
        []() { getGlobalPyTorchLoaderSettingsMutable().dumpGlowDag = true; });

  /// Disable dumping Glow DAG to file after model loading finishes.
  m.def("disableDumpGlowDag",
        []() { getGlobalPyTorchLoaderSettingsMutable().dumpGlowDag = false; });

  /// Enable printing index of all jit node for debugging
  m.def("enable_printing_jit_node_indices",
        []() { getGlobalPyTorchLoaderSettingsMutable().printJITIndex = true; });

  /// Disable printing index of all jit node for debugging
  m.def("disable_printing_jit_node_indices", []() {
    getGlobalPyTorchLoaderSettingsMutable().printJITIndex = false;
  });

  /// Enable ignoring rounding arg in aten::div
  /// TODO: Handle this case with FloorDiv
  m.def("enable_ignore_div_rounding_args", []() {
    getGlobalPyTorchLoaderSettingsMutable().ignoreDivRoundingArgs = true;
  });

  /// Disable ignoring rounding arg in aten::div
  m.def("disable_ignore_div_rounding_args", []() {
    getGlobalPyTorchLoaderSettingsMutable().ignoreDivRoundingArgs = false;
  });

  /// Enable converting fp32 ops to fp16.
  m.def("enable_convert_to_fp16",
        []() { getGlobalPyTorchLoaderSettingsMutable().convertToFP16 = true; });

  /// Disable converting fp32 ops to fp16.
  m.def("disable_convert_to_fp16", []() {
    getGlobalPyTorchLoaderSettingsMutable().convertToFP16 = false;
  });

  /// Get status of converting fp32 ops to fp16.
  m.def("get_convert_to_fp16",
        []() { return getGlobalPyTorchLoaderSettingsMutable().convertToFP16; });

  /// Enable clipping of fp16.
  m.def("enable_clip_fp16",
        []() { getGlobalPyTorchLoaderSettingsMutable().clipFP16 = true; });

  /// Disable clipping of fp16.
  m.def("disable_clip_fp16",
        []() { getGlobalPyTorchLoaderSettingsMutable().clipFP16 = false; });

  /// Get status of clipping fp16.
  m.def("get_clip_fp16",
        []() { return getGlobalPyTorchLoaderSettingsMutable().clipFP16; });

  /// Enable converting fp32 fused ops to fp16.
  m.def("enable_convert_fused_to_fp16", []() {
    getGlobalPyTorchLoaderSettingsMutable().convertFusedToFP16 = true;
  });

  /// Disable converting fp32 fused ops to fp16.
  m.def("disable_convert_fused_to_fp16", []() {
    getGlobalPyTorchLoaderSettingsMutable().convertFusedToFP16 = false;
  });

  /// Get status of converting fp32 fused ops to fp16.
  m.def("get_convert_fused_to_fp16", []() {
    return getGlobalPyTorchLoaderSettingsMutable().convertFusedToFP16;
  });

  /// Enable dumping the final Glow dag after compilation.
  m.def("enable_dump_final_glow_graph", []() {
    getGlobalPyTorchLoaderSettingsMutable().dumpFinalGlowGraph = true;
  });

  /// Disable dumping the final Glow dag after compilation.
  m.def("disable_dump_final_glow_graph", []() {
    getGlobalPyTorchLoaderSettingsMutable().dumpFinalGlowGraph = false;
  });

  /// Enable tracing in Glow runtime.
  m.def("enable_glow_tracing", []() {
    getGlobalPyTorchLoaderSettingsMutable().enableGlowTracing = true;
  });

  // Enable the auto removal of mutation in JIT graph, i.e, inline ops.
  m.def("enable_remove_mutation", []() {
    getGlobalPyTorchLoaderSettingsMutable().enableRemoveMutation = true;
  });

  // Disable the auto removal of mutation in JIT graph
  m.def("disable_remove_mutation", []() {
    getGlobalPyTorchLoaderSettingsMutable().enableRemoveMutation = false;
  });

  /// Set the number of traces to dump per trace file.
  m.def("set_num_traces_per_dump", [](size_t numTracesPerDump) {
    getGlobalPyTorchLoaderSettingsMutable().numTracesPerDump = numTracesPerDump;
  });

  /// Set the number of replications on each device.
  m.def("set_replication_count", [](size_t replicationCount) {
    getGlobalPyTorchLoaderSettingsMutable().replicationCount = replicationCount;
  });

  /// Disable tracing in Glow runtime.
  m.def("disable_glow_tracing", []() {
    getGlobalPyTorchLoaderSettingsMutable().enableGlowTracing = false;
  });

  /// Enable write Glow graph to onnx after model loading finishes.
  m.def("enable_write_to_onnx",
        []() { getGlobalPyTorchLoaderSettingsMutable().writeToOnnx = true; });

  /// Disable write Glow graph to onnx after model loading finishes.
  m.def("disable_write_to_onnx",
        []() { getGlobalPyTorchLoaderSettingsMutable().writeToOnnx = false; });

  /// Enable zip mode when writing ONNX model to file
  m.def("enable_onnx_zip_mode",
        []() { getGlobalPyTorchLoaderSettingsMutable().onnxZipMode = true; });

  /// Disable zip mode when writing ONNX model to file
  m.def("disable_onnx_zip_mode",
        []() { getGlobalPyTorchLoaderSettingsMutable().onnxZipMode = false; });

  /// Set a specific filename for writing ONNX to
  m.def("set_onnx_file_name_prefix", [](const std::string &prefix) {
    getGlobalPyTorchLoaderSettingsMutable().onnxFileNamePrefix = prefix;
  });

  /// Enable randomizing Constants in loaded Functions.
  m.def("enable_randomize_constants", []() {
    getGlobalPyTorchLoaderSettingsMutable().randomizeConstants = true;
  });

  /// Disable randomizing Constants in loaded Functions.
  m.def("disable_randomize_constants", []() {
    getGlobalPyTorchLoaderSettingsMutable().randomizeConstants = false;
  });

  /// Enable writing to Onnx without randomizing constants.
  m.def("enable_write_without_randomize", []() {
    getGlobalPyTorchLoaderSettingsMutable().writeWithoutRandomize = true;
  });

  /// Disable writing to Onnx without randomizing constants.
  m.def("disable_write_without_randomize", []() {
    getGlobalPyTorchLoaderSettingsMutable().writeWithoutRandomize = false;
  });

  /// Enable check Glow vs jit correctness.
  m.def("enable_jit_vs_glow_compare", []() {
    getGlobalPyTorchLoaderSettingsMutable().jitVsGlowCompare = true;
  });

  /// Disable check Glow vs jit correctness.
  m.def("disable_jit_vs_glow_compare", []() {
    getGlobalPyTorchLoaderSettingsMutable().jitVsGlowCompare = false;
  });

  /// Enable saturateHost mode in Glow runtime.
  m.def("enable_saturate_host",
        []() { getGlobalPyTorchLoaderSettingsMutable().saturateHost = true; });

  /// Disable saturateHost mode in Glow runtime.
  m.def("disable_saturate_host",
        []() { getGlobalPyTorchLoaderSettingsMutable().saturateHost = false; });

  /// Enable shape inference engine.
  m.def("enable_shape_inference_engine", []() {
    getGlobalPyTorchLoaderSettingsMutable().runShapeInference = true;
  });

  /// Disable shape inference engine.
  m.def("disable_shape_inference_engine", []() {
    getGlobalPyTorchLoaderSettingsMutable().runShapeInference = false;
  });

  /// Defer compilation to runtime.
  m.def("enable_lazy_compile",
        []() { getGlobalPyTorchLoaderSettingsMutable().lazyCompile = true; });

  /// Set interpreter device memory (in KiB).
  m.def("set_interpreter_memory", [](const unsigned &memorySize) {
    glow::runtime::flags::InterpreterMemory = memorySize;
  });

  /// Enable NNPI private transforms
  m.def("enable_nnpi_private_transforms",
        []() { glow::nnpi::flags::EnablePrivateTransforms = true; });

  /// Add all of the symbols in \p blacklist to the fusion blacklist so that
  /// nodes with these symbols will not be fused to Glow.
  m.def("setFusionBlacklist", [](const std::vector<std::string> &blacklist) {
    auto &bl = getGlobalPyTorchLoaderSettingsMutable().opBlacklist;
    bl.clear();
    for (const auto &kind : blacklist) {
      bl.insert(torch::jit::Symbol::fromQualString(kind));
    }
  });

  /// Get the fusion blacklist.
  m.def("getFusionBlacklist", []() {
    auto &symbols = getGlobalPyTorchLoaderSettingsMutable().opBlacklist;
    std::vector<std::string> strings;
    std::transform(symbols.begin(), symbols.end(), std::back_inserter(strings),
                   [](torch::jit::Symbol s) { return s.toQualString(); });
    return strings;
  });

  /// Clear the fusion blacklist.
  m.def("clearFusionBlacklist",
        []() { getGlobalPyTorchLoaderSettingsMutable().opBlacklist.clear(); });

  /// Set the index (inclusive) of the first node in the graph to fuse.
  m.def("setFusionStartIndex", [](int64_t startIndex) {
    getGlobalPyTorchLoaderSettingsMutable().fusionStartIndex = startIndex;
  });

  /// Set the index (exclusive) of the last node in the graph to fuse.
  m.def("setFusionEndIndex", [](int64_t endIndex) {
    getGlobalPyTorchLoaderSettingsMutable().fusionEndIndex = endIndex;
  });

  /// Clear the start and end fusion indices.
  m.def("clearFusionIndices", []() {
    getGlobalPyTorchLoaderSettingsMutable().fusionStartIndex = -1;
    getGlobalPyTorchLoaderSettingsMutable().fusionEndIndex = -1;
  });

  /// Disable dumping statistics about the operators and fusion support in the
  /// graph.
  m.def("disable_dump_operator_inventory", []() {
    getGlobalPyTorchLoaderSettingsMutable().dumpOperatorInventory = false;
  });

  /// Enable dumping statistics about the operators and fusion support in the
  /// graph.
  m.def("enable_dump_operator_inventory", []() {
    getGlobalPyTorchLoaderSettingsMutable().dumpOperatorInventory = true;
  });

  m.def("enable_accept_all_layout", []() {
    getGlobalPyTorchLoaderSettingsMutable().disableLayoutVerifying = true;
  });
  m.def("disable_accept_all_layout", []() {
    getGlobalPyTorchLoaderSettingsMutable().disableLayoutVerifying = false;
  });

  /// Set the active HostManager to one that owns 1 of type \p backendName.
  m.def("setGlowBackend", [](const std::string &backendName) {
    getGlobalPyTorchLoaderSettingsMutable().backendName = backendName;
  });

  /// \returns the name of the device backend used by the active HostManager.
  m.def("getGlowBackendName",
        []() { return getGlobalPyTorchLoaderSettingsMutable().backendName; });

  /// Set the quantity of the device backends used by the active
  /// HostManager.
  m.def("setGlowBackendNumDevices", [](int32_t numDevices) {
    return getGlobalPyTorchLoaderSettingsMutable().numDevices = numDevices;
  });

  /// \returns the quantity of the device backends used by the active
  /// HostManager.
  m.def("getGlowBackendNumDevices",
        []() { return getGlobalPyTorchLoaderSettingsMutable().numDevices; });

  /// Inform host manager to load backend specific options from YAML file.
  m.def("loadBackendSpecificOptions", [](const std::string &yamlFile) {
    getGlobalPyTorchLoaderSettingsMutable().backendOptionsFile = yamlFile;
  });

  /// Calls all of the fusion passes that get run before the PyTorchModelLoader
  /// run.
  /// NOTE: This is only exposed for testing.
  m.def("fuseKnownPatterns_", fuseKnownPatterns);

  /// Calls the removeException pass.
  /// NOTE: This is only exposed for testing.
  m.def("removeExceptions_", glow::detail::removeExceptions);

  /// Calls the fuseBranchedLinearPattern pass.
  /// NOTE: This is only exposed for testing.
  m.def("fuseBranchedLinearPattern_", glow::detail::fuseBranchedLinearPattern);

  /// Set the minimum fusion group size.
  m.def("setMinFusionGroupSize", [](size_t k) {
    getGlobalPyTorchLoaderSettingsMutable().minFusionGroupSize = k;
  });

  /// Set the maximum fusion merge size
  m.def("setMaxFusionMergeSize", [](size_t k) {
    getGlobalPyTorchLoaderSettingsMutable().maxFusionMergeSize = k;
  });

  /// Call the Glow fuser and accept all node kinds but don't actually run the
  /// PyTorchModelLoader.
  /// NOTE: This is only exposed for testing.
  m.def("glowCustomFuseDebug_", [](std::shared_ptr<torch::jit::Graph> graph) {
    return glowCustomFuse(graph, getGlobalPyTorchLoaderSettingsMutable());
  });

  /// NOTE: This is only exposed for testing.
  m.def("glowCustomFuseDebug_", [](std::shared_ptr<torch::jit::Graph> graph,
                                   std::vector<std::string> &acceptableKinds) {
    return glowCustomFuseDebug(graph, getGlobalPyTorchLoaderSettingsMutable(),
                               acceptableKinds);
  });

  /// Enable running fusion pass in to_glow as a debug flow
  m.def("enable_debug_fuser", []() {
    getGlobalPyTorchLoaderSettingsMutable().enableDebugFuser = true;
  });

  /// Disable running fusion pass in to_glow as a debug flow
  m.def("disable_debug_fuser", []() {
    getGlobalPyTorchLoaderSettingsMutable().enableDebugFuser = false;
  });

  /// Enable continuously verifying Glow graph during model loading
  m.def("enable_debug_continuously_verify_during_model_loading", []() {
    getGlobalPyTorchLoaderSettingsMutable()
        .debugContinuouslyVerifyDuringModelLoading = true;
  });

  /// Disable continuously verifying Glow graph during model loading
  m.def("disable_debug_continuously_verify_during_model_loading", []() {
    getGlobalPyTorchLoaderSettingsMutable()
        .debugContinuouslyVerifyDuringModelLoading = false;
  });

  /// Calls TorchGlowBackend::preview to lowering info on a model before
  /// compiling.
  m.def("to_glow_preview", [](const torch::jit::Module &orig_module) {
    TorchGlowBackend::preview(orig_module);
  });

  /// Set avaialable devices to those listed in \p availableDevices.
  m.def("set_available_devices", [](std::vector<int32_t> availableDevices) {
    getGlobalPyTorchLoaderSettingsMutable().availableDevices = availableDevices;
  });

  /// Set avaialable devices to all devices on the host
  m.def("clear_available_devices", []() {
    getGlobalPyTorchLoaderSettingsMutable().availableDevices = {};
  });

  /// \returns the list of avaialble devices
  m.def("get_available_devices", []() {
    return getGlobalPyTorchLoaderSettingsMutable().availableDevices;
  });

  /// Enable device tracing from HostManager. Must be set before compilaion.
  m.def("enable_device_tracing", []() {
    getGlobalPyTorchLoaderSettingsMutable().enableDeviceTracing = true;
  });

  /// Disable device tracing from HostManager. Must be set before compilaion.
  m.def("disable_device_tracing", []() {
    getGlobalPyTorchLoaderSettingsMutable().enableDeviceTracing = false;
  });
}
