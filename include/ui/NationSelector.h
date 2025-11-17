#pragma once

#include <string>
#include <vector>
#include <functional>

namespace ui {
    /**
     * NationSelector - Nation selection screen for new game setup
     * Displays a map, nation list, and game options
     */
    class NationSelector {
    public:
        struct NationInfo {
            std::string name;
            std::string tag;
            int provinces;
            int population;
            bool is_playable;
        };

        struct GameOptions {
            int start_year = 1000;
            bool ironman_mode = false;
            std::string difficulty = "Normal";
            bool historical_events = true;
        };

        NationSelector();
        ~NationSelector() = default;

        void Render();
        void Update();

        bool IsGameReady() const { return game_ready_; }
        const NationInfo* GetSelectedNation() const { return selected_nation_; }
        const GameOptions& GetGameOptions() const { return game_options_; }

        void SetAvailableNations(const std::vector<NationInfo>& nations);

    private:
        bool game_ready_;
        const NationInfo* selected_nation_;
        GameOptions game_options_;
        std::vector<NationInfo> available_nations_;
        int selected_nation_index_;
        char search_buffer_[256];

        void RenderMapView();
        void RenderNationList();
        void RenderNationDetails();
        void RenderGameOptions();
        void RenderStartButton();
    };
}
