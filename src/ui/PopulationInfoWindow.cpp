#include "ui/PopulationInfoWindow.h"
#include "map/render/MapRenderer.h"
#include "game/population/PopulationComponents.h"
#include "map/ProvinceRenderComponent.h"
#include "utils/PlatformCompat.h"
#include <sstream>

namespace ui {
    
    PopulationInfoWindow::PopulationInfoWindow(
        ::core::ecs::EntityManager& entity_manager,
        game::map::MapRenderer& map_renderer
    )
        : entity_manager_(entity_manager)
        , map_renderer_(map_renderer)
    {
    }
    
    PopulationInfoWindow::~PopulationInfoWindow() {
    }
    
    void PopulationInfoWindow::Render() {
        if (!is_visible_) {
            return;
        }

        // Get selected province from map renderer
        auto selected_province = map_renderer_.GetSelectedProvince();
        if (selected_province.id == 0) {
            // No province selected - show placeholder
            ImGui::Begin("Population Info", &is_visible_);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Select a province to view population data");
            ImGui::End();
            return;
        }

        // Get province name from ProvinceRenderComponent
        auto render_comp = entity_manager_.GetComponent<game::map::ProvinceRenderComponent>(selected_province);
        std::string province_name = render_comp ? render_comp->name : "Unknown Province";

        // Create window with province name
        std::string window_title = "Population Info - " + province_name;
        ImGui::Begin(window_title.c_str(), &is_visible_);

        // Get PopulationComponent
        auto pop_comp = entity_manager_.GetComponent<game::population::PopulationComponent>(selected_province);
        
        if (!pop_comp) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "No population data available for this province");
            ImGui::End();
            return;
        }

        // Render population statistics
        RenderPopulationStats();
        ImGui::Separator();
        
        RenderDemographics();
        ImGui::Separator();
        
        RenderEmployment();
        ImGui::Separator();
        
        RenderCultureReligion();

        ImGui::End();
    }
    
    void PopulationInfoWindow::Update() {
        // No per-frame updates needed currently
    }

    void PopulationInfoWindow::RenderPopulationStats() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto pop_comp = entity_manager_.GetComponent<game::population::PopulationComponent>(selected_province);
        
        if (!pop_comp) return;

        ImGui::Text("Total Population:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%d", pop_comp->total_population);

        ImGui::Text("Population Density:");
        ImGui::SameLine();
        ImGui::Text("%.1f per sq km", pop_comp->population_density);

        ImGui::Text("Growth Rate:");
        ImGui::SameLine();
        float growth_pct = pop_comp->growth_rate * 100.0f;
        ImVec4 growth_color = growth_pct >= 0 ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
        ImGui::TextColored(growth_color, "%.2f%%", growth_pct);

        ImGui::Spacing();
        ImGui::Text("Birth Rate: %.2f%%", pop_comp->birth_rate_average * 100.0f);
        ImGui::Text("Death Rate: %.2f%%", pop_comp->death_rate_average * 100.0f);
        ImGui::Text("Migration Rate: %.2f%%", pop_comp->migration_net_rate * 100.0f);
    }

    void PopulationInfoWindow::RenderDemographics() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto pop_comp = entity_manager_.GetComponent<game::population::PopulationComponent>(selected_province);
        
        if (!pop_comp) return;

        if (ImGui::CollapsingHeader("Demographics", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Age distribution
            ImGui::Text("Age Distribution:");
            ImGui::Indent();
            
            int total = pop_comp->total_population;
            if (total > 0) {
                float children_pct = (pop_comp->total_children * 100.0f) / total;
                float adults_pct = (pop_comp->total_adults * 100.0f) / total;
                float elderly_pct = (pop_comp->total_elderly * 100.0f) / total;
                
                ImGui::Text("Children: %d (%.1f%%)", pop_comp->total_children, children_pct);
                ImGui::Text("Adults: %d (%.1f%%)", pop_comp->total_adults, adults_pct);
                ImGui::Text("Elderly: %d (%.1f%%)", pop_comp->total_elderly, elderly_pct);
            }
            ImGui::Unindent();

            ImGui::Spacing();

            // Gender distribution
            ImGui::Text("Gender Distribution:");
            ImGui::Indent();
            if (total > 0) {
                float males_pct = (pop_comp->total_males * 100.0f) / total;
                float females_pct = (pop_comp->total_females * 100.0f) / total;
                ImGui::Text("Males: %d (%.1f%%)", pop_comp->total_males, males_pct);
                ImGui::Text("Females: %d (%.1f%%)", pop_comp->total_females, females_pct);
            }
            ImGui::Unindent();

            ImGui::Spacing();

            // Quality of life metrics
            ImGui::Text("Quality of Life:");
            ImGui::Indent();
            ImGui::Text("Happiness: %.1f%%", pop_comp->average_happiness * 100.0f);
            ImGui::Text("Health: %.1f%%", pop_comp->average_health * 100.0f);
            ImGui::Text("Literacy: %.1f%%", pop_comp->average_literacy * 100.0f);
            ImGui::Text("Wealth: %.0f gold", pop_comp->average_wealth);
            ImGui::Unindent();
        }
    }

    void PopulationInfoWindow::RenderEmployment() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto pop_comp = entity_manager_.GetComponent<game::population::PopulationComponent>(selected_province);
        
        if (!pop_comp) return;

        if (ImGui::CollapsingHeader("Employment", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Employment Rate: %.1f%%", pop_comp->overall_employment_rate * 100.0f);
            
            ImGui::Spacing();
            ImGui::Text("Labor Force:");
            ImGui::Indent();
            ImGui::Text("Productive Workers: %d", pop_comp->productive_workers);
            ImGui::Text("Non-Productive Income: %d", pop_comp->non_productive_income);
            ImGui::Text("Unemployed Seeking: %d", pop_comp->unemployed_seeking);
            ImGui::Text("Unemployable: %d", pop_comp->unemployable);
            ImGui::Text("Dependents: %d", pop_comp->dependents);
            ImGui::Unindent();

            ImGui::Spacing();
            ImGui::Text("Military:");
            ImGui::Indent();
            ImGui::Text("Eligible for Service: %d", pop_comp->total_military_eligible);
            ImGui::Text("Military Quality: %.1f%%", pop_comp->average_military_quality * 100.0f);
            ImGui::Text("Service Obligation: %d", pop_comp->total_military_service_obligation);
            ImGui::Unindent();
        }
    }

    void PopulationInfoWindow::RenderCultureReligion() {
        auto selected_province = map_renderer_.GetSelectedProvince();
        auto pop_comp = entity_manager_.GetComponent<game::population::PopulationComponent>(selected_province);
        
        if (!pop_comp) return;

        if (ImGui::CollapsingHeader("Culture & Religion")) {
            // Culture distribution
            if (!pop_comp->culture_distribution.empty()) {
                ImGui::Text("Cultural Groups:");
                ImGui::Indent();
                for (const auto& [culture, count] : pop_comp->culture_distribution) {
                    float pct = (count * 100.0f) / pop_comp->total_population;
                    ImGui::Text("%s: %d (%.1f%%)", culture.c_str(), count, pct);
                }
                ImGui::Unindent();
                ImGui::Spacing();
            }

            // Religion distribution
            if (!pop_comp->religion_distribution.empty()) {
                ImGui::Text("Religious Groups:");
                ImGui::Indent();
                for (const auto& [religion, count] : pop_comp->religion_distribution) {
                    float pct = (count * 100.0f) / pop_comp->total_population;
                    ImGui::Text("%s: %d (%.1f%%)", religion.c_str(), count, pct);
                }
                ImGui::Unindent();
                ImGui::Spacing();
            }

            // Social dynamics
            ImGui::Text("Social Dynamics:");
            ImGui::Indent();
            ImGui::Text("Cultural Assimilation: %.2f%%", pop_comp->cultural_assimilation_rate * 100.0f);
            ImGui::Text("Religious Conversion: %.2f%%", pop_comp->religious_conversion_rate * 100.0f);
            ImGui::Text("Social Mobility: %.2f%%", pop_comp->social_mobility_average * 100.0f);
            ImGui::Text("Inter-Class Tension: %.1f%%", pop_comp->inter_class_tension * 100.0f);
            ImGui::Unindent();
        }
    }

} // namespace ui