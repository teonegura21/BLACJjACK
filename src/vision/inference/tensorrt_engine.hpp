#pragma once

#include "../../core/types.hpp"
#include <string>
#include <vector>
#include <memory>
#include <NvInfer.h>
#include <NvOnnxParser.h>
#include <cuda_runtime_api.h>
#include <chrono>
#include <fstream>

namespace vision {

// Custom TensorRT logger
class TRTLogger : public nvinfer1::ILogger {
public:
    void log(Severity severity, const char* msg) noexcept override;
};

// RAII wrapper for CUDA resources
template<typename T>
struct CUDADeleter {
    void operator()(T* ptr) const {
        if (ptr) cudaFree(ptr);
    }
};

template<typename T>
using CUDAPtr = std::unique_ptr<T, CUDADeleter<T>>;

class TensorRTEngine {
public:
    explicit TensorRTEngine(const core::VisionConfig& config);
    ~TensorRTEngine();

    // Model loading and building
    bool loadSerializedEngine(const std::string& enginePath);
    bool buildEngineFromOnnx(const std::string& onnxPath);
    bool saveEngine(const std::string& outputPath);

    // Inference
    bool infer(const float* inputTensor,
               std::vector<core::Detection>& detections,
               float confThreshold,
               float nmsThreshold);

    bool inferAsync(const float* inputTensor,
                    cudaStream_t stream,
                    std::vector<core::Detection>& detections);

    // Getters
    uint32_t getInputWidth() const { return m_inputWidth; }
    uint32_t getInputHeight() const { return m_inputHeight; }
    uint32_t getBatchSize() const { return m_batchSize; }
    size_t getNumClasses() const { return m_numClasses; }

    // Performance metrics
    float getAverageInferenceTime() const;
    float getLastInferenceTime() const { return m_lastInferenceTime; }
    size_t getTotalInferences() const { return m_inferenceCount; }

    // Warmup for optimal performance
    void warmup(int iterations = 10);

private:
    // Engine creation and management
    bool createExecutionContext();
    bool allocateBuffers();
    void deallocateBuffers();

    // Post-processing
    void parseYOLOv11Output(const float* output,
                           std::vector<core::Detection>& detections,
                           float confThreshold,
                           float nmsThreshold);

    std::vector<int> performNMS(const std::vector<core::Detection>& boxes,
                                float nmsThreshold);

    float computeIoU(const core::Detection& box1,
                    const core::Detection& box2);

    // TensorRT components
    std::unique_ptr<nvinfer1::IRuntime> m_runtime;
    std::unique_ptr<nvinfer1::ICudaEngine> m_engine;
    std::unique_ptr<nvinfer1::IExecutionContext> m_context;
    TRTLogger m_logger;

    // CUDA resources
    cudaStream_t m_stream{nullptr};
    cudaEvent_t m_startEvent{nullptr};
    cudaEvent_t m_endEvent{nullptr};

    // Buffers
    void* m_deviceInputBuffer{nullptr};
    void* m_deviceOutputBuffer{nullptr};
    std::vector<float> m_hostOutputBuffer;

    // Model configuration
    const core::VisionConfig& m_config;
    uint32_t m_inputWidth{1280};
    uint32_t m_inputHeight{1280};
    uint32_t m_batchSize{1};
    size_t m_numClasses{52};  // 52 cards in a deck

    // Input/Output dimensions
    size_t m_inputSize{0};
    size_t m_outputSize{0};
    int m_inputIndex{-1};
    int m_outputIndex{-1};

    // Performance tracking
    float m_avgInferenceTime{0.0f};
    float m_lastInferenceTime{0.0f};
    size_t m_inferenceCount{0};

    // CUDA Graphs for optimization
    bool m_useCudaGraphs{true};
    cudaGraph_t m_cudaGraph{nullptr};
    cudaGraphExec_t m_cudaGraphExec{nullptr};
    bool m_graphCaptured{false};
};

} // namespace vision
