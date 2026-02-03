// RAEL V49 - Reinforcement Learning (#34) & Federated Learning (#35)
// Machine learning framework
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include <mutex>
#include <chrono>
#include <random>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  TENSOR & BASIC TYPES
// ═══════════════════════════════════════════════════════════════════════════

class Tensor {
public:
    Tensor();
    Tensor(const std::vector<size_t>& shape);
    Tensor(const std::vector<size_t>& shape, const std::vector<float>& data);

    // Shape
    std::vector<size_t> shape() const;
    size_t size() const;
    size_t ndim() const;

    // Access
    float& at(const std::vector<size_t>& indices);
    float at(const std::vector<size_t>& indices) const;
    float* data();
    const float* data() const;

    // Operations
    Tensor operator+(const Tensor& other) const;
    Tensor operator-(const Tensor& other) const;
    Tensor operator*(const Tensor& other) const;
    Tensor operator*(float scalar) const;
    Tensor matmul(const Tensor& other) const;
    Tensor transpose() const;
    Tensor reshape(const std::vector<size_t>& new_shape) const;

    // Reductions
    float sum() const;
    float mean() const;
    float max() const;
    float min() const;
    Tensor argmax(int axis = -1) const;

    // Activation functions
    Tensor relu() const;
    Tensor sigmoid() const;
    Tensor tanh() const;
    Tensor softmax(int axis = -1) const;

    // Initialization
    static Tensor zeros(const std::vector<size_t>& shape);
    static Tensor ones(const std::vector<size_t>& shape);
    static Tensor randn(const std::vector<size_t>& shape, float mean = 0, float std = 1);
    static Tensor uniform(const std::vector<size_t>& shape, float low = 0, float high = 1);

private:
    std::vector<size_t> shape_;
    std::vector<float> data_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  REINFORCEMENT LEARNING (#34)
// ═══════════════════════════════════════════════════════════════════════════

struct Experience {
    Tensor state;
    int action;
    float reward;
    Tensor next_state;
    bool done;
};

class ReplayBuffer {
public:
    explicit ReplayBuffer(size_t capacity = 100000);

    void add(const Experience& exp);
    std::vector<Experience> sample(size_t batch_size);
    size_t size() const;
    void clear();

private:
    std::vector<Experience> buffer_;
    size_t capacity_;
    size_t position_ = 0;
    std::mt19937 rng_;
    mutable std::mutex mutex_;
};

class PrioritizedReplayBuffer {
public:
    PrioritizedReplayBuffer(size_t capacity, float alpha = 0.6, float beta = 0.4);

    void add(const Experience& exp, float priority);
    std::vector<std::pair<Experience, float>> sample(size_t batch_size);
    void update_priorities(const std::vector<size_t>& indices,
                           const std::vector<float>& priorities);

private:
    std::vector<std::pair<Experience, float>> buffer_;
    std::vector<float> priorities_;
    float alpha_, beta_;
    size_t capacity_;
};

// Policy Networks
class PolicyNetwork {
public:
    virtual ~PolicyNetwork() = default;
    virtual Tensor forward(const Tensor& state) = 0;
    virtual int select_action(const Tensor& state, float epsilon = 0) = 0;
    virtual void update(const std::vector<Experience>& batch, float lr) = 0;
    virtual Tensor get_parameters() const = 0;
    virtual void set_parameters(const Tensor& params) = 0;
};

class DQNNetwork : public PolicyNetwork {
public:
    DQNNetwork(int state_dim, int action_dim, const std::vector<int>& hidden_dims);

    Tensor forward(const Tensor& state) override;
    int select_action(const Tensor& state, float epsilon) override;
    void update(const std::vector<Experience>& batch, float lr) override;
    Tensor get_parameters() const override;
    void set_parameters(const Tensor& params) override;

    void update_target_network();

private:
    std::vector<Tensor> weights_;
    std::vector<Tensor> target_weights_;
    int state_dim_, action_dim_;
    std::mt19937 rng_;
};

class PPONetwork : public PolicyNetwork {
public:
    PPONetwork(int state_dim, int action_dim, const std::vector<int>& hidden_dims);

    Tensor forward(const Tensor& state) override;
    int select_action(const Tensor& state, float epsilon) override;
    void update(const std::vector<Experience>& batch, float lr) override;
    Tensor get_parameters() const override;
    void set_parameters(const Tensor& params) override;

    Tensor get_value(const Tensor& state);
    std::pair<Tensor, Tensor> get_action_and_value(const Tensor& state);

private:
    std::vector<Tensor> actor_weights_;
    std::vector<Tensor> critic_weights_;
    int state_dim_, action_dim_;
};

// RL Agent
struct RLConfig {
    float learning_rate = 0.001f;
    float gamma = 0.99f;
    float epsilon_start = 1.0f;
    float epsilon_end = 0.01f;
    float epsilon_decay = 0.995f;
    int batch_size = 64;
    int target_update_freq = 100;
    size_t replay_buffer_size = 100000;
};

class RLAgent {
public:
    RLAgent(std::unique_ptr<PolicyNetwork> policy, const RLConfig& config = {});

    // Training
    int act(const Tensor& state);
    void observe(const Tensor& state, int action, float reward,
                 const Tensor& next_state, bool done);
    void train_step();

    // Evaluation
    float evaluate(std::function<std::pair<Tensor, float>(int)> env_step, int episodes);

    // Save/Load
    bool save(const std::string& path);
    bool load(const std::string& path);

    // Stats
    float current_epsilon() const;
    size_t total_steps() const;
    float average_reward() const;

private:
    std::unique_ptr<PolicyNetwork> policy_;
    std::unique_ptr<ReplayBuffer> replay_buffer_;
    RLConfig config_;
    float epsilon_;
    size_t steps_ = 0;
    std::vector<float> episode_rewards_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  FEDERATED LEARNING (#35)
// ═══════════════════════════════════════════════════════════════════════════

struct FederatedConfig {
    int num_rounds = 100;
    int local_epochs = 5;
    float learning_rate = 0.01f;
    int min_clients = 2;
    float client_fraction = 1.0f;
    bool differential_privacy = false;
    float dp_epsilon = 1.0f;
    float dp_delta = 1e-5f;
};

struct ClientUpdate {
    std::string client_id;
    Tensor parameters;
    size_t num_samples;
    float loss;
    std::chrono::system_clock::time_point timestamp;
};

class FederatedClient {
public:
    FederatedClient(const std::string& id, std::unique_ptr<PolicyNetwork> model);

    // Local training
    ClientUpdate train_local(const std::vector<std::pair<Tensor, Tensor>>& data,
                              int epochs, float lr);

    // Receive global model
    void receive_model(const Tensor& global_params);

    // Privacy
    Tensor add_noise(const Tensor& gradients, float epsilon, float delta);

    std::string id() const;

private:
    std::string id_;
    std::unique_ptr<PolicyNetwork> model_;
    Tensor local_params_;
};

class FederatedServer {
public:
    explicit FederatedServer(const FederatedConfig& config = {});

    // Model management
    void set_global_model(std::unique_ptr<PolicyNetwork> model);
    Tensor get_global_parameters() const;

    // Aggregation
    void receive_update(const ClientUpdate& update);
    bool aggregate();  // FedAvg
    Tensor federated_averaging(const std::vector<ClientUpdate>& updates);
    Tensor federated_proximal(const std::vector<ClientUpdate>& updates, float mu);

    // Round management
    void start_round();
    bool is_round_complete() const;
    int current_round() const;

    // Client selection
    std::vector<std::string> select_clients(const std::vector<std::string>& available);

    // Metrics
    struct Metrics {
        int round;
        float global_loss;
        float accuracy;
        int participating_clients;
        std::chrono::milliseconds round_time;
    };
    Metrics get_metrics() const;
    std::vector<Metrics> get_history() const;

private:
    FederatedConfig config_;
    std::unique_ptr<PolicyNetwork> global_model_;
    std::vector<ClientUpdate> round_updates_;
    std::vector<Metrics> history_;
    int current_round_ = 0;
    mutable std::mutex mutex_;
};

class FederatedLearning {
public:
    FederatedLearning(const FederatedConfig& config = {});

    static FederatedLearning& instance();

    // Setup
    void set_server(std::unique_ptr<FederatedServer> server);
    void add_client(std::unique_ptr<FederatedClient> client);
    void remove_client(const std::string& client_id);

    // Training
    void run_round();
    void run_training(int rounds);
    void stop();

    // Access
    FederatedServer& server();
    FederatedClient* get_client(const std::string& id);
    std::vector<std::string> list_clients() const;

private:
    FederatedConfig config_;
    std::unique_ptr<FederatedServer> server_;
    std::map<std::string, std::unique_ptr<FederatedClient>> clients_;
    bool running_ = false;
    mutable std::mutex mutex_;
};

FederatedLearning& federated();

} // namespace rael
