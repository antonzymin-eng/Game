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

struct FactionsPanel {
    static void Draw(core::SimulationState& sim, float width);
    static void DrawDetails(core::SimulationState& sim);
};


    void FactionsPanel::Draw(core::SimulationState&, float) {
        if (Begin("FactionsPanel")) {
            Text("FactionsPanel panel (placeholder)");
        }
        End();
    }

    void FactionsPanel::DrawDetails(core::SimulationState&) {
        if (Begin("FactionsPanel Details")) {
            Text("FactionsPanel details (placeholder)");
        }
        End();
    }

    } } // namespace ui::panels
