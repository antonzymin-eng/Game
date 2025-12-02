#include "ui/EconomyWindow.h"
#include "ui/WindowManager.h"
#include "ui/Toast.h"
#include <cmath>
#include <algorithm>
#include <mutex>

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

    // Refresh cached component pointers once per frame for performance
    RefreshCachedComponents();

    // Sync tax rate slider with actual component value (only when not actively dragging)
    if (cached_data_.economic && !tax_slider_active_) {
        tax_rate_slider_ = static_cast<float>(cached_data_.economic->tax_rate);
    }

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

    // Get real income data (using cached components for performance)
    int tax_income = GetTaxIncome();
    int trade_income = GetTradeIncome();
    int production_income = GetProductionIncome();
    int tribute_income = GetTributeIncome();
    int other_income = GetOtherIncome();
    int total_income = tax_income + trade_income + production_income + tribute_income + other_income;

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

    // Helper lambda to render income row
    auto render_income_row = [&](const char* name, int amount, int total) {
        ImGui::Text("%s", name);
        ImGui::NextColumn();
        ImGui::Text("$%d", amount);
        ImGui::NextColumn();
        if (total > 0) {
            float percentage = (static_cast<float>(amount) / static_cast<float>(total)) * 100.0f;
            ImGui::Text("%.1f%%", percentage);
        } else {
            ImGui::Text("0%%");
        }
        ImGui::NextColumn();
    };

    // Income sources with real data
    render_income_row("Taxes", tax_income, total_income);
    render_income_row("Trade", trade_income, total_income);
    render_income_row("Production", production_income, total_income);
    render_income_row("Vassals", tribute_income, total_income);
    render_income_row("Other", other_income, total_income);

    ImGui::Separator();

    ImGui::Text("TOTAL");
    ImGui::NextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
    ImGui::Text("$%d", total_income);
    ImGui::PopStyleColor();
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
        // Apply the new tax rate to the economic component
        ApplyTaxRate(tax_rate_slider_);
        tax_slider_active_ = true;
    }

    // Debounced toast notification - only show when slider is released
    if (tax_slider_active_ && !ImGui::IsItemActive()) {
        // Check if value changed significantly (at least 0.5%)
        if (std::abs(previous_tax_rate_ - tax_rate_slider_) > 0.005f) {
            Toast::ShowInfo("Tax rate adjusted to " + std::to_string(static_cast<int>(tax_rate_percent)) + "%");
            previous_tax_rate_ = tax_rate_slider_;
        }
        tax_slider_active_ = false;
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Adjust the base tax rate (affects income and stability)");
    }

    ImGui::Spacing();

    // Show estimated impact based on current taxable population (use cached component)
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.55f, 0.48f, 1.0f));
    if (cached_data_.economic && cached_data_.economic->taxable_population > 0) {
        double estimated_income = cached_data_.economic->taxable_population *
                                 cached_data_.economic->average_wages *
                                 tax_rate_slider_ *
                                 cached_data_.economic->tax_collection_efficiency;
        ImGui::Text("Estimated monthly tax income: $%.0f", estimated_income);
        ImGui::Text("Current tax efficiency: %.0f%%", cached_data_.economic->tax_collection_efficiency * 100.0f);
    } else {
        ImGui::Text("Estimated monthly income change: +$%.0f", tax_rate_slider_ * 1000.0f); // Rough estimate
    }
    ImGui::Text("Stability impact: %.1f", -tax_rate_slider_ * 20.0f); // Higher taxes = lower stability
    ImGui::PopStyleColor();
}

void EconomyWindow::RenderExpensesTab() {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("EXPENSES");
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    // Get real expense data (using cached components for performance)
    int military_expenses = GetMilitaryExpenses();
    int administrative_expenses = GetAdministrativeExpenses();
    int infrastructure_expenses = GetInfrastructureExpenses();
    int interest_expenses = GetInterestExpenses();
    int other_expenses = GetOtherExpenses();
    int total_expenses = military_expenses + administrative_expenses + infrastructure_expenses + interest_expenses + other_expenses;

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

    // Helper lambda to render expense row
    auto render_expense_row = [&](const char* name, int amount, int total) {
        ImGui::Text("%s", name);
        ImGui::NextColumn();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Red for expenses
        ImGui::Text("$%d", amount);
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        if (total > 0) {
            float percentage = (static_cast<float>(amount) / static_cast<float>(total)) * 100.0f;
            ImGui::Text("%.1f%%", percentage);
        } else {
            ImGui::Text("0%%");
        }
        ImGui::NextColumn();
    };

    // Expense categories with real data
    render_expense_row("Military", military_expenses, total_expenses);
    render_expense_row("Administrative", administrative_expenses, total_expenses);
    render_expense_row("Infrastructure", infrastructure_expenses, total_expenses);
    render_expense_row("Interest", interest_expenses, total_expenses);
    render_expense_row("Other", other_expenses, total_expenses);

    ImGui::Separator();

    ImGui::Text("TOTAL");
    ImGui::NextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Red
    ImGui::Text("$%d", total_expenses);
    ImGui::PopStyleColor();
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
                std::string msg = std::string("Queued ") + building_name + " for construction!";
                Toast::ShowSuccess(msg.c_str());
            } else {
                std::string msg = std::string("Failed to queue ") + building_name + ". Check funds and capacity.";
                Toast::ShowError(msg.c_str());
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
                        std::string msg = std::string("Cancelled construction of ") + building_name;
                        Toast::ShowInfo(msg.c_str());
                    }
                }
            } else {
                // Queued
                ImGui::Text("%d. %s (Queued)", static_cast<int>(i + 1), building_name);
                ImGui::SameLine(ImGui::GetWindowWidth() - 160);
                if (ImGui::Button("Remove", ImVec2(140, 0))) {
                    if (province_system_.CancelConstruction(selected_province_for_building_, i)) {
                        std::string msg = std::string("Removed ") + building_name + " from queue";
                        Toast::ShowInfo(msg.c_str());
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

// ============================================================================
// Component Caching
// ============================================================================

void EconomyWindow::RefreshCachedComponents() {
    if (current_player_entity_ == 0) {
        cached_data_.economic = nullptr;
        cached_data_.treasury = nullptr;
        return;
    }

    ::core::ecs::EntityID entity_handle(static_cast<uint64_t>(current_player_entity_), 1);
    cached_data_.economic = entity_manager_.GetComponent<game::economy::EconomicComponent>(entity_handle);
    cached_data_.treasury = entity_manager_.GetComponent<game::economy::TreasuryComponent>(entity_handle);
}

// ============================================================================
// Helper Methods for Income/Expense Tracking (Optimized with Cached Components)
// ============================================================================

int EconomyWindow::GetTaxIncome() const {
    if (!cached_data_.economic) return 0;

    // Thread-safe read using mutex if available
    if (cached_data_.economic->trade_routes_mutex.try_lock()) {
        int value = cached_data_.economic->tax_income;
        cached_data_.economic->trade_routes_mutex.unlock();
        return value;
    }
    // Fallback: read without lock (acceptable for display-only UI)
    return cached_data_.economic->tax_income;
}

int EconomyWindow::GetTradeIncome() const {
    if (!cached_data_.economic) return 0;

    // Thread-safe read using mutex
    if (cached_data_.economic->trade_routes_mutex.try_lock()) {
        int value = cached_data_.economic->trade_income;
        cached_data_.economic->trade_routes_mutex.unlock();
        return value;
    }
    return cached_data_.economic->trade_income;
}

int EconomyWindow::GetTributeIncome() const {
    if (!cached_data_.economic) return 0;
    return cached_data_.economic->tribute_income;
}

int EconomyWindow::GetProductionIncome() const {
    if (!cached_data_.economic) return 0;

    // Production income is not directly tracked as separate income
    // It's included in monthly_income calculations by the EconomicSystem
    // Return 0 here to avoid double-counting
    return 0;
}

int EconomyWindow::GetOtherIncome() const {
    // Other income sources (gifts, events, etc.)
    // Calculate as difference between total income and known sources
    int monthly_income = economic_system_.GetMonthlyIncome(current_player_entity_);
    int known_income = GetTaxIncome() + GetTradeIncome() + GetTributeIncome() + GetProductionIncome();
    int other = monthly_income - known_income;
    return other > 0 ? other : 0;
}

int EconomyWindow::GetMilitaryExpenses() const {
    return cached_data_.treasury ? cached_data_.treasury->military_expenses : 0;
}

int EconomyWindow::GetAdministrativeExpenses() const {
    return cached_data_.treasury ? cached_data_.treasury->administrative_expenses : 0;
}

int EconomyWindow::GetInfrastructureExpenses() const {
    return cached_data_.economic ? cached_data_.economic->infrastructure_investment : 0;
}

int EconomyWindow::GetInterestExpenses() const {
    return cached_data_.treasury ? cached_data_.treasury->debt_payments : 0;
}

int EconomyWindow::GetOtherExpenses() const {
    // Other expenses
    int monthly_expenses = economic_system_.GetMonthlyExpenses(current_player_entity_);
    int known_expenses = GetMilitaryExpenses() + GetAdministrativeExpenses() +
                        GetInfrastructureExpenses() + GetInterestExpenses();
    int other = monthly_expenses - known_expenses;
    return other > 0 ? other : 0;
}

void EconomyWindow::ApplyTaxRate(float new_tax_rate) {
    if (current_player_entity_ == 0 || !cached_data_.economic) {
        return;
    }

    // Clamp tax rate to valid range [0.0, 0.5]
    new_tax_rate = std::max(0.0f, std::min(0.5f, new_tax_rate));

    // Thread-safe modification using component's mutex
    std::lock_guard<std::mutex> lock(cached_data_.economic->trade_routes_mutex);

    // Update tax rate
    cached_data_.economic->tax_rate = new_tax_rate;

    // Let EconomicSystem handle the recalculation to avoid duplication
    // The monthly update will recalculate with the new rate
    // For immediate visual feedback, do a quick recalculation here
    if (cached_data_.economic->taxable_population > 0) {
        // Use same formula as EconomicSystem::CalculateMonthlyTotals (line 407-427)
        cached_data_.economic->tax_income = static_cast<int>(
            cached_data_.economic->taxable_population *
            cached_data_.economic->average_wages *
            cached_data_.economic->tax_rate *
            cached_data_.economic->tax_collection_efficiency
        );

        // Update monthly totals
        cached_data_.economic->monthly_income = cached_data_.economic->tax_income +
                                               cached_data_.economic->trade_income +
                                               cached_data_.economic->tribute_income;
        cached_data_.economic->net_income = cached_data_.economic->monthly_income -
                                           cached_data_.economic->monthly_expenses;
    }
}

} // namespace ui
