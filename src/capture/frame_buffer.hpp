#pragma once

#include "capture_interface.hpp"
#include <array>
#include <atomic>
#include <memory>

namespace capture {

template<size_t BufferCount = 8>
class FrameBuffer {
public:
    FrameBuffer(uint32_t width, uint32_t height);
    ~FrameBuffer();

    bool initialize();
    
    Frame* acquireWriteBuffer();
    void releaseWriteBuffer(Frame* frame);
    
    Frame* acquireReadBuffer();
    void releaseReadBuffer(Frame* frame);
    
    uint32_t getAvailableFrames() const;

private:
    struct BufferSlot {
        Frame frame;
        std::atomic<bool> inUse{false};
        uint8_t* cudaMemory{nullptr};
    };
    
    std::array<BufferSlot, BufferCount> m_buffers;
    uint32_t m_width;
    uint32_t m_height;
    std::atomic<uint32_t> m_writeIndex{0};
    std::atomic<uint32_t> m_readIndex{0};
};

} // namespace capture
