#pragma once

#include "core/ECS/EntityManager.h"
#include "game/systems/CharacterSystem.h"
#include "core/types/game_types.h"
#include "imgui.h"

namespace ui {
    class WindowManager; // Forward declaration
    class PortraitGenerator; // Forward declaration

    /**
     * CharacterWindow - Displays character information
     * Provides both a list view and detailed character sheets
     */
    class CharacterWindow {
    public:
        CharacterWindow(core::ecs::EntityManager& entity_manager,
                       game::character::CharacterSystem& character_system);
        ~CharacterWindow() = default;

        void Render(WindowManager& window_manager, game::types::EntityID player_entity);

        /**
         * @brief Set the portrait generator for rendering character portraits
         */
        void SetPortraitGenerator(PortraitGenerator* generator) { portrait_generator_ = generator; }

        /**
         * @brief Open the window and show a specific character
         */
        void ShowCharacter(core::ecs::EntityID character_id);

    private:
        core::ecs::EntityManager& entity_manager_;
        game::character::CharacterSystem& character_system_;
        PortraitGenerator* portrait_generator_ = nullptr;

        // UI state
        core::ecs::EntityID selected_character_;  // Currently selected character
        int selected_tab_ = 0;                     // 0 = List, 1 = Details
        char search_buffer_[256] = {0};            // Search filter
        int sort_mode_ = 0;                        // 0 = Name, 1 = Age, 2 = Realm
        bool show_dead_characters_ = false;        // Include dead characters in list

        // Filtering cache (C2 fix: avoid per-frame allocations)
        std::vector<core::ecs::EntityID> cached_filtered_characters_;
        std::string last_search_text_;
        int last_sort_mode_ = -1;
        bool last_show_dead_ = false;
        bool filter_cache_dirty_ = true;

        // Render methods
        void RenderCharacterList();
        void RenderCharacterDetails();
        void RenderCharacterListItem(core::ecs::EntityID char_id);
        void RenderBasicInfo(core::ecs::EntityID char_id);
        void RenderStatsPanel(core::ecs::EntityID char_id);
        void RenderTraitsPanel(core::ecs::EntityID char_id);
        void RenderRelationshipsPanel(core::ecs::EntityID char_id);
        void RenderLifeEventsPanel(core::ecs::EntityID char_id);
        void RenderEducationPanel(core::ecs::EntityID char_id);
    };
}
