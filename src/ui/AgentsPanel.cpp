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

struct AgentsPanel {
    static void Draw(core::SimulationState& sim, float width);
    static void DrawDetails(core::SimulationState& sim);
};


    void AgentsPanel::Draw(core::SimulationState&, float) {
        if (Begin("AgentsPanel")) {
            Text("AgentsPanel panel (placeholder)");
        }
        End();
    }

    void AgentsPanel::DrawDetails(core::SimulationState&) {
        if (Begin("AgentsPanel Details")) {
            Text("AgentsPanel details (placeholder)");
        }
        End();
    }

    } } // namespace ui::panels
