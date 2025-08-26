
#pragma once
#include <functional>
#include <string>
#include <vector>
namespace core { class SimulationState; }
namespace core::bus {
enum class CommandType { ProposeTruce, OfferAlliance, DeclareWar, RaiseLevies, DispatchAgent, RedirectTradeRoute, InvestTech, InvestEconomy, SuppressUnrest, MoveArmy, BuildShips, ToggleRoute, AcknowledgeEvent, Custom };
struct Command { CommandType type=CommandType::Custom; int idA=0, idB=0; std::string s1, s2; double v1=0.0; bool flag=false; };
using Handler = std::function<void(const Command&)>;
void push(const Command&); void registerHandler(const Handler&); void clear(); std::size_t pending(); void process();
} // namespace core::bus
