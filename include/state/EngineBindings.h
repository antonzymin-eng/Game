#pragma once
#include <string>
#include <functional>
#include <cstdint>
namespace engine {
struct FactionV { int id; std::string name; float power; };
struct DipRelationV { int aId, bId; int value; bool truce, alliance, war; };
struct AgentV { int id; std::string name, location, mission, status; };
struct MarketGoodV { std::string name; double price, supply, demand; };
struct ArmyV { int id; std::string name; int strength; float morale; };
struct PopulationRegionV { std::string name; std::int64_t population; std::string culture; float unrest; };
struct TradeRouteV { std::string origin, dest; double throughput; bool disrupted; };
struct TechNodeV { int id; std::string name; bool unlocked; };
struct EventV { int id; std::string title, status; };
struct WeatherRegionV { std::string name; std::string weather; float visibility; };
struct NavalFleetV { int id; std::string name; int ships; std::string location; };
void ForEachFaction(const std::function<void(const FactionV&)>& cb);
void ForEachRelation(const std::function<void(const DipRelationV&)>& cb);
void ForEachAgent(const std::function<void(const AgentV&)>& cb);
void ForEachMarketGood(const std::function<void(const MarketGoodV&)>& cb);
void ForEachArmy(const std::function<void(const ArmyV&)>& cb);
void ForEachPopulation(const std::function<void(const PopulationRegionV&)>& cb);
void ForEachTradeRoute(const std::function<void(const TradeRouteV&)>& cb);
void ForEachTech(const std::function<void(const TechNodeV&)>& cb);
void ForEachEvent(const std::function<void(const EventV&)>& cb);
void ForEachWeather(const std::function<void(const WeatherRegionV&)>& cb);
void ForEachFleet(const std::function<void(const NavalFleetV&)>& cb);
} // namespace engine