
#include "imgui.h"
namespace ui {
    struct AppState; // forward-declare; not needed for this simple implementation

    // Draw a very simple Credits window
    void DrawCredits(AppState&) {
        ImGui::Begin("Credits");
        ImGui::Text("Mechanica Imperii");
        ImGui::Separator();
        ImGui::Text("Programming / Design: <your name here>");
        ImGui::End();
    }
} // namespace ui
