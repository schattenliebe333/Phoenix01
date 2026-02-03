// RAEL V49 - Distributed Task Queue (#29)
// Scalable task distribution and execution
#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <optional>
#include <mutex>
#include <chrono>
#include <atomic>
#include <future>
#include <queue>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  TASK DEFINITION
// ═══════════════════════════════════════════════════════════════════════════

enum class TaskState {
    PENDING,
    QUEUED,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED,
    RETRY
};

enum class TaskPriority {
    LOW = 0,
    NORMAL = 5,
    HIGH = 10,
    CRITICAL = 100
};

struct TaskResult {
    bool success;
    std::string output;
    std::string error;
    int exit_code;
    std::chrono::milliseconds duration;
    std::map<std::string, std::string> metadata;
};

struct Task {
    std::string id;
    std::string name;
    std::string type;
    std::map<std::string, std::string> params;
    std::vector<uint8_t> payload;
    TaskPriority priority = TaskPriority::NORMAL;
    TaskState state = TaskState::PENDING;

    // Scheduling
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point scheduled;
    std::chrono::system_clock::time_point started;
    std::chrono::system_clock::time_point completed;
    std::chrono::seconds timeout{3600};

    // Retry
    int max_retries = 3;
    int retry_count = 0;
    std::chrono::seconds retry_delay{60};

    // Dependencies
    std::vector<std::string> depends_on;
    std::string parent_id;
    std::vector<std::string> child_ids;

    // Assignment
    std::string assigned_worker;
    std::string queue_name;

    // Result
    std::optional<TaskResult> result;

    // Tags
    std::vector<std::string> tags;
    std::map<std::string, std::string> metadata;
};

// ═══════════════════════════════════════════════════════════════════════════
//  TASK HANDLER
// ═══════════════════════════════════════════════════════════════════════════

using TaskHandler = std::function<TaskResult(const Task&)>;

class TaskRegistry {
public:
    void register_handler(const std::string& task_type, TaskHandler handler);
    void unregister_handler(const std::string& task_type);
    std::optional<TaskHandler> get_handler(const std::string& task_type) const;
    std::vector<std::string> list_types() const;

private:
    std::map<std::string, TaskHandler> handlers_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  TASK QUEUE
// ═══════════════════════════════════════════════════════════════════════════

struct QueueConfig {
    std::string name;
    int max_size = 10000;
    int max_workers = 4;
    TaskPriority min_priority = TaskPriority::LOW;
    std::vector<std::string> allowed_types;
    std::chrono::seconds visibility_timeout{300};
    bool enable_deduplication = true;
};

class DistributedTaskQueue {
public:
    explicit DistributedTaskQueue(const QueueConfig& config);

    // Enqueue
    bool enqueue(const Task& task);
    bool enqueue_delayed(const Task& task, std::chrono::seconds delay);
    bool enqueue_batch(const std::vector<Task>& tasks);

    // Dequeue
    std::optional<Task> dequeue();
    std::vector<Task> dequeue_batch(int max_count);

    // Peek
    std::optional<Task> peek() const;
    std::vector<Task> peek_batch(int max_count) const;

    // Acknowledge
    void ack(const std::string& task_id);
    void nack(const std::string& task_id, bool requeue = true);

    // Query
    size_t size() const;
    bool empty() const;
    std::vector<Task> get_pending() const;
    std::vector<Task> get_running() const;

    // Config
    QueueConfig config() const;
    void update_config(const QueueConfig& config);

private:
    QueueConfig config_;

    struct TaskCompare {
        bool operator()(const Task& a, const Task& b) const {
            return static_cast<int>(a.priority) < static_cast<int>(b.priority);
        }
    };

    std::priority_queue<Task, std::vector<Task>, TaskCompare> pending_;
    std::map<std::string, Task> running_;
    std::set<std::string> seen_ids_;  // For deduplication
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  WORKER
// ═══════════════════════════════════════════════════════════════════════════

struct WorkerConfig {
    std::string id;
    std::string name;
    int concurrency = 4;
    std::vector<std::string> queues;
    std::chrono::seconds heartbeat_interval{30};
    std::chrono::seconds task_timeout{3600};
};

class Worker {
public:
    explicit Worker(const WorkerConfig& config, TaskRegistry& registry);
    ~Worker();

    // Lifecycle
    void start();
    void stop();
    void pause();
    void resume();
    bool is_running() const;

    // Queue binding
    void bind_queue(std::shared_ptr<DistributedTaskQueue> queue);
    void unbind_queue(const std::string& queue_name);

    // Stats
    struct Stats {
        size_t tasks_completed;
        size_t tasks_failed;
        size_t tasks_active;
        double avg_duration_ms;
        std::chrono::system_clock::time_point started;
        std::chrono::system_clock::time_point last_heartbeat;
    };
    Stats stats() const;

    // Config
    WorkerConfig config() const;
    std::string id() const;

private:
    void worker_loop();
    void execute_task(Task& task);
    void send_heartbeat();

    WorkerConfig config_;
    TaskRegistry& registry_;
    std::map<std::string, std::shared_ptr<DistributedTaskQueue>> queues_;

    std::vector<std::thread> threads_;
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};

    Stats stats_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  SCHEDULER
// ═══════════════════════════════════════════════════════════════════════════

struct ScheduleEntry {
    std::string id;
    std::string name;
    Task template_task;
    std::string cron_expression;
    bool enabled = true;
    std::chrono::system_clock::time_point next_run;
    std::chrono::system_clock::time_point last_run;
};

class TaskScheduler {
public:
    TaskScheduler();
    ~TaskScheduler();

    // Schedule management
    std::string schedule(const std::string& name, const Task& task,
                         const std::string& cron_expr);
    bool unschedule(const std::string& schedule_id);
    bool enable(const std::string& schedule_id);
    bool disable(const std::string& schedule_id);

    // Query
    std::vector<ScheduleEntry> list_schedules() const;
    std::optional<ScheduleEntry> get_schedule(const std::string& id) const;

    // Scheduler lifecycle
    void start();
    void stop();
    bool is_running() const;

    // Set task submission callback
    using SubmitCallback = std::function<void(const Task&)>;
    void on_submit(SubmitCallback cb);

private:
    void scheduler_loop();
    std::chrono::system_clock::time_point next_run_time(const std::string& cron);

    std::map<std::string, ScheduleEntry> schedules_;
    std::vector<SubmitCallback> submit_callbacks_;
    std::thread scheduler_thread_;
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  DISTRIBUTED COORDINATOR
// ═══════════════════════════════════════════════════════════════════════════

struct CoordinatorConfig {
    std::string node_id;
    std::vector<std::string> peer_nodes;
    std::chrono::seconds leader_timeout{30};
    bool enable_replication = true;
};

class DistributedCoordinator {
public:
    explicit DistributedCoordinator(const CoordinatorConfig& config);
    ~DistributedCoordinator();

    // Cluster
    void join_cluster();
    void leave_cluster();
    std::vector<std::string> get_nodes() const;
    std::string get_leader() const;
    bool is_leader() const;

    // Task distribution
    std::string select_worker(const Task& task);
    void redistribute_tasks(const std::string& failed_worker);

    // Leader election
    void start_election();
    void on_leader_change(std::function<void(const std::string&)> cb);

private:
    void heartbeat_loop();
    void election_loop();

    CoordinatorConfig config_;
    std::map<std::string, std::chrono::system_clock::time_point> node_heartbeats_;
    std::string current_leader_;
    std::vector<std::function<void(const std::string&)>> leader_callbacks_;
    std::atomic<bool> running_{false};
    std::thread heartbeat_thread_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  TASK MANAGER
// ═══════════════════════════════════════════════════════════════════════════

struct TaskManagerConfig {
    int default_workers = 4;
    int max_queues = 100;
    bool enable_persistence = false;
    std::string storage_path;
    bool enable_scheduling = true;
    bool enable_distribution = false;
};

class TaskManager {
public:
    explicit TaskManager(const TaskManagerConfig& config = {});
    ~TaskManager();

    static TaskManager& instance();

    // Task submission
    std::string submit(const Task& task);
    std::string submit(const std::string& type, const std::map<std::string, std::string>& params);
    std::vector<std::string> submit_batch(const std::vector<Task>& tasks);

    // Task control
    bool cancel(const std::string& task_id);
    bool retry(const std::string& task_id);
    std::optional<Task> get_task(const std::string& task_id) const;
    std::optional<TaskResult> get_result(const std::string& task_id) const;

    // Async result
    std::future<TaskResult> submit_async(const Task& task);

    // Queue management
    std::shared_ptr<DistributedTaskQueue> create_queue(const QueueConfig& config);
    std::shared_ptr<DistributedTaskQueue> get_queue(const std::string& name);
    bool delete_queue(const std::string& name);
    std::vector<std::string> list_queues() const;

    // Worker management
    std::shared_ptr<Worker> create_worker(const WorkerConfig& config);
    std::shared_ptr<Worker> get_worker(const std::string& id);
    bool delete_worker(const std::string& id);
    std::vector<std::string> list_workers() const;

    // Task registry
    TaskRegistry& registry();

    // Scheduler
    TaskScheduler& scheduler();

    // Stats
    struct Stats {
        size_t total_tasks;
        size_t pending_tasks;
        size_t running_tasks;
        size_t completed_tasks;
        size_t failed_tasks;
        size_t active_workers;
        size_t active_queues;
    };
    Stats stats() const;

    // Lifecycle
    void start();
    void stop();

private:
    std::string generate_task_id();

    TaskManagerConfig config_;
    std::map<std::string, std::shared_ptr<DistributedTaskQueue>> queues_;
    std::map<std::string, std::shared_ptr<Worker>> workers_;
    std::map<std::string, Task> all_tasks_;
    std::unique_ptr<TaskRegistry> registry_;
    std::unique_ptr<TaskScheduler> scheduler_;
    std::unique_ptr<DistributedCoordinator> coordinator_;

    std::atomic<uint64_t> next_task_id_{1};
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
};

TaskManager& tasks();

} // namespace rael
