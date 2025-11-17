#pragma once

#include <memory>

namespace pipeline {

class PipelineManager {
public:
    PipelineManager();
    ~PipelineManager();

    bool initialize();
    bool start();
    bool stop();
    
    bool isRunning() const { return m_running; }
    
    // Get performance metrics
    float getAverageLatency() const;
    uint32_t getFramesProcessed() const;
    uint32_t getFramesDropped() const;

private:
    void captureThreadFunc();
    void preprocessThreadFunc();
    void inferenceThreadFunc();
    void postprocessThreadFunc();
    void countingThreadFunc();
    void strategyThreadFunc();
    void uiThreadFunc();
    
    bool m_running{false};
    bool m_initialized{false};
};

} // namespace pipeline
