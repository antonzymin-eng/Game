// Created: September 17, 2025 - 16:15:42
// Location: include/core/Threading/ThreadedSystemManager.h  
// Mechanica Imperii - Multi-threaded System Coordination (FIXED)

#pragma once

#include "core/ECS/ComponentAccessManager.h"
#include "core/ECS/MessageBus.h"
// #include "core/ecs/ISystem.h"
#include "core/types/game_types.h"
#include "core/threading/ThreadingTypes.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <unordered_map>
#include <future>
#include <typeinfo>

namespace core::threading {

    // ============================================================================
    // Forward Declarations
    // ============================================================================

    class ThreadSafeMessageBus;
    class ThreadPool;
    class FrameBarrier;
    class PerformanceMonitor;

    // ============================================================================
    // Threading Strategy Types - Now defined in ThreadingTypes.h
    // ============================================================================

    // ============================================================================
    // System Threading Information
    // ============================================================================

    struct SystemThreadingInfo {
        ThreadingStrategy strategy = ThreadingStrategy::THREAD_POOL;
        std::thread::id assigned_thread_id;
        double average_execution_time_ms = 0.0;
        double peak_execution_time_ms = 0.0;
        uint64_t total_executions = 0;
        std::chrono::steady_clock::time_point last_execution;
        bool is_performance_critical = false;
        
        // Load balancing counters
        uint64_t promotion_frame_count = 0;
        uint64_t demotion_frame_count = 0;
        double target_interval_ms = 16.67; // ~60 FPS default
    };

    // ============================================================================
    // Game Clock - Thread-safe timing
    // ============================================================================

    class GameClock {
    private:
        std::atomic<double> m_game_time{ 0.0 };
        std::atomic<double> m_delta_time{ 0.0 };
        std::atomic<uint64_t> m_frame_number{ 0 };
        std::chrono::steady_clock::time_point m_start_time;
        std::chrono::steady_clock::time_point m_last_frame_time;

    public:
        GameClock();

        void Update();
        void Reset();

        double GetGameTime() const { return m_game_time.load(); }
        double GetDeltaTime() const { return m_delta_time.load(); }
        uint64_t GetFrameNumber() const { return m_frame_number.load(); }
        double GetFPS() const;
    };

    // ============================================================================
    // System Info Structure
    // ============================================================================

    struct SystemInfo {
        std::unique_ptr<game::core::ISystem> system;
        ThreadingStrategy strategy;
        std::thread worker_thread;
        std::atomic<bool> thread_running{ false };
        std::atomic<bool> needs_update{ true };
        std::chrono::steady_clock::time_point last_update;
        double target_interval_ms = 16.67; // ~60 FPS default

        SystemInfo() = default;
        SystemInfo(std::unique_ptr<game::core::ISystem> sys, ThreadingStrategy strat);
        ~SystemInfo();

        // Make non-copyable but movable
        SystemInfo(const SystemInfo&) = delete;
        SystemInfo& operator=(const SystemInfo&) = delete;
        SystemInfo(SystemInfo&&) = default;
        SystemInfo& operator=(SystemInfo&&) = default;
    };

    // ============================================================================
    // Thread Pool with Task Tracking
    // ============================================================================

    class ThreadPool {
    private:
        std::vector<std::thread> m_workers;
        std::queue<std::function<void()>> m_tasks;
        mutable std::mutex m_queue_mutex;
        std::condition_variable m_condition;
        std::atomic<bool> m_running{ true };
        
        // Task tracking for metrics
        std::atomic<size_t> m_active_tasks{ 0 };
        std::atomic<uint64_t> m_total_tasks_submitted{ 0 };
        std::atomic<double> m_total_task_time_ms{ 0.0 };

    public:
        explicit ThreadPool(size_t thread_count = std::thread::hardware_concurrency());
        ~ThreadPool();

        template<typename F, typename... Args>
        auto Submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

        void Shutdown();
        size_t GetWorkerCount() const { return m_workers.size(); }
        
        // Task metrics
        size_t GetQueuedTaskCount() const;
        size_t GetActiveTaskCount() const;
        double GetAverageTaskTime() const;

    private:
        void WorkerLoop();
    };

    // ============================================================================
    // Performance Monitor
    // ============================================================================

    class PerformanceMonitor {
    private:
        struct SystemData {
            std::string name;
            std::atomic<double> last_update_time_ms{ 0.0 };
            std::atomic<double> average_update_time_ms{ 0.0 };
            std::atomic<uint64_t> update_count{ 0 };
            std::atomic<double> peak_update_time_ms{ 0.0 };
        };

        mutable std::mutex m_data_mutex;
        std::unordered_map<std::string, std::unique_ptr<SystemData>> m_system_data;
        std::atomic<double> m_total_frame_time_ms{ 0.0 };
        std::atomic<double> m_average_fps{ 0.0 };
        std::atomic<uint64_t> m_total_frames{ 0 };

    public:
        PerformanceMonitor() = default;

        void RecordSystemUpdate(const std::string& system_name, double update_time_ms);
        void RecordFrameTime(double frame_time_ms);

        double GetSystemAverageTime(const std::string& system_name) const;
        double GetSystemPeakTime(const std::string& system_name) const;
        uint64_t GetSystemUpdateCount(const std::string& system_name) const;

        double GetAverageFPS() const;
        uint64_t GetTotalFrames() const;

        void Reset();
        std::vector<std::string> GetMonitoredSystems() const;
    };

    // ============================================================================
    // Frame Barrier - Fixed Cyclic Barrier
    // ============================================================================

    class FrameBarrier {
    private:
        std::mutex m_mutex;
        std::condition_variable m_condition;
        std::atomic<bool> m_frame_ready{ false };
        std::atomic<uint64_t> m_epoch{ 0 };
        size_t m_waiting_count = 0;
        size_t m_total_threads = 0;

    public:
        explicit FrameBarrier(size_t thread_count = 0);

        void SetThreadCount(size_t count);
        void WaitForFrame();
        void BeginFrame();
        void EndFrame();
    };

    // ============================================================================
    // Thread Pool Information
    // ============================================================================

    struct ThreadPoolInfo {
        size_t worker_count = 0;
        size_t queued_tasks = 0;
        size_t active_tasks = 0;
        double average_task_time_ms = 0.0;
    };

    // ============================================================================
    // System Error Tracking
    // ============================================================================

    struct SystemErrorInfo {
        size_t error_count = 0;
        bool is_disabled = false;
        std::string last_error = "";
        std::chrono::steady_clock::time_point first_error_time;
        std::chrono::steady_clock::time_point last_error_time;
    };

    // ============================================================================
    // Dedicated Thread Data
    // ============================================================================

    struct DedicatedThreadData {
        std::thread thread;
        std::atomic<bool> stop_flag{ false };
        std::atomic<bool> is_active{ false };
    };

    // ============================================================================
    // Threaded System Manager - Main coordination class
    // ============================================================================

    class ThreadedSystemManager {
    private:
        // Core systems
        core::ecs::ComponentAccessManager* m_access_manager;
        ThreadSafeMessageBus* m_message_bus;

        // Systems management
        std::vector<std::unique_ptr<game::core::ISystem>> m_systems;
        std::unordered_map<std::string, SystemThreadingInfo> m_system_info;
        mutable std::mutex m_systems_mutex;

        // Threading infrastructure
        std::unique_ptr<ThreadPool> m_thread_pool;
        std::unique_ptr<FrameBarrier> m_frame_barrier;
        std::unique_ptr<PerformanceMonitor> m_performance_monitor;
        GameClock m_game_clock;

        // Dedicated thread management
        std::unordered_map<std::string, DedicatedThreadData> m_dedicated_threads;

        // Runtime state
        std::atomic<bool> m_is_running{ false };
        std::atomic<bool> m_is_paused{ false };
        std::atomic<bool> m_performance_monitoring_enabled{ true };
        std::atomic<bool> m_frame_limiting{ true };
        ThreadingStrategy m_default_strategy = ThreadingStrategy::HYBRID;

        // Performance tracking
        std::atomic<double> m_frame_time_ms{ 0.0 };
        std::atomic<double> m_system_sync_time_ms{ 0.0 };
        size_t m_max_threads;

        // Error handling
        std::unordered_map<std::string, SystemErrorInfo> m_system_errors;
        mutable std::mutex m_errors_mutex;

    public:
        // Constructor/Destructor
        ThreadedSystemManager(core::ecs::ComponentAccessManager* access_manager,
            ThreadSafeMessageBus* message_bus);
        ~ThreadedSystemManager();

        // System management - Template version for type safety
        template<typename SystemType, typename... Args>
        SystemType* AddSystem(ThreadingStrategy strategy, Args&&... args);

        // System management - Runtime version
        void AddSystem(std::unique_ptr<game::core::ISystem> system,
            ThreadingStrategy strategy = ThreadingStrategy::THREAD_POOL);

        template<typename SystemType>
        SystemType* GetSystem();

        game::core::ISystem* GetSystem(const std::string& system_name);
        const game::core::ISystem* GetSystem(const std::string& system_name) const;

        void RemoveSystem(const std::string& system_name);

        // Lifecycle
        void Initialize();
        void StartSystems();
        void Update(float delta_time);
        void StopSystems();
        void Shutdown();

        // Threading control
        void SetDefaultThreadingStrategy(ThreadingStrategy strategy);
        ThreadingStrategy GetDefaultThreadingStrategy() const;

        void SetThreadingStrategy(const std::string& system_name, ThreadingStrategy strategy);
        void SetMaxThreads(size_t max_threads);
        void SetPerformanceCritical(const std::string& system_name, bool is_critical);

        void EnableFrameLimiting(bool enable);
        bool IsFrameLimitingEnabled() const;

        void EnablePerformanceMonitoring(bool enable);
        bool IsPerformanceMonitoringEnabled() const;

        // State queries
        bool IsRunning() const { return m_is_running.load(); }
        bool IsPaused() const { return m_is_paused.load(); }
        void SetPaused(bool paused) { m_is_paused.store(paused); }

        // Performance monitoring
        double GetFrameTime() const;
        double GetSystemSyncTime() const;
        double GetFPS() const;

        const PerformanceMonitor* GetPerformanceMonitor() const;
        const GameClock& GetGameClock() const;

        SystemThreadingInfo GetSystemInfo(const std::string& system_name) const;
        ThreadPoolInfo GetThreadPoolInfo() const;
        std::vector<std::string> GetPerformanceReport() const;
        void ResetPerformanceCounters();

        // Synchronization
        void WaitForFrameCompletion();
        void BeginFrame();
        void EndFrame();

        // System queries
        std::vector<std::string> GetSystemNames() const;
        size_t GetSystemCount() const;
        bool IsSystemRunning(const std::string& system_name) const;
        bool AreAllSystemsIdle() const;

    private:
        // Internal system management
        void InitializeThreadPool();
        void ShutdownThreadPool();

        void UpdateSystemsByStrategy(float delta_time);
        void UpdateSystemSingleThreaded(game::core::ISystem* system, float delta_time, SystemThreadingInfo& info);
        
        // Dedicated thread management
        void StartDedicatedThread(const std::string& system_name);
        void StopDedicatedThread(const std::string& system_name);
        void UpdateFrameBarrierCount();

        void UpdatePerformanceMetrics(const std::string& system_name, double execution_time_ms);

        // Strategy determination
        ThreadingStrategy DetermineOptimalStrategy(game::core::ISystem* system);
        void BalanceThreadLoad();

        // Error handling
        void HandleSystemError(const std::string& system_name, const std::exception& error);
    };

} // namespace core::threading

// Template definitions
#include "ThreadedSystemManager.inl"
