// Created: November 10, 2025
// Location: include/game/components/NobleArtsComponent.h

#pragma once

#include "utils/PlatformMacros.h"
#include "core/ECS/IComponent.h"
#include "core/types/game_types.h"
#include <memory>
#include <vector>
#include <string>

namespace game {
namespace character {

// Noble arts and cultural component for characters
// Tracks skills in arts, culture, and noble pursuits
class NobleArtsComponent : public game::core::Component<NobleArtsComponent> {
public:
    NobleArtsComponent()
        : m_poetry(0)
        , m_music(0)
        , m_painting(0)
        , m_philosophy(0)
        , m_theology(0)
        , m_architecture(0)
        , m_strategy(0)
        , m_etiquette(0) {}

    ~NobleArtsComponent() = default;

    // Arts skill accessors (0-10 scale)
    uint8_t GetPoetrySkill() const { return m_poetry; }
    void SetPoetrySkill(uint8_t skill) { m_poetry = skill; }

    uint8_t GetMusicSkill() const { return m_music; }
    void SetMusicSkill(uint8_t skill) { m_music = skill; }

    uint8_t GetPaintingSkill() const { return m_painting; }
    void SetPaintingSkill(uint8_t skill) { m_painting = skill; }

    uint8_t GetPhilosophySkill() const { return m_philosophy; }
    void SetPhilosophySkill(uint8_t skill) { m_philosophy = skill; }

    uint8_t GetTheologySkill() const { return m_theology; }
    void SetTheologySkill(uint8_t skill) { m_theology = skill; }

    uint8_t GetArchitectureSkill() const { return m_architecture; }
    void SetArchitectureSkill(uint8_t skill) { m_architecture = skill; }

    uint8_t GetStrategySkill() const { return m_strategy; }
    void SetStrategySkill(uint8_t skill) { m_strategy = skill; }

    uint8_t GetEtiquetteSkill() const { return m_etiquette; }
    void SetEtiquetteSkill(uint8_t skill) { m_etiquette = skill; }

    // Works and achievements
    const std::vector<std::string>& GetCreatedWorks() const { return m_createdWorks; }
    void AddCreatedWork(const std::string& work) { m_createdWorks.push_back(work); }

    // Cultural influence
    float GetCulturalInfluence() const {
        return (m_poetry + m_music + m_painting + m_philosophy +
                m_theology + m_architecture + m_etiquette) / 7.0f;
    }

    // Component interface
    std::unique_ptr<game::core::IComponent> Clone() const override {
        auto clone = std::make_unique<NobleArtsComponent>();
        clone->m_poetry = m_poetry;
        clone->m_music = m_music;
        clone->m_painting = m_painting;
        clone->m_philosophy = m_philosophy;
        clone->m_theology = m_theology;
        clone->m_architecture = m_architecture;
        clone->m_strategy = m_strategy;
        clone->m_etiquette = m_etiquette;
        clone->m_createdWorks = m_createdWorks;
        return clone;
    }

private:
    uint8_t m_poetry;
    uint8_t m_music;
    uint8_t m_painting;
    uint8_t m_philosophy;
    uint8_t m_theology;
    uint8_t m_architecture;
    uint8_t m_strategy;
    uint8_t m_etiquette;

    std::vector<std::string> m_createdWorks;
};

} // namespace character
} // namespace game
