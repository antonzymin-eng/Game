#include "ui/TechnologyInfoWindow.h"
#include "ui/WindowManager.h"
#include "imgui.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>  // For memset

namespace ui {

TechnologyInfoWindow::TechnologyInfoWindow(
    ::core::ecs::EntityManager& entity_manager,
    game::technology::TechnologySystem& tech_system
)
    : entity_manager_(entity_manager),
      tech_system_(tech_system),
      current_tab_(0),
      current_player_entity_(0),
      selected_technology_(game::technology::TechnologyType::INVALID),
      filter_by_category_(false),
      selected_category_(game::technology::TechnologyCategory::AGRICULTURAL),
      show_only_available_(false),
      show_only_researching_(false) {
    std::memset(search_buffer_, 0, sizeof(search_buffer_));
}

void TechnologyInfoWindow::Render(WindowManager& window_manager, game::types::EntityID player_entity) {
    current_player_entity_ = player_entity;

    if (!window_manager.BeginManagedWindow(WindowManager::WindowType::TECHNOLOGY, "Technology")) {
        return;
    }

    RenderHeader();

    ImGui::Separator();

    // Tab bar for different views
    if (ImGui::BeginTabBar("TechnologyTabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Research Overview")) {
            RenderResearchOverviewTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Technology Tree")) {
            RenderTechnologyTreeTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Innovation")) {
            RenderInnovationTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Knowledge Network")) {
            RenderKnowledgeNetworkTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    window_manager.EndManagedWindow();
}

void TechnologyInfoWindow::Update() {
    // Deprecated - kept for compatibility
}

void TechnologyInfoWindow::RenderHeader() {
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Technology & Innovation");

    if (current_player_entity_ == 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "No nation selected");
        return;
    }

    // Get research component for selected entity
    auto* research_comp = tech_system_.GetResearchComponent(current_player_entity_);
    if (!research_comp) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "No research data available");
        return;
    }

    ImGui::Spacing();

    // Display research statistics
    int total_discovered = 0;
    int total_researching = 0;
    int total_implemented = 0;

    for (const auto& [tech, state] : research_comp->technology_states) {
        if (state == game::technology::ResearchState::DISCOVERED ||
            state == game::technology::ResearchState::IMPLEMENTING ||
            state == game::technology::ResearchState::IMPLEMENTED) {
            total_discovered++;
        }
        if (state == game::technology::ResearchState::RESEARCHING) {
            total_researching++;
        }
        if (state == game::technology::ResearchState::IMPLEMENTED) {
            total_implemented++;
        }
    }

    ImGui::Text("Technologies Discovered: %d", total_discovered);
    ImGui::SameLine(250);
    ImGui::Text("Currently Researching: %d", total_researching);
    ImGui::SameLine(500);
    ImGui::Text("Fully Implemented: %d", total_implemented);

    ImGui::Spacing();
    ImGui::Text("Monthly Research Budget: %.1f", research_comp->monthly_research_budget);
    ImGui::SameLine(250);
    ImGui::Text("Research Efficiency: %.1f%%", research_comp->base_research_efficiency * 100.0);
    ImGui::SameLine(500);
    ImGui::Text("Scholars: %u", research_comp->scholar_population);
}

void TechnologyInfoWindow::RenderResearchOverviewTab() {
    auto* research_comp = tech_system_.GetResearchComponent(current_player_entity_);
    if (!research_comp) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "No research data available");
        return;
    }

    ImGui::BeginChild("ResearchOverview", ImVec2(0, 0), true);

    // Current research focus
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Current Research Focus");
    ImGui::Separator();

    if (research_comp->current_focus != game::technology::TechnologyType::INVALID) {
        std::string tech_name = GetTechnologyName(research_comp->current_focus);
        ImGui::Text("Researching: %s", tech_name.c_str());

        auto progress_it = research_comp->research_progress.find(research_comp->current_focus);
        if (progress_it != research_comp->research_progress.end()) {
            float progress = static_cast<float>(progress_it->second);
            ImGui::ProgressBar(progress, ImVec2(-1, 0), "");
            ImGui::SameLine(0, 10);
            ImGui::Text("%.1f%%", progress * 100.0f);
        }
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No current research focus");
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // Research infrastructure
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Research Infrastructure");
    ImGui::Separator();

    ImGui::Text("Universities: %u", research_comp->universities);
    ImGui::SameLine(200);
    ImGui::Text("Monasteries: %u", research_comp->monasteries);

    ImGui::Text("Libraries: %u", research_comp->libraries);
    ImGui::SameLine(200);
    ImGui::Text("Workshops: %u", research_comp->workshops);

    ImGui::Spacing();
    ImGui::Spacing();

    // Category investments
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Research Investment by Category");
    ImGui::Separator();

    for (int i = 0; i < static_cast<int>(game::technology::TechnologyCategory::COUNT); ++i) {
        auto category = static_cast<game::technology::TechnologyCategory>(i);
        std::string category_name = GetCategoryName(category);

        auto it = research_comp->category_investment.find(category);
        double investment = (it != research_comp->category_investment.end()) ? it->second : 0.0;

        ImGui::Text("%s %s:", GetCategoryIcon(category), category_name.c_str());
        ImGui::SameLine(200);
        ImGui::Text("%.1f gold/month", investment);

        // Progress bar showing investment percentage
        float percent = (research_comp->monthly_research_budget > 0)
                            ? static_cast<float>(investment / research_comp->monthly_research_budget)
                            : 0.0f;
        ImVec4 category_color = GetCategoryColor(category);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, category_color);
        ImGui::ProgressBar(percent, ImVec2(300, 0), "");
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // Research modifiers
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Research Modifiers");
    ImGui::Separator();

    ImGui::Text("Literacy Bonus: +%.1f%%", research_comp->literacy_bonus * 100.0);
    ImGui::Text("Trade Network Bonus: +%.1f%%", research_comp->trade_network_bonus * 100.0);
    ImGui::Text("Stability Bonus: +%.1f%%", research_comp->stability_bonus * 100.0);

    ImGui::EndChild();
}

void TechnologyInfoWindow::RenderTechnologyTreeTab() {
    auto* research_comp = tech_system_.GetResearchComponent(current_player_entity_);
    if (!research_comp) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "No research data available");
        return;
    }

    // Category selection
    ImGui::Text("Select Category:");
    ImGui::SameLine();

    for (int i = 0; i < static_cast<int>(game::technology::TechnologyCategory::COUNT); ++i) {
        auto category = static_cast<game::technology::TechnologyCategory>(i);
        if (i > 0) ImGui::SameLine();

        bool is_selected = (selected_category_ == category);
        if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, GetCategoryColor(category));
        }

        if (ImGui::Button(GetCategoryName(category).c_str())) {
            selected_category_ = category;
        }

        if (is_selected) {
            ImGui::PopStyleColor();
        }
    }

    ImGui::Separator();

    // Render the tree for selected category
    ImGui::BeginChild("TechnologyTree", ImVec2(0, 0), true);
    RenderCategoryTree(selected_category_, research_comp);
    ImGui::EndChild();
}

void TechnologyInfoWindow::RenderCategoryTree(
    game::technology::TechnologyCategory category,
    const game::technology::ResearchComponent* research_comp
) {
    auto techs_in_category = GetTechnologiesInCategory(category);

    if (techs_in_category.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No technologies in this category");
        return;
    }

    // Display technologies in a grid
    float item_width = 200.0f;
    float item_spacing = 20.0f;
    int columns = std::max(1, static_cast<int>((ImGui::GetContentRegionAvail().x + item_spacing) / (item_width + item_spacing)));

    int current_column = 0;
    for (const auto& tech : techs_in_category) {
        if (current_column > 0) {
            ImGui::SameLine(0, item_spacing);
        }

        // Get technology state
        game::technology::ResearchState state = game::technology::ResearchState::UNKNOWN;
        auto state_it = research_comp->technology_states.find(tech);
        if (state_it != research_comp->technology_states.end()) {
            state = state_it->second;
        }

        // Color based on state
        ImVec4 tech_color = GetResearchStateColor(state);
        ImGui::PushStyleColor(ImGuiCol_Button, tech_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(tech_color.x * 1.2f, tech_color.y * 1.2f, tech_color.z * 1.2f, 1.0f));

        std::string tech_name = GetTechnologyName(tech);
        if (ImGui::Button(tech_name.c_str(), ImVec2(item_width, 60))) {
            selected_technology_ = tech;
        }

        ImGui::PopStyleColor(2);

        // Show progress if researching
        if (state == game::technology::ResearchState::RESEARCHING) {
            auto progress_it = research_comp->research_progress.find(tech);
            if (progress_it != research_comp->research_progress.end()) {
                float progress = static_cast<float>(progress_it->second);
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (current_column * (item_width + item_spacing)));
                ImGui::ProgressBar(progress, ImVec2(item_width, 0), "");
            }
        }

        current_column++;
        if (current_column >= columns) {
            current_column = 0;
        }
    }

    // Show details for selected technology
    if (selected_technology_ != game::technology::TechnologyType::INVALID) {
        ImGui::Separator();
        ImGui::Spacing();
        RenderTechnologyDetails(selected_technology_);
    }
}

void TechnologyInfoWindow::RenderTechnologyDetails(game::technology::TechnologyType tech_type) {
    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Technology Details");
    ImGui::Separator();

    std::string tech_name = GetTechnologyName(tech_type);
    std::string tech_desc = GetTechnologyDescription(tech_type);

    ImGui::Text("Name: %s", tech_name.c_str());
    ImGui::TextWrapped("%s", tech_desc.c_str());

    auto* research_comp = tech_system_.GetResearchComponent(current_player_entity_);
    if (research_comp) {
        auto state_it = research_comp->technology_states.find(tech_type);
        if (state_it != research_comp->technology_states.end()) {
            std::string state_name = GetResearchStateName(state_it->second);
            ImGui::Text("Status: %s", state_name.c_str());
        }
    }
}

void TechnologyInfoWindow::RenderInnovationTab() {
    auto* innovation_comp = tech_system_.GetInnovationComponent(current_player_entity_);
    if (!innovation_comp) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "No innovation data available");
        return;
    }

    ImGui::BeginChild("Innovation", ImVec2(0, 0), true);

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Innovation Capacity");
    ImGui::Separator();

    ImGui::Text("Innovation Rate: %.2f%%", innovation_comp->innovation_rate * 100.0);
    ImGui::Text("Breakthrough Chance: %.2f%%", innovation_comp->breakthrough_chance * 100.0);
    ImGui::Text("Invention Quality: %.2f%%", innovation_comp->invention_quality * 100.0);

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Innovation Sources");
    ImGui::Separator();

    ImGui::Text("Inventors: %u", innovation_comp->inventors);
    ImGui::Text("Craftsmen Innovators: %u", innovation_comp->craftsmen_innovators);
    ImGui::Text("Scholar Innovators: %u", innovation_comp->scholar_innovators);
    ImGui::Text("Foreign Scholars: %u", innovation_comp->foreign_scholars);

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Innovation Environment");
    ImGui::Separator();

    ImGui::Text("Cultural Openness: %.1f%%", innovation_comp->cultural_openness * 100.0);
    ImGui::Text("Innovation Encouragement: %.1f%%", innovation_comp->innovation_encouragement * 100.0);
    ImGui::Text("Experimentation Freedom: %.1f%%", innovation_comp->experimentation_freedom * 100.0);
    ImGui::Text("Knowledge Preservation: %.1f%%", innovation_comp->knowledge_preservation_rate * 100.0);

    ImGui::Spacing();
    ImGui::Spacing();

    // Recent discoveries
    if (!innovation_comp->recent_discoveries.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Recent Discoveries");
        ImGui::Separator();

        for (const auto& tech : innovation_comp->recent_discoveries) {
            std::string tech_name = GetTechnologyName(tech);
            ImGui::BulletText("%s", tech_name.c_str());
        }
    }

    ImGui::EndChild();
}

void TechnologyInfoWindow::RenderKnowledgeNetworkTab() {
    auto* knowledge_comp = tech_system_.GetKnowledgeComponent(current_player_entity_);
    if (!knowledge_comp) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "No knowledge data available");
        return;
    }

    ImGui::BeginChild("Knowledge", ImVec2(0, 0), true);

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Knowledge Infrastructure");
    ImGui::Separator();

    ImGui::Text("Manuscripts: %u", knowledge_comp->manuscripts);
    ImGui::Text("Scribes: %u", knowledge_comp->scribes);
    ImGui::Text("Translators: %u", knowledge_comp->translators);
    ImGui::Text("Book Production Capacity: %u books/year", knowledge_comp->book_production_capacity);

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Knowledge Preservation");
    ImGui::Separator();

    ImGui::Text("Preservation Quality: %.1f%%", knowledge_comp->knowledge_preservation_quality * 100.0);
    ImGui::Text("Manuscript Durability: %.1f%%", knowledge_comp->manuscript_durability * 100.0);
    ImGui::Text("Translation Accuracy: %.1f%%", knowledge_comp->translation_accuracy * 100.0);
    ImGui::Text("Knowledge Loss Rate: %.2f%%/month", knowledge_comp->knowledge_loss_rate * 100.0);

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Knowledge Transmission");
    ImGui::Separator();

    ImGui::Text("Transmission Rate: %.1f%%", knowledge_comp->knowledge_transmission_rate * 100.0);
    ImGui::Text("Cultural Absorption: %.1f%%", knowledge_comp->cultural_knowledge_absorption * 100.0);
    ImGui::Text("Foreign Knowledge Acceptance: %.1f%%", knowledge_comp->foreign_knowledge_acceptance * 100.0);

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Literacy");
    ImGui::Separator();

    ImGui::Text("Overall Literacy Rate: %.1f%%", knowledge_comp->literacy_rate * 100.0);
    ImGui::Text("Scholarly Literacy Rate: %.1f%%", knowledge_comp->scholarly_literacy_rate * 100.0);

    ImGui::Spacing();
    ImGui::Spacing();

    // Known languages
    if (!knowledge_comp->known_languages.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.5f, 1.0f), "Known Languages");
        ImGui::Separator();

        for (const auto& lang : knowledge_comp->known_languages) {
            ImGui::BulletText("%s", lang.c_str());
        }
    }

    ImGui::EndChild();
}

// Utility method implementations

std::string TechnologyInfoWindow::GetTechnologyName(game::technology::TechnologyType tech) const {
    switch (tech) {
        // Agricultural
        case game::technology::TechnologyType::THREE_FIELD_SYSTEM: return "Three-Field System";
        case game::technology::TechnologyType::HEAVY_PLOW: return "Heavy Plow";
        case game::technology::TechnologyType::HORSE_COLLAR: return "Horse Collar";
        case game::technology::TechnologyType::WINDMILL: return "Windmill";
        case game::technology::TechnologyType::WATERMILL: return "Watermill";
        case game::technology::TechnologyType::CROP_ROTATION: return "Crop Rotation";
        case game::technology::TechnologyType::SELECTIVE_BREEDING: return "Selective Breeding";
        case game::technology::TechnologyType::AGRICULTURAL_MANUAL: return "Agricultural Manual";
        case game::technology::TechnologyType::IRRIGATION_SYSTEMS: return "Irrigation Systems";
        case game::technology::TechnologyType::NEW_WORLD_CROPS: return "New World Crops";

        // Military
        case game::technology::TechnologyType::CHAINMAIL_ARMOR: return "Chainmail Armor";
        case game::technology::TechnologyType::PLATE_ARMOR: return "Plate Armor";
        case game::technology::TechnologyType::CROSSBOW: return "Crossbow";
        case game::technology::TechnologyType::LONGBOW: return "Longbow";
        case game::technology::TechnologyType::GUNPOWDER: return "Gunpowder";
        case game::technology::TechnologyType::CANNONS: return "Cannons";
        case game::technology::TechnologyType::ARQUEBUS: return "Arquebus";
        case game::technology::TechnologyType::MUSKET: return "Musket";
        case game::technology::TechnologyType::STAR_FORTRESS: return "Star Fortress";
        case game::technology::TechnologyType::MILITARY_ENGINEERING: return "Military Engineering";

        // Craft
        case game::technology::TechnologyType::BLAST_FURNACE: return "Blast Furnace";
        case game::technology::TechnologyType::WATER_POWERED_MACHINERY: return "Water-Powered Machinery";
        case game::technology::TechnologyType::MECHANICAL_CLOCK: return "Mechanical Clock";
        case game::technology::TechnologyType::PRINTING_PRESS: return "Printing Press";
        case game::technology::TechnologyType::DOUBLE_ENTRY_BOOKKEEPING: return "Double-Entry Bookkeeping";
        case game::technology::TechnologyType::PAPER_MAKING: return "Paper Making";
        case game::technology::TechnologyType::GLASS_MAKING: return "Glass Making";
        case game::technology::TechnologyType::TEXTILE_MACHINERY: return "Textile Machinery";
        case game::technology::TechnologyType::ADVANCED_METALLURGY: return "Advanced Metallurgy";
        case game::technology::TechnologyType::PRECISION_INSTRUMENTS: return "Precision Instruments";

        // Administrative
        case game::technology::TechnologyType::WRITTEN_LAW_CODES: return "Written Law Codes";
        case game::technology::TechnologyType::BUREAUCRATIC_ADMINISTRATION: return "Bureaucratic Administration";
        case game::technology::TechnologyType::CENSUS_TECHNIQUES: return "Census Techniques";
        case game::technology::TechnologyType::TAX_COLLECTION_SYSTEMS: return "Tax Collection Systems";
        case game::technology::TechnologyType::DIPLOMATIC_PROTOCOLS: return "Diplomatic Protocols";
        case game::technology::TechnologyType::RECORD_KEEPING: return "Record Keeping";
        case game::technology::TechnologyType::STANDARDIZED_WEIGHTS: return "Standardized Weights";
        case game::technology::TechnologyType::POSTAL_SYSTEMS: return "Postal Systems";
        case game::technology::TechnologyType::PROFESSIONAL_ARMY: return "Professional Army";
        case game::technology::TechnologyType::STATE_MONOPOLIES: return "State Monopolies";

        // Academic
        case game::technology::TechnologyType::SCHOLASTIC_METHOD: return "Scholastic Method";
        case game::technology::TechnologyType::UNIVERSITY_SYSTEM: return "University System";
        case game::technology::TechnologyType::VERNACULAR_WRITING: return "Vernacular Writing";
        case game::technology::TechnologyType::NATURAL_PHILOSOPHY: return "Natural Philosophy";
        case game::technology::TechnologyType::MATHEMATICAL_NOTATION: return "Mathematical Notation";
        case game::technology::TechnologyType::EXPERIMENTAL_METHOD: return "Experimental Method";
        case game::technology::TechnologyType::HUMANIST_EDUCATION: return "Humanist Education";
        case game::technology::TechnologyType::SCIENTIFIC_INSTRUMENTS: return "Scientific Instruments";
        case game::technology::TechnologyType::OPTICAL_DEVICES: return "Optical Devices";
        case game::technology::TechnologyType::CARTOGRAPHY: return "Cartography";

        // Naval
        case game::technology::TechnologyType::IMPROVED_SHIP_DESIGN: return "Improved Ship Design";
        case game::technology::TechnologyType::NAVIGATION_INSTRUMENTS: return "Navigation Instruments";
        case game::technology::TechnologyType::COMPASS_NAVIGATION: return "Compass Navigation";
        case game::technology::TechnologyType::NAVAL_ARTILLERY: return "Naval Artillery";
        case game::technology::TechnologyType::OCEAN_NAVIGATION: return "Ocean Navigation";
        case game::technology::TechnologyType::SHIPYARD_TECHNIQUES: return "Shipyard Techniques";
        case game::technology::TechnologyType::MARITIME_LAW: return "Maritime Law";
        case game::technology::TechnologyType::NAVAL_TACTICS: return "Naval Tactics";
        case game::technology::TechnologyType::LIGHTHOUSE_SYSTEMS: return "Lighthouse Systems";
        case game::technology::TechnologyType::HARBOR_ENGINEERING: return "Harbor Engineering";

        default: return "Unknown Technology";
    }
}

std::string TechnologyInfoWindow::GetTechnologyDescription(game::technology::TechnologyType tech) const {
    // Simplified descriptions - could be expanded
    return "A valuable technology that improves various aspects of your realm.";
}

std::string TechnologyInfoWindow::GetCategoryName(game::technology::TechnologyCategory category) const {
    switch (category) {
        case game::technology::TechnologyCategory::AGRICULTURAL: return "Agricultural";
        case game::technology::TechnologyCategory::MILITARY: return "Military";
        case game::technology::TechnologyCategory::CRAFT: return "Craft";
        case game::technology::TechnologyCategory::ADMINISTRATIVE: return "Administrative";
        case game::technology::TechnologyCategory::ACADEMIC: return "Academic";
        case game::technology::TechnologyCategory::NAVAL: return "Naval";
        default: return "Unknown";
    }
}

std::string TechnologyInfoWindow::GetResearchStateName(game::technology::ResearchState state) const {
    switch (state) {
        case game::technology::ResearchState::UNKNOWN: return "Unknown";
        case game::technology::ResearchState::AVAILABLE: return "Available";
        case game::technology::ResearchState::RESEARCHING: return "Researching";
        case game::technology::ResearchState::DISCOVERED: return "Discovered";
        case game::technology::ResearchState::IMPLEMENTING: return "Implementing";
        case game::technology::ResearchState::IMPLEMENTED: return "Implemented";
        default: return "Unknown";
    }
}

const char* TechnologyInfoWindow::GetCategoryIcon(game::technology::TechnologyCategory category) const {
    switch (category) {
        case game::technology::TechnologyCategory::AGRICULTURAL: return "[F]";
        case game::technology::TechnologyCategory::MILITARY: return "[M]";
        case game::technology::TechnologyCategory::CRAFT: return "[C]";
        case game::technology::TechnologyCategory::ADMINISTRATIVE: return "[A]";
        case game::technology::TechnologyCategory::ACADEMIC: return "[S]";
        case game::technology::TechnologyCategory::NAVAL: return "[N]";
        default: return "[?]";
    }
}

ImVec4 TechnologyInfoWindow::GetResearchStateColor(game::technology::ResearchState state) const {
    switch (state) {
        case game::technology::ResearchState::UNKNOWN:
            return ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        case game::technology::ResearchState::AVAILABLE:
            return ImVec4(0.5f, 0.5f, 0.8f, 1.0f);
        case game::technology::ResearchState::RESEARCHING:
            return ImVec4(0.8f, 0.8f, 0.3f, 1.0f);
        case game::technology::ResearchState::DISCOVERED:
            return ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
        case game::technology::ResearchState::IMPLEMENTING:
            return ImVec4(0.3f, 0.7f, 0.7f, 1.0f);
        case game::technology::ResearchState::IMPLEMENTED:
            return ImVec4(0.2f, 0.6f, 0.2f, 1.0f);
        default:
            return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}

ImVec4 TechnologyInfoWindow::GetCategoryColor(game::technology::TechnologyCategory category) const {
    switch (category) {
        case game::technology::TechnologyCategory::AGRICULTURAL:
            return ImVec4(0.4f, 0.7f, 0.3f, 1.0f);
        case game::technology::TechnologyCategory::MILITARY:
            return ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
        case game::technology::TechnologyCategory::CRAFT:
            return ImVec4(0.7f, 0.6f, 0.3f, 1.0f);
        case game::technology::TechnologyCategory::ADMINISTRATIVE:
            return ImVec4(0.6f, 0.5f, 0.8f, 1.0f);
        case game::technology::TechnologyCategory::ACADEMIC:
            return ImVec4(0.3f, 0.6f, 0.9f, 1.0f);
        case game::technology::TechnologyCategory::NAVAL:
            return ImVec4(0.2f, 0.5f, 0.7f, 1.0f);
        default:
            return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    }
}

ImVec4 TechnologyInfoWindow::GetProgressColor(double progress) const {
    if (progress < 0.33) {
        return ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
    } else if (progress < 0.67) {
        return ImVec4(0.8f, 0.8f, 0.3f, 1.0f);
    } else {
        return ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
    }
}

bool TechnologyInfoWindow::IsTechnologyAvailable(
    game::technology::TechnologyType tech,
    const game::technology::ResearchComponent* research_comp
) const {
    if (!research_comp) return false;

    auto it = research_comp->technology_states.find(tech);
    if (it == research_comp->technology_states.end()) {
        return false;
    }

    return it->second == game::technology::ResearchState::AVAILABLE ||
           it->second == game::technology::ResearchState::RESEARCHING;
}

std::vector<game::technology::TechnologyType> TechnologyInfoWindow::GetPrerequisites(
    game::technology::TechnologyType tech
) const {
    // Simplified - would need to be expanded with actual prerequisite data
    return {};
}

std::vector<game::technology::TechnologyType> TechnologyInfoWindow::GetTechnologiesInCategory(
    game::technology::TechnologyCategory category
) const {
    std::vector<game::technology::TechnologyType> result;

    // Map technologies to categories based on their enum ranges
    int start_range = 0;
    int end_range = 0;

    switch (category) {
        case game::technology::TechnologyCategory::AGRICULTURAL:
            start_range = 1001; end_range = 1010; break;
        case game::technology::TechnologyCategory::MILITARY:
            start_range = 1101; end_range = 1110; break;
        case game::technology::TechnologyCategory::CRAFT:
            start_range = 1201; end_range = 1210; break;
        case game::technology::TechnologyCategory::ADMINISTRATIVE:
            start_range = 1301; end_range = 1310; break;
        case game::technology::TechnologyCategory::ACADEMIC:
            start_range = 1401; end_range = 1410; break;
        case game::technology::TechnologyCategory::NAVAL:
            start_range = 1501; end_range = 1510; break;
        default:
            return result;
    }

    for (int i = start_range; i <= end_range; ++i) {
        result.push_back(static_cast<game::technology::TechnologyType>(i));
    }

    return result;
}

} // namespace ui
