#include "ui/DiplomacyWindow.h"
#include "ui/WindowManager.h"
#include "ui/PortraitGenerator.h"
#include "game/components/CharacterComponent.h"

namespace ui {

DiplomacyWindow::DiplomacyWindow(core::ecs::EntityManager& entity_manager,
                                 game::diplomacy::DiplomacySystem& diplomacy_system)
    : entity_manager_(entity_manager)
    , diplomacy_system_(diplomacy_system)
    , portrait_generator_(nullptr) {
}

void DiplomacyWindow::Render(WindowManager& window_manager, game::types::EntityID player_entity) {
    (void)player_entity; // Mark as unused for now

    if (!window_manager.BeginManagedWindow(WindowManager::WindowType::DIPLOMACY, "Diplomacy")) {
        return;
    }

    if (ImGui::BeginTabBar("DiplomacyTabs")) {
        if (ImGui::BeginTabItem("Overview")) {
            RenderOverviewTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Relations")) {
            RenderRelationsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Treaties")) {
            RenderTreatiesTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Wars")) {
            RenderWarTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    window_manager.EndManagedWindow();
}

void DiplomacyWindow::RenderOverviewTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("DIPLOMATIC OVERVIEW");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Text("Diplomatic relations and status");
}

void DiplomacyWindow::RenderRelationsTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("RELATIONS");
    ImGui::PopStyleColor();
    ImGui::Separator();

    // Placeholder text explaining portrait integration
    ImGui::TextWrapped("Relation standings with other nations");
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
                       "Portrait system ready - portraits will appear here when character data is available");

    // Example of how portraits would be displayed
    if (portrait_generator_) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("When integrated with character system, each nation's ruler portrait will appear here:");
        ImGui::Spacing();

        // Demonstration layout for future character portraits
        ImGui::BeginChild("RelationsList", ImVec2(0, 0), true);

        ImGui::Text("Example: [64x64 Portrait] Nation Name - Opinion: +50 (Friendly)");
        ImGui::Text("Future: Portraits will be generated procedurally for each ruler");

        ImGui::EndChild();
    }
}

void DiplomacyWindow::RenderTreatiesTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("TREATIES & AGREEMENTS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Active treaties
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Active Treaties:");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    struct Treaty {
        const char* type;
        const char* partner;
        int years_remaining;
    };

    Treaty treaties[] = {
        {"Alliance", "Francia", 10},
        {"Trade Agreement", "Byzantine Empire", 5},
        {"Non-Aggression Pact", "Kingdom of England", 8}
    };

    for (const auto& treaty : treaties) {
        ImGui::PushID(treaty.partner);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("%s", treaty.type);
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("  Partner: %s | Years Remaining: %d", treaty.partner, treaty.years_remaining);
        ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetWindowWidth() - 150);
        if (ImGui::Button("Break Treaty", ImVec2(130, 0))) {
            // TODO: Implement break treaty
            // diplomacy_system_.BreakTreaty(current_player_entity_, treaty_id);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Breaking a treaty will damage relations and reputation");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PopID();
    }

    // Propose new treaty
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("PROPOSE NEW TREATY");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    if (ImGui::Button("Propose Alliance", ImVec2(150, 0))) {
        // TODO: Open nation selector for alliance
    }
    ImGui::SameLine();
    if (ImGui::Button("Propose Trade Deal", ImVec2(150, 0))) {
        // TODO: Open nation selector for trade
    }
    ImGui::SameLine();
    if (ImGui::Button("Propose Peace", ImVec2(150, 0))) {
        // TODO: Open peace negotiation dialog
    }
}

void DiplomacyWindow::RenderWarTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("WARS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Active wars
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Active Wars:");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::Text("Currently at peace");
    ImGui::PopStyleColor();

    // TODO: Display active wars with:
    // - Enemy nation
    // - War score
    // - Battles won/lost
    // - Sue for peace button

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Declare war
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("DECLARE WAR");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::TextWrapped("Select a nation and casus belli (justification) to declare war:");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    if (ImGui::Button("Declare War", ImVec2(150, 0))) {
        // TODO: Open nation selector and CB selection dialog
        // diplomacy_system_.DeclareWar(current_player_entity_, target_nation, casus_belli);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Requires a valid casus belli to avoid diplomatic penalties");
    }
}

} // namespace ui
