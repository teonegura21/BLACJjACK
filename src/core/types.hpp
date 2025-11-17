#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <vector>

namespace core {

// System configuration
struct SystemConfig {
    uint32_t latency_target_ms = 8;
    int gpu_device_id = 0;
    bool thread_affinity_enabled = true;
    bool real_time_priority = true;
    std::string cuda_stream_priority = "high";
    std::vector<int> cpu_core_affinity = {0, 1, 2, 3};
    uint32_t memory_pool_size_mb = 2048;
    bool enable_nvtx_markers = true;
    uint32_t gpu_clock_lock_mhz = 2400;
};

// Capture configuration
struct CaptureConfig {
    enum class CaptureMethod { DXGI, NVFBC };
    CaptureMethod method = CaptureMethod::DXGI;
    uint32_t frame_rate = 120;
    uint32_t buffer_count = 16;
    float motion_detection_threshold = 0.015f;
    bool use_hardware_encoding = true;
    std::string color_space = "bt709";
    bool hdr_enabled = false;
    bool async_copy = true;
};

// Vision configuration
struct VisionConfig {
    std::string model_path = "./models/yolov11x_card_detector.trt";
    std::string model_type = "yolov11x";
    std::array<uint32_t, 2> input_resolution = {1280, 1280};
    float confidence_threshold = 0.65f;
    float nms_threshold = 0.45f;
    uint32_t batch_size = 1;
    bool use_fp16 = true;
    bool use_int8 = false;
    int dla_core = -1;
    uint32_t max_workspace_size_mb = 4096;
    bool enable_cuda_graphs = true;
    bool enable_tactic_sources = true;
    std::string profiling_verbosity = "detailed";
};

// Counting configuration
struct CountingConfig {
    enum class CountingSystem { HiLo, KO, Omega2, HalvesCount };
    CountingSystem system = CountingSystem::HiLo;
    uint32_t deck_count = 6;
    float penetration = 0.75f;
    uint32_t history_size = 512;
};

// Strategy configuration
struct StrategyConfig {
    std::string basic_strategy_rules = "s17_das";
    bool deviations_enabled = true;
    bool illustrious_18 = true;
    bool fab_4 = true;
};

// Betting configuration
struct BettingConfig {
    float kelly_fraction = 0.25f;
    double min_bet = 10.0;
    double max_bet = 500.0;
    std::array<uint32_t, 5> spread = {1, 2, 4, 8, 12};
};

// UI configuration
struct UIConfig {
    bool overlay_enabled = true;
    float transparency = 0.7f;
    std::string color_scheme = "dark";
    bool show_performance_metrics = true;
    bool audio_alerts = true;
};

// Card representation
enum class CardRank : uint8_t {
    Ace = 1, Two, Three, Four, Five, Six, Seven, Eight, Nine, Ten,
    Jack, Queen, King
};

enum class CardSuit : uint8_t {
    Hearts, Diamonds, Clubs, Spades
};

struct Card {
    CardRank rank;
    CardSuit suit;
    uint8_t confidence;
    uint64_t timestamp_ns;
    
    int getValue() const {
        if (rank == CardRank::Ace) return 11;
        if (static_cast<int>(rank) >= 10) return 10;
        return static_cast<int>(rank);
    }
};

// Detection result
struct Detection {
    float x, y, width, height;  // Bounding box
    uint8_t card_id;             // 0-51 for 52 cards
    float confidence;
    uint64_t timestamp_ns;
};

} // namespace core
