#pragma once

#include "../../core/types.hpp"
#include <vector>
#include <map>

namespace vision {

struct TrackedCard {
    core::Detection detection;
    uint32_t trackId;
    uint32_t age;  // Frames since last seen
    float kalmanX, kalmanY;  // Predicted position
    std::vector<core::Detection> history;
};

class CardTracker {
public:
    CardTracker();
    ~CardTracker();

    void update(const std::vector<core::Detection>& newDetections);
    std::vector<TrackedCard> getTrackedCards() const;
    
    void reset();
    
    // Configuration
    void setMaxAge(uint32_t maxAge) { m_maxAge = maxAge; }
    void setIoUThreshold(float threshold) { m_iouThreshold = threshold; }

private:
    float calculateIoU(const core::Detection& a, const core::Detection& b);
    void associateDetections(const std::vector<core::Detection>& detections);
    void updateKalmanFilters();
    void removeOldTracks();
    
    std::map<uint32_t, TrackedCard> m_tracks;
    uint32_t m_nextTrackId{0};
    uint32_t m_maxAge{30};  // Remove after 30 frames
    float m_iouThreshold{0.3f};
};

} // namespace vision
