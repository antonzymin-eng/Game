// ============================================================================
// Date/Time Created: Tuesday, September 16, 2025 - 11:45 AM PST
// Intended Folder Location: include/game/gameplay/GameWorld.h
// GameWorld.h - Fixed Structure and Ownership Issues
// ============================================================================

#pragma once

#include "game/province/ProvinceManagementSystem.h"

#include "game/config/GameConfig.h"

#include <jsoncpp/json/json.h>

#include <vector>

namespace game {

    class GameWorld {
    public:
        std::vector<Province> provinces;
        int selected_province_id = -1;

        GameWorld() = default;
        ~GameWorld() = default;

        // FIXED: Remove misplaced compatibility fields
        // These were being set in province loop but belonged elsewhere
        // If needed, they should be Province members or removed entirely

        // Initialization methods
        void initializeTestProvinces();
        void initializeTestWorld() { initializeTestProvinces(); }

        // Province access - mark simple getters as noexcept
        Province* getProvinceById(int id) noexcept;
        const Province* getProvinceById(int id) const noexcept;
        int getProvinceCount() const noexcept { return static_cast<int>(provinces.size()); }

        // Province management - FIXED with proper memory management
        void addProvince(const Province& province);
        void addProvince(Province&& province);  // Move version for efficiency
        bool removeProvinceById(int id);
        void reserveProvinces(size_t count);  // Pre-allocate for performance

        // Selection management
        void selectProvince(int id) noexcept;
        void clearSelection() noexcept { selected_province_id = -1; }
        Province* getSelectedProvince() noexcept;
        const Province* getSelectedProvince() const noexcept;
        bool hasSelection() const noexcept { return selected_province_id >= 0; }

        // Utility methods
        void clear();
        bool empty() const noexcept { return provinces.empty(); }
        size_t size() const noexcept { return provinces.size(); }

        // Iterator support for range-based loops
        std::vector<Province>::iterator begin() { return provinces.begin(); }
        std::vector<Province>::iterator end() { return provinces.end(); }
        std::vector<Province>::const_iterator begin() const { return provinces.begin(); }
        std::vector<Province>::const_iterator end() const { return provinces.end(); }

    private:
        // FIXED: Validate province ID before operations
        bool isValidProvinceId(int id) const noexcept;
    };

} // namespace game
