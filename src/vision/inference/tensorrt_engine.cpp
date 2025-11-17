#include "tensorrt_engine.hpp"
#include "../../utils/logger.hpp"
#include <algorithm>
#include <iostream>
#include <cassert>

namespace vision {

// TensorRT Logger Implementation
void TRTLogger::log(Severity severity, const char* msg) noexcept {
    auto& logger = utils::Logger::getInstance();

    switch (severity) {
        case Severity::kINTERNAL_ERROR:
        case Severity::kERROR:
            logger.error("[TensorRT] {}", msg);
            break;
        case Severity::kWARNING:
            logger.warn("[TensorRT] {}", msg);
            break;
        case Severity::kINFO:
            logger.info("[TensorRT] {}", msg);
            break;
        case Severity::kVERBOSE:
            logger.debug("[TensorRT] {}", msg);
            break;
    }
}

// Constructor
TensorRTEngine::TensorRTEngine(const core::VisionConfig& config)
    : m_config(config)
    , m_inputWidth(config.input_resolution[0])
    , m_inputHeight(config.input_resolution[1])
    , m_batchSize(config.batch_size)
    , m_useCudaGraphs(config.enable_cuda_graphs) {

    auto& logger = utils::Logger::getInstance();
    logger.info("Initializing TensorRT Engine with {}x{} resolution",
                m_inputWidth, m_inputHeight);

    // Create CUDA stream with high priority if configured
    int leastPriority, greatestPriority;
    cudaDeviceGetStreamPriorityRange(&leastPriority, &greatestPriority);

    if (m_config.cuda_stream_priority == "high") {
        cudaStreamCreateWithPriority(&m_stream, cudaStreamNonBlocking, greatestPriority);
    } else {
        cudaStreamCreate(&m_stream);
    }

    // Create CUDA events for timing
    cudaEventCreate(&m_startEvent);
    cudaEventCreate(&m_endEvent);

    logger.info("CUDA resources initialized successfully");
}

// Destructor
TensorRTEngine::~TensorRTEngine() {
    deallocateBuffers();

    if (m_cudaGraphExec) cudaGraphExecDestroy(m_cudaGraphExec);
    if (m_cudaGraph) cudaGraphDestroy(m_cudaGraph);
    if (m_startEvent) cudaEventDestroy(m_startEvent);
    if (m_endEvent) cudaEventDestroy(m_endEvent);
    if (m_stream) cudaStreamDestroy(m_stream);

    auto& logger = utils::Logger::getInstance();
    logger.info("TensorRT Engine destroyed");
}

// Load serialized TensorRT engine
bool TensorRTEngine::loadSerializedEngine(const std::string& enginePath) {
    auto& logger = utils::Logger::getInstance();
    logger.info("Loading serialized TensorRT engine from: {}", enginePath);

    std::ifstream file(enginePath, std::ios::binary);
    if (!file.good()) {
        logger.error("Failed to open engine file: {}", enginePath);
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> engineData(size);
    file.read(engineData.data(), size);
    file.close();

    logger.info("Read {} bytes from engine file", size);

    // Create runtime and deserialize engine
    m_runtime.reset(nvinfer1::createInferRuntime(m_logger));
    if (!m_runtime) {
        logger.error("Failed to create TensorRT runtime");
        return false;
    }

    m_engine.reset(m_runtime->deserializeCudaEngine(engineData.data(), size));
    if (!m_engine) {
        logger.error("Failed to deserialize CUDA engine");
        return false;
    }

    logger.info("Engine deserialized successfully");

    // Get input/output indices and sizes
    m_inputIndex = m_engine->getBindingIndex("images");
    m_outputIndex = m_engine->getBindingIndex("output0");

    if (m_inputIndex < 0 || m_outputIndex < 0) {
        logger.error("Failed to find input/output bindings");
        return false;
    }

    auto inputDims = m_engine->getBindingDimensions(m_inputIndex);
    auto outputDims = m_engine->getBindingDimensions(m_outputIndex);

    // Calculate buffer sizes
    m_inputSize = m_batchSize * 3 * m_inputWidth * m_inputHeight;

    // YOLOv11 output format: [batch, num_predictions, 56] (52 classes + 4 bbox)
    m_outputSize = m_batchSize * outputDims.d[1] * outputDims.d[2];

    logger.info("Input tensor: {} x {} x {} x {}", m_batchSize, 3, m_inputHeight, m_inputWidth);
    logger.info("Output tensor: {} x {} x {}", outputDims.d[0], outputDims.d[1], outputDims.d[2]);

    if (!createExecutionContext()) {
        return false;
    }

    if (!allocateBuffers()) {
        return false;
    }

    logger.info("TensorRT engine loaded successfully");
    return true;
}

// Build engine from ONNX model
bool TensorRTEngine::buildEngineFromOnnx(const std::string& onnxPath) {
    auto& logger = utils::Logger::getInstance();
    logger.info("Building TensorRT engine from ONNX: {}", onnxPath);

    // Create builder
    auto builder = std::unique_ptr<nvinfer1::IBuilder>(
        nvinfer1::createInferBuilder(m_logger));
    if (!builder) {
        logger.error("Failed to create TensorRT builder");
        return false;
    }

    // Network flags
    const auto explicitBatch = 1U << static_cast<uint32_t>(
        nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);

    auto network = std::unique_ptr<nvinfer1::INetworkDefinition>(
        builder->createNetworkV2(explicitBatch));
    if (!network) {
        logger.error("Failed to create network definition");
        return false;
    }

    // ONNX parser
    auto parser = std::unique_ptr<nvonnxparser::IParser>(
        nvonnxparser::createParser(*network, m_logger));
    if (!parser) {
        logger.error("Failed to create ONNX parser");
        return false;
    }

    // Parse ONNX file
    if (!parser->parseFromFile(onnxPath.c_str(),
            static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
        logger.error("Failed to parse ONNX file");
        for (int i = 0; i < parser->getNbErrors(); ++i) {
            logger.error("Parser error: {}", parser->getError(i)->desc());
        }
        return false;
    }

    // Build configuration
    auto config = std::unique_ptr<nvinfer1::IBuilderConfig>(
        builder->createBuilderConfig());
    if (!config) {
        logger.error("Failed to create builder config");
        return false;
    }

    // Set workspace size
    config->setMaxWorkspaceSize(static_cast<size_t>(m_config.max_workspace_size_mb) * (1 << 20));

    // Enable FP16 if requested
    if (m_config.use_fp16 && builder->platformHasFastFp16()) {
        config->setFlag(nvinfer1::BuilderFlag::kFP16);
        logger.info("FP16 mode enabled");
    }

    // Enable INT8 if requested
    if (m_config.use_int8 && builder->platformHasFastInt8()) {
        config->setFlag(nvinfer1::BuilderFlag::kINT8);
        logger.info("INT8 mode enabled");
        // Note: INT8 calibration would be needed here for best results
    }

    // Enable TF32 for Ampere and newer
    config->setFlag(nvinfer1::BuilderFlag::kTF32);

    // Set profiling verbosity
    if (m_config.profiling_verbosity == "detailed") {
        config->setProfilingVerbosity(nvinfer1::ProfilingVerbosity::kDETAILED);
    }

    // Build engine
    logger.info("Building CUDA engine (this may take several minutes)...");
    m_engine.reset(builder->buildEngineWithConfig(*network, *config));

    if (!m_engine) {
        logger.error("Failed to build CUDA engine");
        return false;
    }

    logger.info("CUDA engine built successfully");

    // Get binding indices
    m_inputIndex = m_engine->getBindingIndex("images");
    m_outputIndex = m_engine->getBindingIndex("output0");

    if (!createExecutionContext()) {
        return false;
    }

    if (!allocateBuffers()) {
        return false;
    }

    return true;
}

// Save engine to file
bool TensorRTEngine::saveEngine(const std::string& outputPath) {
    auto& logger = utils::Logger::getInstance();
    logger.info("Saving TensorRT engine to: {}", outputPath);

    if (!m_engine) {
        logger.error("No engine to save");
        return false;
    }

    auto serialized = std::unique_ptr<nvinfer1::IHostMemory>(
        m_engine->serialize());
    if (!serialized) {
        logger.error("Failed to serialize engine");
        return false;
    }

    std::ofstream file(outputPath, std::ios::binary);
    if (!file.good()) {
        logger.error("Failed to open output file: {}", outputPath);
        return false;
    }

    file.write(static_cast<const char*>(serialized->data()), serialized->size());
    file.close();

    logger.info("Engine saved successfully ({} bytes)", serialized->size());
    return true;
}

// Create execution context
bool TensorRTEngine::createExecutionContext() {
    auto& logger = utils::Logger::getInstance();

    m_context.reset(m_engine->createExecutionContext());
    if (!m_context) {
        logger.error("Failed to create execution context");
        return false;
    }

    logger.info("Execution context created successfully");
    return true;
}

// Allocate buffers
bool TensorRTEngine::allocateBuffers() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Allocating inference buffers");

    // Allocate device memory
    size_t inputBytes = m_inputSize * sizeof(float);
    size_t outputBytes = m_outputSize * sizeof(float);

    cudaError_t status;

    status = cudaMalloc(&m_deviceInputBuffer, inputBytes);
    if (status != cudaSuccess) {
        logger.error("Failed to allocate input buffer: {}", cudaGetErrorString(status));
        return false;
    }

    status = cudaMalloc(&m_deviceOutputBuffer, outputBytes);
    if (status != cudaSuccess) {
        logger.error("Failed to allocate output buffer: {}", cudaGetErrorString(status));
        cudaFree(m_deviceInputBuffer);
        return false;
    }

    // Allocate host memory
    m_hostOutputBuffer.resize(m_outputSize);

    logger.info("Allocated {:.2f} MB for input buffer", inputBytes / (1024.0f * 1024.0f));
    logger.info("Allocated {:.2f} MB for output buffer", outputBytes / (1024.0f * 1024.0f));

    return true;
}

// Deallocate buffers
void TensorRTEngine::deallocateBuffers() {
    if (m_deviceInputBuffer) {
        cudaFree(m_deviceInputBuffer);
        m_deviceInputBuffer = nullptr;
    }
    if (m_deviceOutputBuffer) {
        cudaFree(m_deviceOutputBuffer);
        m_deviceOutputBuffer = nullptr;
    }
    m_hostOutputBuffer.clear();
}

// Synchronous inference
bool TensorRTEngine::infer(const float* inputTensor,
                          std::vector<core::Detection>& detections,
                          float confThreshold,
                          float nmsThreshold) {
    auto& logger = utils::Logger::getInstance();

    if (!m_context) {
        logger.error("Execution context not initialized");
        return false;
    }

    // Start timing
    cudaEventRecord(m_startEvent, m_stream);

    // Copy input to device
    size_t inputBytes = m_inputSize * sizeof(float);
    cudaError_t status = cudaMemcpyAsync(
        m_deviceInputBuffer, inputTensor, inputBytes,
        cudaMemcpyHostToDevice, m_stream);

    if (status != cudaSuccess) {
        logger.error("Failed to copy input to device: {}", cudaGetErrorString(status));
        return false;
    }

    // Execute inference
    void* bindings[] = {m_deviceInputBuffer, m_deviceOutputBuffer};

    if (m_useCudaGraphs && m_graphCaptured) {
        // Use CUDA Graph for maximum performance
        cudaGraphLaunch(m_cudaGraphExec, m_stream);
    } else if (m_useCudaGraphs && !m_graphCaptured) {
        // Capture CUDA Graph on first inference
        cudaStreamBeginCapture(m_stream, cudaStreamCaptureModeGlobal);
        m_context->enqueueV2(bindings, m_stream, nullptr);
        cudaStreamEndCapture(m_stream, &m_cudaGraph);
        cudaGraphInstantiate(&m_cudaGraphExec, m_cudaGraph, nullptr, nullptr, 0);
        m_graphCaptured = true;
    } else {
        // Standard execution
        if (!m_context->enqueueV2(bindings, m_stream, nullptr)) {
            logger.error("Failed to execute inference");
            return false;
        }
    }

    // Copy output to host
    size_t outputBytes = m_outputSize * sizeof(float);
    status = cudaMemcpyAsync(
        m_hostOutputBuffer.data(), m_deviceOutputBuffer, outputBytes,
        cudaMemcpyDeviceToHost, m_stream);

    if (status != cudaSuccess) {
        logger.error("Failed to copy output to host: {}", cudaGetErrorString(status));
        return false;
    }

    // Wait for completion
    cudaStreamSynchronize(m_stream);

    // End timing
    cudaEventRecord(m_endEvent, m_stream);
    cudaEventSynchronize(m_endEvent);

    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, m_startEvent, m_endEvent);

    m_lastInferenceTime = milliseconds;
    m_avgInferenceTime = (m_avgInferenceTime * m_inferenceCount + milliseconds) /
                         (m_inferenceCount + 1);
    m_inferenceCount++;

    // Parse output
    parseYOLOv11Output(m_hostOutputBuffer.data(), detections,
                      confThreshold, nmsThreshold);

    return true;
}

// Parse YOLOv11 output
void TensorRTEngine::parseYOLOv11Output(const float* output,
                                       std::vector<core::Detection>& detections,
                                       float confThreshold,
                                       float nmsThreshold) {
    detections.clear();

    // YOLOv11 output format: [batch, num_predictions, 56]
    // 56 = 4 (bbox) + 52 (classes)
    const int numPredictions = m_outputSize / 56;

    for (int i = 0; i < numPredictions; i++) {
        const float* pred = output + i * 56;

        // Get bbox coordinates (center x, center y, width, height)
        float cx = pred[0];
        float cy = pred[1];
        float w = pred[2];
        float h = pred[3];

        // Find class with highest confidence
        float maxConf = 0.0f;
        int maxClassId = -1;

        for (int j = 0; j < 52; j++) {
            float conf = pred[4 + j];
            if (conf > maxConf) {
                maxConf = conf;
                maxClassId = j;
            }
        }

        // Filter by confidence threshold
        if (maxConf < confThreshold) {
            continue;
        }

        // Convert to corner coordinates
        float x = cx - w / 2.0f;
        float y = cy - h / 2.0f;

        core::Detection det;
        det.x = x;
        det.y = y;
        det.width = w;
        det.height = h;
        det.card_id = static_cast<uint8_t>(maxClassId);
        det.confidence = maxConf;
        det.timestamp_ns = std::chrono::high_resolution_clock::now()
                          .time_since_epoch().count();

        detections.push_back(det);
    }

    // Apply NMS
    std::vector<int> keepIndices = performNMS(detections, nmsThreshold);

    std::vector<core::Detection> filteredDetections;
    for (int idx : keepIndices) {
        filteredDetections.push_back(detections[idx]);
    }

    detections = std::move(filteredDetections);
}

// Non-Maximum Suppression
std::vector<int> TensorRTEngine::performNMS(const std::vector<core::Detection>& boxes,
                                            float nmsThreshold) {
    std::vector<int> indices(boxes.size());
    std::iota(indices.begin(), indices.end(), 0);

    // Sort by confidence (descending)
    std::sort(indices.begin(), indices.end(),
              [&boxes](int a, int b) {
                  return boxes[a].confidence > boxes[b].confidence;
              });

    std::vector<bool> suppressed(boxes.size(), false);
    std::vector<int> keep;

    for (size_t i = 0; i < indices.size(); i++) {
        int idx = indices[i];
        if (suppressed[idx]) continue;

        keep.push_back(idx);

        for (size_t j = i + 1; j < indices.size(); j++) {
            int idx2 = indices[j];
            if (suppressed[idx2]) continue;

            float iou = computeIoU(boxes[idx], boxes[idx2]);
            if (iou > nmsThreshold) {
                suppressed[idx2] = true;
            }
        }
    }

    return keep;
}

// Compute Intersection over Union
float TensorRTEngine::computeIoU(const core::Detection& box1,
                                const core::Detection& box2) {
    float x1 = std::max(box1.x, box2.x);
    float y1 = std::max(box1.y, box2.y);
    float x2 = std::min(box1.x + box1.width, box2.x + box2.width);
    float y2 = std::min(box1.y + box1.height, box2.y + box2.height);

    float intersection = std::max(0.0f, x2 - x1) * std::max(0.0f, y2 - y1);
    float area1 = box1.width * box1.height;
    float area2 = box2.width * box2.height;
    float unionArea = area1 + area2 - intersection;

    return unionArea > 0 ? intersection / unionArea : 0.0f;
}

// Warmup
void TensorRTEngine::warmup(int iterations) {
    auto& logger = utils::Logger::getInstance();
    logger.info("Warming up TensorRT engine ({} iterations)", iterations);

    std::vector<float> dummyInput(m_inputSize, 0.5f);
    std::vector<core::Detection> dummyOutput;

    for (int i = 0; i < iterations; i++) {
        infer(dummyInput.data(), dummyOutput, 0.5f, 0.4f);
    }

    logger.info("Warmup completed. Average inference time: {:.2f} ms",
                m_avgInferenceTime);
}

// Get average inference time
float TensorRTEngine::getAverageInferenceTime() const {
    return m_avgInferenceTime;
}

} // namespace vision
