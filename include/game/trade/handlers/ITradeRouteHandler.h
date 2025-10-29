// ============================================================================
// Mechanica Imperii - Trade Route Handler Interface
// Strategy Pattern for Trade Route Operations
// ============================================================================

#pragma once

#include "core/types/game_types.h"
#include "game/trade/TradeSystem.h"
#include <string>
#include <unordered_map>

namespace game::trade {

    // Forward declarations
    class TradeRepository;
    class TradeCalculator;
    class TradePathfinder;

    /**
     * @brief Result of a trade route operation
     */
    struct TradeRouteOperationResult {
        bool success = false;
        std::string message;
        std::string route_id; // ID of the affected route
        double economic_impact = 0.0; // Expected profit/loss per month

        static TradeRouteOperationResult Success(const std::string& msg, const std::string& id = "",
                                                double impact = 0.0) {
            TradeRouteOperationResult result;
            result.success = true;
            result.message = msg;
            result.route_id = id;
            result.economic_impact = impact;
            return result;
        }

        static TradeRouteOperationResult Failure(const std::string& msg) {
            TradeRouteOperationResult result;
            result.success = false;
            result.message = msg;
            return result;
        }
    };

    /**
     * @brief Base interface for all trade route operations
     *
     * Uses Strategy Pattern to encapsulate different route operations:
     * - Route establishment
     * - Route disruption
     * - Route restoration
     * - Route abandonment
     */
    class ITradeRouteHandler {
    public:
        virtual ~ITradeRouteHandler() = default;

        /**
         * @brief Execute the trade route operation
         * @param parameters Operation-specific parameters
         * @return Result of the operation
         */
        virtual TradeRouteOperationResult Execute(
            const std::unordered_map<std::string, double>& parameters
        ) = 0;

        /**
         * @brief Validate if the operation can be performed
         * @return true if operation is valid
         */
        virtual bool Validate(std::string& failure_reason) const = 0;

        /**
         * @brief Get the name of this operation
         */
        virtual std::string GetOperationName() const = 0;

        /**
         * @brief Get estimated cost/benefit of this operation
         */
        virtual double GetEstimatedImpact() const = 0;
    };

} // namespace game::trade
