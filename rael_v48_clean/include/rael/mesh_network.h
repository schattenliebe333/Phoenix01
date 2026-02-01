// RAEL V49 - P2P Mesh Network Engine (#21)
// Decentralized RAEL-to-RAEL communication
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

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  PEER IDENTITY
// ═══════════════════════════════════════════════════════════════════════════

struct PeerId {
    std::string id;          // 256-bit hex identifier
    std::string public_key;  // Ed25519 public key

    bool operator==(const PeerId& other) const { return id == other.id; }
    bool operator<(const PeerId& other) const { return id < other.id; }
    std::string short_id() const { return id.substr(0, 8); }
};

struct PeerInfo {
    PeerId peer_id;
    std::string name;
    std::string version;
    std::vector<std::string> addresses;  // multiaddr format
    std::map<std::string, std::string> metadata;
    std::chrono::system_clock::time_point last_seen;
    bool is_relay = false;
    int reputation = 100;
};

// ═══════════════════════════════════════════════════════════════════════════
//  MESSAGE TYPES
// ═══════════════════════════════════════════════════════════════════════════

enum class MessageType : uint8_t {
    PING = 0,
    PONG = 1,
    FIND_NODE = 2,
    FIND_NODE_RESPONSE = 3,
    STORE = 4,
    STORE_RESPONSE = 5,
    DATA = 10,
    DATA_ACK = 11,
    BROADCAST = 20,
    CONSENSUS_PROPOSE = 30,
    CONSENSUS_VOTE = 31,
    CONSENSUS_COMMIT = 32,
    STREAM_OPEN = 40,
    STREAM_DATA = 41,
    STREAM_CLOSE = 42
};

struct Message {
    std::string id;
    MessageType type;
    PeerId from;
    PeerId to;
    std::vector<uint8_t> payload;
    std::vector<uint8_t> signature;
    uint64_t timestamp;
    uint32_t ttl = 10;
    std::map<std::string, std::string> headers;

    std::string payload_string() const {
        return std::string(payload.begin(), payload.end());
    }
    void set_payload(const std::string& s) {
        payload = std::vector<uint8_t>(s.begin(), s.end());
    }
};

// ═══════════════════════════════════════════════════════════════════════════
//  DHT (Distributed Hash Table)
// ═══════════════════════════════════════════════════════════════════════════

class KademliaTable {
public:
    static constexpr int K = 20;        // Bucket size
    static constexpr int ALPHA = 3;     // Parallelism parameter
    static constexpr int ID_BITS = 256;

    KademliaTable(const PeerId& local_id);

    // Bucket operations
    void add_peer(const PeerInfo& peer);
    void remove_peer(const PeerId& id);
    bool has_peer(const PeerId& id) const;
    std::optional<PeerInfo> get_peer(const PeerId& id) const;

    // Lookup
    std::vector<PeerInfo> closest_peers(const std::string& key, int count = K) const;
    std::vector<PeerInfo> random_peers(int count) const;

    // Maintenance
    void refresh_bucket(int bucket_index);
    void evict_stale_peers(std::chrono::seconds max_age);

    // Stats
    size_t total_peers() const;
    std::vector<size_t> bucket_sizes() const;

private:
    int distance_bucket(const PeerId& a, const PeerId& b) const;
    std::string xor_distance(const std::string& a, const std::string& b) const;

    PeerId local_id_;
    std::vector<std::vector<PeerInfo>> buckets_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  DHT VALUE STORE
// ═══════════════════════════════════════════════════════════════════════════

struct DHTValue {
    std::string key;
    std::vector<uint8_t> value;
    PeerId publisher;
    std::chrono::system_clock::time_point published;
    std::chrono::system_clock::time_point expires;
    std::vector<uint8_t> signature;
};

class DHTStore {
public:
    DHTStore(size_t max_entries = 10000);

    bool put(const std::string& key, const std::vector<uint8_t>& value,
             const PeerId& publisher, std::chrono::seconds ttl = std::chrono::hours(24));
    std::optional<DHTValue> get(const std::string& key) const;
    bool remove(const std::string& key);
    bool has(const std::string& key) const;

    std::vector<std::string> keys() const;
    void cleanup_expired();
    size_t size() const;

private:
    std::map<std::string, DHTValue> store_;
    size_t max_entries_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  GOSSIP PROTOCOL
// ═══════════════════════════════════════════════════════════════════════════

struct GossipMessage {
    std::string id;
    std::string topic;
    std::vector<uint8_t> data;
    PeerId origin;
    uint64_t timestamp;
    uint32_t hop_count = 0;
    std::set<std::string> seen_by;
};

class GossipProtocol {
public:
    using MessageHandler = std::function<void(const GossipMessage&)>;

    GossipProtocol(int fanout = 6, int history_size = 1000);

    // Subscribe to topic
    void subscribe(const std::string& topic, MessageHandler handler);
    void unsubscribe(const std::string& topic);
    std::vector<std::string> subscriptions() const;

    // Publish message
    void publish(const std::string& topic, const std::vector<uint8_t>& data);

    // Receive from network
    void receive(const GossipMessage& msg);

    // Get peers to forward to
    std::vector<PeerId> select_peers(const GossipMessage& msg,
                                     const std::vector<PeerInfo>& available) const;

    // Check if already seen
    bool is_seen(const std::string& msg_id) const;

private:
    int fanout_;
    std::map<std::string, std::vector<MessageHandler>> handlers_;
    std::set<std::string> seen_messages_;
    std::queue<std::string> seen_order_;
    int history_size_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  CONSENSUS PROTOCOL
// ═══════════════════════════════════════════════════════════════════════════

enum class ConsensusState {
    IDLE,
    PROPOSING,
    VOTING,
    COMMITTED,
    FAILED
};

struct ConsensusRound {
    uint64_t round_id;
    std::string proposal_hash;
    std::vector<uint8_t> proposal;
    PeerId proposer;
    std::map<PeerId, bool> votes;
    ConsensusState state = ConsensusState::IDLE;
    std::chrono::system_clock::time_point started;
    std::chrono::system_clock::time_point deadline;
};

class ConsensusProtocol {
public:
    using DecisionHandler = std::function<void(const std::vector<uint8_t>& decision, bool accepted)>;

    ConsensusProtocol(double quorum_threshold = 0.67);

    // Start consensus round
    uint64_t propose(const std::vector<uint8_t>& value, DecisionHandler on_decision);

    // Receive messages
    void on_propose(const PeerId& from, uint64_t round_id, const std::vector<uint8_t>& value);
    void on_vote(const PeerId& from, uint64_t round_id, bool vote);
    void on_commit(const PeerId& from, uint64_t round_id);

    // Vote on proposal
    void vote(uint64_t round_id, bool accept);

    // Get current state
    ConsensusState state(uint64_t round_id) const;
    std::optional<ConsensusRound> get_round(uint64_t round_id) const;

    // Set participants
    void set_participants(const std::vector<PeerId>& peers);

private:
    void check_quorum(uint64_t round_id);

    double quorum_threshold_;
    std::map<uint64_t, ConsensusRound> rounds_;
    std::map<uint64_t, DecisionHandler> handlers_;
    std::vector<PeerId> participants_;
    std::atomic<uint64_t> next_round_id_{1};
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  NAT TRAVERSAL
// ═══════════════════════════════════════════════════════════════════════════

struct STUNResult {
    std::string external_ip;
    uint16_t external_port;
    std::string nat_type;
    bool success;
};

class NATTraversal {
public:
    NATTraversal();

    // STUN
    STUNResult stun_query(const std::string& server = "stun.l.google.com:19302");

    // Hole punching
    bool punch_hole(const PeerInfo& peer);

    // Relay
    void set_relay(const PeerInfo& relay);
    bool is_relayed() const;

    // UPnP
    bool try_upnp_mapping(uint16_t internal_port, uint16_t external_port);
    void remove_upnp_mapping(uint16_t external_port);

private:
    std::optional<PeerInfo> relay_;
    std::map<uint16_t, uint16_t> upnp_mappings_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  TRANSPORT
// ═══════════════════════════════════════════════════════════════════════════

class Transport {
public:
    virtual ~Transport() = default;

    virtual bool listen(const std::string& address) = 0;
    virtual void stop_listening() = 0;
    virtual bool connect(const PeerInfo& peer) = 0;
    virtual void disconnect(const PeerId& peer) = 0;
    virtual bool send(const PeerId& peer, const Message& msg) = 0;
    virtual bool is_connected(const PeerId& peer) const = 0;
    virtual std::vector<PeerId> connected_peers() const = 0;

    using MessageCallback = std::function<void(const PeerId&, const Message&)>;
    using ConnectionCallback = std::function<void(const PeerId&, bool connected)>;

    void set_message_callback(MessageCallback cb) { on_message_ = cb; }
    void set_connection_callback(ConnectionCallback cb) { on_connection_ = cb; }

protected:
    MessageCallback on_message_;
    ConnectionCallback on_connection_;
};

class TCPTransport : public Transport {
public:
    TCPTransport();
    ~TCPTransport() override;

    bool listen(const std::string& address) override;
    void stop_listening() override;
    bool connect(const PeerInfo& peer) override;
    void disconnect(const PeerId& peer) override;
    bool send(const PeerId& peer, const Message& msg) override;
    bool is_connected(const PeerId& peer) const override;
    std::vector<PeerId> connected_peers() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  ENCRYPTION
// ═══════════════════════════════════════════════════════════════════════════

class CryptoProvider {
public:
    CryptoProvider();

    // Key generation
    void generate_keypair();
    std::string public_key() const;
    std::string peer_id() const;

    // Signing
    std::vector<uint8_t> sign(const std::vector<uint8_t>& data) const;
    bool verify(const std::vector<uint8_t>& data, const std::vector<uint8_t>& signature,
               const std::string& public_key) const;

    // Encryption (for peer-to-peer)
    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data,
                                const std::string& recipient_pubkey) const;
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext) const;

    // Shared secret (Diffie-Hellman)
    std::vector<uint8_t> derive_shared_secret(const std::string& peer_pubkey) const;

    // Hashing
    static std::string hash(const std::vector<uint8_t>& data);
    static std::string hash(const std::string& data);

private:
    std::vector<uint8_t> private_key_;
    std::vector<uint8_t> public_key_;
    std::string peer_id_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  MESH NODE
// ═══════════════════════════════════════════════════════════════════════════

struct MeshConfig {
    std::string listen_address = "0.0.0.0:0";
    std::string node_name;
    std::vector<std::string> bootstrap_peers;
    int max_connections = 50;
    int target_connections = 20;
    bool enable_relay = true;
    bool enable_dht = true;
    bool enable_gossip = true;
    std::chrono::seconds peer_discovery_interval{60};
    std::chrono::seconds heartbeat_interval{30};
};

class MeshNode {
public:
    MeshNode(const MeshConfig& config = {});
    ~MeshNode();

    // Lifecycle
    bool start();
    void stop();
    bool is_running() const;

    // Identity
    PeerId local_id() const;
    PeerInfo local_info() const;

    // Peer management
    bool connect(const std::string& address);
    bool connect(const PeerInfo& peer);
    void disconnect(const PeerId& peer);
    std::vector<PeerInfo> connected_peers() const;
    std::vector<PeerInfo> known_peers() const;
    std::optional<PeerInfo> find_peer(const PeerId& id) const;

    // Messaging
    bool send(const PeerId& peer, const std::vector<uint8_t>& data);
    bool send(const PeerId& peer, const std::string& data);

    // Broadcast (gossip)
    void broadcast(const std::string& topic, const std::vector<uint8_t>& data);
    void subscribe(const std::string& topic, GossipProtocol::MessageHandler handler);
    void unsubscribe(const std::string& topic);

    // DHT operations
    bool dht_put(const std::string& key, const std::vector<uint8_t>& value);
    std::optional<std::vector<uint8_t>> dht_get(const std::string& key);

    // Consensus
    uint64_t propose_consensus(const std::vector<uint8_t>& value,
                               ConsensusProtocol::DecisionHandler on_decision);
    void vote_consensus(uint64_t round_id, bool accept);

    // Callbacks
    using MessageHandler = std::function<void(const PeerId&, const std::vector<uint8_t>&)>;
    using PeerHandler = std::function<void(const PeerInfo&, bool connected)>;

    void on_message(MessageHandler handler);
    void on_peer_change(PeerHandler handler);

    // Stats
    struct Stats {
        uint64_t messages_sent;
        uint64_t messages_received;
        uint64_t bytes_sent;
        uint64_t bytes_received;
        size_t connected_peers;
        size_t known_peers;
        std::chrono::system_clock::time_point started;
    };
    Stats get_stats() const;

private:
    void discovery_loop();
    void heartbeat_loop();
    void handle_message(const PeerId& from, const Message& msg);

    MeshConfig config_;
    std::unique_ptr<CryptoProvider> crypto_;
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<KademliaTable> routing_;
    std::unique_ptr<DHTStore> dht_store_;
    std::unique_ptr<GossipProtocol> gossip_;
    std::unique_ptr<ConsensusProtocol> consensus_;
    std::unique_ptr<NATTraversal> nat_;

    MessageHandler message_handler_;
    PeerHandler peer_handler_;

    std::atomic<bool> running_{false};
    std::thread discovery_thread_;
    std::thread heartbeat_thread_;
    mutable std::mutex mutex_;
    Stats stats_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  GLOBAL MESH ACCESS
// ═══════════════════════════════════════════════════════════════════════════

MeshNode& mesh();

} // namespace rael
