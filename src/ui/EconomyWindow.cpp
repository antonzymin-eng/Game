#include "ui/EconomyWindow.h"
#include "ui/WindowManager.h"

namespace ui {

EconomyWindow::EconomyWindow(core::ecs::EntityManager& entity_manager,
                             game::economy::EconomicSystem& economic_system)
    : entity_manager_(entity_manager)
    , economic_system_(economic_system) {
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
        // Simple implementation: Add $1000 to treasury
        // In a full implementation, this would open a dialog with amount slider and interest rate
        if (current_player_entity_ != 0) {
            economic_system_.AddMoney(current_player_entity_, 1000);
            // Note: Interest payments would be tracked separately in a full implementation
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Take a loan to increase treasury (+$1000)");
    }

    ImGui::SameLine();
    if (ImGui::Button("Emergency Tax", ImVec2(150, 0))) {
        // Levy emergency tax: Add money but reduce stability
        if (current_player_entity_ != 0) {
            economic_system_.AddMoney(current_player_entity_, 500);
            // Note: Stability reduction would be handled by a separate system in full implementation
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Levy emergency taxes (+$500)");
    }

    ImGui::SameLine();
    if (ImGui::Button("Send Gift", ImVec2(150, 0))) {
        // Simple implementation: Spend $200 from treasury
        // In a full implementation, this would open a nation selector and amount dialog
        if (current_player_entity_ != 0) {
            economic_system_.SpendMoney(current_player_entity_, 200);
            // Note: Diplomatic effects would be handled by DiplomacySystem in full implementation
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Send monetary gift to improve relations (-$200)");
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

void EconomyWindow::RenderBuildingsTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("BUILDINGS");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Building list with construction UI
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.79f, 0.66f, 0.38f, 1.0f));
    ImGui::Text("Available Buildings");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    // Building table
    struct Building {
        const char* name;
        const char* description;
        int cost;
        int time_days;
        const char* benefit;
    };

    Building buildings[] = {
        {"Workshop", "Increases production efficiency", 500, 180, "+10% Production"},
        {"Market", "Boosts trade income", 750, 240, "+15% Trade Income"},
        {"Barracks", "Enables unit recruitment", 600, 150, "Unlocks units"},
        {"Temple", "Improves stability and culture", 800, 300, "+5 Stability"},
        {"University", "Accelerates research", 1200, 360, "+20% Research"}
    };

    for (const auto& building : buildings) {
        ImGui::PushID(building.name);

        // Building info panel
        ImGui::BeginGroup();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
        ImGui::Text("%s", building.name);
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
        ImGui::Text("  %s", building.description);
        ImGui::Text("  Cost: $%d | Time: %d days | Benefit: %s",
                    building.cost, building.time_days, building.benefit);
        ImGui::PopStyleColor();

        ImGui::EndGroup();

        // Build button on same line
        ImGui::SameLine(ImGui::GetWindowWidth() - 120);
        if (ImGui::Button("Build", ImVec2(100, 0))) {
            // Simple implementation: Spend money from treasury to "build"
            if (current_player_entity_ != 0) {
                if (economic_system_.SpendMoney(current_player_entity_, building.cost)) {
                    // Building purchased successfully
                    // Note: Full implementation would:
                    // 1. Add building to a construction queue
                    // 2. Track construction progress over time
                    // 3. Apply building effects when complete
                    // 4. Store building data in a component
                }
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Start construction of %s (Cost: $%d)", building.name, building.cost);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PopID();
    }

    // Construction queue
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("CONSTRUCTION QUEUE");
    ImGui::PopStyleColor();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    ImGui::Text("No buildings under construction");
    ImGui::PopStyleColor();
    // TODO: Display active construction queue with progress bars
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
