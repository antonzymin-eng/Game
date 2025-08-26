#pragma once
#include "state/Systems.h"
#include "state/Time.h"
#include "state/Diplomacy.h"
#include "state/Weather.h"
#include <vector>
#include <string>

namespace core {

struct MetaSimState {
    bool paused = false;
    int  speed  = 1; // 1..4
};

class SimulationState {
public:
    static SimulationState& I();

    // Meta
    const MetaSimState& getMeta() const { return meta_; }
    MetaSimState&       getMeta()       { return meta_; }

    // Subsystems
    std::vector<Faction>&      factions()      { return factions_; }
    const std::vector<Faction>& factions() const { return factions_; }

    Market&                    market()        { return market_; }
    const Market&              market()  const { return market_; }

    std::vector<Agent>&        agents()        { return agents_; }
    const std::vector<Agent>&  agents()  const { return agents_; }

    std::vector<DipRelation>&  diplomacy()     { return diplomacy_; }
    const std::vector<DipRelation>& diplomacy() const { return diplomacy_; }

    std::vector<WeatherInfo>&  weather()       { return weather_; }
    const std::vector<WeatherInfo>& weather() const { return weather_; }

    GameClock&                 clock()         { return clock_; }
    const GameClock&           clock()   const { return clock_; }

    // Helpers
    std::string factionName(int id) const;

    // Stubs used by UI
    void clearEspionageSignal() {}

private:
    SimulationState() = default;

    MetaSimState meta_{};
    GameClock    clock_{};

    std::vector<Faction>      factions_{};
    Market                    market_{};
    std::vector<Agent>        agents_{};
    std::vector<DipRelation>  diplomacy_{};
    std::vector<WeatherInfo>  weather_{};
};

} // namespace core
