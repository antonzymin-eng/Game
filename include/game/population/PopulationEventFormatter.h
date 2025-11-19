// ============================================================================
// PopulationEventFormatter.h - Population Event Formatting System
// Created: 2025-11-18
// Location: include/game/population/PopulationEventFormatter.h
// ============================================================================

#pragma once

#include "game/population/PopulationEvents.h"
#include "game/population/PopulationTypes.h"
#include "core/types/game_types.h"
#include <string>
#include <sstream>

namespace game::population {

    /**
     * @brief Formats population events into human-readable strings
     *
     * Provides consistent formatting for logging, UI display, and event
     * notifications.
     */
    class PopulationEventFormatter {
    public:
        PopulationEventFormatter() = default;
        ~PopulationEventFormatter() = default;

        // Event formatting methods
        std::string FormatPopulationUpdate(const PopulationUpdateEvent& event) const;

        std::string FormatDemographicChange(const DemographicChangeEvent& event) const;

        std::string FormatHealthCrisis(const HealthCrisisEvent& event) const;

        std::string FormatSocialMobility(const SocialMobilityEvent& event) const;

        std::string FormatLegalStatusChange(const LegalStatusChangeEvent& event) const;

        std::string FormatGuildAdvancement(const GuildAdvancementEvent& event) const;

        std::string FormatMigration(const MigrationEvent& event) const;

        std::string FormatCulturalAssimilation(const CulturalAssimilationEvent& event) const;

        std::string FormatReligiousConversion(const ReligiousConversionEvent& event) const;

        std::string FormatEmploymentShift(const EmploymentShiftEvent& event) const;

        std::string FormatSettlementEvolution(const SettlementEvolutionEvent& event) const;

        std::string FormatPlague(const PlagueEvent& event) const;

        std::string FormatFamine(const FamineEvent& event) const;

        std::string FormatNaturalDisaster(const NaturalDisasterEvent& event) const;

        std::string FormatSocialUnrest(const SocialUnrestEvent& event) const;

        // Summary formatting
        std::string FormatPopulationSummary(const PopulationComponent& population,
                                           game::types::EntityID entity_id) const;

        std::string FormatSettlementSummary(const SettlementComponent& settlements,
                                           game::types::EntityID entity_id) const;

        // Utility formatting
        std::string FormatPercentage(double value, int decimals = 1) const;

        std::string FormatPopulationCount(int count) const;

        std::string FormatEntityId(game::types::EntityID entity_id) const;

        std::string FormatDuration(int months) const;

        std::string FormatSeverity(double severity) const;

    private:
        // Helper methods
        std::string GetSocialClassName(SocialClass social_class) const;

        std::string GetLegalStatusName(LegalStatus legal_status) const;

        std::string GetEmploymentName(EmploymentType employment) const;

        std::string GetSettlementTypeName(SettlementType type) const;

        std::string FormatThousands(int value) const;

        std::string GetTrendIndicator(double growth_rate) const;
    };

} // namespace game::population
