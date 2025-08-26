
#pragma once
#include <functional>
namespace core { class SimulationState; }
namespace core::feeds {
using Feed = std::function<void(SimulationState&)>;
void setFactionsFeed(Feed); void setDiplomacyFeed(Feed); void setAgentsFeed(Feed);
void setMarketFeed(Feed); void setArmiesFeed(Feed); void setPopulationsFeed(Feed);
void setTradeRoutesFeed(Feed); void setTechFeed(Feed); void setEventsFeed(Feed);
void setWeatherFeed(Feed); void setFleetsFeed(Feed);
void applyAll(SimulationState& sim); void clearAllFeeds(); bool hasAnyFeed();
} // namespace core::feeds
