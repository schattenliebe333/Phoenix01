// RAEL V49 - Agent Swarm Orchestrator Implementation
#include "rael/swarm_orchestrator.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

static std::string generate_uuid() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << dis(gen);
    return oss.str();
}

// ═══════════════════════════════════════════════════════════════════════════
//  TASK QUEUE
// ═══════════════════════════════════════════════════════════════════════════

TaskQueue::TaskQueue() {}

void TaskQueue::enqueue(const Task& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    all_tasks_[task.id] = task;
    task_states_[task.id] = TaskState::QUEUED;
    queue_.push(task);
}

std::optional<Task> TaskQueue::dequeue() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return std::nullopt;

    Task task = queue_.top();
    queue_.pop();
    task_states_[task.id] = TaskState::ASSIGNED;
    return task;
}

std::optional<Task> TaskQueue::dequeue_for_capability(const std::string& capability) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Rebuild queue to find matching task
    std::vector<Task> temp;
    std::optional<Task> result;

    while (!queue_.empty()) {
        Task task = queue_.top();
        queue_.pop();

        if (!result && (!task.required_capability ||
                        *task.required_capability == capability)) {
            result = task;
            task_states_[task.id] = TaskState::ASSIGNED;
        } else {
            temp.push_back(task);
        }
    }

    for (const auto& t : temp) {
        queue_.push(t);
    }

    return result;
}

std::optional<Task> TaskQueue::peek() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return std::nullopt;
    return queue_.top();
}

bool TaskQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t TaskQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

std::vector<Task> TaskQueue::pending_tasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Task> result;
    for (const auto& [id, state] : task_states_) {
        if (state == TaskState::PENDING || state == TaskState::QUEUED) {
            auto it = all_tasks_.find(id);
            if (it != all_tasks_.end()) {
                result.push_back(it->second);
            }
        }
    }
    return result;
}

std::optional<Task> TaskQueue::get_task(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = all_tasks_.find(id);
    if (it != all_tasks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool TaskQueue::update_task(const Task& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = all_tasks_.find(task.id);
    if (it != all_tasks_.end()) {
        it->second = task;
        task_states_[task.id] = task.state;
        return true;
    }
    return false;
}

bool TaskQueue::cancel_task(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = task_states_.find(id);
    if (it != task_states_.end()) {
        it->second = TaskState::CANCELLED;
        if (all_tasks_.count(id)) {
            all_tasks_[id].state = TaskState::CANCELLED;
        }
        return true;
    }
    return false;
}

bool TaskQueue::requeue_task(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = all_tasks_.find(id);
    if (it != all_tasks_.end()) {
        it->second.state = TaskState::QUEUED;
        it->second.retry_count++;
        task_states_[id] = TaskState::QUEUED;
        queue_.push(it->second);
        return true;
    }
    return false;
}

std::vector<Task> TaskQueue::get_ready_tasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Task> result;
    for (const auto& [id, task] : all_tasks_) {
        if (task_states_.at(id) == TaskState::QUEUED && are_dependencies_met(task)) {
            result.push_back(task);
        }
    }
    return result;
}

bool TaskQueue::are_dependencies_met(const Task& task) const {
    for (const auto& dep : task.dependencies) {
        auto it = task_states_.find(dep.task_id);
        if (it == task_states_.end()) return false;
        if (dep.required && it->second != TaskState::COMPLETED) return false;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  LOAD BALANCER
// ═══════════════════════════════════════════════════════════════════════════

LoadBalancer::LoadBalancer(LoadBalanceStrategy strategy) : strategy_(strategy) {}

std::optional<AgentId> LoadBalancer::select(const Task& task,
                                            const std::vector<AgentInfo>& agents) const {
    if (agents.empty()) return std::nullopt;

    switch (strategy_) {
        case LoadBalanceStrategy::ROUND_ROBIN:
            return round_robin(agents);
        case LoadBalanceStrategy::LEAST_LOADED:
            return least_loaded(agents);
        case LoadBalanceStrategy::CAPABILITY_MATCH:
            return capability_match(task, agents);
        case LoadBalanceStrategy::WEIGHTED:
            return weighted_select(agents);
        case LoadBalanceStrategy::RANDOM: {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, static_cast<int>(agents.size()) - 1);
            return agents[dis(gen)].agent_id;
        }
        default:
            return least_loaded(agents);
    }
}

void LoadBalancer::set_strategy(LoadBalanceStrategy strategy) {
    std::lock_guard<std::mutex> lock(mutex_);
    strategy_ = strategy;
}

LoadBalanceStrategy LoadBalancer::get_strategy() const {
    return strategy_;
}

void LoadBalancer::set_weight(const AgentId& agent, double weight) {
    std::lock_guard<std::mutex> lock(mutex_);
    weights_[agent.id] = weight;
}

double LoadBalancer::get_weight(const AgentId& agent) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = weights_.find(agent.id);
    return it != weights_.end() ? it->second : 1.0;
}

std::optional<AgentId> LoadBalancer::round_robin(const std::vector<AgentInfo>& agents) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (agents.empty()) return std::nullopt;
    round_robin_index_ = (round_robin_index_ + 1) % agents.size();
    return agents[round_robin_index_].agent_id;
}

std::optional<AgentId> LoadBalancer::least_loaded(const std::vector<AgentInfo>& agents) const {
    const AgentInfo* best = nullptr;
    for (const auto& agent : agents) {
        if (agent.state != AgentState::IDLE && agent.state != AgentState::BUSY) continue;
        if (!best || agent.current_tasks < best->current_tasks) {
            best = &agent;
        }
    }
    return best ? std::optional<AgentId>(best->agent_id) : std::nullopt;
}

std::optional<AgentId> LoadBalancer::capability_match(const Task& task,
                                                      const std::vector<AgentInfo>& agents) const {
    if (!task.required_capability) {
        return least_loaded(agents);
    }

    std::vector<AgentInfo> matching;
    for (const auto& agent : agents) {
        for (const auto& cap : agent.capabilities) {
            if (cap.name == *task.required_capability) {
                matching.push_back(agent);
                break;
            }
        }
    }

    if (matching.empty()) return std::nullopt;
    return least_loaded(matching);
}

std::optional<AgentId> LoadBalancer::weighted_select(const std::vector<AgentInfo>& agents) const {
    std::lock_guard<std::mutex> lock(mutex_);

    double total_weight = 0;
    for (const auto& agent : agents) {
        auto it = weights_.find(agent.agent_id.id);
        total_weight += it != weights_.end() ? it->second : 1.0;
    }

    if (total_weight <= 0) return std::nullopt;

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, total_weight);
    double r = dis(gen);

    double cumulative = 0;
    for (const auto& agent : agents) {
        auto it = weights_.find(agent.agent_id.id);
        cumulative += it != weights_.end() ? it->second : 1.0;
        if (r <= cumulative) {
            return agent.agent_id;
        }
    }

    return agents.back().agent_id;
}

// ═══════════════════════════════════════════════════════════════════════════
//  AGENT REGISTRY
// ═══════════════════════════════════════════════════════════════════════════

AgentRegistry::AgentRegistry() {}

bool AgentRegistry::register_agent(const AgentInfo& agent) {
    std::lock_guard<std::mutex> lock(mutex_);
    agents_[agent.agent_id.id] = agent;
    return true;
}

bool AgentRegistry::unregister_agent(const AgentId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return agents_.erase(id.id) > 0;
}

bool AgentRegistry::update_agent(const AgentInfo& agent) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = agents_.find(agent.agent_id.id);
    if (it != agents_.end()) {
        it->second = agent;
        return true;
    }
    return false;
}

std::optional<AgentInfo> AgentRegistry::get_agent(const AgentId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = agents_.find(id.id);
    if (it != agents_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<AgentInfo> AgentRegistry::get_all_agents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AgentInfo> result;
    for (const auto& [_, agent] : agents_) {
        result.push_back(agent);
    }
    return result;
}

std::vector<AgentInfo> AgentRegistry::get_agents_by_role(AgentRole role) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AgentInfo> result;
    for (const auto& [_, agent] : agents_) {
        if (agent.role == role) {
            result.push_back(agent);
        }
    }
    return result;
}

std::vector<AgentInfo> AgentRegistry::get_agents_by_state(AgentState state) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AgentInfo> result;
    for (const auto& [_, agent] : agents_) {
        if (agent.state == state) {
            result.push_back(agent);
        }
    }
    return result;
}

std::vector<AgentInfo> AgentRegistry::get_agents_with_capability(const std::string& cap) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AgentInfo> result;
    for (const auto& [_, agent] : agents_) {
        for (const auto& c : agent.capabilities) {
            if (c.name == cap) {
                result.push_back(agent);
                break;
            }
        }
    }
    return result;
}

std::vector<AgentInfo> AgentRegistry::get_available_agents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AgentInfo> result;
    for (const auto& [_, agent] : agents_) {
        if (agent.state == AgentState::IDLE || agent.state == AgentState::BUSY) {
            result.push_back(agent);
        }
    }
    return result;
}

void AgentRegistry::heartbeat(const AgentId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = agents_.find(id.id);
    if (it != agents_.end()) {
        it->second.last_heartbeat = std::chrono::system_clock::now();
    }
}

void AgentRegistry::check_health(std::chrono::seconds timeout) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();

    for (auto& [_, agent] : agents_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - agent.last_heartbeat);
        if (elapsed > timeout) {
            agent.state = AgentState::OFFLINE;
        }
    }
}

std::vector<AgentId> AgentRegistry::get_stale_agents(std::chrono::seconds threshold) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AgentId> result;
    auto now = std::chrono::system_clock::now();

    for (const auto& [_, agent] : agents_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - agent.last_heartbeat);
        if (elapsed > threshold) {
            result.push_back(agent.agent_id);
        }
    }
    return result;
}

size_t AgentRegistry::agent_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return agents_.size();
}

size_t AgentRegistry::available_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [_, agent] : agents_) {
        if (agent.state == AgentState::IDLE || agent.state == AgentState::BUSY) {
            count++;
        }
    }
    return count;
}

// ═══════════════════════════════════════════════════════════════════════════
//  FAILOVER MANAGER
// ═══════════════════════════════════════════════════════════════════════════

FailoverManager::FailoverManager() {}

void FailoverManager::set_backup(const AgentId& primary, const AgentId& backup) {
    std::lock_guard<std::mutex> lock(mutex_);
    backup_mapping_[primary.id] = backup;
}

void FailoverManager::remove_backup(const AgentId& primary) {
    std::lock_guard<std::mutex> lock(mutex_);
    backup_mapping_.erase(primary.id);
}

std::optional<AgentId> FailoverManager::get_backup(const AgentId& primary) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = backup_mapping_.find(primary.id);
    if (it != backup_mapping_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool FailoverManager::trigger_failover(const AgentId& failed_agent,
                                        AgentRegistry& registry,
                                        TaskQueue& queue) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto backup = get_backup(failed_agent);
    if (!backup) return false;

    auto backup_info = registry.get_agent(*backup);
    if (!backup_info || backup_info->state == AgentState::OFFLINE) {
        return false;
    }

    // Reassign tasks (simplified - would need actual task tracking)
    FailoverEvent event;
    event.failed_agent = failed_agent;
    event.backup_agent = *backup;
    event.timestamp = std::chrono::system_clock::now();
    event.tasks_reassigned = 0;
    event.success = true;

    history_.push_back(event);
    return true;
}

void FailoverManager::on_agent_recovered(const AgentId& agent) {
    // Could implement recovery logic
}

std::vector<FailoverManager::FailoverEvent> FailoverManager::get_failover_history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

// ═══════════════════════════════════════════════════════════════════════════
//  SWARM CONSENSUS
// ═══════════════════════════════════════════════════════════════════════════

SwarmConsensus::SwarmConsensus(double quorum) : quorum_(quorum) {}

std::optional<AgentId> SwarmConsensus::elect_leader(const std::vector<AgentInfo>& candidates) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (candidates.empty()) return std::nullopt;

    // Simple: pick agent with lowest ID (deterministic)
    const AgentInfo* leader = &candidates[0];
    for (const auto& agent : candidates) {
        if (agent.agent_id.id < leader->agent_id.id) {
            leader = &agent;
        }
    }

    leader_ = leader->agent_id;
    return leader_;
}

std::optional<AgentId> SwarmConsensus::current_leader() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return leader_;
}

bool SwarmConsensus::is_leader(const AgentId& agent) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return leader_ && leader_->id == agent.id;
}

std::string SwarmConsensus::propose(const std::string& type, const std::vector<uint8_t>& data,
                                    const AgentId& proposer) {
    std::lock_guard<std::mutex> lock(mutex_);

    Proposal p;
    p.id = generate_uuid();
    p.type = type;
    p.data = data;
    p.proposer = proposer;
    p.deadline = std::chrono::system_clock::now() + std::chrono::seconds(30);

    proposals_[p.id] = p;
    return p.id;
}

void SwarmConsensus::vote(const std::string& proposal_id, const AgentId& voter, bool accept) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = proposals_.find(proposal_id);
    if (it != proposals_.end()) {
        it->second.votes[voter.id] = accept;
    }
}

std::optional<SwarmConsensus::Proposal> SwarmConsensus::get_proposal(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = proposals_.find(id);
    if (it != proposals_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool SwarmConsensus::is_accepted(const std::string& proposal_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = proposals_.find(proposal_id);
    if (it == proposals_.end()) return false;

    int yes_votes = 0;
    for (const auto& [_, vote] : it->second.votes) {
        if (vote) yes_votes++;
    }

    int total = static_cast<int>(it->second.votes.size());
    return total > 0 && (static_cast<double>(yes_votes) / total) >= quorum_;
}

// ═══════════════════════════════════════════════════════════════════════════
//  SWARM ORCHESTRATOR
// ═══════════════════════════════════════════════════════════════════════════

SwarmOrchestrator::SwarmOrchestrator(const SwarmConfig& config)
    : config_(config)
    , registry_(std::make_unique<AgentRegistry>())
    , task_queue_(std::make_unique<TaskQueue>())
    , load_balancer_(std::make_unique<LoadBalancer>(config.load_balance))
    , failover_(std::make_unique<FailoverManager>())
    , consensus_(std::make_unique<SwarmConsensus>()) {

    // Initialize local agent
    local_agent_.agent_id.id = generate_uuid();
    local_agent_.agent_id.name = config.swarm_name + "_agent";
    local_agent_.agent_id.group = config.swarm_name;
    local_agent_.role = config.local_role;
    local_agent_.state = AgentState::INITIALIZING;
    local_agent_.joined = std::chrono::system_clock::now();
    local_agent_.last_heartbeat = local_agent_.joined;

    stats_ = {};
    stats_.started = local_agent_.joined;
}

SwarmOrchestrator::~SwarmOrchestrator() {
    stop();
}

SwarmOrchestrator& SwarmOrchestrator::instance() {
    static SwarmOrchestrator inst;
    return inst;
}

bool SwarmOrchestrator::start() {
    if (running_) return false;

    local_agent_.state = AgentState::IDLE;
    registry_->register_agent(local_agent_);

    running_ = true;

    scheduler_thread_ = std::thread(&SwarmOrchestrator::scheduler_loop, this);
    heartbeat_thread_ = std::thread(&SwarmOrchestrator::heartbeat_loop, this);
    health_thread_ = std::thread(&SwarmOrchestrator::health_check_loop, this);

    // Start worker threads
    for (int i = 0; i < config_.max_workers; i++) {
        worker_threads_.emplace_back(&SwarmOrchestrator::worker_loop, this);
    }

    return true;
}

void SwarmOrchestrator::stop() {
    running_ = false;
    cv_.notify_all();

    if (scheduler_thread_.joinable()) scheduler_thread_.join();
    if (heartbeat_thread_.joinable()) heartbeat_thread_.join();
    if (health_thread_.joinable()) health_thread_.join();

    for (auto& t : worker_threads_) {
        if (t.joinable()) t.join();
    }
    worker_threads_.clear();

    local_agent_.state = AgentState::OFFLINE;
}

bool SwarmOrchestrator::is_running() const {
    return running_;
}

AgentId SwarmOrchestrator::local_agent_id() const {
    return local_agent_.agent_id;
}

AgentInfo SwarmOrchestrator::local_agent_info() const {
    return local_agent_;
}

void SwarmOrchestrator::set_local_capabilities(const std::vector<AgentCapability>& caps) {
    std::lock_guard<std::mutex> lock(mutex_);
    local_agent_.capabilities = caps;
    registry_->update_agent(local_agent_);
}

bool SwarmOrchestrator::join_swarm(const std::string& master_address) {
    // Would implement network joining
    return true;
}

void SwarmOrchestrator::leave_swarm() {
    registry_->unregister_agent(local_agent_.agent_id);
}

std::vector<AgentInfo> SwarmOrchestrator::list_agents() const {
    return registry_->get_all_agents();
}

std::optional<AgentInfo> SwarmOrchestrator::get_agent(const AgentId& id) const {
    return registry_->get_agent(id);
}

std::string SwarmOrchestrator::generate_id() {
    std::ostringstream oss;
    oss << "task_" << std::hex << std::setw(16) << std::setfill('0') << next_id_++;
    return oss.str();
}

std::string SwarmOrchestrator::submit_task(const Task& task) {
    Task t = task;
    if (t.id.empty()) {
        t.id = generate_id();
    }
    t.created = std::chrono::system_clock::now();
    t.state = TaskState::PENDING;

    task_queue_->enqueue(t);
    stats_.total_tasks++;
    stats_.pending_tasks++;
    cv_.notify_one();

    return t.id;
}

std::string SwarmOrchestrator::submit_task(const std::string& type,
                                           const std::vector<uint8_t>& input,
                                           TaskPriority priority) {
    Task task;
    task.type = type;
    task.input_data = input;
    task.priority = priority;
    return submit_task(task);
}

std::vector<std::string> SwarmOrchestrator::submit_batch(const std::vector<Task>& tasks) {
    std::vector<std::string> ids;
    for (const auto& task : tasks) {
        ids.push_back(submit_task(task));
    }
    return ids;
}

std::optional<Task> SwarmOrchestrator::get_task(const std::string& id) const {
    return task_queue_->get_task(id);
}

TaskState SwarmOrchestrator::get_task_state(const std::string& id) const {
    auto task = task_queue_->get_task(id);
    return task ? task->state : TaskState::PENDING;
}

bool SwarmOrchestrator::cancel_task(const std::string& id) {
    return task_queue_->cancel_task(id);
}

std::vector<Task> SwarmOrchestrator::list_tasks(TaskState state) const {
    if (state == TaskState::PENDING) {
        return task_queue_->pending_tasks();
    }
    return {};
}

void SwarmOrchestrator::register_handler(const std::string& task_type, TaskHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[task_type] = handler;
}

void SwarmOrchestrator::unregister_handler(const std::string& task_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.erase(task_type);
}

void SwarmOrchestrator::on_task_complete(TaskCallback callback) {
    on_complete_ = callback;
}

void SwarmOrchestrator::on_task_failed(TaskCallback callback) {
    on_failed_ = callback;
}

void SwarmOrchestrator::on_agent_change(AgentCallback callback) {
    on_agent_change_ = callback;
}

std::string SwarmOrchestrator::propose(const std::string& type, const std::vector<uint8_t>& data) {
    return consensus_->propose(type, data, local_agent_.agent_id);
}

void SwarmOrchestrator::vote(const std::string& proposal_id, bool accept) {
    consensus_->vote(proposal_id, local_agent_.agent_id, accept);
}

SwarmOrchestrator::Stats SwarmOrchestrator::get_stats() const {
    Stats s = stats_;
    s.total_agents = registry_->agent_count();
    s.active_agents = registry_->available_count();
    s.pending_tasks = task_queue_->size();
    return s;
}

AgentRegistry& SwarmOrchestrator::registry() {
    return *registry_;
}

TaskQueue& SwarmOrchestrator::task_queue() {
    return *task_queue_;
}

LoadBalancer& SwarmOrchestrator::load_balancer() {
    return *load_balancer_;
}

void SwarmOrchestrator::scheduler_loop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(100));

        if (!running_) break;

        // Get ready tasks
        auto ready = task_queue_->get_ready_tasks();

        for (auto& task : ready) {
            dispatch_task(task);
        }
    }
}

void SwarmOrchestrator::heartbeat_loop() {
    while (running_) {
        std::this_thread::sleep_for(config_.heartbeat_interval);
        if (!running_) break;

        local_agent_.last_heartbeat = std::chrono::system_clock::now();
        registry_->heartbeat(local_agent_.agent_id);
    }
}

void SwarmOrchestrator::health_check_loop() {
    while (running_) {
        std::this_thread::sleep_for(config_.health_check_interval);
        if (!running_) break;

        registry_->check_health(config_.health_check_interval * 2);

        auto stale = registry_->get_stale_agents(config_.health_check_interval * 3);
        for (const auto& agent : stale) {
            if (config_.enable_failover) {
                handle_agent_failure(agent);
            }
        }
    }
}

void SwarmOrchestrator::worker_loop() {
    while (running_) {
        std::optional<Task> task;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(100));

            if (!running_) break;

            // Try to get a task for our capabilities
            for (const auto& cap : local_agent_.capabilities) {
                task = task_queue_->dequeue_for_capability(cap.name);
                if (task) break;
            }

            if (!task) {
                task = task_queue_->dequeue();
            }
        }

        if (task) {
            local_agent_.state = AgentState::BUSY;
            local_agent_.current_tasks++;

            auto start = std::chrono::steady_clock::now();

            TaskResult result;
            result.task_id = task->id;

            auto it = handlers_.find(task->type);
            if (it != handlers_.end()) {
                try {
                    result = it->second(*task);
                } catch (const std::exception& e) {
                    result.success = false;
                    result.error = e.what();
                }
            } else {
                result.success = false;
                result.error = "No handler for task type: " + task->type;
            }

            auto end = std::chrono::steady_clock::now();
            result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            handle_task_complete(task->id, result);

            local_agent_.current_tasks--;
            if (local_agent_.current_tasks == 0) {
                local_agent_.state = AgentState::IDLE;
            }
        }
    }
}

void SwarmOrchestrator::dispatch_task(Task& task) {
    auto available = registry_->get_available_agents();
    auto selected = load_balancer_->select(task, available);

    if (selected) {
        task.assigned_to = selected;
        task.state = TaskState::ASSIGNED;
        task_queue_->update_task(task);
        stats_.pending_tasks--;
        stats_.running_tasks++;
    }
}

void SwarmOrchestrator::handle_task_complete(const std::string& task_id, const TaskResult& result) {
    auto task = task_queue_->get_task(task_id);
    if (!task) return;

    Task t = *task;
    t.output_data = result.output;

    if (result.success) {
        t.state = TaskState::COMPLETED;
        t.completed = std::chrono::system_clock::now();
        stats_.completed_tasks++;
        stats_.running_tasks--;

        local_agent_.completed_tasks++;

        if (on_complete_) {
            on_complete_(t, result);
        }
    } else {
        if (t.retry_count < t.max_retries) {
            t.state = TaskState::QUEUED;
            t.retry_count++;
            task_queue_->requeue_task(task_id);
        } else {
            t.state = TaskState::FAILED;
            t.error_message = result.error;
            stats_.failed_tasks++;
            stats_.running_tasks--;

            local_agent_.failed_tasks++;

            if (on_failed_) {
                on_failed_(t, result);
            }
        }
    }

    task_queue_->update_task(t);
}

void SwarmOrchestrator::handle_agent_failure(const AgentId& agent) {
    if (config_.enable_failover) {
        failover_->trigger_failover(agent, *registry_, *task_queue_);
    }

    if (on_agent_change_) {
        auto info = registry_->get_agent(agent);
        if (info) {
            on_agent_change_(*info, false);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  EMERGENT BEHAVIOR
// ═══════════════════════════════════════════════════════════════════════════

EmergentBehavior::EmergentBehavior() {}

std::vector<AgentId> EmergentBehavior::aco_route(const Task& task,
                                                  const std::vector<AgentInfo>& agents,
                                                  SwarmState& state) {
    if (agents.empty()) return {};

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::vector<AgentId> route;
    std::set<std::string> visited;

    // Start from random agent
    std::uniform_int_distribution<> dis(0, static_cast<int>(agents.size()) - 1);
    int start = dis(gen);
    route.push_back(agents[start].agent_id);
    visited.insert(agents[start].agent_id.id);

    // Build route using ACO probability
    while (route.size() < agents.size()) {
        const auto& current = route.back();

        std::vector<std::pair<AgentId, double>> candidates;
        double total = 0;

        for (const auto& agent : agents) {
            if (visited.count(agent.agent_id.id)) continue;

            std::string key = current.id + "->" + agent.agent_id.id;
            double pheromone = state.pheromones.count(key) ?
                              state.pheromones[key] : 0.1;
            double heuristic = 1.0 / (1.0 + agent.current_tasks);

            double prob = std::pow(pheromone, alpha_) * std::pow(heuristic, beta_);
            candidates.emplace_back(agent.agent_id, prob);
            total += prob;
        }

        if (candidates.empty()) break;

        // Roulette selection
        std::uniform_real_distribution<> prob_dis(0, total);
        double r = prob_dis(gen);
        double cumulative = 0;

        for (const auto& [agent_id, prob] : candidates) {
            cumulative += prob;
            if (r <= cumulative) {
                route.push_back(agent_id);
                visited.insert(agent_id.id);
                break;
            }
        }
    }

    return route;
}

std::map<std::string, double> EmergentBehavior::pso_optimize(
    const std::vector<Task>& tasks,
    const std::vector<AgentInfo>& agents) {

    std::map<std::string, double> weights;

    // Simple PSO simulation
    for (const auto& agent : agents) {
        double load_factor = 1.0 / (1.0 + agent.current_tasks);
        double perf_factor = 1.0;

        for (const auto& cap : agent.capabilities) {
            perf_factor = std::max(perf_factor, cap.performance_score);
        }

        weights[agent.agent_id.id] = load_factor * perf_factor;
    }

    return weights;
}

void EmergentBehavior::reinforce(const std::string& path, double reward, SwarmState& state) {
    state.pheromones[path] += reward;
    state.global_fitness += reward;
}

void EmergentBehavior::evaporate(SwarmState& state, double rate) {
    for (auto& [key, pheromone] : state.pheromones) {
        pheromone *= (1.0 - rate);
        if (pheromone < 0.001) pheromone = 0.001;
    }
    state.generation++;
}

// Global access
SwarmOrchestrator& swarm() {
    return SwarmOrchestrator::instance();
}

} // namespace rael
