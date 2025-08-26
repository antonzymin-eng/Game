
#include "state/EngineBindings.h"

// NOTE: This file is intentionally a stub. Replace bodies with your engine's containers.
// Example (pseudo-code):
//   void engine::ForEachFaction(const std::function<void(const FactionV&)>& cb) {
//       for (const auto& f : Game::Factions()) cb({f.id, f.name, f.power});
//   }

namespace engine {

#ifndef MI_BINDINGS_USE_DEMO

void ForEachFaction(const std::function<void(const FactionV&)>&){}
void ForEachRelation(const std::function<void(const DipRelationV&)>&){}
void ForEachAgent(const std::function<void(const AgentV&)>&){}
void ForEachMarketGood(const std::function<void(const MarketGoodV&)>&){}
void ForEachArmy(const std::function<void(const ArmyV&)>&){}
void ForEachPopulation(const std::function<void(const PopulationRegionV&)>&){}
void ForEachTradeRoute(const std::function<void(const TradeRouteV&)>&){}
void ForEachTech(const std::function<void(const TechNodeV&)>&){}
void ForEachEvent(const std::function<void(const EventV&)>&){}
void ForEachWeather(const std::function<void(const WeatherRegionV&)>&){}
void ForEachFleet(const std::function<void(const NavalFleetV&)>&){}

#else
// Simple demo data for bring-up. Enable with -DMI_BINDINGS_USE_DEMO=ON
#include <vector>

void ForEachFaction(const std::function<void(const FactionV&)>& cb){
    for (auto&& f : std::vector<FactionV>{{1,"France",78.0f},{2,"Spain",82.5f},{3,"Ottomans",95.0f}}) cb(f);
}
void ForEachRelation(const std::function<void(const DipRelationV&)>& cb){
    for (auto&& r : std::vector<DipRelationV>{{1,2,-20,false,true,false},{2,3,-60,false,false,true}}) cb(r);
}
void ForEachAgent(const std::function<void(const AgentV&)>& cb){
    for (auto&& a : std::vector<AgentV>{{10,"Du Tillet","Paris","Counterintel","Active"}}) cb(a);
}
void ForEachMarketGood(const std::function<void(const MarketGoodV&)>& cb){
    for (auto&& g : std::vector<MarketGoodV>{{"Grain",1.2,120,150},{"Iron",3.5,40,55}}) cb(g);
}
void ForEachArmy(const std::function<void(const ArmyV&)>& cb){
    for (auto&& a : std::vector<ArmyV>{{100,"Armee du Nord",22000,0.78f}}) cb(a);
}
void ForEachPopulation(const std::function<void(const PopulationRegionV&)>& cb){
    for (auto&& p : std::vector<PopulationRegionV>{{"Île-de-France",1200000,"French",8.5f}}) cb(p);
}
void ForEachTradeRoute(const std::function<void(const TradeRouteV&)>& cb){
    for (auto&& r : std::vector<TradeRouteV>{{"Seville","Antwerp",12.3,false}}) cb(r);
}
void ForEachTech(const std::function<void(const TechNodeV&)>& cb){
    for (auto&& t : std::vector<TechNodeV>{{1,"Flintlock",false}}) cb(t);
}
void ForEachEvent(const std::function<void(const EventV&)>& cb){
    for (auto&& e : std::vector<EventV>{{500,"Succession Crisis","Pending"}}) cb(e);
}
void ForEachWeather(const std::function<void(const WeatherRegionV&)>& cb){
    for (auto&& w : std::vector<WeatherRegionV>{{"Paris","Clear",0.9f}}) cb(w);
}
void ForEachFleet(const std::function<void(const NavalFleetV&)>& cb){
    for (auto&& f : std::vector<NavalFleetV>{{300,"Armada del Mar Oceano",45,"Cadiz"}}) cb(f);
}
#endif

} // namespace engine
