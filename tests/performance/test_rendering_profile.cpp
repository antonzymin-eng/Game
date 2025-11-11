#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "core/ECS/EntityManager.h"
#include "map/ProvinceRenderComponent.h"
#include "map/render/ViewportCuller.h"

int main() {
    using namespace game::map;

    constexpr int kProvinceCount = 5000;
    constexpr int kIterations = 50;

    core::ecs::EntityManager entity_manager;

    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> position_dist(-5000.0f, 5000.0f);
    std::uniform_real_distribution<float> size_dist(10.0f, 120.0f);

    for (int i = 0; i < kProvinceCount; ++i) {
        auto entity = entity_manager.CreateEntity("Province" + std::to_string(i));
        auto component = entity_manager.AddComponent<ProvinceRenderComponent>(entity);
        component->province_id = static_cast<uint32_t>(i);

        const float cx = position_dist(rng);
        const float cy = position_dist(rng);
        const float half_width = size_dist(rng);
        const float half_height = size_dist(rng);

        component->center_position = Vector2(cx, cy);
        component->bounding_box = Rect(cx - half_width, cy - half_height, cx + half_width, cy + half_height);
        component->name = "Province_" + std::to_string(i);

        component->boundary_points = {
            {cx - half_width, cy - half_height},
            {cx + half_width, cy - half_height},
            {cx + half_width, cy + half_height},
            {cx - half_width, cy + half_height},
        };

        component->features.clear();
        component->fill_color = Color(100, 120, 200, 255);
        component->border_color = Color(40, 40, 80, 255);
    }

    Camera2D camera;
    camera.position = {0.0f, 0.0f};
    camera.zoom = 1.0f;
    camera.viewport_width = 1920.0f;
    camera.viewport_height = 1080.0f;

    ViewportCuller culler;

    std::uniform_real_distribution<float> camera_move_dist(-2000.0f, 2000.0f);

    auto start = std::chrono::high_resolution_clock::now();
    for (int iteration = 0; iteration < kIterations; ++iteration) {
        camera.position = {camera_move_dist(rng), camera_move_dist(rng)};
        culler.UpdateViewport(camera);
        culler.UpdateProvinceVisibility(entity_manager);
    }
    auto end = std::chrono::high_resolution_clock::now();

    const double total_ms = std::chrono::duration<double, std::milli>(end - start).count();
    const double avg_ms = total_ms / static_cast<double>(kIterations);

    std::cout << "Viewport culling profile" << std::endl;
    std::cout << "Provinces: " << kProvinceCount << ", iterations: " << kIterations << std::endl;
    std::cout << "Total time: " << total_ms << " ms" << std::endl;
    std::cout << "Average per iteration: " << avg_ms << " ms" << std::endl;
    std::cout << "Last visible count: " << culler.GetVisibleProvinceCount() << std::endl;

    return 0;
}
