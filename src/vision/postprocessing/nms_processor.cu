// CUDA Non-Maximum Suppression Implementation

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace vision {
namespace cuda {

struct Box {
    float x, y, width, height;
    float confidence;
    uint8_t classId;
};

/**
 * Calculate Intersection over Union (IoU) between two boxes
 */
__device__ float calculateIoU(const Box& a, const Box& b) {
    const float x1 = fmaxf(a.x, b.x);
    const float y1 = fmaxf(a.y, b.y);
    const float x2 = fminf(a.x + a.width, b.x + b.width);
    const float y2 = fminf(a.y + a.height, b.y + b.height);
    
    const float intersectionArea = fmaxf(0.0f, x2 - x1) * fmaxf(0.0f, y2 - y1);
    const float areaA = a.width * a.height;
    const float areaB = b.width * b.height;
    const float unionArea = areaA + areaB - intersectionArea;
    
    return unionArea > 0.0f ? intersectionArea / unionArea : 0.0f;
}

/**
 * GPU-accelerated Non-Maximum Suppression
 */
__global__ void nmsKernel(const Box* __restrict__ boxes,
                          bool* __restrict__ suppressed,
                          uint32_t numBoxes,
                          float iouThreshold) {
    const uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx >= numBoxes || suppressed[idx]) return;
    
    const Box& current = boxes[idx];
    
    // Check against all other boxes
    for (uint32_t i = idx + 1; i < numBoxes; i++) {
        if (suppressed[i]) continue;
        
        const Box& other = boxes[i];
        
        // Same class only
        if (current.classId != other.classId) continue;
        
        // Calculate IoU
        const float iou = calculateIoU(current, other);
        
        // Suppress if IoU exceeds threshold and confidence is lower
        if (iou > iouThreshold && other.confidence < current.confidence) {
            suppressed[i] = true;
        }
    }
}

} // namespace cuda
} // namespace vision
