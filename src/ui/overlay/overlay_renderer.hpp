#pragma once

#include <memory>

namespace ui {

class OverlayRenderer {
public:
    OverlayRenderer();
    ~OverlayRenderer();

    bool initialize();
    void render();
    void shutdown();
    
    void setTransparency(float alpha);
    void setVisible(bool visible);
    
    // Update display information
    void updateCount(int runningCount, float trueCount);
    void updateAction(const std::string& action);
    void updateBet(double betAmount);
    void updateMetrics(float fps, float latency);

private:
    bool createOverlayWindow();
    bool initializeOpenGL();
    
    void* m_window{nullptr};
    void* m_glContext{nullptr};
    
    bool m_visible{true};
    float m_transparency{0.7f};
};

} // namespace ui
