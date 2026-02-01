// RAEL V49 - ML Framework Implementation (#34, #35)
#include "rael/ml_framework.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <random>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  TENSOR
// ═══════════════════════════════════════════════════════════════════════════

Tensor::Tensor() : shape_({0}), data_({}) {}

Tensor::Tensor(const std::vector<size_t>& shape) : shape_(shape) {
    size_t total = 1;
    for (auto s : shape_) total *= s;
    data_.resize(total, 0.0f);
}

Tensor::Tensor(const std::vector<size_t>& shape, const std::vector<float>& data)
    : shape_(shape), data_(data) {}

std::vector<size_t> Tensor::shape() const { return shape_; }
size_t Tensor::size() const { return data_.size(); }
size_t Tensor::ndim() const { return shape_.size(); }

float& Tensor::at(const std::vector<size_t>& indices) {
    size_t idx = 0;
    size_t stride = 1;
    for (int i = shape_.size() - 1; i >= 0; --i) {
        idx += indices[i] * stride;
        stride *= shape_[i];
    }
    return data_[idx];
}

float Tensor::at(const std::vector<size_t>& indices) const {
    size_t idx = 0;
    size_t stride = 1;
    for (int i = shape_.size() - 1; i >= 0; --i) {
        idx += indices[i] * stride;
        stride *= shape_[i];
    }
    return data_[idx];
}

float* Tensor::data() { return data_.data(); }
const float* Tensor::data() const { return data_.data(); }

Tensor Tensor::operator+(const Tensor& other) const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] + other.data_[i];
    }
    return result;
}

Tensor Tensor::operator-(const Tensor& other) const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] - other.data_[i];
    }
    return result;
}

Tensor Tensor::operator*(const Tensor& other) const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] * other.data_[i];
    }
    return result;
}

Tensor Tensor::operator*(float scalar) const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = data_[i] * scalar;
    }
    return result;
}

Tensor Tensor::matmul(const Tensor& other) const {
    if (shape_.size() != 2 || other.shape_.size() != 2) {
        return Tensor();
    }
    if (shape_[1] != other.shape_[0]) {
        return Tensor();
    }

    size_t m = shape_[0], k = shape_[1], n = other.shape_[1];
    Tensor result({m, n});

    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < n; ++j) {
            float sum = 0;
            for (size_t l = 0; l < k; ++l) {
                sum += data_[i * k + l] * other.data_[l * n + j];
            }
            result.data_[i * n + j] = sum;
        }
    }
    return result;
}

Tensor Tensor::transpose() const {
    if (shape_.size() != 2) return *this;
    Tensor result({shape_[1], shape_[0]});
    for (size_t i = 0; i < shape_[0]; ++i) {
        for (size_t j = 0; j < shape_[1]; ++j) {
            result.data_[j * shape_[0] + i] = data_[i * shape_[1] + j];
        }
    }
    return result;
}

Tensor Tensor::reshape(const std::vector<size_t>& new_shape) const {
    size_t new_size = 1;
    for (auto s : new_shape) new_size *= s;
    if (new_size != data_.size()) return *this;
    return Tensor(new_shape, data_);
}

float Tensor::sum() const {
    return std::accumulate(data_.begin(), data_.end(), 0.0f);
}

float Tensor::mean() const {
    if (data_.empty()) return 0;
    return sum() / data_.size();
}

float Tensor::max() const {
    if (data_.empty()) return 0;
    return *std::max_element(data_.begin(), data_.end());
}

float Tensor::min() const {
    if (data_.empty()) return 0;
    return *std::min_element(data_.begin(), data_.end());
}

Tensor Tensor::argmax(int axis) const {
    (void)axis;
    if (data_.empty()) return Tensor();
    auto it = std::max_element(data_.begin(), data_.end());
    Tensor result({1});
    result.data_[0] = static_cast<float>(std::distance(data_.begin(), it));
    return result;
}

Tensor Tensor::relu() const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = std::max(0.0f, data_[i]);
    }
    return result;
}

Tensor Tensor::sigmoid() const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = 1.0f / (1.0f + std::exp(-data_[i]));
    }
    return result;
}

Tensor Tensor::tanh() const {
    Tensor result(shape_);
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = std::tanh(data_[i]);
    }
    return result;
}

Tensor Tensor::softmax(int axis) const {
    (void)axis;
    Tensor result(shape_);
    float max_val = max();
    float sum_exp = 0;
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] = std::exp(data_[i] - max_val);
        sum_exp += result.data_[i];
    }
    for (size_t i = 0; i < data_.size(); ++i) {
        result.data_[i] /= sum_exp;
    }
    return result;
}

Tensor Tensor::zeros(const std::vector<size_t>& shape) {
    return Tensor(shape);
}

Tensor Tensor::ones(const std::vector<size_t>& shape) {
    Tensor t(shape);
    std::fill(t.data_.begin(), t.data_.end(), 1.0f);
    return t;
}

Tensor Tensor::randn(const std::vector<size_t>& shape, float mean, float std) {
    Tensor t(shape);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(mean, std);
    for (auto& v : t.data_) v = dist(gen);
    return t;
}

Tensor Tensor::uniform(const std::vector<size_t>& shape, float low, float high) {
    Tensor t(shape);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(low, high);
    for (auto& v : t.data_) v = dist(gen);
    return t;
}

// ═══════════════════════════════════════════════════════════════════════════
//  REPLAY BUFFER
// ═══════════════════════════════════════════════════════════════════════════

ReplayBuffer::ReplayBuffer(size_t capacity) : capacity_(capacity), rng_(std::random_device{}()) {}

void ReplayBuffer::add(const Experience& exp) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (buffer_.size() < capacity_) {
        buffer_.push_back(exp);
    } else {
        buffer_[position_] = exp;
    }
    position_ = (position_ + 1) % capacity_;
}

std::vector<Experience> ReplayBuffer::sample(size_t batch_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Experience> batch;
    batch_size = std::min(batch_size, buffer_.size());

    std::uniform_int_distribution<size_t> dist(0, buffer_.size() - 1);
    for (size_t i = 0; i < batch_size; ++i) {
        batch.push_back(buffer_[dist(rng_)]);
    }
    return batch;
}

size_t ReplayBuffer::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return buffer_.size();
}

void ReplayBuffer::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    buffer_.clear();
    position_ = 0;
}

// ═══════════════════════════════════════════════════════════════════════════
//  PRIORITIZED REPLAY BUFFER
// ═══════════════════════════════════════════════════════════════════════════

PrioritizedReplayBuffer::PrioritizedReplayBuffer(size_t capacity, float alpha, float beta)
    : alpha_(alpha), beta_(beta), capacity_(capacity) {}

void PrioritizedReplayBuffer::add(const Experience& exp, float priority) {
    if (buffer_.size() < capacity_) {
        buffer_.push_back({exp, priority});
        priorities_.push_back(std::pow(priority, alpha_));
    }
}

std::vector<std::pair<Experience, float>> PrioritizedReplayBuffer::sample(size_t batch_size) {
    std::vector<std::pair<Experience, float>> batch;
    if (buffer_.empty()) return batch;

    float total = std::accumulate(priorities_.begin(), priorities_.end(), 0.0f);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0, total);

    for (size_t i = 0; i < batch_size && i < buffer_.size(); ++i) {
        float r = dist(gen);
        float cumsum = 0;
        for (size_t j = 0; j < buffer_.size(); ++j) {
            cumsum += priorities_[j];
            if (cumsum >= r) {
                float weight = std::pow(buffer_.size() * priorities_[j] / total, -beta_);
                batch.push_back({buffer_[j].first, weight});
                break;
            }
        }
    }
    return batch;
}

void PrioritizedReplayBuffer::update_priorities(const std::vector<size_t>& indices,
                                                 const std::vector<float>& priorities) {
    for (size_t i = 0; i < indices.size(); ++i) {
        if (indices[i] < priorities_.size()) {
            priorities_[indices[i]] = std::pow(priorities[i], alpha_);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  DQN NETWORK
// ═══════════════════════════════════════════════════════════════════════════

DQNNetwork::DQNNetwork(int state_dim, int action_dim, const std::vector<int>& hidden_dims)
    : state_dim_(state_dim), action_dim_(action_dim), rng_(std::random_device{}()) {

    // Initialize weights
    int prev_dim = state_dim;
    for (int dim : hidden_dims) {
        weights_.push_back(Tensor::randn({static_cast<size_t>(prev_dim), static_cast<size_t>(dim)}, 0, 0.1f));
        prev_dim = dim;
    }
    weights_.push_back(Tensor::randn({static_cast<size_t>(prev_dim), static_cast<size_t>(action_dim)}, 0, 0.1f));

    target_weights_ = weights_;
}

Tensor DQNNetwork::forward(const Tensor& state) {
    Tensor x = state;
    for (size_t i = 0; i < weights_.size() - 1; ++i) {
        x = x.matmul(weights_[i]).relu();
    }
    return x.matmul(weights_.back());
}

int DQNNetwork::select_action(const Tensor& state, float epsilon) {
    std::uniform_real_distribution<float> dist(0, 1);
    if (dist(rng_) < epsilon) {
        std::uniform_int_distribution<int> action_dist(0, action_dim_ - 1);
        return action_dist(rng_);
    }
    Tensor q_values = forward(state);
    return static_cast<int>(q_values.argmax().data()[0]);
}

void DQNNetwork::update(const std::vector<Experience>& batch, float lr) {
    // Simplified gradient descent
    for (const auto& exp : batch) {
        Tensor q_current = forward(exp.state);
        Tensor q_next = forward(exp.next_state);  // Using target network in practice

        float target = exp.reward;
        if (!exp.done) {
            target += 0.99f * q_next.max();
        }

        // Update weights (simplified)
        float error = target - q_current.data()[exp.action];
        for (auto& w : weights_) {
            for (size_t i = 0; i < w.size(); ++i) {
                w.data()[i] += lr * error * 0.01f;
            }
        }
    }
}

Tensor DQNNetwork::get_parameters() const {
    std::vector<float> all_params;
    for (const auto& w : weights_) {
        all_params.insert(all_params.end(), w.data(), w.data() + w.size());
    }
    return Tensor({all_params.size()}, all_params);
}

void DQNNetwork::set_parameters(const Tensor& params) {
    size_t offset = 0;
    for (auto& w : weights_) {
        std::copy(params.data() + offset, params.data() + offset + w.size(), w.data());
        offset += w.size();
    }
}

void DQNNetwork::update_target_network() {
    target_weights_ = weights_;
}

// ═══════════════════════════════════════════════════════════════════════════
//  PPO NETWORK
// ═══════════════════════════════════════════════════════════════════════════

PPONetwork::PPONetwork(int state_dim, int action_dim, const std::vector<int>& hidden_dims)
    : state_dim_(state_dim), action_dim_(action_dim) {

    int prev_dim = state_dim;
    for (int dim : hidden_dims) {
        actor_weights_.push_back(Tensor::randn({static_cast<size_t>(prev_dim), static_cast<size_t>(dim)}, 0, 0.1f));
        critic_weights_.push_back(Tensor::randn({static_cast<size_t>(prev_dim), static_cast<size_t>(dim)}, 0, 0.1f));
        prev_dim = dim;
    }
    actor_weights_.push_back(Tensor::randn({static_cast<size_t>(prev_dim), static_cast<size_t>(action_dim)}, 0, 0.1f));
    critic_weights_.push_back(Tensor::randn({static_cast<size_t>(prev_dim), 1}, 0, 0.1f));
}

Tensor PPONetwork::forward(const Tensor& state) {
    Tensor x = state;
    for (size_t i = 0; i < actor_weights_.size() - 1; ++i) {
        x = x.matmul(actor_weights_[i]).relu();
    }
    return x.matmul(actor_weights_.back()).softmax();
}

int PPONetwork::select_action(const Tensor& state, float epsilon) {
    (void)epsilon;
    Tensor probs = forward(state);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<int> dist(probs.data(), probs.data() + probs.size());
    return dist(gen);
}

void PPONetwork::update(const std::vector<Experience>& batch, float lr) {
    (void)batch;
    (void)lr;
    // PPO update would implement clipped surrogate objective
}

Tensor PPONetwork::get_parameters() const {
    std::vector<float> all_params;
    for (const auto& w : actor_weights_) {
        all_params.insert(all_params.end(), w.data(), w.data() + w.size());
    }
    for (const auto& w : critic_weights_) {
        all_params.insert(all_params.end(), w.data(), w.data() + w.size());
    }
    return Tensor({all_params.size()}, all_params);
}

void PPONetwork::set_parameters(const Tensor& params) {
    size_t offset = 0;
    for (auto& w : actor_weights_) {
        std::copy(params.data() + offset, params.data() + offset + w.size(), w.data());
        offset += w.size();
    }
    for (auto& w : critic_weights_) {
        std::copy(params.data() + offset, params.data() + offset + w.size(), w.data());
        offset += w.size();
    }
}

Tensor PPONetwork::get_value(const Tensor& state) {
    Tensor x = state;
    for (size_t i = 0; i < critic_weights_.size() - 1; ++i) {
        x = x.matmul(critic_weights_[i]).relu();
    }
    return x.matmul(critic_weights_.back());
}

std::pair<Tensor, Tensor> PPONetwork::get_action_and_value(const Tensor& state) {
    return {forward(state), get_value(state)};
}

// ═══════════════════════════════════════════════════════════════════════════
//  RL AGENT
// ═══════════════════════════════════════════════════════════════════════════

RLAgent::RLAgent(std::unique_ptr<PolicyNetwork> policy, const RLConfig& config)
    : policy_(std::move(policy))
    , replay_buffer_(std::make_unique<ReplayBuffer>(config.replay_buffer_size))
    , config_(config)
    , epsilon_(config.epsilon_start) {}

int RLAgent::act(const Tensor& state) {
    return policy_->select_action(state, epsilon_);
}

void RLAgent::observe(const Tensor& state, int action, float reward,
                       const Tensor& next_state, bool done) {
    Experience exp{state, action, reward, next_state, done};
    replay_buffer_->add(exp);
    steps_++;

    if (done) {
        episode_rewards_.push_back(reward);
    }

    epsilon_ = std::max(config_.epsilon_end, epsilon_ * config_.epsilon_decay);
}

void RLAgent::train_step() {
    if (replay_buffer_->size() < static_cast<size_t>(config_.batch_size)) return;

    auto batch = replay_buffer_->sample(config_.batch_size);
    policy_->update(batch, config_.learning_rate);
}

float RLAgent::evaluate(std::function<std::pair<Tensor, float>(int)> env_step, int episodes) {
    float total_reward = 0;
    for (int ep = 0; ep < episodes; ++ep) {
        float ep_reward = 0;
        auto [state, _] = env_step(-1);  // Reset
        bool done = false;
        while (!done) {
            int action = policy_->select_action(state, 0);  // No exploration
            auto [next_state, reward] = env_step(action);
            ep_reward += reward;
            state = next_state;
            done = reward < -100;  // Simplified termination
        }
        total_reward += ep_reward;
    }
    return total_reward / episodes;
}

bool RLAgent::save(const std::string& path) { (void)path; return true; }
bool RLAgent::load(const std::string& path) { (void)path; return true; }

float RLAgent::current_epsilon() const { return epsilon_; }
size_t RLAgent::total_steps() const { return steps_; }

float RLAgent::average_reward() const {
    if (episode_rewards_.empty()) return 0;
    return std::accumulate(episode_rewards_.end() - std::min(100UL, episode_rewards_.size()),
                          episode_rewards_.end(), 0.0f) / std::min(100UL, episode_rewards_.size());
}

// ═══════════════════════════════════════════════════════════════════════════
//  FEDERATED CLIENT
// ═══════════════════════════════════════════════════════════════════════════

FederatedClient::FederatedClient(const std::string& id, std::unique_ptr<PolicyNetwork> model)
    : id_(id), model_(std::move(model)) {
    local_params_ = model_->get_parameters();
}

ClientUpdate FederatedClient::train_local(const std::vector<std::pair<Tensor, Tensor>>& data,
                                           int epochs, float lr) {
    // Simulate local training
    for (int e = 0; e < epochs; ++e) {
        for (const auto& [input, target] : data) {
            (void)input;
            (void)target;
            // Train on batch
        }
    }

    ClientUpdate update;
    update.client_id = id_;
    update.parameters = model_->get_parameters();
    update.num_samples = data.size();
    update.loss = 0.1f;  // Simulated
    update.timestamp = std::chrono::system_clock::now();
    return update;
}

void FederatedClient::receive_model(const Tensor& global_params) {
    model_->set_parameters(global_params);
    local_params_ = global_params;
}

Tensor FederatedClient::add_noise(const Tensor& gradients, float epsilon, float delta) {
    // Add Gaussian noise for differential privacy
    float sensitivity = 1.0f;
    float sigma = sensitivity * std::sqrt(2 * std::log(1.25f / delta)) / epsilon;

    Tensor noisy = gradients;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0, sigma);

    for (size_t i = 0; i < noisy.size(); ++i) {
        noisy.data()[i] += dist(gen);
    }
    return noisy;
}

std::string FederatedClient::id() const { return id_; }

// ═══════════════════════════════════════════════════════════════════════════
//  FEDERATED SERVER
// ═══════════════════════════════════════════════════════════════════════════

FederatedServer::FederatedServer(const FederatedConfig& config) : config_(config) {}

void FederatedServer::set_global_model(std::unique_ptr<PolicyNetwork> model) {
    std::lock_guard<std::mutex> lock(mutex_);
    global_model_ = std::move(model);
}

Tensor FederatedServer::get_global_parameters() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (global_model_) {
        return global_model_->get_parameters();
    }
    return Tensor();
}

void FederatedServer::receive_update(const ClientUpdate& update) {
    std::lock_guard<std::mutex> lock(mutex_);
    round_updates_.push_back(update);
}

bool FederatedServer::aggregate() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (round_updates_.size() < static_cast<size_t>(config_.min_clients)) {
        return false;
    }

    Tensor aggregated = federated_averaging(round_updates_);
    if (global_model_) {
        global_model_->set_parameters(aggregated);
    }

    Metrics m;
    m.round = current_round_;
    m.participating_clients = round_updates_.size();
    history_.push_back(m);

    round_updates_.clear();
    current_round_++;
    return true;
}

Tensor FederatedServer::federated_averaging(const std::vector<ClientUpdate>& updates) {
    if (updates.empty()) return Tensor();

    size_t total_samples = 0;
    for (const auto& u : updates) {
        total_samples += u.num_samples;
    }

    Tensor result = updates[0].parameters * (static_cast<float>(updates[0].num_samples) / total_samples);

    for (size_t i = 1; i < updates.size(); ++i) {
        float weight = static_cast<float>(updates[i].num_samples) / total_samples;
        result = result + updates[i].parameters * weight;
    }

    return result;
}

Tensor FederatedServer::federated_proximal(const std::vector<ClientUpdate>& updates, float mu) {
    (void)mu;
    return federated_averaging(updates);  // Simplified
}

void FederatedServer::start_round() {
    std::lock_guard<std::mutex> lock(mutex_);
    round_updates_.clear();
}

bool FederatedServer::is_round_complete() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return round_updates_.size() >= static_cast<size_t>(config_.min_clients);
}

int FederatedServer::current_round() const {
    return current_round_;
}

std::vector<std::string> FederatedServer::select_clients(const std::vector<std::string>& available) {
    std::vector<std::string> selected;
    size_t count = static_cast<size_t>(available.size() * config_.client_fraction);
    count = std::max(count, static_cast<size_t>(config_.min_clients));
    count = std::min(count, available.size());

    std::random_device rd;
    std::mt19937 gen(rd());
    std::sample(available.begin(), available.end(), std::back_inserter(selected), count, gen);
    return selected;
}

FederatedServer::Metrics FederatedServer::get_metrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (history_.empty()) return {};
    return history_.back();
}

std::vector<FederatedServer::Metrics> FederatedServer::get_history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

// ═══════════════════════════════════════════════════════════════════════════
//  FEDERATED LEARNING
// ═══════════════════════════════════════════════════════════════════════════

FederatedLearning::FederatedLearning(const FederatedConfig& config) : config_(config) {}

FederatedLearning& FederatedLearning::instance() {
    static FederatedLearning inst;
    return inst;
}

void FederatedLearning::set_server(std::unique_ptr<FederatedServer> server) {
    std::lock_guard<std::mutex> lock(mutex_);
    server_ = std::move(server);
}

void FederatedLearning::add_client(std::unique_ptr<FederatedClient> client) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_[client->id()] = std::move(client);
}

void FederatedLearning::remove_client(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients_.erase(client_id);
}

void FederatedLearning::run_round() {
    if (!server_) return;

    std::vector<std::string> available;
    for (const auto& [id, _] : clients_) {
        available.push_back(id);
    }

    auto selected = server_->select_clients(available);
    Tensor global_params = server_->get_global_parameters();

    server_->start_round();

    for (const auto& client_id : selected) {
        auto it = clients_.find(client_id);
        if (it == clients_.end()) continue;

        it->second->receive_model(global_params);
        auto update = it->second->train_local({}, config_.local_epochs, config_.learning_rate);
        server_->receive_update(update);
    }

    server_->aggregate();
}

void FederatedLearning::run_training(int rounds) {
    running_ = true;
    for (int r = 0; r < rounds && running_; ++r) {
        run_round();
    }
    running_ = false;
}

void FederatedLearning::stop() {
    running_ = false;
}

FederatedServer& FederatedLearning::server() { return *server_; }

FederatedClient* FederatedLearning::get_client(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = clients_.find(id);
    if (it != clients_.end()) return it->second.get();
    return nullptr;
}

std::vector<std::string> FederatedLearning::list_clients() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids;
    for (const auto& [id, _] : clients_) {
        ids.push_back(id);
    }
    return ids;
}

FederatedLearning& federated() {
    return FederatedLearning::instance();
}

} // namespace rael
