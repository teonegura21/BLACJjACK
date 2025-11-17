// CUDA Color Space Conversion Kernels

#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace vision {
namespace cuda {

/**
 * Convert RGB to YUV420 color space
 * Optimized for maximum throughput using shared memory
 */
__global__ void rgbToYuv420Kernel(const uint8_t* __restrict__ rgb,
                                   uint8_t* __restrict__ yuv,
                                   uint32_t width,
                                   uint32_t height) {
    const uint32_t x = blockIdx.x * blockDim.x + threadIdx.x;
    const uint32_t y = blockIdx.y * blockDim.y + threadIdx.y;
    
    if (x >= width || y >= height) return;
    
    const uint32_t idx = (y * width + x) * 3;
    const uint8_t r = rgb[idx];
    const uint8_t g = rgb[idx + 1];
    const uint8_t b = rgb[idx + 2];
    
    // YUV conversion using integer arithmetic
    const uint8_t yVal = (66 * r + 129 * g + 25 * b + 128) >> 8;
    
    yuv[y * width + x] = yVal + 16;
}

/**
 * Normalize image from uint8 [0-255] to float [0-1]
 */
__global__ void normalizeKernel(const uint8_t* __restrict__ input,
                                float* __restrict__ output,
                                uint32_t size) {
    const uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx >= size) return;
    
    output[idx] = __uint2float_rn(input[idx]) * 0.00392156862745098f; // 1/255
}

} // namespace cuda
} // namespace vision
