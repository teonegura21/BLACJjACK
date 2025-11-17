#pragma once

#include "../../core/types.hpp"
#include <vector>
#include <memory>

namespace vision {

class Preprocessor {
public:
    Preprocessor();
    ~Preprocessor();

    bool initialize(uint32_t inputWidth, uint32_t inputHeight);
    
    // Process frame and prepare for inference
    bool process(const uint8_t* inputFrame, 
                 uint32_t width, 
                 uint32_t height,
                 float* outputTensor);
    
    // GPU-accelerated operations
    void convertColorSpace(const uint8_t* input, uint8_t* output);
    void normalizeImage(const uint8_t* input, float* output);
    void applyHistogramEqualization(uint8_t* image);

private:
    uint32_t m_inputWidth{0};
    uint32_t m_inputHeight{0};
    
    // CUDA resources
    void* m_cudaWorkspace{nullptr};
    size_t m_workspaceSize{0};
};

} // namespace vision
