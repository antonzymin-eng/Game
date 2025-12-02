#include "ui/EconomyWindow.h"
#include "ui/WindowManager.h"
#include "ui/Toast.h"

namespace ui {

EconomyWindow::EconomyWindow(core::ecs::EntityManager& entity_manager,
                             game::economy::EconomicSystem& economic_system,
                             game::province::ProvinceSystem& province_system)
    : entity_manager_(entity_manager)
    , economic_system_(economic_system)
    , province_system_(province_system) {
}

void EconomyWindow::Render(WindowManager& window_manager, game::types::EntityID player_entity) {
    current_player_entity_ = player_entity;

    if (!window_manager.BeginManagedWindow(WindowManager::WindowType::ECONOMY, "Economy")) {
        return;
    }

    // Tab bar
    if (ImGui::BeginTabBar("EconomyTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Treasury")) {
            RenderTreasuryTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Income")) {
            RenderIncomeTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Expenses")) {
            RenderExpensesTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Buildings")) {
            RenderBuildingsTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Development")) {
            RenderDevelopmentTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    window_manager.EndManagedWindow();
}

void EconomyWindow::RenderTreasuryTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("TREASURY");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Get real data from economic system
    int treasury = economic_system_.GetTreasury(current_player_entity_);
    int monthly_income = economic_system_.GetMonthlyIncome(current_player_entity_);
    int monthly_expenses = economic_system_.GetMonthlyExpenses(current_player_entity_);
    int net_income = economic_system_.GetNetIncome(current_player_entity_);

    // Treasury overview
    ImGui::Columns(2, "treasury", false);
    ImGui::SetColumnWidth(0, 200);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

    ImGui::Text("Current Treasury:");
    ImGui::NextColumn();
    ImGui::Text("$%d", treasury);
    ImGui::NextColumn();

    ImGui::Text("Monthly Income:");
    ImGui::NextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
    ImGui::Text("+$%d", monthly_income);
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::Text("Monthly Expenses:");
    ImGui::NextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Red
    ImGui::Text("-$%d", monthly_expenses);
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::Text("Net Income:");
    ImGui::NextColumn();
    if (net_income >= 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
        ImGui::Text("+$%d", net_income);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Red
        ImGui::Text("$%d", net_income);
    }
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::PopStyleColor();

    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Quick Actions
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("QUICK ACTIONS");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Action buttons in a row
    if (ImGui::Button("Borrow Money", ImVec2(150, 0))) {
        if (current_player_entity_ != 0) {
            economic_system_.AddMoney(current_player_entity_, EconomyWindow::LOAN_AMOUNT);
            Toast::ShowSuccess("Borrowed $1,000 (interest tracking not yet implemented)");
            // Note: Interest payments would be tracked separately in a full implementation
        } else {
            Toast::ShowError("Cannot borrow money: Invalid player entity");
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Take a loan (+$%d)\nNote: Interest tracking not yet implemented", EconomyWindow::LOAN_AMOUNT);
    }

    ImGui::SameLine();
    if (ImGui::Button("Emergency Tax", ImVec2(150, 0))) {
        if (current_player_entity_ != 0) {
            economic_system_.AddMoney(current_player_entity_, EconomyWindow::EMERGENCY_TAX_REVENUE);
            Toast::ShowWarning("Emergency tax collected: +$500 (stability effects not yet implemented)");
            // Note: Stability reduction would be handled by a separate system in full implementation
        } else {
            Toast::ShowError("Cannot levy emergency tax: Invalid player entity");
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Levy emergency taxes (+$%d)\nNote: Stability effects not yet implemented", EconomyWindow::EMERGENCY_TAX_REVENUE);
    }

    ImGui::SameLine();
    if (ImGui::Button("Send Gift", ImVec2(150, 0))) {
        if (current_player_entity_ != 0) {
            int current_treasury = economic_system_.GetTreasury(current_player_entity_);
            if (current_treasury >= EconomyWindow::DIPLOMATIC_GIFT_AMOUNT) {
                bool success = economic_system_.SpendMoney(current_player_entity_, EconomyWindow::DIPLOMATIC_GIFT_AMOUNT);
                if (success) {
                    Toast::ShowSuccess("Gift sent: -$200 (diplomatic effects not yet implemented)");
                } else {
                    Toast::ShowError("Failed to send gift.");
                }
                // Note: Diplomatic effects would be handled by DiplomacySystem in full implementation
            } else {
                Toast::ShowError("Insufficient funds to send gift (need $200)");
            }
        } else {
            Toast::ShowError("Cannot send gift: Invalid player entity");
        }
    }
    if (ImGui::IsItemHovered()) {
        int current_treasury = economic_system_.GetTreasury(current_player_entity_);
        if (current_treasury >= EconomyWindow::DIPLOMATIC_GIFT_AMOUNT) {
            ImGui::SetTooltip("Send monetary gift (-$%d)\nNote: Diplomatic effects not yet implemented", EconomyWindow::DIPLOMATIC_GIFT_AMOUNT);
        } else {
            ImGui::SetTooltip("Insufficient funds (need $%d)", EconomyWindow::DIPLOMATIC_GIFT_AMOUNT);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Treasury History");
    ImGui::PopStyleColor();

    // TODO: Add graph showing treasury over time
    ImGui::Text("(Treasury graph will be displayed here)");
}

void EconomyWindow::RenderIncomeTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("INCOME SOURCES");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(3, "income", false);
    ImGui::SetColumnWidth(0, 200);
    ImGui::SetColumnWidth(1, 100);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

    // Header
    ImGui::Text("Source");
    ImGui::NextColumn();
    ImGui::Text("Amount");
    ImGui::NextColumn();
    ImGui::Text("Percentage");
    ImGui::NextColumn();

    ImGui::Separator();

    // Income sources
    const char* sources[] = {"Taxes", "Trade", "Production", "Vassals", "Other"};
    for (const char* source : sources) {
        ImGui::Text("%s", source);
        ImGui::NextColumn();
        ImGui::Text("$0");
        ImGui::NextColumn();
        ImGui::Text("0%%");
        ImGui::NextColumn();
    }

    ImGui::Separator();

    ImGui::Text("TOTAL");
    ImGui::NextColumn();
    ImGui::Text("$0");
    ImGui::NextColumn();
    ImGui::Text("100%%");
    ImGui::NextColumn();

    ImGui::PopStyleColor();

    ImGui::Columns(1);

    // Tax Rate Controls
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("TAX POLICY");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::Text("Base Tax Rate:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);

    // Convert to percentage for display (0.0-0.5 → 0-50)
    float tax_rate_percent = tax_rate_slider_ * 100.0f;
    if (ImGui::SliderFloat("##tax_rate", &tax_rate_percent, 0.0f, 50.0f, "%.1f%%")) {
        // Convert back to decimal (0-50 → 0.0-0.5)
        tax_rate_slider_ = tax_rate_percent / 100.0f;
        // Note: Full implementation would call economic_system_.SetTaxRate()
        // For now, this updates the slider value for visual feedback
        // The actual tax rate would need to be stored in an EconomicComponent
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Adjust the base tax rate (affects income and stability)");
    }

    ImGui::Spacing();

    // Show estimated impact
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::Text("Estimated monthly income change: +$%.0f", tax_rate_slider_ * 1000.0f); // Rough estimate
    ImGui::Text("Stability impact: %.1f", -tax_rate_slider_ * 20.0f); // Higher taxes = lower stability
    ImGui::PopStyleColor();
}

void EconomyWindow::RenderExpensesTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("EXPENSES");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(3, "expenses", false);
    ImGui::SetColumnWidth(0, 200);
    ImGui::SetColumnWidth(1, 100);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));

    // Header
    ImGui::Text("Category");
    ImGui::NextColumn();
    ImGui::Text("Amount");
    ImGui::NextColumn();
    ImGui::Text("Percentage");
    ImGui::NextColumn();

    ImGui::Separator();

    // Expense categories
    const char* categories[] = {"Army Maintenance", "Navy Maintenance", "Advisors", "Interest", "Other"};
    for (const char* category : categories) {
        ImGui::Text("%s", category);
        ImGui::NextColumn();
        ImGui::Text("$0");
        ImGui::NextColumn();
        ImGui::Text("0%%");
        ImGui::NextColumn();
    }

    ImGui::Separator();

    ImGui::Text("TOTAL");
    ImGui::NextColumn();
    ImGui::Text("$0");
    ImGui::NextColumn();
    ImGui::Text("100%%");
    ImGui::NextColumn();

    ImGui::PopStyleColor();

    ImGui::Columns(1);
}

namespace {
    // Helper function to get building name string
    const char* GetBuildingName(game::province::ProductionBuilding type) {
        using game::province::ProductionBuilding;
        switch (type) {
            case ProductionBuilding::FARM: return "Farm";
            case ProductionBuilding::MARKET: return "Market";
            case ProductionBuilding::SMITHY: return "Smithy";
            case ProductionBuilding::WORKSHOP: return "Workshop";
            case ProductionBuilding::MINE: return "Mine";
            case ProductionBuilding::TEMPLE: return "Temple";
            default: return "Unknown";
        }
    }
}

void EconomyWindow::RenderBuildingsTab() {
    using namespace game::province;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("PROVINCE BUILDINGS");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Province selector
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Select Province:");
    ImGui::PopStyleColor();
    ImGui::SameLine();

    // Get all provinces and filter to player-owned only
    auto all_provinces = province_system_.GetAllProvinces();
    std::vector<game::types::EntityID> player_provinces;

    for (const auto& province_id : all_provinces) {
        auto* province_data = province_system_.GetProvinceData(province_id);
        if (province_data && province_data->owner_nation == current_player_entity_) {
            player_provinces.push_back(province_id);
        }
    }

    if (player_provinces.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No provinces owned by player");
        return;  // No style color to pop - TextColored manages its own style
    }

    // Province dropdown
    if (selected_province_for_building_ == 0 && !player_provinces.empty()) {
        selected_province_for_building_ = player_provinces[0];
    }

    std::string selected_name = province_system_.GetProvinceName(selected_province_for_building_);
    if (ImGui::BeginCombo("##ProvinceSelector", selected_name.c_str())) {
        for (const auto& province_id : player_provinces) {
            std::string prov_name = province_system_.GetProvinceName(province_id);
            bool is_selected = (province_id == selected_province_for_building_);
            if (ImGui::Selectable(prov_name.c_str(), is_selected)) {
                selected_province_for_building_ = province_id;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Building options
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Available Buildings");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Define building data
    struct BuildingInfo {
        ProductionBuilding type;
        const char* description;
        const char* benefit;
    };

    BuildingInfo buildings[] = {
        {ProductionBuilding::FARM, "Increases food production", "+20% Food Output"},
        {ProductionBuilding::MARKET, "Boosts trade income", "+15% Trade Income"},
        {ProductionBuilding::SMITHY, "Improves equipment quality", "+10% Military Production"},
        {ProductionBuilding::WORKSHOP, "Increases production efficiency", "+10% Production"},
        {ProductionBuilding::MINE, "Extracts mineral resources", "+25% Resource Income"},
        {ProductionBuilding::TEMPLE, "Improves stability and culture", "+5% Stability"}
    };

    int treasury = economic_system_.GetTreasury(current_player_entity_);

    for (const auto& building : buildings) {
        ImGui::PushID(static_cast<int>(building.type));

        int current_level = province_system_.GetBuildingLevel(selected_province_for_building_, building.type);
        int cost = static_cast<int>(province_system_.CalculateBuildingCost(building.type, current_level));
        double time_days = province_system_.CalculateConstructionTime(building.type, current_level);
        bool can_afford = treasury >= cost;
        bool can_build = province_system_.CanConstructBuilding(selected_province_for_building_, building.type);

        // Building info panel
        ImGui::BeginGroup();

        // Get building name using helper function
        const char* building_name = GetBuildingName(building.type);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("%s (Level %d)", building_name, current_level);
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("  %s", building.description);

        // Cost coloring based on affordability
        if (can_afford) {
            ImGui::Text("  Cost: $%d | Time: %.0f days | Benefit: %s",
                        cost, time_days, building.benefit);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                "  Cost: $%d (insufficient funds) | Time: %.0f days", cost, time_days);
        }
        ImGui::PopStyleColor();

        ImGui::EndGroup();

        // Build button
        ImGui::SameLine(ImGui::GetWindowWidth() - 150);

        if (!can_build || current_level >= 10) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Build", ImVec2(130, 0))) {
            if (province_system_.QueueBuilding(selected_province_for_building_, building.type)) {
                Toast::ShowSuccess("Queued %s for construction!", building_name);
            } else {
                Toast::ShowError("Failed to queue %s. Check funds and capacity.", building_name);
            }
        }

        if (!can_build || current_level >= 10) {
            ImGui::EndDisabled();
        }

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::BeginTooltip();
            ImGui::Text("Building: %s", building_name);
            ImGui::Text("Current Level: %d / 10", current_level);
            ImGui::Text("Upgrade Cost: $%d", cost);
            ImGui::Text("Construction Time: %.0f days", time_days);
            ImGui::Separator();
            if (current_level >= 10) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Maximum level reached");
            } else if (!can_afford) {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Insufficient funds");
                ImGui::Text("Treasury: $%d / $%d", treasury, cost);
            } else if (!can_build) {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Cannot build - check capacity");
            } else {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Ready to build!");
            }
            ImGui::EndTooltip();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PopID();
    }

    // Construction queue display
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("CONSTRUCTION QUEUE");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    auto queue = province_system_.GetConstructionQueue(selected_province_for_building_);

    if (queue.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("No buildings under construction");
        ImGui::PopStyleColor();
    } else {
        double progress = province_system_.GetConstructionProgress(selected_province_for_building_);

        for (size_t i = 0; i < queue.size(); ++i) {
            ProductionBuilding building_type = queue[i];

            // Get building name using helper function
            const char* building_name = GetBuildingName(building_type);

            ImGui::PushID(static_cast<int>(i));

            if (i == 0) {
                // Currently building - show progress bar
                ImGui::Text("%d. %s (Building...)", static_cast<int>(i + 1), building_name);
                ImGui::ProgressBar(static_cast<float>(progress), ImVec2(-150, 0));
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(140, 0))) {
                    if (province_system_.CancelConstruction(selected_province_for_building_, i)) {
                        Toast::ShowInfo("Cancelled construction of %s", building_name);
                    }
                }
            } else {
                // Queued
                ImGui::Text("%d. %s (Queued)", static_cast<int>(i + 1), building_name);
                ImGui::SameLine(ImGui::GetWindowWidth() - 160);
                if (ImGui::Button("Remove", ImVec2(140, 0))) {
                    if (province_system_.CancelConstruction(selected_province_for_building_, i)) {
                        Toast::ShowInfo("Removed %s from queue", building_name);
                    }
                }
            }

            ImGui::PopID();
        }
    }
}

void EconomyWindow::RenderDevelopmentTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("PROVINCE DEVELOPMENT");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Province development overview and improvement options");

    // TODO: Add province development interface
}

} // namespace ui
