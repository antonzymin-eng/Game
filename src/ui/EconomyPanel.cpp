#include "imgui.h"



// Minimal forward declarations to avoid including the whole project headers
namespace core { class SimulationState; }

namespace ui { namespace panels {
struct FactionsPanel; struct AgentsPanel; struct EconomyPanel; struct TradePanel;
struct RoutesPanel; struct MilitaryPanel; struct NavalPanel; struct TechPanel;
struct PopulationPanel; struct EventsPanel;
} } // namespaces

    using namespace ImGui;

    namespace ui { namespace panels {

struct EconomyPanel {
    static void Draw(core::SimulationState& sim, float width);
    static void DrawDetails(core::SimulationState& sim);
};


    void EconomyPanel::Draw(core::SimulationState&, float) {
        if (Begin("EconomyPanel")) {
            Text("EconomyPanel panel (placeholder)");
        }
        End();
    }

    void EconomyPanel::DrawDetails(core::SimulationState&) {
        if (Begin("EconomyPanel Details")) {
            Text("EconomyPanel details (placeholder)");
        }
        End();
    }

    } } // namespace ui::panels
