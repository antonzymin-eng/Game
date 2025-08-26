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

struct NavalPanel {
    static void Draw(core::SimulationState& sim, float width);
    static void DrawDetails(core::SimulationState& sim);
};


    void NavalPanel::Draw(core::SimulationState&, float) {
        if (Begin("NavalPanel")) {
            Text("NavalPanel panel (placeholder)");
        }
        End();
    }

    void NavalPanel::DrawDetails(core::SimulationState&) {
        if (Begin("NavalPanel Details")) {
            Text("NavalPanel details (placeholder)");
        }
        End();
    }

    } } // namespace ui::panels
