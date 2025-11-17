#pragma once

#include <memory>
#include <cstdint>

namespace capture {

struct Frame {
    uint8_t* data;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint64_t timestamp_ns;
    uint32_t frame_id;
};

class CaptureInterface {
public:
    virtual ~CaptureInterface() = default;

    virtual bool initialize() = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool captureFrame(Frame& frame) = 0;
    virtual void releaseFrame(Frame& frame) = 0;
    
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual uint32_t getFrameRate() const = 0;
};

std::unique_ptr<CaptureInterface> createCapture(const std::string& method);

} // namespace capture
