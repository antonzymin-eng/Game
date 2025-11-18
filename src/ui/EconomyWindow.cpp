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

    ImGui::Text("Available buildings and construction queue will be displayed here");

    // TODO: Add building categories, construction queue, etc.
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
