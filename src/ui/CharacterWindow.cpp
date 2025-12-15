#include "ui/CharacterWindow.h"
#include "ui/WindowManager.h"
#include "game/components/CharacterComponent.h"
#include "game/components/TraitsComponent.h"
#include "game/character/CharacterRelationships.h"
#include "game/character/CharacterEducation.h"
#include "game/character/CharacterLifeEvents.h"
#include "core/logging/Logger.h"
#include <algorithm>
#include <vector>

namespace ui {

CharacterWindow::CharacterWindow(
    core::ecs::EntityManager& entity_manager,
    game::character::CharacterSystem& character_system)
    : entity_manager_(entity_manager)
    , character_system_(character_system)
    , selected_character_{}
{
}

void CharacterWindow::Render(WindowManager& window_manager, game::types::EntityID player_entity) {
    if (!window_manager.BeginManagedWindow(WindowManager::WindowType::CHARACTER, "Characters")) {
        window_manager.EndManagedWindow();
        return;
    }

    // Tab bar
    if (ImGui::BeginTabBar("CharacterTabs")) {
        if (ImGui::BeginTabItem("Character List")) {
            RenderCharacterList();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Character Details")) {
            RenderCharacterDetails();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    window_manager.EndManagedWindow();
}

void CharacterWindow::ShowCharacter(core::ecs::EntityID character_id) {
    selected_character_ = character_id;
    selected_tab_ = 1; // Switch to details tab
}

void CharacterWindow::RenderCharacterList() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));

    // Search and filter controls
    ImGui::Text("Search:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(250);
    ImGui::InputText("##CharacterSearch", search_buffer_, sizeof(search_buffer_));

    ImGui::SameLine(0, 20);
    ImGui::Text("Sort by:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    const char* sort_items[] = {"Name", "Age", "Realm"};
    ImGui::Combo("##SortMode", &sort_mode_, sort_items, IM_ARRAYSIZE(sort_items));

    ImGui::SameLine(0, 20);
    ImGui::Checkbox("Show Dead", &show_dead_characters_);

    ImGui::PopStyleVar();

    ImGui::Separator();
    ImGui::Spacing();

    // Character list (scrollable)
    ImGui::BeginChild("CharacterListScroll", ImVec2(0, 0), true);

    // C2 FIX: Check if filtering cache needs rebuild
    bool needs_rebuild = filter_cache_dirty_ ||
                         std::string(search_buffer_) != last_search_text_ ||
                         sort_mode_ != last_sort_mode_ ||
                         show_dead_characters_ != last_show_dead_;

    if (needs_rebuild) {
        // Get all characters
        const auto& all_characters = character_system_.GetAllCharacters();

        // C1 FIX: Cache component data before sorting to avoid repeated lookups
        struct CharacterSortData {
            core::ecs::EntityID id;
            std::string name;
            uint32_t age;
            game::types::EntityID realm;
        };

        std::vector<CharacterSortData> sort_cache;
        sort_cache.reserve(all_characters.size());

        std::string search_lower(search_buffer_);
        std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

        // Filter and cache component data in one pass
        for (const auto& char_id : all_characters) {
            auto char_comp = entity_manager_.GetComponent<game::character::CharacterComponent>(char_id);
            if (!char_comp) continue;

            // Filter by search
            if (search_lower.length() > 0) {
                std::string name_lower = char_comp->GetName();
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
                if (name_lower.find(search_lower) == std::string::npos) {
                    continue;
                }
            }

            // M1 FIX: Filter by alive/dead using proper death detection
            if (!show_dead_characters_ && IsCharacterDead(char_id)) {
                continue;
            }

            // Cache data for sorting
            sort_cache.push_back({
                char_id,
                char_comp->GetName(),
                char_comp->GetAge(),
                char_comp->GetPrimaryTitle()
            });
        }

        // Sort using cached data (no component lookups in comparator)
        if (sort_mode_ == 0) { // By name
            std::sort(sort_cache.begin(), sort_cache.end(),
                [](const CharacterSortData& a, const CharacterSortData& b) {
                    return a.name < b.name;
                });
        } else if (sort_mode_ == 1) { // By age
            std::sort(sort_cache.begin(), sort_cache.end(),
                [](const CharacterSortData& a, const CharacterSortData& b) {
                    return a.age > b.age;
                });
        } else if (sort_mode_ == 2) { // By realm
            std::sort(sort_cache.begin(), sort_cache.end(),
                [](const CharacterSortData& a, const CharacterSortData& b) {
                    return a.realm < b.realm;
                });
        }

        // Extract sorted IDs into cache
        cached_filtered_characters_.clear();
        cached_filtered_characters_.reserve(sort_cache.size());
        for (const auto& data : sort_cache) {
            cached_filtered_characters_.push_back(data.id);
        }

        // Update cache state
        last_search_text_ = search_buffer_;
        last_sort_mode_ = sort_mode_;
        last_show_dead_ = show_dead_characters_;
        filter_cache_dirty_ = false;

        // M2 FIX: Reset to page 0 when filter changes
        current_page_ = 0;
    }

    // M2 FIX: Pagination support
    const int total_items = static_cast<int>(cached_filtered_characters_.size());
    const int total_pages = (total_items + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;

    // Clamp current page to valid range
    if (current_page_ >= total_pages && total_pages > 0) {
        current_page_ = total_pages - 1;
    }
    if (current_page_ < 0) {
        current_page_ = 0;
    }

    // Calculate pagination range
    const int start_idx = current_page_ * ITEMS_PER_PAGE;
    const int end_idx = std::min(start_idx + ITEMS_PER_PAGE, total_items);

    // Pagination controls
    ImGui::Separator();
    ImGui::Text("Total: %d characters", total_items);
    if (total_pages > 1) {
        ImGui::SameLine();
        ImGui::Text("| Page %d of %d", current_page_ + 1, total_pages);
        ImGui::SameLine();
        if (ImGui::Button("< Prev")) {
            if (current_page_ > 0) current_page_--;
        }
        ImGui::SameLine();
        if (ImGui::Button("Next >")) {
            if (current_page_ < total_pages - 1) current_page_++;
        }
    }
    ImGui::Separator();

    // Display header
    ImGui::Columns(5, "CharacterColumns");
    ImGui::SetColumnWidth(0, 250);
    ImGui::SetColumnWidth(1, 80);
    ImGui::SetColumnWidth(2, 200);
    ImGui::SetColumnWidth(3, 120);
    ImGui::SetColumnWidth(4, 120);

    ImGui::Text("Name");
    ImGui::NextColumn();
    ImGui::Text("Age");
    ImGui::NextColumn();
    ImGui::Text("Realm");
    ImGui::NextColumn();
    ImGui::Text("Diplomacy");
    ImGui::NextColumn();
    ImGui::Text("Martial");
    ImGui::NextColumn();
    ImGui::Separator();

    // M2 FIX: Display only current page of characters
    for (int i = start_idx; i < end_idx; i++) {
        RenderCharacterListItem(cached_filtered_characters_[i]);
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}

void CharacterWindow::RenderCharacterListItem(core::ecs::EntityID char_id) {
    auto char_comp = entity_manager_.GetComponent<game::character::CharacterComponent>(char_id);
    if (!char_comp) return;

    // Name (clickable)
    if (ImGui::Selectable(char_comp->GetName().c_str(),
                          selected_character_.id == char_id.id,
                          ImGuiSelectableFlags_SpanAllColumns)) {
        selected_character_ = char_id;
        selected_tab_ = 1; // Switch to details tab
    }
    ImGui::NextColumn();

    // Age
    ImGui::Text("%u", char_comp->GetAge());
    ImGui::NextColumn();

    // Realm
    game::types::EntityID realm_id = char_comp->GetPrimaryTitle();
    if (realm_id != 0) {
        ImGui::Text("Realm %u", realm_id);
    } else {
        ImGui::TextDisabled("None");
    }
    ImGui::NextColumn();

    // Diplomacy stat
    ImGui::Text("%d", char_comp->GetDiplomacy());
    ImGui::NextColumn();

    // Martial stat
    ImGui::Text("%d", char_comp->GetMartial());
    ImGui::NextColumn();
}

void CharacterWindow::RenderCharacterDetails() {
    if (!selected_character_.IsValid()) {
        ImGui::TextWrapped("No character selected. Select a character from the Character List tab.");
        return;
    }

    auto char_comp = entity_manager_.GetComponent<game::character::CharacterComponent>(selected_character_);
    if (!char_comp) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                          "Error: Character data not found (ID: %llu)", selected_character_.id);
        return;
    }

    // Character name header
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("%s", char_comp->GetName().c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::TextDisabled("(ID: %llu)", selected_character_.id);

    ImGui::Separator();
    ImGui::Spacing();

    // Two-column layout
    ImGui::BeginChild("LeftPanel", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0), false);
    RenderBasicInfo(selected_character_);
    ImGui::Spacing();
    RenderStatsPanel(selected_character_);
    ImGui::Spacing();
    RenderTraitsPanel(selected_character_);
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("RightPanel", ImVec2(0, 0), false);
    RenderRelationshipsPanel(selected_character_);
    ImGui::Spacing();
    RenderLifeEventsPanel(selected_character_);
    ImGui::Spacing();
    RenderEducationPanel(selected_character_);
    ImGui::EndChild();
}

void CharacterWindow::RenderBasicInfo(core::ecs::EntityID char_id) {
    auto char_comp = entity_manager_.GetComponent<game::character::CharacterComponent>(char_id);
    if (!char_comp) return;

    ImGui::BeginChild("BasicInfo", ImVec2(0, 150), true);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("BASIC INFORMATION");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Age: %u", char_comp->GetAge());
    ImGui::Text("Health: %.1f%%", char_comp->GetHealth());
    ImGui::Text("Prestige: %.1f", char_comp->GetPrestige());
    ImGui::Text("Gold: %.1f", char_comp->GetGold());

    game::types::EntityID realm_id = char_comp->GetPrimaryTitle();
    if (realm_id != 0) {
        ImGui::Text("Primary Title: Realm %u", realm_id);
    } else {
        ImGui::TextDisabled("Primary Title: None");
    }

    ImGui::EndChild();
}

void CharacterWindow::RenderStatsPanel(core::ecs::EntityID char_id) {
    auto char_comp = entity_manager_.GetComponent<game::character::CharacterComponent>(char_id);
    if (!char_comp) return;

    ImGui::BeginChild("Stats", ImVec2(0, 200), true);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("ATTRIBUTES");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Diplomacy
    ImGui::Text("Diplomacy:");
    ImGui::SameLine(120);
    ImGui::ProgressBar(char_comp->GetDiplomacy() / 20.0f, ImVec2(-1, 0),
                      std::to_string(char_comp->GetDiplomacy()).c_str());

    // Martial
    ImGui::Text("Martial:");
    ImGui::SameLine(120);
    ImGui::ProgressBar(char_comp->GetMartial() / 20.0f, ImVec2(-1, 0),
                      std::to_string(char_comp->GetMartial()).c_str());

    // Stewardship
    ImGui::Text("Stewardship:");
    ImGui::SameLine(120);
    ImGui::ProgressBar(char_comp->GetStewardship() / 20.0f, ImVec2(-1, 0),
                      std::to_string(char_comp->GetStewardship()).c_str());

    // Intrigue
    ImGui::Text("Intrigue:");
    ImGui::SameLine(120);
    ImGui::ProgressBar(char_comp->GetIntrigue() / 20.0f, ImVec2(-1, 0),
                      std::to_string(char_comp->GetIntrigue()).c_str());

    // Learning
    ImGui::Text("Learning:");
    ImGui::SameLine(120);
    ImGui::ProgressBar(char_comp->GetLearning() / 20.0f, ImVec2(-1, 0),
                      std::to_string(char_comp->GetLearning()).c_str());

    ImGui::EndChild();
}

void CharacterWindow::RenderTraitsPanel(core::ecs::EntityID char_id) {
    auto traits_comp = entity_manager_.GetComponent<game::character::TraitsComponent>(char_id);
    if (!traits_comp) return;

    ImGui::BeginChild("Traits", ImVec2(0, 150), true);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("TRAITS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    const auto& active_traits = traits_comp->active_traits;

    if (active_traits.empty()) {
        ImGui::TextDisabled("No traits");
    } else {
        for (const auto& active_trait : active_traits) {
            ImGui::BulletText("%s", active_trait.trait_id.c_str());
        }
    }

    ImGui::EndChild();
}

void CharacterWindow::RenderRelationshipsPanel(core::ecs::EntityID char_id) {
    auto rel_comp = entity_manager_.GetComponent<game::character::CharacterRelationshipsComponent>(char_id);
    if (!rel_comp) return;

    ImGui::BeginChild("Relationships", ImVec2(0, 250), true);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("RELATIONSHIPS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    // Friends
    ImGui::Text("Friends:");
    auto friends_list = rel_comp->GetFriends();
    if (friends_list.empty()) {
        ImGui::Indent();
        ImGui::TextDisabled("None");
        ImGui::Unindent();
    } else {
        ImGui::Indent();
        for (const auto& friend_id : friends_list) {
            // C3 FIX: Use proper versioned EntityID conversion
            auto friend_versioned_id = character_system_.LegacyToVersionedEntityID(friend_id);
            auto friend_comp = entity_manager_.GetComponent<game::character::CharacterComponent>(friend_versioned_id);
            if (friend_comp) {
                // M3 FIX: Make relationship names clickable
                if (ImGui::Selectable(friend_comp->GetName().c_str(), false)) {
                    ShowCharacter(friend_versioned_id);
                }
                ImGui::SameLine();
                double bond_strength = rel_comp->GetFriendshipBondStrength(friend_id);
                ImGui::Text("(Bond: %.1f)", bond_strength);
            }
        }
        ImGui::Unindent();
    }

    ImGui::Spacing();

    // Rivals
    ImGui::Text("Rivals:");
    auto rivals_list = rel_comp->GetRivals();
    if (rivals_list.empty()) {
        ImGui::Indent();
        ImGui::TextDisabled("None");
        ImGui::Unindent();
    } else {
        ImGui::Indent();
        for (const auto& rival_id : rivals_list) {
            // C3 FIX: Use proper versioned EntityID conversion
            auto rival_versioned_id = character_system_.LegacyToVersionedEntityID(rival_id);
            auto rival_comp = entity_manager_.GetComponent<game::character::CharacterComponent>(rival_versioned_id);
            if (rival_comp) {
                // M3 FIX: Make relationship names clickable
                if (ImGui::Selectable(rival_comp->GetName().c_str(), false)) {
                    ShowCharacter(rival_versioned_id);
                }
            }
        }
        ImGui::Unindent();
    }

    ImGui::Spacing();

    // Family
    ImGui::Text("Family:");
    ImGui::Indent();
    if (rel_comp->father != 0) {
        // C3 FIX: Use proper versioned EntityID conversion
        auto father_versioned_id = character_system_.LegacyToVersionedEntityID(rel_comp->father);
        auto father_comp = entity_manager_.GetComponent<game::character::CharacterComponent>(father_versioned_id);
        if (father_comp) {
            ImGui::Text("Father: ");
            ImGui::SameLine();
            // M3 FIX: Make relationship names clickable
            if (ImGui::Selectable(father_comp->GetName().c_str(), false)) {
                ShowCharacter(father_versioned_id);
            }
        }
    }
    if (rel_comp->mother != 0) {
        // C3 FIX: Use proper versioned EntityID conversion
        auto mother_versioned_id = character_system_.LegacyToVersionedEntityID(rel_comp->mother);
        auto mother_comp = entity_manager_.GetComponent<game::character::CharacterComponent>(mother_versioned_id);
        if (mother_comp) {
            ImGui::Text("Mother: ");
            ImGui::SameLine();
            // M3 FIX: Make relationship names clickable
            if (ImGui::Selectable(mother_comp->GetName().c_str(), false)) {
                ShowCharacter(mother_versioned_id);
            }
        }
    }
    if (!rel_comp->children.empty()) {
        ImGui::Text("Children: %zu", rel_comp->children.size());
    }
    ImGui::Unindent();

    ImGui::EndChild();
}

void CharacterWindow::RenderLifeEventsPanel(core::ecs::EntityID char_id) {
    auto events_comp = entity_manager_.GetComponent<game::character::CharacterLifeEventsComponent>(char_id);
    if (!events_comp) return;

    ImGui::BeginChild("LifeEvents", ImVec2(0, 200), true);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("LIFE EVENTS");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    const auto& events = events_comp->life_events;

    if (events.empty()) {
        ImGui::TextDisabled("No events recorded");
    } else {
        // Show most recent events first
        int count = 0;
        for (auto it = events.rbegin(); it != events.rend() && count < 10; ++it, ++count) {
            ImGui::BulletText("%s", it->description.c_str());
        }

        if (events.size() > 10) {
            ImGui::TextDisabled("... and %zu more events", events.size() - 10);
        }
    }

    ImGui::EndChild();
}

void CharacterWindow::RenderEducationPanel(core::ecs::EntityID char_id) {
    auto edu_comp = entity_manager_.GetComponent<game::character::CharacterEducationComponent>(char_id);
    if (!edu_comp) return;

    ImGui::BeginChild("Education", ImVec2(0, 150), true);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.83f, 0.69f, 0.22f, 1.0f));
    ImGui::Text("EDUCATION");
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();

    if (edu_comp->IsInEducation()) {
        ImGui::Text("Currently in education");
        ImGui::Text("Focus: %s", edu_comp->GetEducationFocusString().c_str());

        // Show XP progress for the education focus
        int xp_value = 0;
        switch (edu_comp->education_focus) {
            case game::character::EducationFocus::DIPLOMACY:
                xp_value = edu_comp->skill_xp.diplomacy_xp;
                break;
            case game::character::EducationFocus::MARTIAL:
                xp_value = edu_comp->skill_xp.martial_xp;
                break;
            case game::character::EducationFocus::STEWARDSHIP:
                xp_value = edu_comp->skill_xp.stewardship_xp;
                break;
            case game::character::EducationFocus::INTRIGUE:
                xp_value = edu_comp->skill_xp.intrigue_xp;
                break;
            case game::character::EducationFocus::LEARNING:
                xp_value = edu_comp->skill_xp.learning_xp;
                break;
            default:
                break;
        }

        if (xp_value > 0) {
            ImGui::Text("Experience: %d XP", xp_value);
        }

        // Show duration
        int duration = edu_comp->GetEducationDurationYears();
        if (duration > 0) {
            ImGui::Text("Duration: %d year%s", duration, duration == 1 ? "" : "s");
        }
    } else {
        ImGui::TextDisabled("Not currently in education");
    }

    ImGui::Spacing();

    // Show completed education
    if (!edu_comp->IsInEducation() && edu_comp->is_educated) {
        ImGui::Text("Education completed: %s", edu_comp->GetEducationFocusString().c_str());
        ImGui::Text("Quality: %s", edu_comp->GetEducationQualityString().c_str());
    }

    ImGui::EndChild();
}

// ============================================================================
// Helper Methods
// ============================================================================

bool CharacterWindow::IsCharacterDead(core::ecs::EntityID char_id) const {
    // M1 FIX: Proper death detection via CharacterLifeEventsComponent
    auto events_comp = entity_manager_.GetComponent<game::character::CharacterLifeEventsComponent>(char_id);
    if (events_comp) {
        // Check if death_date has been set (non-zero time_since_epoch)
        return events_comp->death_date.time_since_epoch().count() != 0;
    }
    // Fallback: If no events component, assume alive
    return false;
}

} // namespace ui
