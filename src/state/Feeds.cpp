
#include "state/Feeds.h"
#include "state/SimulationState.h"
namespace core::feeds {
static Feed g_factions, g_diplomacy, g_agents, g_market, g_armies, g_pops, g_routes, g_tech, g_events, g_weather, g_fleets;
void setFactionsFeed(Feed f){ g_factions=std::move(f);} void setDiplomacyFeed(Feed f){ g_diplomacy=std::move(f);}
void setAgentsFeed(Feed f){ g_agents=std::move(f);} void setMarketFeed(Feed f){ g_market=std::move(f);}
void setArmiesFeed(Feed f){ g_armies=std::move(f);} void setPopulationsFeed(Feed f){ g_pops=std::move(f);}
void setTradeRoutesFeed(Feed f){ g_routes=std::move(f);} void setTechFeed(Feed f){ g_tech=std::move(f);}
void setEventsFeed(Feed f){ g_events=std::move(f);} void setWeatherFeed(Feed f){ g_weather=std::move(f);}
void setFleetsFeed(Feed f){ g_fleets=std::move(f);}
void applyAll(SimulationState& s){ if(g_factions)g_factions(s); if(g_diplomacy)g_diplomacy(s); if(g_agents)g_agents(s);
 if(g_market)g_market(s); if(g_armies)g_armies(s); if(g_pops)g_pops(s); if(g_routes)g_routes(s); if(g_tech)g_tech(s);
 if(g_events)g_events(s); if(g_weather)g_weather(s); if(g_fleets)g_fleets(s); }
void clearAllFeeds(){ g_factions={}; g_diplomacy={}; g_agents={}; g_market={}; g_armies={}; g_pops={}; g_routes={}; g_tech={}; g_events={}; g_weather={}; g_fleets={}; }
bool hasAnyFeed(){ return (bool)g_factions||(bool)g_diplomacy||(bool)g_agents||(bool)g_market||(bool)g_armies||(bool)g_pops||(bool)g_routes||(bool)g_tech||(bool)g_events||(bool)g_weather||(bool)g_fleets; }
} // namespace core::feeds
