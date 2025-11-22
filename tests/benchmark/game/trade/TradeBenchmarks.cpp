// ============================================================================
// Mechanica Imperii - Trade System Performance Benchmarks
// Performance testing and optimization validation
// Created: 2025-11-22
// Location: tests/benchmark/game/trade/TradeBenchmarks.cpp
// ============================================================================

#include <benchmark/benchmark.h>
#include "game/trade/TradeSystem.h"
#include "game/trade/TradeCalculator.h"
#include "core/ECS/EntityManager.h"
#include "core/threading/ThreadSafeMessageBus.h"
#include <memory>
#include <random>

using namespace game::trade;
using namespace game::types;

// ============================================================================
// Benchmark Fixtures
// ============================================================================

class TradeBenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        entity_manager = std::make_unique<core::ecs::EntityManager>();
        message_bus = std::make_unique<::core::threading::ThreadSafeMessageBus>();
        access_manager = std::make_unique<::core::ecs::ComponentAccessManager>(*entity_manager);

        trade_system = std::make_unique<TradeSystem>(*access_manager, *message_bus);
        trade_system->Initialize();

        // Create provinces for benchmarking
        for (int i = 0; i < 100; ++i) {
            provinces.push_back(entity_manager->CreateEntity());
        }
    }

    void TearDown(const ::benchmark::State& state) override {
        trade_system->Shutdown();
        provinces.clear();
    }

    EntityID GetProvinceID(size_t index) const {
        return static_cast<EntityID>(provinces[index].id);
    }

    std::unique_ptr<core::ecs::EntityManager> entity_manager;
    std::unique_ptr<::core::threading::ThreadSafeMessageBus> message_bus;
    std::unique_ptr<::core::ecs::ComponentAccessManager> access_manager;
    std::unique_ptr<TradeSystem> trade_system;
    std::vector<core::ecs::EntityID> provinces;
};

// ============================================================================
// Route Establishment Benchmarks
// ============================================================================

BENCHMARK_F(TradeBenchmarkFixture, BM_EstablishSingleRoute)(benchmark::State& state) {
    for (auto _ : state) {
        EntityID src = GetProvinceID(0);
        EntityID dst = GetProvinceID(1);

        state.PauseTiming();
        // Clean up route from previous iteration
        auto routes = trade_system->GetAllTradeRoutes();
        for (const auto& route : routes) {
            trade_system->AbandonTradeRoute(route.route_id);
        }
        state.ResumeTiming();

        auto route = trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
        benchmark::DoNotOptimize(route);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(TradeBenchmarkFixture, BM_Establish100Routes)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        // Clean up routes from previous iteration
        auto routes = trade_system->GetAllTradeRoutes();
        for (const auto& route : routes) {
            trade_system->AbandonTradeRoute(route.route_id);
        }
        state.ResumeTiming();

        for (int i = 0; i < 100; ++i) {
            EntityID src = GetProvinceID(i % 50);
            EntityID dst = GetProvinceID((i + 1) % 50);
            auto route = trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
            benchmark::DoNotOptimize(route);
        }
    }

    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(TradeBenchmarkFixture, BM_Establish1000Routes)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        auto routes = trade_system->GetAllTradeRoutes();
        for (const auto& route : routes) {
            trade_system->AbandonTradeRoute(route.route_id);
        }
        state.ResumeTiming();

        for (int i = 0; i < 1000; ++i) {
            EntityID src = GetProvinceID(i % provinces.size());
            EntityID dst = GetProvinceID((i + 1) % provinces.size());
            ResourceType resource = static_cast<ResourceType>(i % 3);
            auto route = trade_system->EstablishTradeRoute(src, dst, resource);
            benchmark::DoNotOptimize(route);
        }
    }

    state.SetItemsProcessed(state.iterations() * 1000);
}

// ============================================================================
// Update Benchmarks
// ============================================================================

BENCHMARK_F(TradeBenchmarkFixture, BM_UpdateWith10Routes)(benchmark::State& state) {
    // Setup - Create 10 routes
    for (int i = 0; i < 10; ++i) {
        EntityID src = GetProvinceID(i);
        EntityID dst = GetProvinceID((i + 1) % 20);
        trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
    }

    for (auto _ : state) {
        trade_system->Update(0.016f);  // ~60 FPS
    }

    state.SetItemsProcessed(state.iterations() * 10);
}

BENCHMARK_F(TradeBenchmarkFixture, BM_UpdateWith100Routes)(benchmark::State& state) {
    // Setup - Create 100 routes
    for (int i = 0; i < 100; ++i) {
        EntityID src = GetProvinceID(i % 50);
        EntityID dst = GetProvinceID((i + 1) % 50);
        trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
    }

    for (auto _ : state) {
        trade_system->Update(0.016f);
    }

    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(TradeBenchmarkFixture, BM_UpdateWith500Routes)(benchmark::State& state) {
    // Setup - Create 500 routes
    for (int i = 0; i < 500; ++i) {
        EntityID src = GetProvinceID(i % provinces.size());
        EntityID dst = GetProvinceID((i + 1) % provinces.size());
        ResourceType resource = static_cast<ResourceType>(i % 3);
        trade_system->EstablishTradeRoute(src, dst, resource);
    }

    for (auto _ : state) {
        trade_system->Update(0.016f);
    }

    state.SetItemsProcessed(state.iterations() * 500);
}

// ============================================================================
// Route Query Benchmarks
// ============================================================================

BENCHMARK_F(TradeBenchmarkFixture, BM_GetRouteQuery)(benchmark::State& state) {
    // Setup
    EntityID src = GetProvinceID(0);
    EntityID dst = GetProvinceID(1);
    std::string route_id = trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);

    for (auto _ : state) {
        auto route = trade_system->GetRoute(route_id);
        benchmark::DoNotOptimize(route);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(TradeBenchmarkFixture, BM_GetAllTradeRoutes_100Routes)(benchmark::State& state) {
    // Setup - Create 100 routes
    for (int i = 0; i < 100; ++i) {
        EntityID src = GetProvinceID(i % 50);
        EntityID dst = GetProvinceID((i + 1) % 50);
        trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
    }

    for (auto _ : state) {
        auto routes = trade_system->GetAllTradeRoutes();
        benchmark::DoNotOptimize(routes);
    }

    state.SetItemsProcessed(state.iterations() * 100);
}

BENCHMARK_F(TradeBenchmarkFixture, BM_GetRoutesFromProvince)(benchmark::State& state) {
    // Setup - Create routes from province 0
    EntityID hub = GetProvinceID(0);
    for (int i = 1; i < 20; ++i) {
        EntityID spoke = GetProvinceID(i);
        trade_system->EstablishTradeRoute(hub, spoke, ResourceType::FOOD);
    }

    for (auto _ : state) {
        auto routes = trade_system->GetRoutesFromProvince(hub);
        benchmark::DoNotOptimize(routes);
    }

    state.SetItemsProcessed(state.iterations() * 19);
}

// ============================================================================
// Pathfinder Benchmarks
// ============================================================================

BENCHMARK_F(TradeBenchmarkFixture, BM_PathfinderCacheMiss)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        trade_system->ClearPathfinderCache();
        state.ResumeTiming();

        EntityID src = GetProvinceID(state.iterations() % provinces.size());
        EntityID dst = GetProvinceID((state.iterations() + 1) % provinces.size());
        auto route = trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
        benchmark::DoNotOptimize(route);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(TradeBenchmarkFixture, BM_PathfinderCacheHit)(benchmark::State& state) {
    // Warm up cache
    EntityID src = GetProvinceID(0);
    EntityID dst = GetProvinceID(10);
    trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);

    for (auto _ : state) {
        // Same route - should hit cache
        auto route = trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
        benchmark::DoNotOptimize(route);
    }

    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Calculator Benchmarks
// ============================================================================

BENCHMARK(BM_CalculateMarketPrice) {
    for (auto _ : state) {
        double price = TradeCalculator::CalculateMarketPrice(10.0, 1.5, 0.8);
        benchmark::DoNotOptimize(price);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_CalculateSupplyLevel) {
    for (auto _ : state) {
        double supply = TradeCalculator::CalculateSupplyLevel(1, ResourceType::FOOD, 1000);
        benchmark::DoNotOptimize(supply);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_CalculateTransportCost) {
    for (auto _ : state) {
        double cost = TradeCalculator::CalculateTransportCost(500.0, 1.0, 0.1, 0.9);
        benchmark::DoNotOptimize(cost);
    }

    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Hub Management Benchmarks
// ============================================================================

BENCHMARK_F(TradeBenchmarkFixture, BM_CreateTradeHub)(benchmark::State& state) {
    int hub_index = 0;

    for (auto _ : state) {
        EntityID province = GetProvinceID(hub_index % provinces.size());
        trade_system->CreateTradeHub(province, "Benchmark Hub", HubType::LOCAL_MARKET);
        hub_index++;
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(TradeBenchmarkFixture, BM_EvolveHub)(benchmark::State& state) {
    // Setup - Create hub with routes
    EntityID hub = GetProvinceID(0);
    trade_system->CreateTradeHub(hub, "Evolving Hub", HubType::LOCAL_MARKET);

    for (int i = 1; i < 30; ++i) {
        EntityID spoke = GetProvinceID(i);
        trade_system->EstablishTradeRoute(hub, spoke, ResourceType::FOOD);
    }

    for (auto _ : state) {
        trade_system->EvolveTradeHub(hub);
    }

    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Market Dynamics Benchmarks
// ============================================================================

BENCHMARK_F(TradeBenchmarkFixture, BM_CalculateMarketPriceForProvince)(benchmark::State& state) {
    EntityID province = GetProvinceID(0);

    for (auto _ : state) {
        double price = trade_system->CalculateMarketPrice(province, ResourceType::FOOD);
        benchmark::DoNotOptimize(price);
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(TradeBenchmarkFixture, BM_ApplyPriceShock)(benchmark::State& state) {
    EntityID province = GetProvinceID(0);

    for (auto _ : state) {
        trade_system->ApplyPriceShock(province, ResourceType::FOOD, 0.2, "Benchmark shock");
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(TradeBenchmarkFixture, BM_UpdateMarketPrices_100Provinces)(benchmark::State& state) {
    // Setup - Create markets in all provinces
    for (size_t i = 0; i < provinces.size(); ++i) {
        EntityID province = GetProvinceID(i);
        trade_system->EstablishTradeRoute(province, GetProvinceID((i + 1) % provinces.size()),
                                         ResourceType::FOOD);
    }

    for (auto _ : state) {
        trade_system->UpdateMarketPrices();
    }

    state.SetItemsProcessed(state.iterations() * provinces.size());
}

// ============================================================================
// Configuration Benchmarks
// ============================================================================

BENCHMARK_F(TradeBenchmarkFixture, BM_ConfigSave)(benchmark::State& state) {
    const auto& config = trade_system->GetConfig();
    std::string config_file = "/tmp/trade_benchmark_config.json";

    for (auto _ : state) {
        config.SaveToFile(config_file);
    }

    std::remove(config_file.c_str());
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_F(TradeBenchmarkFixture, BM_ConfigLoad)(benchmark::State& state) {
    // Setup - Create config file
    TradeSystemConfig config;
    std::string config_file = "/tmp/trade_benchmark_config.json";
    config.SaveToFile(config_file);

    for (auto _ : state) {
        TradeSystemConfig loaded_config;
        loaded_config.LoadFromFile(config_file);
        benchmark::DoNotOptimize(loaded_config);
    }

    std::remove(config_file.c_str());
    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// Comprehensive System Benchmark
// ============================================================================

BENCHMARK_F(TradeBenchmarkFixture, BM_FullSystemSimulation_1000Frames)(benchmark::State& state) {
    // Setup - Create realistic trade network
    // 20 hubs, 100 routes
    for (int i = 0; i < 20; ++i) {
        EntityID hub = GetProvinceID(i);
        trade_system->CreateTradeHub(hub, "Hub " + std::to_string(i), HubType::REGIONAL_HUB);
    }

    for (int i = 0; i < 100; ++i) {
        EntityID src = GetProvinceID(i % 50);
        EntityID dst = GetProvinceID((i + 1) % 50);
        trade_system->EstablishTradeRoute(src, dst, ResourceType::FOOD);
    }

    for (auto _ : state) {
        // Simulate 1000 game frames
        for (int frame = 0; frame < 1000; ++frame) {
            trade_system->Update(0.016f);  // ~60 FPS
        }
    }

    state.SetItemsProcessed(state.iterations() * 1000);
}

// ============================================================================
// Main Benchmark Runner
// ============================================================================

BENCHMARK_MAIN();
