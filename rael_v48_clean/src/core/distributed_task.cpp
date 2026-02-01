// RAEL V49 - Distributed Task Queue Implementation (#29)
#include "rael/distributed_task.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <thread>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  TASK REGISTRY
// ═══════════════════════════════════════════════════════════════════════════

void TaskRegistry::register_handler(const std::string& task_type, TaskHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[task_type] = handler;
}

void TaskRegistry::unregister_handler(const std::string& task_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.erase(task_type);
}

std::optional<TaskHandler> TaskRegistry::get_handler(const std::string& task_type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = handlers_.find(task_type);
    if (it != handlers_.end()) return it->second;
    return std::nullopt;
}

std::vector<std::string> TaskRegistry::list_types() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> types;
    for (const auto& [type, _] : handlers_) {
        types.push_back(type);
    }
    return types;
}

// ═══════════════════════════════════════════════════════════════════════════
//  TASK QUEUE
// ═══════════════════════════════════════════════════════════════════════════

DistributedTaskQueue::DistributedTaskQueue(const QueueConfig& config) : config_(config) {}

bool DistributedTaskQueue::enqueue(const Task& task) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (pending_.size() >= static_cast<size_t>(config_.max_size)) {
        return false;
    }

    // Check priority
    if (static_cast<int>(task.priority) < static_cast<int>(config_.min_priority)) {
        return false;
    }

    // Check type
    if (!config_.allowed_types.empty()) {
        bool allowed = std::find(config_.allowed_types.begin(),
                                 config_.allowed_types.end(),
                                 task.type) != config_.allowed_types.end();
        if (!allowed) return false;
    }

    // Deduplication
    if (config_.enable_deduplication) {
        if (seen_ids_.count(task.id)) {
            return false;
        }
        seen_ids_.insert(task.id);
    }

    Task t = task;
    t.state = TaskState::QUEUED;
    t.queue_name = config_.name;
    pending_.push(t);

    return true;
}

bool DistributedTaskQueue::enqueue_delayed(const Task& task, std::chrono::seconds delay) {
    Task t = task;
    t.scheduled = std::chrono::system_clock::now() + delay;
    return enqueue(t);
}

bool DistributedTaskQueue::enqueue_batch(const std::vector<Task>& tasks) {
    bool all_success = true;
    for (const auto& task : tasks) {
        if (!enqueue(task)) {
            all_success = false;
        }
    }
    return all_success;
}

std::optional<Task> DistributedTaskQueue::dequeue() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (pending_.empty()) return std::nullopt;

    Task task = pending_.top();
    pending_.pop();

    // Check if scheduled for later
    if (task.scheduled > std::chrono::system_clock::now()) {
        pending_.push(task);
        return std::nullopt;
    }

    task.state = TaskState::RUNNING;
    task.started = std::chrono::system_clock::now();
    running_[task.id] = task;

    return task;
}

std::vector<Task> DistributedTaskQueue::dequeue_batch(int max_count) {
    std::vector<Task> tasks;
    for (int i = 0; i < max_count; ++i) {
        auto task = dequeue();
        if (!task) break;
        tasks.push_back(*task);
    }
    return tasks;
}

std::optional<Task> DistributedTaskQueue::peek() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (pending_.empty()) return std::nullopt;
    return pending_.top();
}

std::vector<Task> DistributedTaskQueue::peek_batch(int max_count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Task> tasks;
    auto temp = pending_;
    for (int i = 0; i < max_count && !temp.empty(); ++i) {
        tasks.push_back(temp.top());
        temp.pop();
    }
    return tasks;
}

void DistributedTaskQueue::ack(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    running_.erase(task_id);
}

void DistributedTaskQueue::nack(const std::string& task_id, bool requeue) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = running_.find(task_id);
    if (it == running_.end()) return;

    Task task = it->second;
    running_.erase(it);

    if (requeue && task.retry_count < task.max_retries) {
        task.retry_count++;
        task.state = TaskState::RETRY;
        task.scheduled = std::chrono::system_clock::now() + task.retry_delay;
        pending_.push(task);
    } else {
        task.state = TaskState::FAILED;
    }
}

size_t DistributedTaskQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_.size();
}

bool DistributedTaskQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_.empty();
}

std::vector<Task> DistributedTaskQueue::get_pending() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Task> tasks;
    auto temp = pending_;
    while (!temp.empty()) {
        tasks.push_back(temp.top());
        temp.pop();
    }
    return tasks;
}

std::vector<Task> DistributedTaskQueue::get_running() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Task> tasks;
    for (const auto& [_, task] : running_) {
        tasks.push_back(task);
    }
    return tasks;
}

QueueConfig DistributedTaskQueue::config() const {
    return config_;
}

void DistributedTaskQueue::update_config(const QueueConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

// ═══════════════════════════════════════════════════════════════════════════
//  WORKER
// ═══════════════════════════════════════════════════════════════════════════

Worker::Worker(const WorkerConfig& config, TaskRegistry& registry)
    : config_(config), registry_(registry) {
    stats_.started = std::chrono::system_clock::now();
}

Worker::~Worker() {
    stop();
}

void Worker::start() {
    if (running_) return;
    running_ = true;

    for (int i = 0; i < config_.concurrency; ++i) {
        threads_.emplace_back(&Worker::worker_loop, this);
    }
}

void Worker::stop() {
    running_ = false;
    for (auto& t : threads_) {
        if (t.joinable()) t.join();
    }
    threads_.clear();
}

void Worker::pause() {
    paused_ = true;
}

void Worker::resume() {
    paused_ = false;
}

bool Worker::is_running() const {
    return running_;
}

void Worker::bind_queue(std::shared_ptr<DistributedTaskQueue> queue) {
    std::lock_guard<std::mutex> lock(mutex_);
    queues_[queue->config().name] = queue;
}

void Worker::unbind_queue(const std::string& queue_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    queues_.erase(queue_name);
}

Worker::Stats Worker::stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

WorkerConfig Worker::config() const {
    return config_;
}

std::string Worker::id() const {
    return config_.id;
}

void Worker::worker_loop() {
    while (running_) {
        if (paused_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        bool processed = false;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& [name, queue] : queues_) {
                auto task = queue->dequeue();
                if (task) {
                    execute_task(*task);
                    queue->ack(task->id);
                    processed = true;
                    break;
                }
            }
        }

        if (!processed) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void Worker::execute_task(Task& task) {
    auto handler = registry_.get_handler(task.type);
    if (!handler) {
        task.state = TaskState::FAILED;
        task.result = TaskResult{false, "", "No handler for task type", -1, {}, {}};
        stats_.tasks_failed++;
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    try {
        auto result = (*handler)(task);
        task.result = result;
        task.state = result.success ? TaskState::COMPLETED : TaskState::FAILED;

        if (result.success) {
            stats_.tasks_completed++;
        } else {
            stats_.tasks_failed++;
        }
    } catch (const std::exception& e) {
        task.state = TaskState::FAILED;
        task.result = TaskResult{false, "", e.what(), -1, {}, {}};
        stats_.tasks_failed++;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    task.result->duration = duration;

    // Update average duration
    double total = stats_.avg_duration_ms * (stats_.tasks_completed + stats_.tasks_failed - 1);
    total += duration.count();
    stats_.avg_duration_ms = total / (stats_.tasks_completed + stats_.tasks_failed);

    task.completed = std::chrono::system_clock::now();
}

void Worker::send_heartbeat() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.last_heartbeat = std::chrono::system_clock::now();
}

// ═══════════════════════════════════════════════════════════════════════════
//  TASK SCHEDULER
// ═══════════════════════════════════════════════════════════════════════════

TaskScheduler::TaskScheduler() {}

TaskScheduler::~TaskScheduler() {
    stop();
}

std::string TaskScheduler::schedule(const std::string& name, const Task& task,
                                     const std::string& cron_expr) {
    std::lock_guard<std::mutex> lock(mutex_);

    ScheduleEntry entry;
    entry.id = "sched_" + std::to_string(schedules_.size() + 1);
    entry.name = name;
    entry.template_task = task;
    entry.cron_expression = cron_expr;
    entry.enabled = true;
    entry.next_run = next_run_time(cron_expr);

    schedules_[entry.id] = entry;
    return entry.id;
}

bool TaskScheduler::unschedule(const std::string& schedule_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return schedules_.erase(schedule_id) > 0;
}

bool TaskScheduler::enable(const std::string& schedule_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = schedules_.find(schedule_id);
    if (it != schedules_.end()) {
        it->second.enabled = true;
        return true;
    }
    return false;
}

bool TaskScheduler::disable(const std::string& schedule_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = schedules_.find(schedule_id);
    if (it != schedules_.end()) {
        it->second.enabled = false;
        return true;
    }
    return false;
}

std::vector<ScheduleEntry> TaskScheduler::list_schedules() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ScheduleEntry> result;
    for (const auto& [_, entry] : schedules_) {
        result.push_back(entry);
    }
    return result;
}

std::optional<ScheduleEntry> TaskScheduler::get_schedule(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = schedules_.find(id);
    if (it != schedules_.end()) return it->second;
    return std::nullopt;
}

void TaskScheduler::start() {
    if (running_) return;
    running_ = true;
    scheduler_thread_ = std::thread(&TaskScheduler::scheduler_loop, this);
}

void TaskScheduler::stop() {
    running_ = false;
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
}

bool TaskScheduler::is_running() const {
    return running_;
}

void TaskScheduler::on_submit(SubmitCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    submit_callbacks_.push_back(cb);
}

void TaskScheduler::scheduler_loop() {
    while (running_) {
        auto now = std::chrono::system_clock::now();

        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& [_, entry] : schedules_) {
                if (!entry.enabled) continue;
                if (entry.next_run <= now) {
                    // Submit task
                    Task task = entry.template_task;
                    task.id = "task_" + std::to_string(std::hash<std::string>{}(entry.id + std::to_string(now.time_since_epoch().count())));
                    task.created = now;

                    for (const auto& cb : submit_callbacks_) {
                        cb(task);
                    }

                    entry.last_run = now;
                    entry.next_run = next_run_time(entry.cron_expression);
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::chrono::system_clock::time_point TaskScheduler::next_run_time(const std::string& cron) {
    // Simplified: just add 1 minute for any cron expression
    (void)cron;
    return std::chrono::system_clock::now() + std::chrono::minutes(1);
}

// ═══════════════════════════════════════════════════════════════════════════
//  DISTRIBUTED COORDINATOR
// ═══════════════════════════════════════════════════════════════════════════

DistributedCoordinator::DistributedCoordinator(const CoordinatorConfig& config)
    : config_(config) {}

DistributedCoordinator::~DistributedCoordinator() {
    leave_cluster();
}

void DistributedCoordinator::join_cluster() {
    running_ = true;
    heartbeat_thread_ = std::thread(&DistributedCoordinator::heartbeat_loop, this);
}

void DistributedCoordinator::leave_cluster() {
    running_ = false;
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
}

std::vector<std::string> DistributedCoordinator::get_nodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> nodes;
    nodes.push_back(config_.node_id);
    for (const auto& [node, _] : node_heartbeats_) {
        nodes.push_back(node);
    }
    return nodes;
}

std::string DistributedCoordinator::get_leader() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_leader_;
}

bool DistributedCoordinator::is_leader() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_leader_ == config_.node_id;
}

std::string DistributedCoordinator::select_worker(const Task& task) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Simple round-robin
    static size_t idx = 0;
    auto nodes = get_nodes();
    if (nodes.empty()) return config_.node_id;

    std::string selected = nodes[idx % nodes.size()];
    idx++;
    (void)task;
    return selected;
}

void DistributedCoordinator::redistribute_tasks(const std::string& failed_worker) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Would redistribute tasks from failed worker
    (void)failed_worker;
}

void DistributedCoordinator::start_election() {
    std::lock_guard<std::mutex> lock(mutex_);
    // Simple: lowest node ID wins
    std::string new_leader = config_.node_id;
    for (const auto& [node, _] : node_heartbeats_) {
        if (node < new_leader) {
            new_leader = node;
        }
    }

    if (new_leader != current_leader_) {
        current_leader_ = new_leader;
        for (const auto& cb : leader_callbacks_) {
            cb(new_leader);
        }
    }
}

void DistributedCoordinator::on_leader_change(std::function<void(const std::string&)> cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    leader_callbacks_.push_back(cb);
}

void DistributedCoordinator::heartbeat_loop() {
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            // Check for dead nodes
            auto now = std::chrono::system_clock::now();
            std::vector<std::string> dead_nodes;

            for (const auto& [node, last_hb] : node_heartbeats_) {
                if (now - last_hb > config_.leader_timeout) {
                    dead_nodes.push_back(node);
                }
            }

            for (const auto& node : dead_nodes) {
                node_heartbeats_.erase(node);
                if (node == current_leader_) {
                    start_election();
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void DistributedCoordinator::election_loop() {
    // Periodic election check
}

// ═══════════════════════════════════════════════════════════════════════════
//  TASK MANAGER
// ═══════════════════════════════════════════════════════════════════════════

TaskManager::TaskManager(const TaskManagerConfig& config)
    : config_(config)
    , registry_(std::make_unique<TaskRegistry>())
    , scheduler_(std::make_unique<TaskScheduler>()) {

    if (config.enable_distribution) {
        CoordinatorConfig coord_config;
        coordinator_ = std::make_unique<DistributedCoordinator>(coord_config);
    }

    // Set up scheduler callback
    scheduler_->on_submit([this](const Task& task) {
        submit(task);
    });
}

TaskManager::~TaskManager() {
    stop();
}

TaskManager& TaskManager::instance() {
    static TaskManager inst;
    return inst;
}

std::string TaskManager::submit(const Task& task) {
    std::lock_guard<std::mutex> lock(mutex_);

    Task t = task;
    if (t.id.empty()) {
        t.id = generate_task_id();
    }
    t.created = std::chrono::system_clock::now();
    t.state = TaskState::PENDING;

    all_tasks_[t.id] = t;

    // Find appropriate queue
    std::string queue_name = t.queue_name.empty() ? "default" : t.queue_name;
    auto it = queues_.find(queue_name);
    if (it != queues_.end()) {
        it->second->enqueue(t);
    }

    return t.id;
}

std::string TaskManager::submit(const std::string& type,
                                 const std::map<std::string, std::string>& params) {
    Task task;
    task.type = type;
    task.params = params;
    return submit(task);
}

std::vector<std::string> TaskManager::submit_batch(const std::vector<Task>& tasks) {
    std::vector<std::string> ids;
    for (const auto& task : tasks) {
        ids.push_back(submit(task));
    }
    return ids;
}

bool TaskManager::cancel(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = all_tasks_.find(task_id);
    if (it == all_tasks_.end()) return false;

    if (it->second.state == TaskState::PENDING ||
        it->second.state == TaskState::QUEUED) {
        it->second.state = TaskState::CANCELLED;
        return true;
    }
    return false;
}

bool TaskManager::retry(const std::string& task_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = all_tasks_.find(task_id);
    if (it == all_tasks_.end()) return false;

    if (it->second.state == TaskState::FAILED) {
        it->second.state = TaskState::PENDING;
        it->second.retry_count++;
        return submit(it->second).length() > 0;
    }
    return false;
}

std::optional<Task> TaskManager::get_task(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = all_tasks_.find(task_id);
    if (it != all_tasks_.end()) return it->second;
    return std::nullopt;
}

std::optional<TaskResult> TaskManager::get_result(const std::string& task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = all_tasks_.find(task_id);
    if (it != all_tasks_.end() && it->second.result) {
        return it->second.result;
    }
    return std::nullopt;
}

std::future<TaskResult> TaskManager::submit_async(const Task& task) {
    std::string task_id = submit(task);

    return std::async(std::launch::async, [this, task_id]() -> TaskResult {
        while (true) {
            auto task = get_task(task_id);
            if (task && (task->state == TaskState::COMPLETED ||
                         task->state == TaskState::FAILED)) {
                return task->result.value_or(TaskResult{false, "", "No result", -1, {}, {}});
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
}

std::shared_ptr<DistributedTaskQueue> TaskManager::create_queue(const QueueConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto queue = std::make_shared<DistributedTaskQueue>(config);
    queues_[config.name] = queue;
    return queue;
}

std::shared_ptr<DistributedTaskQueue> TaskManager::get_queue(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = queues_.find(name);
    if (it != queues_.end()) return it->second;
    return nullptr;
}

bool TaskManager::delete_queue(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return queues_.erase(name) > 0;
}

std::vector<std::string> TaskManager::list_queues() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& [name, _] : queues_) {
        names.push_back(name);
    }
    return names;
}

std::shared_ptr<Worker> TaskManager::create_worker(const WorkerConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto worker = std::make_shared<Worker>(config, *registry_);
    workers_[config.id] = worker;
    return worker;
}

std::shared_ptr<Worker> TaskManager::get_worker(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = workers_.find(id);
    if (it != workers_.end()) return it->second;
    return nullptr;
}

bool TaskManager::delete_worker(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = workers_.find(id);
    if (it != workers_.end()) {
        it->second->stop();
        workers_.erase(it);
        return true;
    }
    return false;
}

std::vector<std::string> TaskManager::list_workers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids;
    for (const auto& [id, _] : workers_) {
        ids.push_back(id);
    }
    return ids;
}

TaskRegistry& TaskManager::registry() { return *registry_; }
TaskScheduler& TaskManager::scheduler() { return *scheduler_; }

TaskManager::Stats TaskManager::stats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    Stats s;
    s.total_tasks = all_tasks_.size();
    s.active_queues = queues_.size();
    s.active_workers = workers_.size();

    for (const auto& [_, task] : all_tasks_) {
        switch (task.state) {
            case TaskState::PENDING:
            case TaskState::QUEUED:
                s.pending_tasks++;
                break;
            case TaskState::RUNNING:
                s.running_tasks++;
                break;
            case TaskState::COMPLETED:
                s.completed_tasks++;
                break;
            case TaskState::FAILED:
                s.failed_tasks++;
                break;
            default:
                break;
        }
    }

    return s;
}

void TaskManager::start() {
    running_ = true;

    // Create default queue
    QueueConfig default_config;
    default_config.name = "default";
    create_queue(default_config);

    // Create default workers
    for (int i = 0; i < config_.default_workers; ++i) {
        WorkerConfig wconfig;
        wconfig.id = "worker_" + std::to_string(i);
        wconfig.queues = {"default"};
        auto worker = create_worker(wconfig);
        worker->bind_queue(queues_["default"]);
        worker->start();
    }

    if (config_.enable_scheduling) {
        scheduler_->start();
    }

    if (coordinator_) {
        coordinator_->join_cluster();
    }
}

void TaskManager::stop() {
    running_ = false;

    scheduler_->stop();

    for (auto& [_, worker] : workers_) {
        worker->stop();
    }

    if (coordinator_) {
        coordinator_->leave_cluster();
    }
}

std::string TaskManager::generate_task_id() {
    std::ostringstream oss;
    oss << "task_" << std::hex << std::setfill('0') << std::setw(16) << next_task_id_++;
    return oss.str();
}

TaskManager& tasks() {
    return TaskManager::instance();
}

} // namespace rael
