// RAEL V49 - Message Queue Implementation (#23)
#include "rael/message_queue.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>
#include <regex>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  MESSAGE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

Message Message::text(const std::string& type, const std::string& content) {
    Message msg;
    msg.header.type = type;
    msg.header.timestamp = std::chrono::system_clock::now();
    msg.payload = content;

    // Generate ID
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss;
    oss << "msg_" << std::hex << dist(gen);
    msg.header.id = oss.str();

    return msg;
}

Message Message::binary(const std::string& type, const std::vector<uint8_t>& data) {
    Message msg;
    msg.header.type = type;
    msg.header.timestamp = std::chrono::system_clock::now();
    msg.payload = data;
    msg.raw_data = data;

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss;
    oss << "msg_" << std::hex << dist(gen);
    msg.header.id = oss.str();

    return msg;
}

Message Message::event(const std::string& event_type) {
    return text(event_type, "");
}

std::string Message::as_string() const {
    if (std::holds_alternative<std::string>(payload)) {
        return std::get<std::string>(payload);
    }
    if (std::holds_alternative<std::vector<uint8_t>>(payload)) {
        const auto& bytes = std::get<std::vector<uint8_t>>(payload);
        return std::string(bytes.begin(), bytes.end());
    }
    if (std::holds_alternative<int64_t>(payload)) {
        return std::to_string(std::get<int64_t>(payload));
    }
    if (std::holds_alternative<double>(payload)) {
        return std::to_string(std::get<double>(payload));
    }
    if (std::holds_alternative<bool>(payload)) {
        return std::get<bool>(payload) ? "true" : "false";
    }
    return "";
}

std::vector<uint8_t> Message::as_bytes() const {
    if (std::holds_alternative<std::vector<uint8_t>>(payload)) {
        return std::get<std::vector<uint8_t>>(payload);
    }
    if (std::holds_alternative<std::string>(payload)) {
        const auto& str = std::get<std::string>(payload);
        return std::vector<uint8_t>(str.begin(), str.end());
    }
    return raw_data;
}

bool Message::is_expired() const {
    if (header.expiry == std::chrono::system_clock::time_point{}) {
        return false;
    }
    return std::chrono::system_clock::now() > header.expiry;
}

std::string Message::to_json() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"id\": \"" << header.id << "\",\n";
    oss << "  \"type\": \"" << header.type << "\",\n";
    oss << "  \"source\": \"" << header.source << "\",\n";
    oss << "  \"destination\": \"" << header.destination << "\",\n";
    oss << "  \"priority\": " << static_cast<int>(header.priority) << ",\n";
    oss << "  \"payload\": \"" << as_string() << "\"\n";
    oss << "}";
    return oss.str();
}

Message Message::from_json(const std::string& json) {
    Message msg;
    // Simple JSON parsing - in production would use proper parser
    auto extract = [&json](const std::string& key) -> std::string {
        std::string pattern = "\"" + key + "\": \"";
        size_t start = json.find(pattern);
        if (start == std::string::npos) return "";
        start += pattern.length();
        size_t end = json.find("\"", start);
        if (end == std::string::npos) return "";
        return json.substr(start, end - start);
    };

    msg.header.id = extract("id");
    msg.header.type = extract("type");
    msg.header.source = extract("source");
    msg.header.destination = extract("destination");
    msg.payload = extract("payload");

    return msg;
}

// ═══════════════════════════════════════════════════════════════════════════
//  MEMORY QUEUE
// ═══════════════════════════════════════════════════════════════════════════

MemoryQueue::MemoryQueue(const QueueConfig& config)
    : config_(config)
    , created_(std::chrono::system_clock::now()) {}

std::string MemoryQueue::name() const {
    return config_.name;
}

bool MemoryQueue::enqueue(const Message& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (queue_.size() >= config_.max_size) {
        return false;
    }

    // Set expiry if not set
    Message m = msg;
    if (m.header.expiry == std::chrono::system_clock::time_point{}) {
        m.header.expiry = std::chrono::system_clock::now() + config_.message_ttl;
    }

    queue_.push(m);
    enqueued_++;
    cv_.notify_one();
    return true;
}

std::optional<Message> MemoryQueue::dequeue() {
    std::lock_guard<std::mutex> lock(mutex_);

    while (!queue_.empty()) {
        Message msg = queue_.top();
        queue_.pop();

        // Skip expired messages
        if (msg.is_expired()) {
            expired_++;
            continue;
        }

        dequeued_++;
        return msg;
    }

    return std::nullopt;
}

std::optional<Message> MemoryQueue::peek() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) return std::nullopt;
    return queue_.top();
}

bool MemoryQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t MemoryQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void MemoryQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
}

QueueStats MemoryQueue::stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    QueueStats s;
    s.name = config_.name;
    s.message_count = queue_.size();
    s.messages_enqueued = enqueued_;
    s.messages_dequeued = dequeued_;
    s.messages_expired = expired_;
    s.created = created_;
    return s;
}

std::optional<Message> MemoryQueue::dequeue_wait(std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (cv_.wait_for(lock, timeout, [this] { return !queue_.empty(); })) {
        Message msg = queue_.top();
        queue_.pop();

        if (msg.is_expired()) {
            expired_++;
            return std::nullopt;
        }

        dequeued_++;
        return msg;
    }

    return std::nullopt;
}

void MemoryQueue::notify() {
    cv_.notify_all();
}

// ═══════════════════════════════════════════════════════════════════════════
//  TOPIC
// ═══════════════════════════════════════════════════════════════════════════

Topic::Topic(const TopicConfig& config) : config_(config) {}

std::string Topic::name() const {
    return config_.name;
}

bool Topic::publish(const Message& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (config_.persistent) {
        messages_.push_back(msg);
    }

    dispatch(msg);
    return true;
}

bool Topic::publish(const std::string& type, const std::string& data) {
    return publish(Message::text(type, data));
}

std::string Topic::subscribe(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);

    Subscription sub;
    sub.id = "sub_" + std::to_string(next_sub_id_++);
    sub.topic = config_.name;
    sub.handler = handler;
    sub.active = true;

    subscriptions_[sub.id] = sub;
    return sub.id;
}

std::string Topic::subscribe(const std::string& pattern, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);

    Subscription sub;
    sub.id = "sub_" + std::to_string(next_sub_id_++);
    sub.topic = config_.name;
    sub.pattern = pattern;
    sub.handler = handler;
    sub.active = true;

    subscriptions_[sub.id] = sub;
    return sub.id;
}

bool Topic::unsubscribe(const std::string& subscription_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return subscriptions_.erase(subscription_id) > 0;
}

std::string Topic::subscribe_group(const std::string& group, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);

    Subscription sub;
    sub.id = "sub_" + std::to_string(next_sub_id_++);
    sub.topic = config_.name;
    sub.group = group;
    sub.handler = handler;
    sub.active = true;

    subscriptions_[sub.id] = sub;
    return sub.id;
}

size_t Topic::subscriber_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return subscriptions_.size();
}

size_t Topic::message_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_.size();
}

void Topic::dispatch(const Message& msg) {
    // Group subscribers by consumer group
    std::map<std::string, std::vector<Subscription*>> groups;
    std::vector<Subscription*> ungrouped;

    for (auto& [_, sub] : subscriptions_) {
        if (!sub.active) continue;

        // Check pattern match
        if (!sub.pattern.empty()) {
            std::regex pattern(sub.pattern);
            if (!std::regex_match(msg.header.type, pattern)) {
                continue;
            }
        }

        if (!sub.group.empty()) {
            groups[sub.group].push_back(&sub);
        } else {
            ungrouped.push_back(&sub);
        }
    }

    // Dispatch to ungrouped (broadcast)
    for (auto* sub : ungrouped) {
        if (sub->handler) {
            sub->handler(msg);
        }
    }

    // Dispatch to groups (round-robin within group)
    static std::map<std::string, size_t> group_indices;
    for (auto& [group, subs] : groups) {
        if (subs.empty()) continue;

        size_t& idx = group_indices[group];
        auto* sub = subs[idx % subs.size()];
        idx++;

        if (sub->handler) {
            sub->handler(msg);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  EXCHANGE
// ═══════════════════════════════════════════════════════════════════════════

Exchange::Exchange(const std::string& name, ExchangeType type)
    : name_(name), type_(type) {}

std::string Exchange::name() const {
    return name_;
}

ExchangeType Exchange::type() const {
    return type_;
}

void Exchange::bind(const std::string& queue_name, const std::string& routing_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    Binding b;
    b.queue_name = queue_name;
    b.routing_key = routing_key;
    bindings_.push_back(b);
}

void Exchange::bind(const std::string& queue_name, const std::string& routing_key,
                    const std::map<std::string, std::string>& args) {
    std::lock_guard<std::mutex> lock(mutex_);
    Binding b;
    b.queue_name = queue_name;
    b.routing_key = routing_key;
    b.arguments = args;
    bindings_.push_back(b);
}

void Exchange::unbind(const std::string& queue_name, const std::string& routing_key) {
    std::lock_guard<std::mutex> lock(mutex_);
    bindings_.erase(
        std::remove_if(bindings_.begin(), bindings_.end(),
            [&](const Binding& b) {
                return b.queue_name == queue_name && b.routing_key == routing_key;
            }),
        bindings_.end());
}

std::vector<std::string> Exchange::route(const Message& msg, const std::string& routing_key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> queues;

    for (const auto& binding : bindings_) {
        bool match = false;

        switch (type_) {
            case ExchangeType::DIRECT:
                match = (binding.routing_key == routing_key);
                break;

            case ExchangeType::FANOUT:
                match = true;
                break;

            case ExchangeType::TOPIC:
                match = matches_pattern(binding.routing_key, routing_key);
                break;

            case ExchangeType::HEADERS:
                match = matches_headers(binding.arguments, msg.header.headers);
                break;
        }

        if (match) {
            queues.push_back(binding.queue_name);
        }
    }

    return queues;
}

std::vector<Binding> Exchange::bindings() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return bindings_;
}

bool Exchange::matches_pattern(const std::string& pattern, const std::string& key) const {
    // AMQP-style topic matching
    // * matches single word, # matches zero or more words
    std::string regex_pattern = pattern;

    // Escape dots
    size_t pos = 0;
    while ((pos = regex_pattern.find(".", pos)) != std::string::npos) {
        regex_pattern.replace(pos, 1, "\\.");
        pos += 2;
    }

    // Replace * with word pattern
    pos = 0;
    while ((pos = regex_pattern.find("*", pos)) != std::string::npos) {
        regex_pattern.replace(pos, 1, "[^.]+");
        pos += 5;
    }

    // Replace # with multi-word pattern
    pos = 0;
    while ((pos = regex_pattern.find("#", pos)) != std::string::npos) {
        regex_pattern.replace(pos, 1, ".*");
        pos += 2;
    }

    std::regex rx(regex_pattern);
    return std::regex_match(key, rx);
}

bool Exchange::matches_headers(const std::map<std::string, std::string>& binding_args,
                               const std::map<std::string, std::string>& msg_headers) const {
    // Check x-match header (all or any)
    bool match_all = true;
    auto it = binding_args.find("x-match");
    if (it != binding_args.end() && it->second == "any") {
        match_all = false;
    }

    int matches = 0;
    int required = 0;

    for (const auto& [key, value] : binding_args) {
        if (key == "x-match") continue;
        required++;

        auto msg_it = msg_headers.find(key);
        if (msg_it != msg_headers.end() && msg_it->second == value) {
            matches++;
        }
    }

    if (match_all) {
        return matches == required;
    } else {
        return matches > 0;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  CONSUMER
// ═══════════════════════════════════════════════════════════════════════════

Consumer::Consumer(const ConsumerConfig& config, std::shared_ptr<IQueue> queue)
    : config_(config), queue_(queue) {}

Consumer::~Consumer() {
    stop();
}

void Consumer::start() {
    if (running_) return;
    running_ = true;
    thread_ = std::thread(&Consumer::consume_loop, this);
}

void Consumer::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool Consumer::is_running() const {
    return running_;
}

void Consumer::set_handler(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handler_ = handler;
}

void Consumer::ack(const std::string& message_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_acks_.erase(message_id);
}

void Consumer::nack(const std::string& message_id, bool requeue) {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_acks_.erase(message_id);

    if (requeue) {
        // Would requeue the message
    }
}

size_t Consumer::messages_processed() const {
    return processed_;
}

void Consumer::consume_loop() {
    auto mem_queue = std::dynamic_pointer_cast<MemoryQueue>(queue_);

    while (running_) {
        std::optional<Message> msg;

        if (mem_queue) {
            msg = mem_queue->dequeue_wait(config_.poll_interval);
        } else {
            msg = queue_->dequeue();
            if (!msg) {
                std::this_thread::sleep_for(config_.poll_interval);
            }
        }

        if (msg && handler_) {
            if (!config_.auto_ack) {
                std::lock_guard<std::mutex> lock(mutex_);
                pending_acks_.insert(msg->header.id);
            }

            handler_(*msg);
            processed_++;

            if (config_.auto_ack) {
                // Auto-acknowledged
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  PRODUCER
// ═══════════════════════════════════════════════════════════════════════════

Producer::Producer(const ProducerConfig& config) : config_(config) {}

bool Producer::send(const std::string& queue_name, const Message& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = queue_cache_.find(queue_name);
    if (it == queue_cache_.end()) {
        // Queue not found in cache - would need broker access
        return false;
    }

    bool success = it->second->enqueue(msg);

    if (config_.confirm_mode && confirm_cb_) {
        confirm_cb_(msg.header.id, success);
    }

    return success;
}

bool Producer::send(const std::string& queue_name, const std::string& data) {
    return send(queue_name, Message::text("text", data));
}

bool Producer::publish(const std::string& exchange, const std::string& routing_key,
                       const Message& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = exchange_cache_.find(exchange);
    if (it == exchange_cache_.end()) {
        return false;
    }

    auto queues = it->second->route(msg, routing_key);
    bool any_success = false;

    for (const auto& queue_name : queues) {
        auto qit = queue_cache_.find(queue_name);
        if (qit != queue_cache_.end()) {
            if (qit->second->enqueue(msg)) {
                any_success = true;
            }
        }
    }

    return any_success;
}

std::optional<Message> Producer::request(const std::string& queue_name, const Message& msg,
                                          std::chrono::milliseconds timeout) {
    // Create temp reply queue
    std::string reply_queue = "reply_" + msg.header.id;
    QueueConfig reply_config;
    reply_config.name = reply_queue;
    auto reply_q = std::make_shared<MemoryQueue>(reply_config);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_cache_[reply_queue] = reply_q;
    }

    // Set reply_to
    Message request = msg;
    request.header.reply_to = reply_queue;

    // Send request
    if (!send(queue_name, request)) {
        return std::nullopt;
    }

    // Wait for reply
    auto reply = reply_q->dequeue_wait(timeout);

    // Cleanup
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_cache_.erase(reply_queue);
    }

    return reply;
}

bool Producer::send_batch(const std::string& queue_name, const std::vector<Message>& messages) {
    bool all_success = true;
    for (const auto& msg : messages) {
        if (!send(queue_name, msg)) {
            all_success = false;
        }
    }
    return all_success;
}

void Producer::set_confirm_callback(ConfirmCallback cb) {
    confirm_cb_ = cb;
}

// ═══════════════════════════════════════════════════════════════════════════
//  DEAD LETTER QUEUE
// ═══════════════════════════════════════════════════════════════════════════

DeadLetterQueue::DeadLetterQueue(const DeadLetterConfig& config) : config_(config) {}

void DeadLetterQueue::send_to_dlq(const Message& msg, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    messages_.push({msg, reason});
}

std::optional<Message> DeadLetterQueue::get() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (messages_.empty()) return std::nullopt;

    auto [msg, reason] = messages_.front();
    messages_.pop();
    return msg;
}

std::vector<Message> DeadLetterQueue::get_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Message> result;

    while (!messages_.empty()) {
        result.push_back(messages_.front().first);
        messages_.pop();
    }

    return result;
}

bool DeadLetterQueue::reprocess(const std::string& message_id, const std::string& target_queue) {
    (void)message_id;
    (void)target_queue;
    // Would send message back to target queue
    return true;
}

bool DeadLetterQueue::reprocess_all(const std::string& target_queue) {
    (void)target_queue;
    return true;
}

size_t DeadLetterQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messages_.size();
}

void DeadLetterQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!messages_.empty()) {
        messages_.pop();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  EVENT BUS
// ═══════════════════════════════════════════════════════════════════════════

PubSubBus::PubSubBus() {}

std::string PubSubBus::on(const std::string& event, EventHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);

    Listener l;
    l.id = "listener_" + std::to_string(next_id_++);
    l.handler = handler;
    l.once = false;

    listeners_[event].push_back(l);
    return l.id;
}

std::string PubSubBus::once(const std::string& event, EventHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);

    Listener l;
    l.id = "listener_" + std::to_string(next_id_++);
    l.handler = handler;
    l.once = true;

    listeners_[event].push_back(l);
    return l.id;
}

void PubSubBus::off(const std::string& subscription_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [_, listeners] : listeners_) {
        listeners.erase(
            std::remove_if(listeners.begin(), listeners.end(),
                [&](const Listener& l) { return l.id == subscription_id; }),
            listeners.end());
    }

    any_listeners_.erase(
        std::remove_if(any_listeners_.begin(), any_listeners_.end(),
            [&](const Listener& l) { return l.id == subscription_id; }),
        any_listeners_.end());
}

void PubSubBus::off_all(const std::string& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    listeners_.erase(event);
}

void PubSubBus::emit(const std::string& event, const MessagePayload& data) {
    std::vector<Listener> to_call;
    std::vector<std::string> to_remove;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Event-specific listeners
        auto it = listeners_.find(event);
        if (it != listeners_.end()) {
            for (const auto& l : it->second) {
                to_call.push_back(l);
                if (l.once) {
                    to_remove.push_back(l.id);
                }
            }
        }

        // Any listeners
        for (const auto& l : any_listeners_) {
            to_call.push_back(l);
            if (l.once) {
                to_remove.push_back(l.id);
            }
        }
    }

    // Call handlers outside lock
    for (const auto& l : to_call) {
        if (l.handler) {
            l.handler(event, data);
        }
    }

    // Remove once listeners
    for (const auto& id : to_remove) {
        off(id);
    }
}

void PubSubBus::emit_async(const std::string& event, const MessagePayload& data) {
    std::thread([this, event, data]() {
        emit(event, data);
    }).detach();
}

void PubSubBus::emit_delayed(const std::string& event, const MessagePayload& data,
                             std::chrono::milliseconds delay) {
    std::thread([this, event, data, delay]() {
        std::this_thread::sleep_for(delay);
        emit(event, data);
    }).detach();
}

std::string PubSubBus::on_any(EventHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);

    Listener l;
    l.id = "listener_" + std::to_string(next_id_++);
    l.handler = handler;
    l.once = false;

    any_listeners_.push_back(l);
    return l.id;
}

size_t PubSubBus::listener_count(const std::string& event) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = listeners_.find(event);
    if (it != listeners_.end()) {
        return it->second.size();
    }
    return 0;
}

std::vector<std::string> PubSubBus::events() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto& [event, _] : listeners_) {
        result.push_back(event);
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
//  MESSAGE BROKER
// ═══════════════════════════════════════════════════════════════════════════

MessageBroker::MessageBroker(const BrokerConfig& config)
    : config_(config)
    , event_bus_(std::make_unique<PubSubBus>())
    , dlq_(std::make_unique<DeadLetterQueue>())
    , started_(std::chrono::system_clock::now()) {}

MessageBroker::~MessageBroker() {
    stop();
}

MessageBroker& MessageBroker::instance() {
    static MessageBroker instance;
    return instance;
}

bool MessageBroker::start() {
    if (running_) return true;
    running_ = true;
    started_ = std::chrono::system_clock::now();

    cleanup_thread_ = std::thread(&MessageBroker::cleanup_loop, this);

    event_bus_->emit("broker.started", config_.name);
    return true;
}

void MessageBroker::stop() {
    if (!running_) return;
    running_ = false;

    if (cleanup_thread_.joinable()) {
        cleanup_thread_.join();
    }

    event_bus_->emit("broker.stopped", config_.name);
}

bool MessageBroker::is_running() const {
    return running_;
}

std::shared_ptr<IQueue> MessageBroker::create_queue(const QueueConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (queues_.size() >= config_.max_queues) {
        return nullptr;
    }

    auto queue = std::make_shared<MemoryQueue>(config);
    queues_[config.name] = queue;

    event_bus_->emit("queue.created", config.name);
    return queue;
}

std::shared_ptr<IQueue> MessageBroker::get_queue(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = queues_.find(name);
    if (it != queues_.end()) {
        return it->second;
    }
    return nullptr;
}

bool MessageBroker::delete_queue(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool deleted = queues_.erase(name) > 0;
    if (deleted) {
        event_bus_->emit("queue.deleted", name);
    }
    return deleted;
}

std::vector<std::string> MessageBroker::list_queues() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& [name, _] : queues_) {
        names.push_back(name);
    }
    return names;
}

std::shared_ptr<Exchange> MessageBroker::create_exchange(const std::string& name,
                                                          ExchangeType type) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (exchanges_.size() >= config_.max_exchanges) {
        return nullptr;
    }

    auto exchange = std::make_shared<Exchange>(name, type);
    exchanges_[name] = exchange;

    event_bus_->emit("exchange.created", name);
    return exchange;
}

std::shared_ptr<Exchange> MessageBroker::get_exchange(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = exchanges_.find(name);
    if (it != exchanges_.end()) {
        return it->second;
    }
    return nullptr;
}

bool MessageBroker::delete_exchange(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool deleted = exchanges_.erase(name) > 0;
    if (deleted) {
        event_bus_->emit("exchange.deleted", name);
    }
    return deleted;
}

std::vector<std::string> MessageBroker::list_exchanges() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& [name, _] : exchanges_) {
        names.push_back(name);
    }
    return names;
}

std::shared_ptr<Topic> MessageBroker::create_topic(const TopicConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto topic = std::make_shared<Topic>(config);
    topics_[config.name] = topic;

    event_bus_->emit("topic.created", config.name);
    return topic;
}

std::shared_ptr<Topic> MessageBroker::get_topic(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = topics_.find(name);
    if (it != topics_.end()) {
        return it->second;
    }
    return nullptr;
}

bool MessageBroker::delete_topic(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    bool deleted = topics_.erase(name) > 0;
    if (deleted) {
        event_bus_->emit("topic.deleted", name);
    }
    return deleted;
}

bool MessageBroker::publish(const std::string& exchange, const std::string& routing_key,
                             const Message& msg) {
    auto ex = get_exchange(exchange);
    if (!ex) return false;

    auto queue_names = ex->route(msg, routing_key);
    bool any_success = false;

    for (const auto& qname : queue_names) {
        auto queue = get_queue(qname);
        if (queue && queue->enqueue(msg)) {
            any_success = true;
            total_messages_++;
        }
    }

    return any_success;
}

std::string MessageBroker::subscribe(const std::string& queue, MessageHandler handler) {
    auto q = get_queue(queue);
    if (!q) return "";

    ConsumerConfig config;
    config.queue_name = queue;
    config.consumer_tag = "consumer_" + std::to_string(total_messages_++);

    auto consumer = std::make_unique<Consumer>(config, q);
    consumer->set_handler(handler);
    consumer->start();

    return config.consumer_tag;
}

PubSubBus& MessageBroker::events() {
    return *event_bus_;
}

DeadLetterQueue& MessageBroker::dlq() {
    return *dlq_;
}

MessageBroker::Stats MessageBroker::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.queue_count = queues_.size();
    s.exchange_count = exchanges_.size();
    s.topic_count = topics_.size();
    s.total_messages = total_messages_;
    s.started = started_;

    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - started_);
    if (duration.count() > 0) {
        s.messages_per_second = total_messages_ / duration.count();
    }

    return s;
}

bool MessageBroker::save_state(const std::string& path) {
    (void)path;
    return true;
}

bool MessageBroker::load_state(const std::string& path) {
    (void)path;
    return true;
}

void MessageBroker::cleanup_loop() {
    while (running_) {
        std::this_thread::sleep_for(config_.cleanup_interval);
        cleanup_expired_messages();
    }
}

void MessageBroker::cleanup_expired_messages() {
    // Cleanup would iterate through queues and remove expired messages
}

// ═══════════════════════════════════════════════════════════════════════════
//  RPC SERVER
// ═══════════════════════════════════════════════════════════════════════════

RPCServer::RPCServer(const std::string& service_name, MessageBroker& broker)
    : service_name_(service_name), broker_(broker) {

    QueueConfig config;
    config.name = "rpc." + service_name;
    request_queue_ = broker_.create_queue(config);
}

RPCServer::~RPCServer() {
    stop();
}

void RPCServer::register_method(const std::string& method, RPCHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    methods_[method] = handler;
}

void RPCServer::unregister_method(const std::string& method) {
    std::lock_guard<std::mutex> lock(mutex_);
    methods_.erase(method);
}

void RPCServer::start() {
    if (running_) return;
    running_ = true;

    ConsumerConfig config;
    config.queue_name = "rpc." + service_name_;

    consumer_ = std::make_unique<Consumer>(config, request_queue_);
    consumer_->set_handler([this](const Message& msg) {
        handle_request(msg);
    });
    consumer_->start();
}

void RPCServer::stop() {
    running_ = false;
    if (consumer_) {
        consumer_->stop();
    }
}

void RPCServer::handle_request(const Message& request) {
    std::string method = request.header.type;
    Message response;
    response.header.correlation_id = request.header.id;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = methods_.find(method);
        if (it != methods_.end()) {
            response = it->second(request);
        } else {
            response = Message::text("error", "Method not found: " + method);
        }
    }

    // Send reply
    if (!request.header.reply_to.empty()) {
        auto reply_queue = broker_.get_queue(request.header.reply_to);
        if (reply_queue) {
            reply_queue->enqueue(response);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  RPC CLIENT
// ═══════════════════════════════════════════════════════════════════════════

RPCClient::RPCClient(const std::string& service_name, MessageBroker& broker)
    : service_name_(service_name), broker_(broker) {

    // Create reply queue
    QueueConfig config;
    config.name = "rpc.reply." + std::to_string(reinterpret_cast<uintptr_t>(this));
    config.exclusive = true;
    config.auto_delete = true;
    reply_queue_ = broker_.create_queue(config);
}

RPCClient::~RPCClient() {
    broker_.delete_queue(reply_queue_->name());
}

std::optional<Message> RPCClient::call(const std::string& method, const Message& request,
                                        std::chrono::milliseconds timeout) {
    Message req = request;
    req.header.type = method;
    req.header.reply_to = reply_queue_->name();

    // Send request
    auto request_queue = broker_.get_queue("rpc." + service_name_);
    if (!request_queue) {
        return std::nullopt;
    }
    request_queue->enqueue(req);

    // Wait for reply
    auto mem_queue = std::dynamic_pointer_cast<MemoryQueue>(reply_queue_);
    if (mem_queue) {
        return mem_queue->dequeue_wait(timeout);
    }

    return std::nullopt;
}

void RPCClient::call_async(const std::string& method, const Message& request,
                            ResponseCallback callback) {
    std::thread([this, method, request, callback]() {
        auto result = call(method, request);
        if (callback) {
            callback(result);
        }
    }).detach();
}

// ═══════════════════════════════════════════════════════════════════════════
//  SAGA ORCHESTRATOR
// ═══════════════════════════════════════════════════════════════════════════

SagaOrchestrator::SagaOrchestrator(const std::string& saga_id)
    : saga_id_(saga_id) {}

SagaOrchestrator& SagaOrchestrator::step(const std::string& name,
                                          std::function<bool()> execute,
                                          std::function<void()> compensate) {
    SagaStep s;
    s.name = name;
    s.execute = execute;
    s.compensate = compensate;
    steps_.push_back(s);
    return *this;
}

bool SagaOrchestrator::execute() {
    std::lock_guard<std::mutex> lock(mutex_);

    status_ = Status::RUNNING;
    completed_steps_.clear();

    for (size_t i = 0; i < steps_.size(); ++i) {
        current_step_ = steps_[i].name;

        if (!steps_[i].execute()) {
            // Step failed - compensate
            status_ = Status::COMPENSATING;

            for (auto it = completed_steps_.rbegin(); it != completed_steps_.rend(); ++it) {
                current_step_ = steps_[*it].name;
                if (steps_[*it].compensate) {
                    steps_[*it].compensate();
                }
            }

            status_ = Status::FAILED;
            return false;
        }

        completed_steps_.push_back(i);
    }

    status_ = Status::COMPLETED;
    current_step_.clear();
    return true;
}

SagaOrchestrator::Status SagaOrchestrator::status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

std::string SagaOrchestrator::current_step() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return current_step_;
}

// ═══════════════════════════════════════════════════════════════════════════
//  GLOBAL ACCESS
// ═══════════════════════════════════════════════════════════════════════════

MessageBroker& mq() {
    return MessageBroker::instance();
}

} // namespace rael
