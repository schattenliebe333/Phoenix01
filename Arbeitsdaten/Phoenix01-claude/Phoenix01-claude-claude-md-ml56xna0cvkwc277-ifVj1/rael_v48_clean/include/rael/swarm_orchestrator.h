// RAEL V49 - Agent Swarm Orchestrator (#27)
// Multi-agent coordination and task distribution
#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <optional>
#include <queue>
#include <condition_variable>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  AGENT TYPES & ROLES
// ═══════════════════════════════════════════════════════════════════════════

enum class AgentRole {
    MASTER,             // Coordinates other agents
    WORKER,             // Executes tasks
    SPECIALIST,         // Domain-specific expertise
    MONITOR,            // Observes and reports
    RELAY,              // Message forwarding
    BACKUP              // Standby for failover
};

enum class AgentState {
    INITIALIZING,
    IDLE,
    BUSY,
    PAUSED,
    ERROR,
    SHUTTING_DOWN,
    OFFLINE
};

struct AgentCapability {
    std::string name;
    std::string description;
    std::vector<std::string> input_types;
    std::vector<std::string> output_types;
    double performance_score = 1.0;
    int max_concurrent = 1;
};

// ═══════════════════════════════════════════════════════════════════════════
//  AGENT IDENTITY
// ═══════════════════════════════════════════════════════════════════════════

struct AgentId {
    std::string id;
    std::string name;
    std::string group;

    bool operator==(const AgentId& other) const { return id == other.id; }
    bool operator<(const AgentId& other) const { return id < other.id; }
};

struct AgentInfo {
    AgentId agent_id;
    AgentRole role = AgentRole::WORKER;
    AgentState state = AgentState::INITIALIZING;
    std::vector<AgentCapability> capabilities;
    std::string address;
    std::chrono::system_clock::time_point joined;
    std::chrono::system_clock::time_point last_heartbeat;
    std::map<std::string, std::string> metadata;

    // Load metrics
    int current_tasks = 0;
    int completed_tasks = 0;
    int failed_tasks = 0;
    double avg_task_duration_ms = 0;
    double cpu_usage = 0;
    double memory_usage = 0;
};

// ═══════════════════════════════════════════════════════════════════════════
//  TASK DEFINITION
// ═══════════════════════════════════════════════════════════════════════════

enum class TaskPriority {
    LOW = 0,
    NORMAL = 5,
    HIGH = 10,
    CRITICAL = 100
};

enum class TaskState {
    PENDING,
    QUEUED,
    ASSIGNED,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED,
    TIMEOUT
};

struct TaskDependency {
    std::string task_id;
    bool required = true;      // Must complete successfully
    bool data_flow = false;    // Output feeds into input
};

struct Task {
    std::string id;
    std::string type;
    std::string description;
    TaskPriority priority = TaskPriority::NORMAL;
    TaskState state = TaskState::PENDING;

    // Input/Output
    std::vector<uint8_t> input_data;
    std::vector<uint8_t> output_data;
    std::map<std::string, std::string> parameters;

    // Scheduling
    std::vector<TaskDependency> dependencies;
    std::optional<AgentId> assigned_to;
    std::optional<std::string> required_capability;

    // Timing
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point started;
    std::chrono::system_clock::time_point completed;
    std::chrono::seconds timeout{300};
    int max_retries = 3;
    int retry_count = 0;

    // Result
    std::string error_message;
    int exit_code = 0;
};

struct TaskResult {
    std::string task_id;
    bool success;
    std::vector<uint8_t> output;
    std::string error;
    std::chrono::milliseconds duration;
};

// ═══════════════════════════════════════════════════════════════════════════
//  TASK QUEUE
// ═══════════════════════════════════════════════════════════════════════════

class TaskQueue {
public:
    TaskQueue();

    // Queue operations
    void enqueue(const Task& task);
    std::optional<Task> dequeue();
    std::optional<Task> dequeue_for_capability(const std::string& capability);
    std::optional<Task> peek() const;

    // Query
    bool empty() const;
    size_t size() const;
    std::vector<Task> pending_tasks() const;
    std::optional<Task> get_task(const std::string& id) const;

    // Modification
    bool update_task(const Task& task);
    bool cancel_task(const std::string& id);
    bool requeue_task(const std::string& id);

    // Dependencies
    std::vector<Task> get_ready_tasks() const;
    bool are_dependencies_met(const Task& task) const;

private:
    struct TaskCompare {
        bool operator()(const Task& a, const Task& b) const {
            return static_cast<int>(a.priority) < static_cast<int>(b.priority);
        }
    };

    std::priority_queue<Task, std::vector<Task>, TaskCompare> queue_;
    std::map<std::string, Task> all_tasks_;
    std::map<std::string, TaskState> task_states_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  LOAD BALANCER
// ═══════════════════════════════════════════════════════════════════════════

enum class LoadBalanceStrategy {
    ROUND_ROBIN,
    LEAST_LOADED,
    CAPABILITY_MATCH,
    RANDOM,
    WEIGHTED,
    LOCALITY_AWARE
};

class LoadBalancer {
public:
    LoadBalancer(LoadBalanceStrategy strategy = LoadBalanceStrategy::LEAST_LOADED);

    // Select agent for task
    std::optional<AgentId> select(const Task& task,
                                   const std::vector<AgentInfo>& agents) const;

    // Strategy
    void set_strategy(LoadBalanceStrategy strategy);
    LoadBalanceStrategy get_strategy() const;

    // Weights (for WEIGHTED strategy)
    void set_weight(const AgentId& agent, double weight);
    double get_weight(const AgentId& agent) const;

private:
    std::optional<AgentId> round_robin(const std::vector<AgentInfo>& agents) const;
    std::optional<AgentId> least_loaded(const std::vector<AgentInfo>& agents) const;
    std::optional<AgentId> capability_match(const Task& task,
                                            const std::vector<AgentInfo>& agents) const;
    std::optional<AgentId> weighted_select(const std::vector<AgentInfo>& agents) const;

    LoadBalanceStrategy strategy_;
    mutable size_t round_robin_index_ = 0;
    std::map<std::string, double> weights_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  AGENT REGISTRY
// ═══════════════════════════════════════════════════════════════════════════

class AgentRegistry {
public:
    AgentRegistry();

    // Registration
    bool register_agent(const AgentInfo& agent);
    bool unregister_agent(const AgentId& id);
    bool update_agent(const AgentInfo& agent);

    // Query
    std::optional<AgentInfo> get_agent(const AgentId& id) const;
    std::vector<AgentInfo> get_all_agents() const;
    std::vector<AgentInfo> get_agents_by_role(AgentRole role) const;
    std::vector<AgentInfo> get_agents_by_state(AgentState state) const;
    std::vector<AgentInfo> get_agents_with_capability(const std::string& cap) const;
    std::vector<AgentInfo> get_available_agents() const;

    // Health
    void heartbeat(const AgentId& id);
    void check_health(std::chrono::seconds timeout = std::chrono::seconds(60));
    std::vector<AgentId> get_stale_agents(std::chrono::seconds threshold) const;

    // Stats
    size_t agent_count() const;
    size_t available_count() const;

private:
    std::map<std::string, AgentInfo> agents_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  FAILOVER MANAGER
// ═══════════════════════════════════════════════════════════════════════════

class FailoverManager {
public:
    FailoverManager();

    // Failover configuration
    void set_backup(const AgentId& primary, const AgentId& backup);
    void remove_backup(const AgentId& primary);
    std::optional<AgentId> get_backup(const AgentId& primary) const;

    // Failover execution
    bool trigger_failover(const AgentId& failed_agent,
                          AgentRegistry& registry,
                          TaskQueue& queue);

    // Recovery
    void on_agent_recovered(const AgentId& agent);

    // Stats
    struct FailoverEvent {
        AgentId failed_agent;
        AgentId backup_agent;
        std::chrono::system_clock::time_point timestamp;
        int tasks_reassigned;
        bool success;
    };
    std::vector<FailoverEvent> get_failover_history() const;

private:
    std::map<std::string, AgentId> backup_mapping_;
    std::vector<FailoverEvent> history_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  CONSENSUS MODULE
// ═══════════════════════════════════════════════════════════════════════════

class SwarmConsensus {
public:
    SwarmConsensus(double quorum = 0.67);

    // Leader election
    std::optional<AgentId> elect_leader(const std::vector<AgentInfo>& candidates);
    std::optional<AgentId> current_leader() const;
    bool is_leader(const AgentId& agent) const;

    // Voting
    struct Proposal {
        std::string id;
        std::string type;
        std::vector<uint8_t> data;
        AgentId proposer;
        std::map<std::string, bool> votes;
        std::chrono::system_clock::time_point deadline;
    };

    std::string propose(const std::string& type, const std::vector<uint8_t>& data,
                       const AgentId& proposer);
    void vote(const std::string& proposal_id, const AgentId& voter, bool accept);
    std::optional<Proposal> get_proposal(const std::string& id) const;
    bool is_accepted(const std::string& proposal_id) const;

private:
    double quorum_;
    std::optional<AgentId> leader_;
    std::map<std::string, Proposal> proposals_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  SWARM ORCHESTRATOR (Main Interface)
// ═══════════════════════════════════════════════════════════════════════════

struct SwarmConfig {
    std::string swarm_name = "default";
    AgentRole local_role = AgentRole::WORKER;
    int max_workers = 10;
    std::chrono::seconds heartbeat_interval{30};
    std::chrono::seconds task_timeout{300};
    std::chrono::seconds health_check_interval{60};
    LoadBalanceStrategy load_balance = LoadBalanceStrategy::LEAST_LOADED;
    bool enable_failover = true;
    bool enable_consensus = true;
};

class SwarmOrchestrator {
public:
    SwarmOrchestrator(const SwarmConfig& config = {});
    ~SwarmOrchestrator();

    // Singleton
    static SwarmOrchestrator& instance();

    // Lifecycle
    bool start();
    void stop();
    bool is_running() const;

    // Local agent
    AgentId local_agent_id() const;
    AgentInfo local_agent_info() const;
    void set_local_capabilities(const std::vector<AgentCapability>& caps);

    // Agent management
    bool join_swarm(const std::string& master_address);
    void leave_swarm();
    std::vector<AgentInfo> list_agents() const;
    std::optional<AgentInfo> get_agent(const AgentId& id) const;

    // Task submission
    std::string submit_task(const Task& task);
    std::string submit_task(const std::string& type, const std::vector<uint8_t>& input,
                           TaskPriority priority = TaskPriority::NORMAL);
    std::vector<std::string> submit_batch(const std::vector<Task>& tasks);

    // Task management
    std::optional<Task> get_task(const std::string& id) const;
    TaskState get_task_state(const std::string& id) const;
    bool cancel_task(const std::string& id);
    std::vector<Task> list_tasks(TaskState state = TaskState::PENDING) const;

    // Task execution (for workers)
    using TaskHandler = std::function<TaskResult(const Task&)>;
    void register_handler(const std::string& task_type, TaskHandler handler);
    void unregister_handler(const std::string& task_type);

    // Callbacks
    using TaskCallback = std::function<void(const Task&, const TaskResult&)>;
    void on_task_complete(TaskCallback callback);
    void on_task_failed(TaskCallback callback);

    using AgentCallback = std::function<void(const AgentInfo&, bool joined)>;
    void on_agent_change(AgentCallback callback);

    // Consensus
    std::string propose(const std::string& type, const std::vector<uint8_t>& data);
    void vote(const std::string& proposal_id, bool accept);

    // Stats
    struct Stats {
        size_t total_agents;
        size_t active_agents;
        size_t total_tasks;
        size_t pending_tasks;
        size_t running_tasks;
        size_t completed_tasks;
        size_t failed_tasks;
        double avg_task_duration_ms;
        std::chrono::system_clock::time_point started;
    };
    Stats get_stats() const;

    // Components access
    AgentRegistry& registry();
    TaskQueue& task_queue();
    LoadBalancer& load_balancer();

private:
    void scheduler_loop();
    void heartbeat_loop();
    void health_check_loop();
    void worker_loop();

    void dispatch_task(Task& task);
    void handle_task_complete(const std::string& task_id, const TaskResult& result);
    void handle_agent_failure(const AgentId& agent);

    std::string generate_id();

    SwarmConfig config_;
    AgentInfo local_agent_;

    std::unique_ptr<AgentRegistry> registry_;
    std::unique_ptr<TaskQueue> task_queue_;
    std::unique_ptr<LoadBalancer> load_balancer_;
    std::unique_ptr<FailoverManager> failover_;
    std::unique_ptr<SwarmConsensus> consensus_;

    std::map<std::string, TaskHandler> handlers_;
    TaskCallback on_complete_;
    TaskCallback on_failed_;
    AgentCallback on_agent_change_;

    std::atomic<bool> running_{false};
    std::thread scheduler_thread_;
    std::thread heartbeat_thread_;
    std::thread health_thread_;
    std::vector<std::thread> worker_threads_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    Stats stats_;
    std::atomic<uint64_t> next_id_{1};
};

// ═══════════════════════════════════════════════════════════════════════════
//  EMERGENT BEHAVIOR MODULE
// ═══════════════════════════════════════════════════════════════════════════

class EmergentBehavior {
public:
    EmergentBehavior();

    // Swarm intelligence patterns
    struct SwarmState {
        std::map<std::string, double> pheromones;  // Task attraction
        std::map<std::string, double> stigmergy;   // Environmental markers
        double global_fitness = 0;
        int generation = 0;
    };

    // Ant Colony Optimization for task routing
    std::vector<AgentId> aco_route(const Task& task,
                                   const std::vector<AgentInfo>& agents,
                                   SwarmState& state);

    // Particle Swarm Optimization for load balancing
    std::map<std::string, double> pso_optimize(
        const std::vector<Task>& tasks,
        const std::vector<AgentInfo>& agents);

    // Update based on outcomes
    void reinforce(const std::string& path, double reward, SwarmState& state);
    void evaporate(SwarmState& state, double rate = 0.1);

private:
    double alpha_ = 1.0;  // Pheromone importance
    double beta_ = 2.0;   // Heuristic importance
    double rho_ = 0.5;    // Evaporation rate
};

// ═══════════════════════════════════════════════════════════════════════════
//  GLOBAL ACCESS
// ═══════════════════════════════════════════════════════════════════════════

SwarmOrchestrator& swarm();

} // namespace rael
