#include "ui/TradeSystemWindow.h"
#include "imgui.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace ui {

TradeSystemWindow::TradeSystemWindow(
    ::core::ecs::EntityManager& entity_manager,
    game::map::MapRenderer& map_renderer,
    game::trade::TradeSystem& trade_system,
    game::economy::EconomicSystem& economic_system
)
    : entity_manager_(entity_manager),
      map_renderer_(map_renderer),
      trade_system_(trade_system),
      economic_system_(economic_system),
      visible_(true),
      current_tab_(0),
      selected_province_(0),
      selected_route_id_(""),
      filter_profitable_only_(false),
      filter_active_only_(true),
      selected_resource_filter_(game::types::ResourceType::FOOD),  // Was GRAIN
      last_cache_update_(0.0) {
}

void TradeSystemWindow::Render() {
    if (!visible_) {
        return;
    }

    // Update cached data periodically
    UpdateCachedData();

    // Set window position and size
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->WorkPos;
    ImVec2 work_size = viewport->WorkSize;

    // Position window on the left side, below game controls
    ImVec2 window_pos = ImVec2(work_pos.x + 10, work_pos.y + 70);
    ImVec2 window_size = ImVec2(700, work_size.y - 100);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);

    // Window flags
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;

    if (ImGui::Begin("Trade System", &visible_, window_flags)) {
        RenderHeader();

        ImGui::Separator();

        // Tab bar for different views
        if (ImGui::BeginTabBar("TradeSystemTabs", ImGuiTabBarFlags_None)) {
            if (ImGui::BeginTabItem("Trade Routes")) {
                RenderTradeRoutesTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Trade Hubs")) {
                RenderTradeHubsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Market Analysis")) {
                RenderMarketAnalysisTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Opportunities")) {
                RenderOpportunitiesTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void TradeSystemWindow::RenderHeader() {
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Trade Network Overview");

    // Display global statistics
    auto all_routes = trade_system_.GetAllTradeRoutes();
    auto all_hubs = trade_system_.GetAllTradeHubs();

    int active_routes = 0;
    int disrupted_routes = 0;
    double total_volume = 0.0;
    double total_profit = 0.0;

    for (const auto& route : all_routes) {
        if (route.status == game::trade::TradeStatus::ACTIVE) {
            active_routes++;
            total_volume += route.current_volume;
            total_profit += route.profitability * route.current_volume;
        } else if (route.status == game::trade::TradeStatus::DISRUPTED) {
            disrupted_routes++;
        }
    }

    ImGui::Spacing();
    ImGui::Columns(4, "TradeStats", false);

    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Active Routes");
    ImGui::Text("%d", active_routes);
    ImGui::NextColumn();

    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Disrupted Routes");
    ImGui::Text("%d", disrupted_routes);
    ImGui::NextColumn();

    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Trade Hubs");
    ImGui::Text("%zu", all_hubs.size());
    ImGui::NextColumn();

    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "Monthly Volume");
    ImGui::Text("%.0f", total_volume);
    ImGui::NextColumn();

    ImGui::Columns(1);
    ImGui::Spacing();
}

void TradeSystemWindow::RenderTradeRoutesTab() {
    ImGui::Spacing();

    // Filters
    ImGui::Text("Filters:");
    ImGui::SameLine();
    ImGui::Checkbox("Active Only", &filter_active_only_);
    ImGui::SameLine();
    ImGui::Checkbox("Profitable Only", &filter_profitable_only_);
    ImGui::SameLine();

    // Search box
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("Search", search_buffer_, sizeof(search_buffer_));

    ImGui::Separator();
    ImGui::Spacing();

    // Get all routes and apply filters
    auto all_routes = trade_system_.GetAllTradeRoutes();
    std::vector<game::trade::TradeRoute> filtered_routes;

    for (const auto& route : all_routes) {
        // Apply filters
        if (filter_active_only_ && route.status != game::trade::TradeStatus::ACTIVE) {
            continue;
        }
        if (filter_profitable_only_ && route.profitability <= 0.0) {
            continue;
        }

        // Apply search filter
        if (search_buffer_[0] != '\0') {
            std::string search_term = search_buffer_;
            std::string source_name = GetProvinceName(route.source_province);
            std::string dest_name = GetProvinceName(route.destination_province);
            std::string resource_name = GetResourceName(route.resource);

            std::transform(search_term.begin(), search_term.end(), search_term.begin(), ::tolower);
            std::transform(source_name.begin(), source_name.end(), source_name.begin(), ::tolower);
            std::transform(dest_name.begin(), dest_name.end(), dest_name.begin(), ::tolower);
            std::transform(resource_name.begin(), resource_name.end(), resource_name.begin(), ::tolower);

            if (source_name.find(search_term) == std::string::npos &&
                dest_name.find(search_term) == std::string::npos &&
                resource_name.find(search_term) == std::string::npos) {
                continue;
            }
        }

        filtered_routes.push_back(route);
    }

    ImGui::Text("Showing %zu of %zu routes", filtered_routes.size(), all_routes.size());
    ImGui::Spacing();

    // Trade routes table
    if (ImGui::BeginTable("TradeRoutesTable", 8,
                          ImGuiTableFlags_Borders |
                          ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_ScrollY |
                          ImGuiTableFlags_Sortable |
                          ImGuiTableFlags_Resizable)) {

        ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Destination", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Resource", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("Profit %", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("Efficiency", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableHeadersRow();

        for (const auto& route : filtered_routes) {
            ImGui::TableNextRow();

            // Source
            ImGui::TableNextColumn();
            ImGui::Text("%s", GetProvinceName(route.source_province).c_str());

            // Destination
            ImGui::TableNextColumn();
            ImGui::Text("%s", GetProvinceName(route.destination_province).c_str());

            // Resource
            ImGui::TableNextColumn();
            const char* icon = GetResourceTypeIcon(route.resource);
            ImGui::Text("%s %s", icon, GetResourceName(route.resource).c_str());

            // Route Type
            ImGui::TableNextColumn();
            ImGui::Text("%s", GetRouteTypeName(route.route_type).c_str());

            // Volume
            ImGui::TableNextColumn();
            ImGui::Text("%.0f", route.current_volume);

            // Profitability
            ImGui::TableNextColumn();
            ImVec4 profit_color = GetProfitabilityColor(route.profitability);
            ImGui::TextColored(profit_color, "%.1f%%", route.profitability * 100.0);

            // Efficiency
            ImGui::TableNextColumn();
            double efficiency_percent = route.efficiency_rating * 100.0;
            ImVec4 efficiency_color = efficiency_percent > 100.0 ?
                ImVec4(0.2f, 0.8f, 0.2f, 1.0f) :
                (efficiency_percent > 75.0 ? ImVec4(0.8f, 0.8f, 0.2f, 1.0f) : ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::TextColored(efficiency_color, "%.0f%%", efficiency_percent);

            // Status
            ImGui::TableNextColumn();
            ImVec4 status_color = GetStatusColor(route.status);
            ImGui::TextColored(status_color, "%s", GetStatusName(route.status).c_str());

            // Make row selectable
            if (ImGui::IsItemClicked() || ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                selected_route_id_ = route.route_id;
            }
        }

        ImGui::EndTable();
    }

    // Display selected route details in a child window
    if (!selected_route_id_.empty()) {
        ImGui::Spacing();
        ImGui::Separator();
        RenderRouteDetailsPanel();
    }
}

void TradeSystemWindow::RenderTradeHubsTab() {
    ImGui::Spacing();

    auto all_hubs = trade_system_.GetAllTradeHubs();

    ImGui::Text("Total Trade Hubs: %zu", all_hubs.size());
    ImGui::Spacing();

    // Sort hubs by utilization
    std::vector<game::trade::TradeHub> sorted_hubs = all_hubs;
    std::sort(sorted_hubs.begin(), sorted_hubs.end(),
              [](const auto& a, const auto& b) {
                  return a.current_utilization > b.current_utilization;
              });

    // Trade hubs table
    if (ImGui::BeginTable("TradeHubsTable", 6,
                          ImGuiTableFlags_Borders |
                          ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_ScrollY |
                          ImGuiTableFlags_Resizable)) {

        ImGui::TableSetupColumn("Province", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Hub Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Capacity", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Utilization", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Routes", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableHeadersRow();

        for (const auto& hub : sorted_hubs) {
            ImGui::TableNextRow();

            // Province
            ImGui::TableNextColumn();
            ImGui::Text("%s", GetProvinceName(hub.province_id).c_str());

            // Hub Name
            ImGui::TableNextColumn();
            ImGui::Text("%s", hub.hub_name.c_str());

            // Hub Type
            ImGui::TableNextColumn();
            ImGui::Text("%s", GetHubTypeName(hub.hub_type).c_str());

            // Capacity
            ImGui::TableNextColumn();
            ImGui::Text("%.0f", hub.max_throughput_capacity);

            // Utilization
            ImGui::TableNextColumn();
            double utilization_percent = (hub.current_utilization / hub.max_throughput_capacity) * 100.0;
            ImVec4 util_color = GetUtilizationColor(utilization_percent / 100.0);
            ImGui::TextColored(util_color, "%.1f%%", utilization_percent);

            // Routes count
            ImGui::TableNextColumn();
            int total_routes = hub.incoming_route_ids.size() + hub.outgoing_route_ids.size();
            ImGui::Text("%d", total_routes);

            // Make row selectable
            if (ImGui::IsItemClicked()) {
                selected_province_ = hub.province_id;
            }
        }

        ImGui::EndTable();
    }

    // Display selected hub details
    if (selected_province_ != 0) {
        ImGui::Spacing();
        ImGui::Separator();
        RenderHubDetailsPanel();
    }
}

void TradeSystemWindow::RenderMarketAnalysisTab() {
    ImGui::Spacing();

    // Get selected province from map renderer
    auto selected_province_data = map_renderer_.GetSelectedProvince();

    if (selected_province_data.id == 0) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f),
                          "Select a province on the map to view market data");
        return;
    }

    game::types::EntityID province_id = selected_province_data.id;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Market Data for: %s",
                      GetProvinceName(province_id).c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // Display market prices for all resources
    std::vector<game::types::ResourceType> resources = {
        game::types::ResourceType::FOOD,      // Was GRAIN
        game::types::ResourceType::WINE,
        game::types::ResourceType::CLOTH,
        game::types::ResourceType::IRON,
        game::types::ResourceType::WOOD,      // Was TOOLS
        game::types::ResourceType::STONE,     // Was POTTERY
        game::types::ResourceType::SPICES,    // Was OLIVE_OIL
        game::types::ResourceType::SALT
    };

    if (ImGui::BeginTable("MarketPricesTable", 5,
                          ImGuiTableFlags_Borders |
                          ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_Resizable)) {

        ImGui::TableSetupColumn("Resource", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Supply", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Demand", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Balance", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableHeadersRow();

        for (auto resource : resources) {
            double price = trade_system_.CalculateMarketPrice(province_id, resource);
            double supply = trade_system_.CalculateSupplyLevel(province_id, resource);
            double demand = trade_system_.CalculateDemandLevel(province_id, resource);
            double balance = supply - demand;

            ImGui::TableNextRow();

            // Resource name with icon
            ImGui::TableNextColumn();
            const char* icon = GetResourceTypeIcon(resource);
            ImGui::Text("%s %s", icon, GetResourceName(resource).c_str());

            // Price
            ImGui::TableNextColumn();
            ImGui::Text("%.2f", price);

            // Supply
            ImGui::TableNextColumn();
            ImVec4 supply_color = supply > 1.0 ? ImVec4(0.5f, 1.0f, 0.5f, 1.0f) : ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
            ImGui::TextColored(supply_color, "%.1f%%", supply * 100.0);

            // Demand
            ImGui::TableNextColumn();
            ImVec4 demand_color = demand > 1.0 ? ImVec4(1.0f, 0.5f, 0.5f, 1.0f) : ImVec4(0.5f, 1.0f, 0.5f, 1.0f);
            ImGui::TextColored(demand_color, "%.1f%%", demand * 100.0);

            // Balance
            ImGui::TableNextColumn();
            ImVec4 balance_color = balance > 0 ?
                ImVec4(0.5f, 1.0f, 0.5f, 1.0f) :
                ImVec4(1.0f, 0.5f, 0.5f, 1.0f);
            ImGui::TextColored(balance_color, "%+.1f%%", balance * 100.0);
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Display active trade routes for this province
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Active Trade Routes");
    ImGui::Spacing();

    auto all_routes = trade_system_.GetAllTradeRoutes();
    std::vector<game::trade::TradeRoute> province_routes;

    for (const auto& route : all_routes) {
        if (route.source_province == province_id || route.destination_province == province_id) {
            province_routes.push_back(route);
        }
    }

    if (province_routes.empty()) {
        ImGui::Text("No active trade routes");
    } else {
        ImGui::Text("Trade Routes: %zu", province_routes.size());

        for (const auto& route : province_routes) {
            bool is_export = (route.source_province == province_id);
            ImGui::BulletText("%s %s %s %s (%.0f units, %.1f%% profit)",
                             is_export ? "Exports" : "Imports",
                             GetResourceName(route.resource).c_str(),
                             is_export ? "to" : "from",
                             GetProvinceName(is_export ? route.destination_province : route.source_province).c_str(),
                             route.current_volume,
                             route.profitability * 100.0);
        }
    }
}

void TradeSystemWindow::RenderOpportunitiesTab() {
    ImGui::Spacing();

    // Get selected province from map renderer
    auto selected_province_data = map_renderer_.GetSelectedProvince();

    if (selected_province_data.id == 0) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f),
                          "Select a province on the map to view trade opportunities");
        return;
    }

    game::types::EntityID province_id = selected_province_data.id;

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Trade Opportunities for: %s",
                      GetProvinceName(province_id).c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // Find profitable route opportunities
    auto opportunities = trade_system_.FindProfitableRouteOpportunities(province_id);

    if (opportunities.empty()) {
        ImGui::Text("No profitable trade opportunities found at this time.");
        ImGui::TextWrapped("This could mean:");
        ImGui::BulletText("All profitable routes are already established");
        ImGui::BulletText("Market conditions are not favorable");
        ImGui::BulletText("Province has insufficient resources");
        return;
    }

    ImGui::Text("Found %zu profitable opportunities:", opportunities.size());
    ImGui::Spacing();

    // Display opportunities
    for (size_t i = 0; i < opportunities.size(); ++i) {
        const auto& opp_id = opportunities[i];

        // Parse opportunity data (format: "source_dest_resource")
        // For now, just display the opportunity ID
        // In a real implementation, you would parse this and get detailed info

        ImGui::PushID(static_cast<int>(i));

        if (ImGui::TreeNode(("Opportunity ##" + std::to_string(i)).c_str())) {
            ImGui::Text("Route ID: %s", opp_id.c_str());

            // You could add more details here by querying the trade system
            // with the parsed route information

            if (ImGui::Button("Establish Route")) {
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f),
                                  "Route establishment would be triggered here");
                // TODO: Call trade_system_.EstablishTradeRoute() with parsed parameters
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }
}

void TradeSystemWindow::RenderRouteDetailsPanel() {
    if (selected_route_id_.empty()) {
        return;
    }

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Route Details: %s", selected_route_id_.c_str());
    ImGui::Spacing();

    // Find the selected route
    auto all_routes = trade_system_.GetAllTradeRoutes();
    game::trade::TradeRoute* selected_route = nullptr;

    for (auto& route : all_routes) {
        if (route.route_id == selected_route_id_) {
            selected_route = &route;
            break;
        }
    }

    if (!selected_route) {
        ImGui::Text("Route not found");
        return;
    }

    ImGui::Columns(2, "RouteDetails", true);

    // Column 1: Basic info
    ImGui::Text("Source:");
    ImGui::Text("Destination:");
    ImGui::Text("Resource:");
    ImGui::Text("Route Type:");
    ImGui::Text("Status:");
    ImGui::Spacing();
    ImGui::Text("Base Volume:");
    ImGui::Text("Current Volume:");
    ImGui::Text("Profitability:");

    ImGui::NextColumn();

    // Column 2: Values
    ImGui::Text("%s", GetProvinceName(selected_route->source_province).c_str());
    ImGui::Text("%s", GetProvinceName(selected_route->destination_province).c_str());
    ImGui::Text("%s", GetResourceName(selected_route->resource).c_str());
    ImGui::Text("%s", GetRouteTypeName(selected_route->route_type).c_str());
    ImVec4 status_color = GetStatusColor(selected_route->status);
    ImGui::TextColored(status_color, "%s", GetStatusName(selected_route->status).c_str());
    ImGui::Spacing();
    ImGui::Text("%.0f", selected_route->base_volume);
    ImGui::Text("%.0f", selected_route->current_volume);
    ImVec4 profit_color = GetProfitabilityColor(selected_route->profitability);
    ImGui::TextColored(profit_color, "%.2f%%", selected_route->profitability * 100.0);

    ImGui::Columns(1);
    ImGui::Spacing();

    ImGui::Separator();
    ImGui::Text("Route Characteristics:");
    ImGui::BulletText("Distance: %.1f km", selected_route->distance_km);
    ImGui::BulletText("Safety: %.0f%%", selected_route->safety_rating * 100.0);
    ImGui::BulletText("Efficiency: %.0f%%", selected_route->efficiency_rating * 100.0);
    ImGui::BulletText("Uses Rivers: %s", selected_route->uses_rivers ? "Yes" : "No");
    ImGui::BulletText("Uses Roads: %s", selected_route->uses_roads ? "Yes" : "No");
    ImGui::BulletText("Uses Sea Route: %s", selected_route->uses_sea_route ? "Yes" : "No");

    ImGui::Spacing();
    if (ImGui::Button("Close Details")) {
        selected_route_id_ = "";
    }
}

void TradeSystemWindow::RenderHubDetailsPanel() {
    if (selected_province_ == 0) {
        return;
    }

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Hub Details");
    ImGui::Spacing();

    // Find the selected hub
    auto all_hubs = trade_system_.GetAllTradeHubs();
    game::trade::TradeHub* selected_hub = nullptr;

    for (auto& hub : all_hubs) {
        if (hub.province_id == selected_province_) {
            selected_hub = &hub;
            break;
        }
    }

    if (!selected_hub) {
        ImGui::Text("No trade hub at this province");
        return;
    }

    ImGui::Text("Hub Name: %s", selected_hub->hub_name.c_str());
    ImGui::Text("Type: %s", GetHubTypeName(selected_hub->hub_type).c_str());
    ImGui::Spacing();

    ImGui::Text("Capacity: %.0f", selected_hub->max_throughput_capacity);
    ImGui::Text("Current Load: %.0f", selected_hub->current_utilization);
    double util_percent = (selected_hub->current_utilization / selected_hub->max_throughput_capacity) * 100.0;
    ImVec4 util_color = GetUtilizationColor(util_percent / 100.0);
    ImGui::TextColored(util_color, "Utilization: %.1f%%", util_percent);

    ImGui::Spacing();
    ImGui::Text("Infrastructure Bonus: %.0f%%", (selected_hub->infrastructure_bonus - 1.0) * 100.0);
    ImGui::Spacing();

    ImGui::Text("Specialized Goods:");
    if (selected_hub->specialized_goods.empty()) {
        ImGui::BulletText("None");
    } else {
        for (const auto& resource : selected_hub->specialized_goods) {
            ImGui::BulletText("%s", GetResourceName(resource).c_str());
        }
    }

    ImGui::Spacing();
    ImGui::Text("Incoming Routes: %zu", selected_hub->incoming_route_ids.size());
    ImGui::Text("Outgoing Routes: %zu", selected_hub->outgoing_route_ids.size());

    ImGui::Spacing();
    if (ImGui::Button("Close Details")) {
        selected_province_ = 0;
    }
}

void TradeSystemWindow::UpdateCachedData() {
    // Update cache periodically to avoid querying every frame
    // This is a placeholder - in a real implementation, you would check elapsed time
    cached_routes_ = trade_system_.GetAllTradeRoutes();
    cached_hubs_ = trade_system_.GetAllTradeHubs();
}

std::string TradeSystemWindow::GetProvinceName(game::types::EntityID province_id) const {
    // Try to get province name from entity manager
    // This is a simplified implementation - you would query the actual province component
    std::ostringstream oss;
    oss << "Province " << province_id;
    return oss.str();
}

std::string TradeSystemWindow::GetResourceName(game::types::ResourceType resource) const {
    switch (resource) {
        case game::types::ResourceType::FOOD: return "Grain";      // Was GRAIN
        case game::types::ResourceType::WINE: return "Wine";
        case game::types::ResourceType::CLOTH: return "Cloth";
        case game::types::ResourceType::IRON: return "Iron";
        case game::types::ResourceType::WOOD: return "Tools";      // Was TOOLS
        case game::types::ResourceType::STONE: return "Pottery";   // Was POTTERY
        case game::types::ResourceType::SPICES: return "Olive Oil"; // Was OLIVE_OIL
        case game::types::ResourceType::SALT: return "Salt";
        default: return "Unknown";
    }
}

std::string TradeSystemWindow::GetRouteTypeName(game::trade::RouteType route_type) const {
    switch (route_type) {
        case game::trade::RouteType::LAND: return "Land";
        case game::trade::RouteType::SEA: return "Sea";
        case game::trade::RouteType::RIVER: return "River";
        default: return "Unknown";
    }
}

std::string TradeSystemWindow::GetStatusName(game::trade::TradeStatus status) const {
    switch (status) {
        case game::trade::TradeStatus::ESTABLISHING: return "Establishing";
        case game::trade::TradeStatus::ACTIVE: return "Active";
        case game::trade::TradeStatus::DISRUPTED: return "Disrupted";
        case game::trade::TradeStatus::SEASONAL_CLOSED: return "Seasonal Closed";
        case game::trade::TradeStatus::ABANDONED: return "Abandoned";
        default: return "Unknown";
    }
}

std::string TradeSystemWindow::GetHubTypeName(game::trade::HubType hub_type) const {
    switch (hub_type) {
        case game::trade::HubType::LOCAL_MARKET: return "Local Market";
        case game::trade::HubType::REGIONAL_HUB: return "Regional Hub";
        case game::trade::HubType::MAJOR_TRADING_CENTER: return "Major Trading Center";
        case game::trade::HubType::INTERNATIONAL_PORT: return "International Port";
        case game::trade::HubType::CROSSROADS: return "Crossroads";
        default: return "Unknown";
    }
}

const char* TradeSystemWindow::GetResourceTypeIcon(game::types::ResourceType resource) const {
    switch (resource) {
        case game::types::ResourceType::FOOD: return "🌾";      // Was GRAIN
        case game::types::ResourceType::WINE: return "🍷";
        case game::types::ResourceType::CLOTH: return "🧵";
        case game::types::ResourceType::IRON: return "⚒️";
        case game::types::ResourceType::WOOD: return "🔨";      // Was TOOLS
        case game::types::ResourceType::STONE: return "🏺";     // Was POTTERY
        case game::types::ResourceType::SPICES: return "🫒";    // Was OLIVE_OIL
        case game::types::ResourceType::SALT: return "🧂";
        default: return "📦";
    }
}

ImVec4 TradeSystemWindow::GetProfitabilityColor(double profitability) const {
    if (profitability > 0.2) {
        return ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // Bright green
    } else if (profitability > 0.0) {
        return ImVec4(0.5f, 0.8f, 0.5f, 1.0f); // Light green
    } else if (profitability > -0.1) {
        return ImVec4(0.8f, 0.8f, 0.2f, 1.0f); // Yellow
    } else {
        return ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Red
    }
}

ImVec4 TradeSystemWindow::GetStatusColor(game::trade::TradeStatus status) const {
    switch (status) {
        case game::trade::TradeStatus::ACTIVE:
            return ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // Green
        case game::trade::TradeStatus::ESTABLISHING:
            return ImVec4(0.5f, 0.8f, 1.0f, 1.0f); // Blue
        case game::trade::TradeStatus::DISRUPTED:
            return ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
        case game::trade::TradeStatus::SEASONAL_CLOSED:
            return ImVec4(0.8f, 0.8f, 0.2f, 1.0f); // Yellow
        case game::trade::TradeStatus::ABANDONED:
            return ImVec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
        default:
            return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
    }
}

ImVec4 TradeSystemWindow::GetUtilizationColor(double utilization) const {
    if (utilization < 0.5) {
        return ImVec4(0.5f, 1.0f, 0.5f, 1.0f); // Green - underutilized
    } else if (utilization < 0.8) {
        return ImVec4(0.5f, 0.8f, 1.0f, 1.0f); // Blue - good utilization
    } else if (utilization < 0.95) {
        return ImVec4(0.8f, 0.8f, 0.2f, 1.0f); // Yellow - high utilization
    } else {
        return ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Red - overcapacity
    }
}

void TradeSystemWindow::SetVisible(bool visible) {
    visible_ = visible;
}

bool TradeSystemWindow::IsVisible() const {
    return visible_;
}

void TradeSystemWindow::ToggleVisibility() {
    visible_ = !visible_;
}

void TradeSystemWindow::SetSelectedProvince(game::types::EntityID province_id) {
    selected_province_ = province_id;
}

void TradeSystemWindow::ClearSelection() {
    selected_province_ = 0;
    selected_route_id_ = "";
}

} // namespace ui
