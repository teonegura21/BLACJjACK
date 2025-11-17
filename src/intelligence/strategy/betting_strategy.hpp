#pragma once

#include <cstdint>
#include <array>

namespace intelligence {

class BettingStrategy {
public:
    BettingStrategy();
    ~BettingStrategy();

    void configure(double minBet, double maxBet, float kellyFraction);
    
    double calculateBet(float trueCount, double bankroll) const;
    double calculateKellyBet(float advantage, double bankroll) const;
    
    void setBankroll(double bankroll) { m_bankroll = bankroll; }
    double getBankroll() const { return m_bankroll; }
    
    // Camouflage betting
    double getCamouflageBet(float trueCount) const;

private:
    double m_minBet{10.0};
    double m_maxBet{500.0};
    float m_kellyFraction{0.25f};
    double m_bankroll{10000.0};
    
    std::array<uint32_t, 5> m_betSpread{1, 2, 4, 8, 12};
};

} // namespace intelligence
