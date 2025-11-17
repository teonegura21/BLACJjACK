#pragma once

#include <cstdint>
#include <vector>

namespace capture {

struct ROI {
    uint32_t x, y, width, height;
};

class ROIDetector {
public:
    ROIDetector();
    ~ROIDetector();

    bool detectTableRegion(const uint8_t* frame, uint32_t width, uint32_t height);
    const ROI& getTableROI() const { return m_tableROI; }
    std::vector<ROI> getCardRegions() const { return m_cardRegions; }
    
    bool needsRecalculation() const;
    void markRecalculated();

private:
    ROI m_tableROI;
    std::vector<ROI> m_cardRegions;
    uint32_t m_framesSinceLastDetection{0};
    static constexpr uint32_t RECALC_INTERVAL = 60;
};

} // namespace capture
