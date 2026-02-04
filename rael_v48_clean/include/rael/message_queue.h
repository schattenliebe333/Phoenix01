// RAEL V49 - Message Queue (#23)
// Asynchronous message passing and event handling
#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <memory>
#include <functional>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>
#include <variant>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  MESSAGE TYPES
// ═══════════════════════════════════════════════════════════════════════════

enum class MessagePriority {
    LOW = 0,
    NORMAL = 5,
    HIGH = 10,
    URGENT = 100
};

enum class DeliveryMode {
    AT_MOST_ONCE,   // Fire and forget
    AT_LEAST_ONCE,  // Retry until ack
    EXACTLY_ONCE    // Dedup + ack
};

using MessagePayload = std::variant<
    std::string,
    std::vector<uint8_t>,
    int64_t,
    double,
    bool
>;

struct MessageHeader {
    std::string id;
    std::string type;
    std::string source;
    std::string destination;
    std::string correlation_id;
    std::string reply_to;
    MessagePriority priority = MessagePriority::NORMAL;
    DeliveryMode delivery = DeliveryMode::AT_LEAST_ONCE;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::system_clock::time_point expiry;
    std::map<std::string, std::string> headers;
    int retry_count = 0;
    int max_retries = 3;
};

struct Message {
    MessageHeader header;
    MessagePayload payload;
    std::vector<uint8_t> raw_data;

    // Convenience constructors
    static Message text(const std::string& type, const std::string& content);
    static Message binary(const std::string& type, const std::vector<uint8_t>& data);
    static Message event(const std::string& event_type);

    // Payload access
    std::string as_string() const;
    std::vector<uint8_t> as_bytes() const;
    template<typename T> T as() const;

    // Utilities
    bool is_expired() const;
    std::string to_json() const;
    static Message from_json(const std::string& json);
};

// ═══════════════════════════════════════════════════════════════════════════
//  QUEUE INTERFACE
// ═══════════════════════════════════════════════════════════════════════════

struct QueueConfig {
    std::string name;
    size_t max_size = 10000;
    std::chrono::seconds message_ttl{3600};
    bool persistent = false;
    bool exclusive = false;
    bool auto_delete = false;
    std::map<std::string, std::string> arguments;
};

struct QueueStats {
    std::string name;
    size_t message_count;
    size_t consumer_count;
    size_t messages_enqueued;
    size_t messages_dequeued;
    size_t messages_expired;
    size_t messages_dlq;  // Dead letter queue
    double avg_wait_time_ms;
    std::chrono::system_clock::time_point created;
};

class IQueue {
public:
    virtual ~IQueue() = default;

    virtual std::string name() const = 0;
    virtual bool enqueue(const Message& msg) = 0;
    virtual std::optional<Message> dequeue() = 0;
    virtual std::optional<Message> peek() const = 0;
    virtual bool empty() const = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;
    virtual QueueStats stats() const = 0;
};

// ═══════════════════════════════════════════════════════════════════════════
//  IN-MEMORY QUEUE
// ═══════════════════════════════════════════════════════════════════════════

class MemoryQueue : public IQueue {
public:
    explicit MemoryQueue(const QueueConfig& config);

    std::string name() const override;
    bool enqueue(const Message& msg) override;
    std::optional<Message> dequeue() override;
    std::optional<Message> peek() const override;
    bool empty() const override;
    size_t size() const override;
    void clear() override;
    QueueStats stats() const override;

    // Blocking operations
    std::optional<Message> dequeue_wait(std::chrono::milliseconds timeout);
    void notify();

private:
    struct MessageCompare {
        bool operator()(const Message& a, const Message& b) const {
            return static_cast<int>(a.header.priority) <
                   static_cast<int>(b.header.priority);
        }
    };

    QueueConfig config_;
    std::priority_queue<Message, std::vector<Message>, MessageCompare> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;

    // Stats
    std::atomic<size_t> enqueued_{0};
    std::atomic<size_t> dequeued_{0};
    std::atomic<size_t> expired_{0};
    std::chrono::system_clock::time_point created_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  TOPIC / PUBSUB
// ═══════════════════════════════════════════════════════════════════════════

struct TopicConfig {
    std::string name;
    bool persistent = false;
    int partitions = 1;
    int replication_factor = 1;
    std::chrono::seconds retention{86400};  // 24 hours
};

using MessageHandler = std::function<void(const Message&)>;

struct Subscription {
    std::string id;
    std::string topic;
    std::string pattern;  // For pattern matching
    MessageHandler handler;
    std::string group;    // Consumer group
    bool active = true;
};

class Topic {
public:
    explicit Topic(const TopicConfig& config);

    std::string name() const;

    // Publishing
    bool publish(const Message& msg);
    bool publish(const std::string& type, const std::string& data);

    // Subscription
    std::string subscribe(MessageHandler handler);
    std::string subscribe(const std::string& pattern, MessageHandler handler);
    bool unsubscribe(const std::string& subscription_id);

    // Consumer groups
    std::string subscribe_group(const std::string& group, MessageHandler handler);

    // Stats
    size_t subscriber_count() const;
    size_t message_count() const;

private:
    void dispatch(const Message& msg);

    TopicConfig config_;
    std::map<std::string, Subscription> subscriptions_;
    std::vector<Message> messages_;  // For persistence
    mutable std::mutex mutex_;
    std::atomic<uint64_t> next_sub_id_{1};
};

// ═══════════════════════════════════════════════════════════════════════════
//  EXCHANGE
// ═══════════════════════════════════════════════════════════════════════════

enum class ExchangeType {
    DIRECT,     // Route by exact key match
    FANOUT,     // Broadcast to all
    TOPIC,      // Route by pattern
    HEADERS     // Route by header values
};

struct Binding {
    std::string queue_name;
    std::string routing_key;
    std::map<std::string, std::string> arguments;
};

class Exchange {
public:
    Exchange(const std::string& name, ExchangeType type);

    std::string name() const;
    ExchangeType type() const;

    // Binding
    void bind(const std::string& queue_name, const std::string& routing_key);
    void bind(const std::string& queue_name, const std::string& routing_key,
              const std::map<std::string, std::string>& args);
    void unbind(const std::string& queue_name, const std::string& routing_key);

    // Routing
    std::vector<std::string> route(const Message& msg, const std::string& routing_key) const;

    // List bindings
    std::vector<Binding> bindings() const;

private:
    bool matches_pattern(const std::string& pattern, const std::string& key) const;
    bool matches_headers(const std::map<std::string, std::string>& binding_args,
                         const std::map<std::string, std::string>& msg_headers) const;

    std::string name_;
    ExchangeType type_;
    std::vector<Binding> bindings_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  CONSUMER
// ═══════════════════════════════════════════════════════════════════════════

struct ConsumerConfig {
    std::string queue_name;
    std::string consumer_tag;
    bool auto_ack = true;
    int prefetch_count = 10;
    std::chrono::milliseconds poll_interval{100};
};

class Consumer {
public:
    Consumer(const ConsumerConfig& config, std::shared_ptr<IQueue> queue);
    ~Consumer();

    // Control
    void start();
    void stop();
    bool is_running() const;

    // Handler
    void set_handler(MessageHandler handler);

    // Acknowledgement
    void ack(const std::string& message_id);
    void nack(const std::string& message_id, bool requeue = true);

    // Stats
    size_t messages_processed() const;

private:
    void consume_loop();

    ConsumerConfig config_;
    std::shared_ptr<IQueue> queue_;
    MessageHandler handler_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    std::set<std::string> pending_acks_;
    std::atomic<size_t> processed_{0};
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  PRODUCER
// ═══════════════════════════════════════════════════════════════════════════

struct ProducerConfig {
    std::string default_exchange;
    bool confirm_mode = false;
    std::chrono::milliseconds timeout{5000};
    int max_retries = 3;
};

class Producer {
public:
    explicit Producer(const ProducerConfig& config = {});

    // Send to queue directly
    bool send(const std::string& queue_name, const Message& msg);
    bool send(const std::string& queue_name, const std::string& data);

    // Publish to exchange
    bool publish(const std::string& exchange, const std::string& routing_key,
                 const Message& msg);

    // Request-reply pattern
    std::optional<Message> request(const std::string& queue_name, const Message& msg,
                                    std::chrono::milliseconds timeout);

    // Batch sending
    bool send_batch(const std::string& queue_name, const std::vector<Message>& messages);

    // Confirmation
    using ConfirmCallback = std::function<void(const std::string& msg_id, bool acked)>;
    void set_confirm_callback(ConfirmCallback cb);

private:
    ProducerConfig config_;
    ConfirmCallback confirm_cb_;
    std::map<std::string, std::shared_ptr<IQueue>> queue_cache_;
    std::map<std::string, std::shared_ptr<Exchange>> exchange_cache_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  DEAD LETTER QUEUE
// ═══════════════════════════════════════════════════════════════════════════

struct DeadLetterConfig {
    std::string dlq_name = "dlq";
    int max_retries = 3;
    std::chrono::seconds retry_delay{60};
    bool enable_reprocessing = true;
};

class DeadLetterQueue {
public:
    explicit DeadLetterQueue(const DeadLetterConfig& config = {});

    // Move to DLQ
    void send_to_dlq(const Message& msg, const std::string& reason);

    // Retrieve
    std::optional<Message> get();
    std::vector<Message> get_all();

    // Reprocessing
    bool reprocess(const std::string& message_id, const std::string& target_queue);
    bool reprocess_all(const std::string& target_queue);

    // Stats
    size_t size() const;
    void clear();

private:
    DeadLetterConfig config_;
    std::queue<std::pair<Message, std::string>> messages_;  // msg + reason
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  PUB-SUB EVENT BUS (renamed from EventBus to avoid conflict with events.h)
// ═══════════════════════════════════════════════════════════════════════════

using EventHandler = std::function<void(const std::string& event, const MessagePayload& data)>;

class PubSubBus {  // AUDIT-FIX: renamed from EventBus to avoid duplicate class definition
public:
    PubSubBus();

    // Subscribe to events
    std::string on(const std::string& event, EventHandler handler);
    std::string once(const std::string& event, EventHandler handler);
    void off(const std::string& subscription_id);
    void off_all(const std::string& event);

    // Emit events
    void emit(const std::string& event, const MessagePayload& data = {});
    void emit_async(const std::string& event, const MessagePayload& data = {});

    // Delayed emit
    void emit_delayed(const std::string& event, const MessagePayload& data,
                      std::chrono::milliseconds delay);

    // Wildcard events
    std::string on_any(EventHandler handler);

    // Stats
    size_t listener_count(const std::string& event) const;
    std::vector<std::string> events() const;

private:
    struct Listener {
        std::string id;
        EventHandler handler;
        bool once;
    };

    std::map<std::string, std::vector<Listener>> listeners_;
    std::vector<Listener> any_listeners_;
    std::atomic<uint64_t> next_id_{1};
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  MESSAGE BROKER
// ═══════════════════════════════════════════════════════════════════════════

struct BrokerConfig {
    std::string name = "rael-broker";
    size_t max_queues = 100;
    size_t max_exchanges = 50;
    size_t max_connections = 1000;
    bool enable_persistence = false;
    std::string storage_path;
    std::chrono::seconds cleanup_interval{60};
};

class MessageBroker {
public:
    explicit MessageBroker(const BrokerConfig& config = {});
    ~MessageBroker();

    // Singleton
    static MessageBroker& instance();

    // Lifecycle
    bool start();
    void stop();
    bool is_running() const;

    // Queue management
    std::shared_ptr<IQueue> create_queue(const QueueConfig& config);
    std::shared_ptr<IQueue> get_queue(const std::string& name);
    bool delete_queue(const std::string& name);
    std::vector<std::string> list_queues() const;

    // Exchange management
    std::shared_ptr<Exchange> create_exchange(const std::string& name, ExchangeType type);
    std::shared_ptr<Exchange> get_exchange(const std::string& name);
    bool delete_exchange(const std::string& name);
    std::vector<std::string> list_exchanges() const;

    // Topic management
    std::shared_ptr<Topic> create_topic(const TopicConfig& config);
    std::shared_ptr<Topic> get_topic(const std::string& name);
    bool delete_topic(const std::string& name);

    // Publish/Subscribe shortcuts
    bool publish(const std::string& exchange, const std::string& routing_key,
                 const Message& msg);
    std::string subscribe(const std::string& queue, MessageHandler handler);

    // Event bus (PubSubBus)
    PubSubBus& events();

    // Dead letter queue
    DeadLetterQueue& dlq();

    // Stats
    struct Stats {
        size_t queue_count;
        size_t exchange_count;
        size_t topic_count;
        size_t total_messages;
        size_t messages_per_second;
        std::chrono::system_clock::time_point started;
    };
    Stats get_stats() const;

    // Persistence
    bool save_state(const std::string& path = "");
    bool load_state(const std::string& path = "");

private:
    void cleanup_loop();
    void cleanup_expired_messages();

    BrokerConfig config_;
    std::map<std::string, std::shared_ptr<IQueue>> queues_;
    std::map<std::string, std::shared_ptr<Exchange>> exchanges_;
    std::map<std::string, std::shared_ptr<Topic>> topics_;
    std::unique_ptr<PubSubBus> event_bus_;
    std::unique_ptr<DeadLetterQueue> dlq_;

    std::atomic<bool> running_{false};
    std::thread cleanup_thread_;

    std::atomic<size_t> total_messages_{0};
    std::chrono::system_clock::time_point started_;

    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  RPC PATTERN
// ═══════════════════════════════════════════════════════════════════════════

class RPCServer {
public:
    RPCServer(const std::string& service_name, MessageBroker& broker);
    ~RPCServer();

    using RPCHandler = std::function<Message(const Message&)>;

    // Register methods
    void register_method(const std::string& method, RPCHandler handler);
    void unregister_method(const std::string& method);

    // Control
    void start();
    void stop();

private:
    void handle_request(const Message& request);

    std::string service_name_;
    MessageBroker& broker_;
    std::map<std::string, RPCHandler> methods_;
    std::shared_ptr<IQueue> request_queue_;
    std::unique_ptr<Consumer> consumer_;
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
};

class RPCClient {
public:
    RPCClient(const std::string& service_name, MessageBroker& broker);
    ~RPCClient();

    // Synchronous call
    std::optional<Message> call(const std::string& method, const Message& request,
                                 std::chrono::milliseconds timeout = std::chrono::seconds(30));

    // Asynchronous call
    using ResponseCallback = std::function<void(const std::optional<Message>&)>;
    void call_async(const std::string& method, const Message& request,
                    ResponseCallback callback);

private:
    std::string service_name_;
    MessageBroker& broker_;
    std::shared_ptr<IQueue> reply_queue_;
    std::map<std::string, ResponseCallback> pending_calls_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  STREAM PROCESSING
// ═══════════════════════════════════════════════════════════════════════════

template<typename T>
class MessageStream {
public:
    using TransformFn = std::function<T(const T&)>;
    using FilterFn = std::function<bool(const T&)>;
    using SinkFn = std::function<void(const T&)>;

    MessageStream() = default;

    // Transformations
    MessageStream& map(TransformFn fn) {
        transforms_.push_back(fn);
        return *this;
    }

    MessageStream& filter(FilterFn fn) {
        filters_.push_back(fn);
        return *this;
    }

    // Aggregations
    MessageStream& batch(size_t size) {
        batch_size_ = size;
        return *this;
    }

    MessageStream& window(std::chrono::milliseconds duration) {
        window_duration_ = duration;
        return *this;
    }

    // Output
    void sink(SinkFn fn) {
        sink_ = fn;
    }

    // Process
    void process(const T& item) {
        T current = item;

        // Apply filters
        for (const auto& filter : filters_) {
            if (!filter(current)) return;
        }

        // Apply transforms
        for (const auto& transform : transforms_) {
            current = transform(current);
        }

        // Handle batching
        if (batch_size_ > 0) {
            batch_buffer_.push_back(current);
            if (batch_buffer_.size() >= batch_size_) {
                flush_batch();
            }
        } else if (sink_) {
            sink_(current);
        }
    }

    void flush() {
        flush_batch();
    }

private:
    void flush_batch() {
        if (sink_) {
            for (const auto& item : batch_buffer_) {
                sink_(item);
            }
        }
        batch_buffer_.clear();
    }

    std::vector<TransformFn> transforms_;
    std::vector<FilterFn> filters_;
    SinkFn sink_;
    size_t batch_size_ = 0;
    std::chrono::milliseconds window_duration_{0};
    std::vector<T> batch_buffer_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  SAGA PATTERN
// ═══════════════════════════════════════════════════════════════════════════

struct SagaStep {
    std::string name;
    std::function<bool()> execute;
    std::function<void()> compensate;
};

class SagaOrchestrator {
public:
    SagaOrchestrator(const std::string& saga_id);

    // Build saga
    SagaOrchestrator& step(const std::string& name,
                           std::function<bool()> execute,
                           std::function<void()> compensate);

    // Execute
    bool execute();

    // Status
    enum class Status { PENDING, RUNNING, COMPLETED, COMPENSATING, FAILED };
    Status status() const;
    std::string current_step() const;

private:
    std::string saga_id_;
    std::vector<SagaStep> steps_;
    std::vector<size_t> completed_steps_;
    Status status_ = Status::PENDING;
    std::string current_step_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  GLOBAL ACCESS
// ═══════════════════════════════════════════════════════════════════════════

MessageBroker& mq();

} // namespace rael
