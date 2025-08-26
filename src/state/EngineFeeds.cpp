
#include "state/EngineFeeds.h"
#include "state/Feeds.h"
#include "state/SimulationState.h"
namespace core {
void RegisterEngineFeeds(SimulationState& /*sim*/) {
// Uncomment and map your engine data into SimulationState containers, e.g.:
// core::feeds::setMarketFeed([](SimulationState& s){ auto& m=s.market(); m.clear(); /* populate fields here */ });
}
} // namespace core
