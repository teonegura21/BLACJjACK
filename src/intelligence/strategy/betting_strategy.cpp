#include "betting_strategy.hpp"
#include "../../utils/logger.hpp"
#include <algorithm>
#include <cmath>

namespace intelligence {

BettingStrategy::BettingStrategy() {
    auto& logger = utils::Logger::getInstance();
    logger.info("Initializing Kelly Criterion betting strategy");
}

BettingStrategy::~BettingStrategy() = default;

void BettingStrategy::configure(double minBet, double maxBet, float kellyFraction) {
    m_minBet = minBet;
    m_maxBet = maxBet;
    m_kellyFraction = kellyFraction;

    auto& logger = utils::Logger::getInstance();
    logger.info("Betting configured: Min=${:.2f}, Max=${:.2f}, Kelly Fraction={:.2f}",
                m_minBet, m_maxBet, m_kellyFraction);
}

double BettingStrategy::calculateBet(float trueCount, double bankroll) const {
    // For true count <= 0, player has no advantage, bet minimum
    if (trueCount <= 0.0f) {
        return m_minBet;
    }

    // Calculate player advantage based on true count
    // Rule of thumb: Each +1 true count = ~0.5% player advantage
    float playerEdge = 0.005f * trueCount;

    // Use Kelly Criterion for optimal bet sizing
    double optimalBet = calculateKellyBet(playerEdge, bankroll);

    // Clamp to min/max limits
    return std::clamp(optimalBet, m_minBet, m_maxBet);
}

double BettingStrategy::calculateKellyBet(float advantage, double bankroll) const {
    // Kelly Criterion Formula:
    // f = edge / variance
    // For blackjack:
    //   - Variance = 1.3225 (standard deviation squared: 1.15^2)
    //   - We use fractional Kelly to reduce risk

    constexpr float BLACKJACK_VARIANCE = 1.3225f;

    // Full Kelly bet percentage
    float fullKellyPercent = advantage / BLACKJACK_VARIANCE;

    // Apply fractional Kelly for risk management
    // Quarter Kelly (0.25) recommended for most players:
    //   - 51% of full Kelly growth rate
    //   - Much lower risk (1 in 213 chance of 80% drawdown vs 1 in 5)
    float fractionalKellyPercent = fullKellyPercent * m_kellyFraction;

    // Calculate bet amount
    double betAmount = bankroll * fractionalKellyPercent;

    // Log calculation for debugging
    auto& logger = utils::Logger::getInstance();
    logger.debug("Kelly: Advantage={:.3f}%, FullKelly={:.3f}%, Fractional={:.3f}%, Bet=${:.2f}",
                 advantage * 100.0f,
                 fullKellyPercent * 100.0f,
                 fractionalKellyPercent * 100.0f,
                 betAmount);

    return betAmount;
}

double BettingStrategy::getCamouflageBet(float trueCount) const {
    // Camouflage betting using spread strategy
    // This is less optimal than Kelly but more resistant to detection

    // Determine bet unit based on true count and spread array
    uint32_t units;

    if (trueCount <= 0) {
        units = m_betSpread[0]; // 1 unit
    } else if (trueCount >= 1 && trueCount < 2) {
        units = m_betSpread[1]; // 2 units
    } else if (trueCount >= 2 && trueCount < 3) {
        units = m_betSpread[2]; // 4 units
    } else if (trueCount >= 3 && trueCount < 4) {
        units = m_betSpread[3]; // 8 units
    } else {
        units = m_betSpread[4]; // 12 units (TC >= 4)
    }

    // Calculate bet amount
    double betAmount = m_minBet * units;

    // Clamp to max bet
    return std::min(betAmount, m_maxBet);
}

} // namespace intelligence
