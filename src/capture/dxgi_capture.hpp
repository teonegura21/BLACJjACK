#pragma once

#include "capture_interface.hpp"
#include <vector>

namespace capture {

class DXGICapture : public CaptureInterface {
public:
    DXGICapture();
    ~DXGICapture() override;

    bool initialize() override;
    bool start() override;
    bool stop() override;
    bool captureFrame(Frame& frame) override;
    void releaseFrame(Frame& frame) override;
    
    uint32_t getWidth() const override { return m_width; }
    uint32_t getHeight() const override { return m_height; }
    uint32_t getFrameRate() const override { return m_frameRate; }

private:
    bool initializeDXGI();
    bool createTextures();
    
    uint32_t m_width{0};
    uint32_t m_height{0};
    uint32_t m_frameRate{60};
    bool m_initialized{false};
    
    // Platform-specific handles
    void* m_device{nullptr};
    void* m_context{nullptr};
    void* m_duplication{nullptr};
};

} // namespace capture
