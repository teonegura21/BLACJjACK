#pragma once

#include <cstdint>

namespace core {
namespace constants {

// Latency budgets (nanoseconds)
constexpr uint64_t CAPTURE_LATENCY_NS = 2'000'000;        // 2ms
constexpr uint64_t PREPROCESSING_LATENCY_NS = 3'000'000;  // 3ms
constexpr uint64_t INFERENCE_LATENCY_NS = 8'000'000;      // 8ms
constexpr uint64_t POSTPROCESSING_LATENCY_NS = 2'000'000; // 2ms
constexpr uint64_t UI_UPDATE_LATENCY_NS = 1'000'000;      // 1ms
constexpr uint64_t TOTAL_LATENCY_NS = 16'000'000;         // 16ms

// Resource constraints
constexpr size_t GPU_MEMORY_LIMIT = 2ULL * 1024 * 1024 * 1024;  // 2GB
constexpr size_t SYSTEM_MEMORY_LIMIT = 1ULL * 1024 * 1024 * 1024; // 1GB
constexpr float CPU_USAGE_LIMIT = 0.25f;  // 25%

// Queue sizes
constexpr uint32_t CAPTURE_QUEUE_SIZE = 16;
constexpr uint32_t INFERENCE_QUEUE_SIZE = 8;
constexpr uint32_t COUNTING_QUEUE_SIZE = 32;

// Card counting constants
constexpr uint32_t STANDARD_DECK_SIZE = 52;
constexpr uint32_t MAX_DECKS = 8;
constexpr uint32_t CARD_HISTORY_SIZE = 512;

// Thread priorities (Linux RT priorities)
constexpr int PRIORITY_CAPTURE = 99;
constexpr int PRIORITY_INFERENCE = 95;
constexpr int PRIORITY_COUNTING = 90;
constexpr int PRIORITY_STRATEGY = 85;
constexpr int PRIORITY_UI = 50;

// Accuracy requirements
constexpr float DETECTION_PRECISION = 0.995f;  // 99.5%
constexpr float DETECTION_RECALL = 0.98f;      // 98%
constexpr float COUNT_ACCURACY = 1.0f;         // 100%

} // namespace constants
} // namespace core
