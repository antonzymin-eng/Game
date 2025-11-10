// Created: September 17, 2025 - 16:15:42
// Location: src/core/Threading/ThreadedSystemManager.cpp
// Mechanica Imperii - Multi-threaded System Coordination Implementation (FIXED)

#include "core/threading/ThreadedSystemManager.h"
#include "core/threading/ScopeGuards.h"
#include "core/Constants.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>

namespace core::threading {

    // ============================================================================
    // GameClock Implementation
    // ============================================================================

    GameClock::GameClock() {
        m_start_time = std::chrono::steady_clock::now();
        m_last_frame_time = m_start_time;
    }

    void GameClock::Update() {
        auto current_time = std::chrono::steady_clock::now();
        
        // Calculate delta time in seconds
        auto delta_duration = current_time - m_last_frame_time;
        double delta_seconds = std::chrono::duration<double>(delta_duration).count();
        m_delta_time.store(delta_seconds);
        
        // Update game time
        double current_game_time = m_game_time.load();
        m_game_time.store(current_game_time + delta_seconds);
        
        // Update frame number
        m_frame_number.fetch_add(1);
        
        m_last_frame_time = current_time;
    }

    void GameClock::Reset() {
        m_start_time = std::chrono::steady_clock::now();
        m_last_frame_time = m_start_time;
        m_game_time.store(0.0);
        m_delta_time.store(0.0);
        m_frame_number.store(0);
    }

    double GameClock::GetFPS() const {
        double delta = m_delta_time.load();
        return (delta > 0.0) ? (1.0 / delta) : 0.0;
    }

    // ============================================================================
    // SystemInfo Implementation with Dedicated Thread Support
    // ============================================================================

    SystemInfo::SystemInfo(std::unique_ptr<game::core::ISystem> sys, ThreadingStrategy strat)
        : system(std::move(sys)), strategy(strat) {
        last_update = std::chrono::steady_clock::now();
    }

    SystemInfo::~SystemInfo() {
        if (worker_thread.joinable()) {
            thread_running.store(false);
            worker_thread.join();
        }
    }

    // ============================================================================
    // ThreadPool Implementation with Task Tracking
    // ============================================================================

    ThreadPool::ThreadPool(size_t thread_count) 
        : m_active_tasks(0), m_total_tasks_submitted(0), m_total_task_time_ms(0.0) {
        m_workers.reserve(thread_count);
        
        for (size_t i = 0; i < thread_count; ++i) {
            m_workers.emplace_back([this] { WorkerLoop(); });
        }
    }

    ThreadPool::~ThreadPool() {
        Shutdown();
    }

    void ThreadPool::Shutdown() {
        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_running.store(false);
        }
        
        m_condition.notify_all();
        
        for (std::thread& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        m_workers.clear();
    }

    size_t ThreadPool::GetQueuedTaskCount() const {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        return m_tasks.size();
    }

    size_t ThreadPool::GetActiveTaskCount() const {
        return m_active_tasks.load();
    }

    double ThreadPool::GetAverageTaskTime() const {
        uint64_t total_tasks = m_total_tasks_submitted.load();
        if (total_tasks == 0) return 0.0;
        
        return m_total_task_time_ms.load() / static_cast<double>(total_tasks);
    }

    void ThreadPool::WorkerLoop() {
        while (m_running.load()) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(m_queue_mutex);
                
                m_condition.wait(lock, [this] {
                    return !m_running.load() || !m_tasks.empty();
                });
                
                if (!m_running.load()) {
                    break;
                }
                
                if (!m_tasks.empty()) {
                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                }
            }
            
            if (task) {
                auto start_time = std::chrono::steady_clock::now();

                // RAII guard ensures counter is decremented even if timing code throws
                AtomicCounterGuard counter_guard(m_active_tasks);

                try {
                    task();
                } catch (const std::exception& e) {
                    std::cerr << "[ThreadPool] Exception in worker thread: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "[ThreadPool] Unknown exception in worker thread" << std::endl;
                }

                // Record task timing (exception-safe - counter will still be decremented)
                auto end_time = std::chrono::steady_clock::now();
                auto duration = end_time - start_time;
                double task_time_ms = std::chrono::duration<double, std::milli>(duration).count();

                m_total_tasks_submitted.fetch_add(1);

                // Atomic add for double (using compare_exchange_weak)
                double expected = m_total_task_time_ms.load();
                while (!m_total_task_time_ms.compare_exchange_weak(expected, expected + task_time_ms)) {
                    // Retry until successful
                }
                // counter_guard destructor called here, decrements m_active_tasks
            }
        }
    }

    // ============================================================================
    // PerformanceMonitor Implementation
    // ============================================================================

    void PerformanceMonitor::RecordSystemUpdate(const std::string& system_name, double update_time_ms) {
        std::lock_guard<std::mutex> lock(m_data_mutex);

        // Use try_emplace for single map lookup (more efficient than find + insert)
        auto [it, inserted] = m_system_data.try_emplace(
            system_name,
            std::make_unique<SystemData>()
        );

        // Initialize name only if newly inserted
        if (inserted) {
            it->second->name = system_name;
        }

        // Get reference to data (no additional map lookup)
        auto& data = it->second;
        data->last_update_time_ms.store(update_time_ms);

        // Update peak time
        double current_peak = data->peak_update_time_ms.load();
        if (update_time_ms > current_peak) {
            data->peak_update_time_ms.store(update_time_ms);
        }

        // Update average time using exponential moving average
        uint64_t count = data->update_count.fetch_add(1);
        double current_avg = data->average_update_time_ms.load();

        const double alpha = 1.0 / std::min(static_cast<double>(count + 1), constants::PERFORMANCE_SAMPLE_WINDOW);
        const double new_avg = (alpha * update_time_ms) + ((1.0 - alpha) * current_avg);
        data->average_update_time_ms.store(new_avg);
    }

    void PerformanceMonitor::RecordFrameTime(double frame_time_ms) {
        m_total_frame_time_ms.store(frame_time_ms);
        
        uint64_t frame_count = m_total_frames.fetch_add(1);
        
        // Calculate FPS using moving average
        if (frame_time_ms > 0.0) {
            double fps = constants::MS_PER_SECOND / frame_time_ms;
            double current_avg_fps = m_average_fps.load();

            double alpha = 1.0 / std::min(static_cast<double>(frame_count + 1), constants::FRAME_TIME_SAMPLE_WINDOW);
            double new_avg_fps = (alpha * fps) + ((1.0 - alpha) * current_avg_fps);
            m_average_fps.store(new_avg_fps);
        }
    }

    double PerformanceMonitor::GetSystemAverageTime(const std::string& system_name) const {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        
        auto it = m_system_data.find(system_name);
        if (it != m_system_data.end()) {
            return it->second->average_update_time_ms.load();
        }
        return 0.0;
    }

    double PerformanceMonitor::GetSystemPeakTime(const std::string& system_name) const {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        
        auto it = m_system_data.find(system_name);
        if (it != m_system_data.end()) {
            return it->second->peak_update_time_ms.load();
        }
        return 0.0;
    }

    uint64_t PerformanceMonitor::GetSystemUpdateCount(const std::string& system_name) const {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        
        auto it = m_system_data.find(system_name);
        if (it != m_system_data.end()) {
            return it->second->update_count.load();
        }
        return 0;
    }

    double PerformanceMonitor::GetAverageFPS() const {
        return m_average_fps.load();
    }

    uint64_t PerformanceMonitor::GetTotalFrames() const {
        return m_total_frames.load();
    }

    void PerformanceMonitor::Reset() {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        
        for (auto& [name, data] : m_system_data) {
            data->last_update_time_ms.store(0.0);
            data->average_update_time_ms.store(0.0);
            data->update_count.store(0);
            data->peak_update_time_ms.store(0.0);
        }
        
        m_total_frame_time_ms.store(0.0);
        m_average_fps.store(0.0);
        m_total_frames.store(0);
    }

    std::vector<std::string> PerformanceMonitor::GetMonitoredSystems() const {
        std::lock_guard<std::mutex> lock(m_data_mutex);
        
        std::vector<std::string> names;
        names.reserve(m_system_data.size());
        
        for (const auto& [name, data] : m_system_data) {
            names.push_back(name);
        }
        
        return names;
    }

    void ThreadedSystemManager::UpdateSystemSingleThreaded(game::core::ISystem* system, float delta_time, SystemThreadingInfo& info) {
        auto start_time = std::chrono::steady_clock::now();
        
        try {
            system->Update(delta_time);
            info.last_execution = start_time;
            info.total_executions++;
        } catch (const std::exception& e) {
            HandleSystemError(system->GetSystemName(), e);
            return; // Skip performance recording for failed updates
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = end_time - start_time;
        double execution_time_ms = std::chrono::duration<double, std::milli>(duration).count();
        
        UpdatePerformanceMetrics(system->GetSystemName(), execution_time_ms);
        
        // Update system info
        info.peak_execution_time_ms = std::max(info.peak_execution_time_ms, execution_time_ms);
        
        // Update average using exponential moving average
        if (info.total_executions == 1) {
            info.average_execution_time_ms = execution_time_ms;
        } else {
            const double alpha = 1.0 / std::min(static_cast<double>(info.total_executions), constants::PERFORMANCE_SAMPLE_WINDOW);
            info.average_execution_time_ms = (alpha * execution_time_ms) +
                                           ((1.0 - alpha) * info.average_execution_time_ms);
        }
    }

    void ThreadedSystemManager::StartDedicatedThread(const std::string& system_name) {
        auto system = GetSystem(system_name);
        if (!system) return;
        
        DedicatedThreadData thread_data;
        thread_data.stop_flag.store(false);
        thread_data.is_active.store(false);
        
        thread_data.thread = std::thread([this, system, system_name]() {
            auto& thread_data = m_dedicated_threads[system_name];
            auto& info = m_system_info[system_name];
            
            std::cout << "[ThreadedSystemManager] Started dedicated thread for: " << system_name << std::endl;
            
            while (!thread_data.stop_flag.load() && m_is_running.load()) {
                auto frame_start = std::chrono::steady_clock::now();
                thread_data.is_active.store(true);
                
                try {
                    // Check if system is disabled
                    {
                        std::lock_guard<std::mutex> error_lock(m_errors_mutex);
                        auto error_it = m_system_errors.find(system_name);
                        if (error_it != m_system_errors.end() && error_it->second.is_disabled) {
                            thread_data.is_active.store(false);
                            std::this_thread::sleep_for(std::chrono::milliseconds(constants::DISABLED_SYSTEM_SLEEP_MS));
                            continue;
                        }
                    }
                    
                    // Update system
                    float delta_time = static_cast<float>(m_game_clock.GetDeltaTime());
                    UpdateSystemSingleThreaded(system, delta_time, info);
                    
                    // Participate in frame synchronization
                    WaitForFrameCompletion();
                    
                } catch (const std::exception& e) {
                    HandleSystemError(system_name, e);
                }
                
                thread_data.is_active.store(false);
                
                // Sleep to maintain target interval
                auto frame_end = std::chrono::steady_clock::now();
                auto frame_duration = frame_end - frame_start;
                auto target_duration = std::chrono::duration<double, std::milli>(info.target_interval_ms);
                
                if (frame_duration < target_duration) {
                    std::this_thread::sleep_for(target_duration - frame_duration);
                }
            }
            
            std::cout << "[ThreadedSystemManager] Stopped dedicated thread for: " << system_name << std::endl;
        });
        
        m_dedicated_threads.emplace(system_name, std::move(thread_data));
    }

    void ThreadedSystemManager::StopDedicatedThread(const std::string& system_name) {
        auto it = m_dedicated_threads.find(system_name);
        if (it != m_dedicated_threads.end()) {
            it->second.stop_flag.store(true);
            if (it->second.thread.joinable()) {
                it->second.thread.join();
            }
            m_dedicated_threads.erase(it);
        }
    }

    void ThreadedSystemManager::UpdateFrameBarrierCount() {
        size_t participants = 1; // Main thread
        
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        for (const auto& [name, info] : m_system_info) {
            if (info.strategy == ThreadingStrategy::THREAD_POOL ||
                info.strategy == ThreadingStrategy::DEDICATED_THREAD ||
                (info.strategy == ThreadingStrategy::HYBRID && 
                 DetermineOptimalStrategy(GetSystem(name)) != ThreadingStrategy::MAIN_THREAD)) {
                participants++;
            }
        }
        
        m_frame_barrier->SetThreadCount(participants);
    }

    void ThreadedSystemManager::UpdatePerformanceMetrics(const std::string& system_name, double execution_time_ms) {
        if (m_performance_monitoring_enabled.load() && m_performance_monitor) {
            m_performance_monitor->RecordSystemUpdate(system_name, execution_time_ms);
        }
    }

    ThreadingStrategy ThreadedSystemManager::DetermineOptimalStrategy(game::core::ISystem* system) {
        if (!system) return ThreadingStrategy::MAIN_THREAD;
        
        std::string name = system->GetSystemName();
        
        // Performance-critical systems get dedicated threads
        if (name.find("Render") != std::string::npos || 
            name.find("Physics") != std::string::npos ||
            name.find("Audio") != std::string::npos) {
            return ThreadingStrategy::DEDICATED_THREAD;
        }
        
        // UI and input systems should run on main thread
        if (name.find("UI") != std::string::npos || 
            name.find("Input") != std::string::npos ||
            name.find("Event") != std::string::npos) {
            return ThreadingStrategy::MAIN_THREAD;
        }
        
        // Check performance history for adaptive strategy
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        auto info_it = m_system_info.find(name);
        if (info_it != m_system_info.end()) {
            // Systems exceeding threshold should consider dedicated threads
            if (info_it->second.average_execution_time_ms > constants::SLOW_SYSTEM_THRESHOLD_MS &&
                info_it->second.total_executions > constants::MIN_EXECUTIONS_FOR_THREADING) {
                return ThreadingStrategy::DEDICATED_THREAD;
            }
        }
        
        // Everything else can use thread pool
        return ThreadingStrategy::THREAD_POOL;
    }

    void ThreadedSystemManager::BalanceThreadLoad() {
        std::lock_guard<std::mutex> lock(m_systems_mutex);

        for (auto& [name, info] : m_system_info) {
            if (info.total_executions < constants::MIN_EXECUTIONS_FOR_THREADING) continue; // Need sufficient data

            // Promotion logic: Move to dedicated thread if consistently slow
            if (info.strategy == ThreadingStrategy::THREAD_POOL &&
                info.average_execution_time_ms > constants::DEFAULT_FRAME_BUDGET_MS && // More than one frame at 60fps
                info.peak_execution_time_ms > constants::PEAK_EXECUTION_PROMOTION_THRESHOLD_MS &&
                info.promotion_frame_count++ > constants::PROMOTION_FRAME_THRESHOLD) { // 3 seconds of consistent slowness
                
                info.strategy = ThreadingStrategy::DEDICATED_THREAD;
                info.promotion_frame_count = 0;
                
                if (m_is_running.load()) {
                    StartDedicatedThread(name);
                }
                
                std::cout << "[ThreadedSystemManager] Promoted " << name 
                          << " to dedicated thread (avg: " << info.average_execution_time_ms << "ms)" << std::endl;
            }
            
            // Demotion logic: Move back to thread pool if performance improves
            else if (info.strategy == ThreadingStrategy::DEDICATED_THREAD &&
                     !info.is_performance_critical &&
                     info.average_execution_time_ms < constants::AVG_EXECUTION_DEMOTION_THRESHOLD_MS && // Less than quarter frame
                     info.peak_execution_time_ms < constants::SLOW_SYSTEM_THRESHOLD_MS &&
                     info.demotion_frame_count++ > constants::DEMOTION_FRAME_THRESHOLD) { // 10 seconds of good performance
                
                StopDedicatedThread(name);
                info.strategy = ThreadingStrategy::THREAD_POOL;
                info.demotion_frame_count = 0;
                
                std::cout << "[ThreadedSystemManager] Demoted " << name 
                          << " to thread pool (avg: " << info.average_execution_time_ms << "ms)" << std::endl;
            }
        }
    }

    void ThreadedSystemManager::HandleSystemError(const std::string& system_name, const std::exception& error) {
        std::lock_guard<std::mutex> lock(m_errors_mutex);
        
        // Initialize error tracking for this system if needed
        if (m_system_errors.find(system_name) == m_system_errors.end()) {
            m_system_errors[system_name] = SystemErrorInfo{};
        }
        
        auto& error_info = m_system_errors[system_name];
        error_info.error_count++;
        error_info.last_error = error.what();
        error_info.last_error_time = std::chrono::steady_clock::now();

        // Disable system if too many errors in short time
        constexpr size_t MAX_ERRORS = constants::MAX_SYSTEM_ERRORS;
        constexpr auto ERROR_WINDOW = std::chrono::seconds(constants::ERROR_COUNT_WINDOW_SECONDS);
        
        if (error_info.error_count >= MAX_ERRORS) {
            auto time_since_first = error_info.last_error_time - error_info.first_error_time;
            if (time_since_first < ERROR_WINDOW) {
                error_info.is_disabled = true;
                std::cerr << "[ThreadedSystemManager] DISABLED system '" << system_name 
                         << "' due to " << error_info.error_count << " errors in " 
                         << std::chrono::duration_cast<std::chrono::seconds>(time_since_first).count() 
                         << " seconds. Last error: " << error.what() << std::endl;
            } else {
                // Reset counter if errors are spread over time
                error_info.error_count = 1;
                error_info.first_error_time = error_info.last_error_time;
            }
        } else if (error_info.error_count == 1) {
            error_info.first_error_time = error_info.last_error_time;
        }
        
        // Log error
        std::string error_msg = "System '" + system_name + "' error (" + 
                               std::to_string(error_info.error_count) + "): " + error.what();
        std::cerr << "[ThreadedSystemManager] " << error_msg << std::endl;
    }

    // ============================================================================
    // FrameBarrier Implementation - Fixed Cyclic Barrier
    // ============================================================================

    FrameBarrier::FrameBarrier(size_t thread_count) 
        : m_total_threads(thread_count), m_epoch(0), m_waiting_count(0) {
    }

    void FrameBarrier::SetThreadCount(size_t count) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_total_threads = count;
    }

    void FrameBarrier::WaitForFrame() {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        uint64_t current_epoch = m_epoch;
        
        if (++m_waiting_count == m_total_threads) {
            // Last thread to arrive - wake everyone and advance epoch
            m_waiting_count = 0;
            ++m_epoch;
            m_frame_ready.store(true);
            lock.unlock();
            m_condition.notify_all();
        } else {
            // Wait for epoch to advance
            m_condition.wait(lock, [this, current_epoch] {
                return m_epoch > current_epoch;
            });
        }
    }

    void FrameBarrier::BeginFrame() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_frame_ready.store(false);
    }

    void FrameBarrier::EndFrame() {
        // Frame barrier automatically advances when all threads arrive
        // This method is kept for API compatibility
    }

    // ============================================================================
    // ThreadedSystemManager Implementation - Complete with Error Handling
    // ============================================================================

    ThreadedSystemManager::ThreadedSystemManager(::core::ecs::ComponentAccessManager* access_manager,
        ThreadSafeMessageBus* message_bus)
        : m_access_manager(access_manager), m_message_bus(message_bus),
          m_max_threads(std::thread::hardware_concurrency()) {

        // Validate non-owning pointers are valid
        if (!m_access_manager) {
            throw std::invalid_argument("ThreadedSystemManager: access_manager cannot be null");
        }
        if (!m_message_bus) {
            throw std::invalid_argument("ThreadedSystemManager: message_bus cannot be null");
        }

        if (m_max_threads == 0) {
            m_max_threads = constants::FALLBACK_THREAD_COUNT;
        }
        
        InitializeThreadPool();
        
        m_frame_barrier = std::make_unique<FrameBarrier>(m_max_threads);
        m_performance_monitor = std::make_unique<PerformanceMonitor>();
    }

    ThreadedSystemManager::~ThreadedSystemManager() {
        Shutdown();
    }

    void ThreadedSystemManager::AddSystem(std::unique_ptr<game::core::ISystem> system,
        ThreadingStrategy strategy) {
        
        if (!system) {
            throw std::invalid_argument("Cannot add null system");
        }
        
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        
        std::string system_name = system->GetSystemName();
        
        // Check for duplicate system names
        for (const auto& existing_system : m_systems) {
            if (existing_system->GetSystemName() == system_name) {
                throw std::runtime_error("System with name '" + system_name + "' already exists");
            }
        }
        
        // Create system threading info
        SystemThreadingInfo info;
        info.strategy = strategy;
        info.assigned_thread_id = std::this_thread::get_id();
        
        m_system_info[system_name] = info;
        m_systems.push_back(std::move(system));
        
        std::cout << "[ThreadedSystemManager] Added system: " << system_name 
                  << " with strategy: " << static_cast<int>(strategy) << std::endl;
    }

    game::core::ISystem* ThreadedSystemManager::GetSystem(const std::string& system_name) {
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        
        for (const auto& system : m_systems) {
            if (system->GetSystemName() == system_name) {
                return system.get();
            }
        }
        
        return nullptr;
    }

    const game::core::ISystem* ThreadedSystemManager::GetSystem(const std::string& system_name) const {
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        
        for (const auto& system : m_systems) {
            if (system->GetSystemName() == system_name) {
                return system.get();
            }
        }
        
        return nullptr;
    }

    void ThreadedSystemManager::RemoveSystem(const std::string& system_name) {
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        
        auto it = std::remove_if(m_systems.begin(), m_systems.end(),
            [&system_name](const std::unique_ptr<game::core::ISystem>& system) {
                return system->GetSystemName() == system_name;
            });
        
        if (it != m_systems.end()) {
            m_systems.erase(it, m_systems.end());
            m_system_info.erase(system_name);
            m_system_errors.erase(system_name);
            
            std::cout << "[ThreadedSystemManager] Removed system: " << system_name << std::endl;
        }
    }

    void ThreadedSystemManager::Initialize() {
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        
        for (const auto& system : m_systems) {
            try {
                system->Initialize();
                std::cout << "[ThreadedSystemManager] Initialized system: " << system->GetSystemName() << std::endl;
            } catch (const std::exception& e) {
                HandleSystemError(system->GetSystemName(), e);
            }
        }
    }

    void ThreadedSystemManager::StartSystems() {
        m_is_running.store(true);
        m_is_paused.store(false);
        
        std::cout << "[ThreadedSystemManager] Starting " << m_systems.size() << " systems" << std::endl;
        
        // Start dedicated thread systems
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        for (const auto& system : m_systems) {
            auto& info = m_system_info[system->GetSystemName()];
            
            if (info.strategy == ThreadingStrategy::DEDICATED_THREAD) {
                StartDedicatedThread(system->GetSystemName());
            }
        }
    }

    void ThreadedSystemManager::Update(float delta_time) {
        if (!m_is_running.load() || m_is_paused.load()) {
            return;
        }
        
        auto frame_start = std::chrono::steady_clock::now();
        
        BeginFrame();
        
        // Update game clock
        m_game_clock.Update();
        
        // Update systems based on their threading strategy
        UpdateSystemsByStrategy(delta_time);
        
        // Wait for all systems to complete
        WaitForFrameCompletion();
        
        EndFrame();
        
        // Record frame performance
        auto frame_end = std::chrono::steady_clock::now();
        auto frame_duration = frame_end - frame_start;
        double frame_time_ms = std::chrono::duration<double, std::milli>(frame_duration).count();
        
        m_frame_time_ms.store(frame_time_ms);
        
        if (m_performance_monitoring_enabled.load()) {
            m_performance_monitor->RecordFrameTime(frame_time_ms);
        }
        
        // Periodic load balancing
        static uint64_t balance_counter = 0;
        if (++balance_counter % constants::LOAD_BALANCE_CHECK_FRAMES == 0) { // Every 5 seconds at 60fps
            BalanceThreadLoad();
        }
    }

    void ThreadedSystemManager::StopSystems() {
        m_is_running.store(false);
        
        // Signal all dedicated threads to stop
        {
            std::lock_guard<std::mutex> lock(m_systems_mutex);
            for (auto& [name, info] : m_system_info) {
                if (info.strategy == ThreadingStrategy::DEDICATED_THREAD) {
                    // Signal dedicated thread to stop
                    auto it = m_dedicated_threads.find(name);
                    if (it != m_dedicated_threads.end()) {
                        it->second.stop_flag.store(true);
                    }
                }
            }
        }
        
        // Wait for all systems to complete current update
        WaitForFrameCompletion();
        
        // Join dedicated threads
        for (auto& [name, thread_data] : m_dedicated_threads) {
            if (thread_data.thread.joinable()) {
                thread_data.thread.join();
            }
        }
        m_dedicated_threads.clear();
        
        std::cout << "[ThreadedSystemManager] Stopped all systems" << std::endl;
    }

    void ThreadedSystemManager::Shutdown() {
        StopSystems();
        ShutdownThreadPool();
        
        // Clear all systems
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        m_systems.clear();
        m_system_info.clear();
        m_system_errors.clear();
        
        std::cout << "[ThreadedSystemManager] Shutdown complete" << std::endl;
    }

    void ThreadedSystemManager::SetDefaultThreadingStrategy(ThreadingStrategy strategy) {
        m_default_strategy = strategy;
    }

    ThreadingStrategy ThreadedSystemManager::GetDefaultThreadingStrategy() const {
        return m_default_strategy;
    }

    void ThreadedSystemManager::SetThreadingStrategy(const std::string& system_name, ThreadingStrategy strategy) {
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        
        auto it = m_system_info.find(system_name);
        if (it != m_system_info.end()) {
            ThreadingStrategy old_strategy = it->second.strategy;
            it->second.strategy = strategy;
            
            // Handle strategy transitions
            if (old_strategy == ThreadingStrategy::DEDICATED_THREAD && strategy != ThreadingStrategy::DEDICATED_THREAD) {
                StopDedicatedThread(system_name);
            } else if (old_strategy != ThreadingStrategy::DEDICATED_THREAD && strategy == ThreadingStrategy::DEDICATED_THREAD) {
                if (m_is_running.load()) {
                    StartDedicatedThread(system_name);
                }
            }
            
            std::cout << "[ThreadedSystemManager] Changed threading strategy for " << system_name 
                      << " to " << static_cast<int>(strategy) << std::endl;
        }
    }

    void ThreadedSystemManager::SetMaxThreads(size_t max_threads) {
        if (max_threads == 0) {
            max_threads = 1;
        }
        
        m_max_threads = max_threads;
        
        // Reinitialize thread pool with new size
        ShutdownThreadPool();
        InitializeThreadPool();
        
        // Update frame barrier to match actual participants
        UpdateFrameBarrierCount();
        
        std::cout << "[ThreadedSystemManager] Set max threads to " << max_threads << std::endl;
    }

    void ThreadedSystemManager::SetPerformanceCritical(const std::string& system_name, bool is_critical) {
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        
        auto it = m_system_info.find(system_name);
        if (it != m_system_info.end()) {
            it->second.is_performance_critical = is_critical;
            
            if (is_critical && it->second.strategy == ThreadingStrategy::THREAD_POOL) {
                // Performance critical systems should use dedicated threads
                SetThreadingStrategy(system_name, ThreadingStrategy::DEDICATED_THREAD);
            }
            
            std::cout << "[ThreadedSystemManager] Set " << system_name 
                      << " performance critical: " << (is_critical ? "true" : "false") << std::endl;
        }
    }

    void ThreadedSystemManager::EnableFrameLimiting(bool enable) {
        m_frame_limiting.store(enable);
    }

    bool ThreadedSystemManager::IsFrameLimitingEnabled() const {
        return m_frame_limiting.load();
    }

    void ThreadedSystemManager::EnablePerformanceMonitoring(bool enable) {
        m_performance_monitoring_enabled.store(enable);
        
        if (!enable && m_performance_monitor) {
            m_performance_monitor->Reset();
        }
    }

    bool ThreadedSystemManager::IsPerformanceMonitoringEnabled() const {
        return m_performance_monitoring_enabled.load();
    }

    double ThreadedSystemManager::GetFrameTime() const {
        return m_frame_time_ms.load();
    }

    double ThreadedSystemManager::GetSystemSyncTime() const {
        return m_system_sync_time_ms.load();
    }

    double ThreadedSystemManager::GetFPS() const {
        // Use smooth FPS from performance monitor instead of raw calculation
        if (m_performance_monitor) {
            return m_performance_monitor->GetAverageFPS();
        }
        return m_game_clock.GetFPS();
    }

    const PerformanceMonitor* ThreadedSystemManager::GetPerformanceMonitor() const {
        return m_performance_monitor.get();
    }

    const GameClock& ThreadedSystemManager::GetGameClock() const {
        return m_game_clock;
    }

    SystemThreadingInfo ThreadedSystemManager::GetSystemInfo(const std::string& system_name) const {
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        
        auto it = m_system_info.find(system_name);
        if (it != m_system_info.end()) {
            return it->second;
        }
        
        return SystemThreadingInfo{};
    }

    ThreadPoolInfo ThreadedSystemManager::GetThreadPoolInfo() const {
        ThreadPoolInfo info;
        
        if (m_thread_pool) {
            info.worker_count = m_thread_pool->GetWorkerCount();
            info.queued_tasks = m_thread_pool->GetQueuedTaskCount();
            info.active_tasks = m_thread_pool->GetActiveTaskCount();
            info.average_task_time_ms = m_thread_pool->GetAverageTaskTime();
        }
        
        return info;
    }

    std::vector<std::string> ThreadedSystemManager::GetPerformanceReport() const {
        std::vector<std::string> report;
        
        if (!m_performance_monitor) {
            report.push_back("Performance monitoring disabled");
            return report;
        }
        
        // Frame performance
        std::ostringstream frame_info;
        frame_info << "Frame Time: " << std::fixed << std::setprecision(2) 
                   << GetFrameTime() << "ms, FPS: " << GetFPS();
        report.push_back(frame_info.str());
        
        // Thread pool info
        auto pool_info = GetThreadPoolInfo();
        std::ostringstream pool_report;
        pool_report << "Thread Pool - Workers: " << pool_info.worker_count
                   << ", Queued: " << pool_info.queued_tasks
                   << ", Active: " << pool_info.active_tasks
                   << ", Avg Task: " << std::fixed << std::setprecision(2)
                   << pool_info.average_task_time_ms << "ms";
        report.push_back(pool_report.str());
        
        // System performance
        auto monitored_systems = m_performance_monitor->GetMonitoredSystems();
        for (const auto& system_name : monitored_systems) {
            std::ostringstream system_info;
            system_info << system_name << " - Avg: " 
                        << std::fixed << std::setprecision(2)
                        << m_performance_monitor->GetSystemAverageTime(system_name) << "ms, Peak: "
                        << m_performance_monitor->GetSystemPeakTime(system_name) << "ms, Updates: "
                        << m_performance_monitor->GetSystemUpdateCount(system_name);
            
            // Add error count if any
            std::lock_guard<std::mutex> lock(m_errors_mutex);
            auto error_it = m_system_errors.find(system_name);
            if (error_it != m_system_errors.end() && error_it->second.error_count > 0) {
                system_info << ", Errors: " << error_it->second.error_count;
                if (error_it->second.is_disabled) {
                    system_info << " (DISABLED)";
                }
            }
            
            report.push_back(system_info.str());
        }
        
        return report;
    }

    void ThreadedSystemManager::ResetPerformanceCounters() {
        if (m_performance_monitor) {
            m_performance_monitor->Reset();
        }
        
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        for (auto& [name, info] : m_system_info) {
            info.average_execution_time_ms = 0.0;
            info.peak_execution_time_ms = 0.0;
            info.total_executions = 0;
        }
        
        // Reset error counters
        std::lock_guard<std::mutex> error_lock(m_errors_mutex);
        for (auto& [name, error_info] : m_system_errors) {
            error_info.error_count = 0;
            error_info.is_disabled = false;
        }
    }

    void ThreadedSystemManager::WaitForFrameCompletion() {
        if (m_frame_barrier) {
            m_frame_barrier->WaitForFrame();
        }
    }

    void ThreadedSystemManager::BeginFrame() {
        if (m_frame_barrier) {
            m_frame_barrier->BeginFrame();
        }
    }

    void ThreadedSystemManager::EndFrame() {
        if (m_frame_barrier) {
            m_frame_barrier->EndFrame();
        }
    }

    std::vector<std::string> ThreadedSystemManager::GetSystemNames() const {
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        
        std::vector<std::string> names;
        names.reserve(m_systems.size());
        
        for (const auto& system : m_systems) {
            names.push_back(system->GetSystemName());
        }
        
        return names;
    }

    size_t ThreadedSystemManager::GetSystemCount() const {
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        return m_systems.size();
    }

    bool ThreadedSystemManager::IsSystemRunning(const std::string& system_name) const {
        if (!m_is_running.load() || m_is_paused.load()) {
            return false;
        }
        
        // Check if system is disabled due to errors
        std::lock_guard<std::mutex> error_lock(m_errors_mutex);
        auto error_it = m_system_errors.find(system_name);
        if (error_it != m_system_errors.end() && error_it->second.is_disabled) {
            return false;
        }
        
        return GetSystem(system_name) != nullptr;
    }

    bool ThreadedSystemManager::AreAllSystemsIdle() const {
        if (!m_is_running.load() || m_is_paused.load()) {
            return true;
        }
        
        // Check thread pool activity
        if (m_thread_pool && m_thread_pool->GetActiveTaskCount() > 0) {
            return false;
        }
        
        // Check dedicated threads
        for (const auto& [name, thread_data] : m_dedicated_threads) {
            if (thread_data.is_active.load()) {
                return false;
            }
        }
        
        return true;
    }

    // ============================================================================
    // Private Method Implementations
    // ============================================================================

    void ThreadedSystemManager::InitializeThreadPool() {
        m_thread_pool = std::make_unique<ThreadPool>(m_max_threads);
        std::cout << "[ThreadedSystemManager] Initialized thread pool with " << m_max_threads << " threads" << std::endl;
    }

    void ThreadedSystemManager::ShutdownThreadPool() {
        if (m_thread_pool) {
            m_thread_pool->Shutdown();
            m_thread_pool.reset();
        }
    }

    void ThreadedSystemManager::UpdateSystemsByStrategy(float delta_time) {
        std::lock_guard<std::mutex> lock(m_systems_mutex);
        
        // Group systems by strategy for efficient processing
        std::vector<game::core::ISystem*> main_thread_systems;
        std::vector<game::core::ISystem*> thread_pool_systems;
        
        for (const auto& system : m_systems) {
            auto& info = m_system_info[system->GetSystemName()];
            
            switch (info.strategy) {
                case ThreadingStrategy::MAIN_THREAD:
                    main_thread_systems.push_back(system.get());
                    break;
                case ThreadingStrategy::THREAD_POOL:
                    thread_pool_systems.push_back(system.get());
                    break;
                case ThreadingStrategy::DEDICATED_THREAD:
                    // Dedicated threads update themselves
                    break;
            }
        }
        
        // Update main thread systems sequentially
        for (auto* system : main_thread_systems) {
            auto& info = m_system_info[system->GetSystemName()];
            UpdateSystemSingleThreaded(system, delta_time, info);
        }
        
        // Submit thread pool systems for parallel execution
        for (auto* system : thread_pool_systems) {
            if (m_thread_pool) {
                m_thread_pool->Submit([this, system, delta_time]() {
                    auto& info = m_system_info[system->GetSystemName()];
                    UpdateSystemSingleThreaded(system, delta_time, info);
                });
            }
        }
    }

} // namespace core::threading
