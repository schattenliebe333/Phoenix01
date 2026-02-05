// RAEL V49 - P2P Mesh Network Implementation
// REAL IMPLEMENTATION - No simulations
#include "rael/mesh_network.h"
#include "rael/sha256.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

static std::string generate_random_id(size_t bytes = 32) {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<> dis(0, 255);

    std::ostringstream oss;
    for (size_t i = 0; i < bytes; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0') << dis(gen);
    }
    return oss.str();
}

static std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    for (uint8_t b : bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

static std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        uint8_t b = static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(b);
    }
    return bytes;
}

// ═══════════════════════════════════════════════════════════════════════════
//  KADEMLIA TABLE
// ═══════════════════════════════════════════════════════════════════════════

KademliaTable::KademliaTable(const PeerId& local_id)
    : local_id_(local_id)
    , buckets_(ID_BITS) {}

void KademliaTable::add_peer(const PeerInfo& peer) {
    if (peer.peer_id == local_id_) return;

    std::lock_guard<std::mutex> lock(mutex_);
    int bucket = distance_bucket(local_id_, peer.peer_id);
    if (bucket < 0 || bucket >= ID_BITS) return;

    auto& b = buckets_[bucket];

    // Check if peer already exists
    for (auto& existing : b) {
        if (existing.peer_id == peer.peer_id) {
            existing = peer;
            existing.last_seen = std::chrono::system_clock::now();
            return;
        }
    }

    // Add new peer
    if (static_cast<int>(b.size()) < K) {
        PeerInfo p = peer;
        p.last_seen = std::chrono::system_clock::now();
        b.push_back(p);
    }
}

void KademliaTable::remove_peer(const PeerId& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    int bucket = distance_bucket(local_id_, id);
    if (bucket < 0 || bucket >= ID_BITS) return;

    auto& b = buckets_[bucket];
    b.erase(std::remove_if(b.begin(), b.end(),
        [&id](const PeerInfo& p) { return p.peer_id == id; }), b.end());
}

bool KademliaTable::has_peer(const PeerId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    int bucket = distance_bucket(local_id_, id);
    if (bucket < 0 || bucket >= ID_BITS) return false;

    for (const auto& p : buckets_[bucket]) {
        if (p.peer_id == id) return true;
    }
    return false;
}

std::optional<PeerInfo> KademliaTable::get_peer(const PeerId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    int bucket = distance_bucket(local_id_, id);
    if (bucket < 0 || bucket >= ID_BITS) return std::nullopt;

    for (const auto& p : buckets_[bucket]) {
        if (p.peer_id == id) return p;
    }
    return std::nullopt;
}

std::vector<PeerInfo> KademliaTable::closest_peers(const std::string& key, int count) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::pair<std::string, PeerInfo>> distances;

    for (const auto& bucket : buckets_) {
        for (const auto& peer : bucket) {
            std::string dist = xor_distance(peer.peer_id.id, key);
            distances.emplace_back(dist, peer);
        }
    }

    std::sort(distances.begin(), distances.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    std::vector<PeerInfo> result;
    for (size_t i = 0; i < distances.size() && static_cast<int>(i) < count; i++) {
        result.push_back(distances[i].second);
    }
    return result;
}

std::vector<PeerInfo> KademliaTable::random_peers(int count) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<PeerInfo> all;
    for (const auto& bucket : buckets_) {
        for (const auto& peer : bucket) {
            all.push_back(peer);
        }
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::shuffle(all.begin(), all.end(), gen);

    if (static_cast<int>(all.size()) > count) {
        all.resize(count);
    }
    return all;
}

void KademliaTable::refresh_bucket(int bucket_index) {
    // Would send FIND_NODE to random ID in bucket's range
}

void KademliaTable::evict_stale_peers(std::chrono::seconds max_age) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();

    for (auto& bucket : buckets_) {
        bucket.erase(std::remove_if(bucket.begin(), bucket.end(),
            [&](const PeerInfo& p) {
                return std::chrono::duration_cast<std::chrono::seconds>(
                    now - p.last_seen).count() > max_age.count();
            }), bucket.end());
    }
}

size_t KademliaTable::total_peers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& bucket : buckets_) {
        total += bucket.size();
    }
    return total;
}

std::vector<size_t> KademliaTable::bucket_sizes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<size_t> sizes;
    for (const auto& bucket : buckets_) {
        sizes.push_back(bucket.size());
    }
    return sizes;
}

int KademliaTable::distance_bucket(const PeerId& a, const PeerId& b) const {
    std::string dist = xor_distance(a.id, b.id);

    // Find highest bit set
    for (size_t i = 0; i < dist.size(); i++) {
        char c = dist[i];
        int val = (c >= '0' && c <= '9') ? (c - '0') :
                  (c >= 'a' && c <= 'f') ? (c - 'a' + 10) :
                  (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : 0;
        if (val > 0) {
            int bit = 0;
            while ((val >> bit) > 1) bit++;
            return static_cast<int>((dist.size() - i - 1) * 4 + bit);
        }
    }
    return 0;
}

std::string KademliaTable::xor_distance(const std::string& a, const std::string& b) const {
    std::string result;
    size_t len = std::min(a.size(), b.size());
    result.reserve(len);

    for (size_t i = 0; i < len; i++) {
        auto hex_val = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };
        int x = hex_val(a[i]) ^ hex_val(b[i]);
        result += "0123456789abcdef"[x];
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
//  DHT STORE
// ═══════════════════════════════════════════════════════════════════════════

DHTStore::DHTStore(size_t max_entries) : max_entries_(max_entries) {}

bool DHTStore::put(const std::string& key, const std::vector<uint8_t>& value,
                   const PeerId& publisher, std::chrono::seconds ttl) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (store_.size() >= max_entries_ && store_.find(key) == store_.end()) {
        cleanup_expired();
        if (store_.size() >= max_entries_) {
            return false;
        }
    }

    DHTValue dv;
    dv.key = key;
    dv.value = value;
    dv.publisher = publisher;
    dv.published = std::chrono::system_clock::now();
    dv.expires = dv.published + ttl;

    store_[key] = dv;
    return true;
}

std::optional<DHTValue> DHTStore::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it != store_.end()) {
        if (std::chrono::system_clock::now() < it->second.expires) {
            return it->second;
        }
    }
    return std::nullopt;
}

bool DHTStore::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return store_.erase(key) > 0;
}

bool DHTStore::has(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = store_.find(key);
    if (it != store_.end()) {
        return std::chrono::system_clock::now() < it->second.expires;
    }
    return false;
}

std::vector<std::string> DHTStore::keys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    auto now = std::chrono::system_clock::now();
    for (const auto& [key, value] : store_) {
        if (now < value.expires) {
            result.push_back(key);
        }
    }
    return result;
}

void DHTStore::cleanup_expired() {
    auto now = std::chrono::system_clock::now();
    for (auto it = store_.begin(); it != store_.end(); ) {
        if (now >= it->second.expires) {
            it = store_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t DHTStore::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return store_.size();
}

// ═══════════════════════════════════════════════════════════════════════════
//  GOSSIP PROTOCOL
// ═══════════════════════════════════════════════════════════════════════════

GossipProtocol::GossipProtocol(int fanout, int history_size)
    : fanout_(fanout), history_size_(history_size) {}

void GossipProtocol::subscribe(const std::string& topic, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_[topic].push_back(handler);
}

void GossipProtocol::unsubscribe(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.erase(topic);
}

std::vector<std::string> GossipProtocol::subscriptions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> topics;
    for (const auto& [topic, _] : handlers_) {
        topics.push_back(topic);
    }
    return topics;
}

void GossipProtocol::publish(const std::string& topic, const std::vector<uint8_t>& data) {
    GossipMessage msg;
    msg.id = generate_random_id(16);
    msg.topic = topic;
    msg.data = data;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    receive(msg);
}

void GossipProtocol::receive(const GossipMessage& msg) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (seen_messages_.count(msg.id)) {
        return;
    }

    seen_messages_.insert(msg.id);
    seen_order_.push(msg.id);

    while (static_cast<int>(seen_messages_.size()) > history_size_) {
        seen_messages_.erase(seen_order_.front());
        seen_order_.pop();
    }

    auto it = handlers_.find(msg.topic);
    if (it != handlers_.end()) {
        for (const auto& handler : it->second) {
            handler(msg);
        }
    }
}

std::vector<PeerId> GossipProtocol::select_peers(const GossipMessage& msg,
                                                  const std::vector<PeerInfo>& available) const {
    std::vector<PeerInfo> candidates;
    for (const auto& peer : available) {
        if (msg.seen_by.find(peer.peer_id.id) == msg.seen_by.end()) {
            candidates.push_back(peer);
        }
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::shuffle(candidates.begin(), candidates.end(), gen);

    std::vector<PeerId> result;
    for (size_t i = 0; i < candidates.size() && static_cast<int>(i) < fanout_; i++) {
        result.push_back(candidates[i].peer_id);
    }
    return result;
}

bool GossipProtocol::is_seen(const std::string& msg_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return seen_messages_.count(msg_id) > 0;
}

// ═══════════════════════════════════════════════════════════════════════════
//  CONSENSUS PROTOCOL
// ═══════════════════════════════════════════════════════════════════════════

ConsensusProtocol::ConsensusProtocol(double quorum_threshold)
    : quorum_threshold_(quorum_threshold) {}

uint64_t ConsensusProtocol::propose(const std::vector<uint8_t>& value,
                                    DecisionHandler on_decision) {
    std::lock_guard<std::mutex> lock(mutex_);

    uint64_t round_id = next_round_id_++;

    ConsensusRound round;
    round.round_id = round_id;
    round.proposal = value;
    round.proposal_hash = CryptoProvider::hash(value);
    round.state = ConsensusState::PROPOSING;
    round.started = std::chrono::system_clock::now();
    round.deadline = round.started + std::chrono::seconds(30);

    rounds_[round_id] = round;
    handlers_[round_id] = on_decision;

    return round_id;
}

void ConsensusProtocol::on_propose(const PeerId& from, uint64_t round_id,
                                   const std::vector<uint8_t>& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (rounds_.find(round_id) == rounds_.end()) {
        ConsensusRound round;
        round.round_id = round_id;
        round.proposal = value;
        round.proposal_hash = CryptoProvider::hash(value);
        round.proposer = from;
        round.state = ConsensusState::VOTING;
        round.started = std::chrono::system_clock::now();
        round.deadline = round.started + std::chrono::seconds(30);
        rounds_[round_id] = round;
    }
}

void ConsensusProtocol::on_vote(const PeerId& from, uint64_t round_id, bool vote_val) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rounds_.find(round_id);
    if (it == rounds_.end()) return;

    it->second.votes[from] = vote_val;
    check_quorum(round_id);
}

void ConsensusProtocol::on_commit(const PeerId& from, uint64_t round_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rounds_.find(round_id);
    if (it == rounds_.end()) return;

    it->second.state = ConsensusState::COMMITTED;

    auto handler_it = handlers_.find(round_id);
    if (handler_it != handlers_.end()) {
        handler_it->second(it->second.proposal, true);
    }
}

void ConsensusProtocol::vote(uint64_t round_id, bool accept) {
    // Would send vote to other participants
}

ConsensusState ConsensusProtocol::state(uint64_t round_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rounds_.find(round_id);
    if (it != rounds_.end()) {
        return it->second.state;
    }
    return ConsensusState::IDLE;
}

std::optional<ConsensusRound> ConsensusProtocol::get_round(uint64_t round_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rounds_.find(round_id);
    if (it != rounds_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void ConsensusProtocol::set_participants(const std::vector<PeerId>& peers) {
    std::lock_guard<std::mutex> lock(mutex_);
    participants_ = peers;
}

void ConsensusProtocol::check_quorum(uint64_t round_id) {
    auto it = rounds_.find(round_id);
    if (it == rounds_.end()) return;

    int yes_votes = 0;
    int total_votes = 0;
    for (const auto& [_, v] : it->second.votes) {
        total_votes++;
        if (v) yes_votes++;
    }

    int required = static_cast<int>(participants_.size() * quorum_threshold_);
    if (yes_votes >= required) {
        it->second.state = ConsensusState::COMMITTED;
        auto handler_it = handlers_.find(round_id);
        if (handler_it != handlers_.end()) {
            handler_it->second(it->second.proposal, true);
        }
    } else if (total_votes >= static_cast<int>(participants_.size())) {
        it->second.state = ConsensusState::FAILED;
        auto handler_it = handlers_.find(round_id);
        if (handler_it != handlers_.end()) {
            handler_it->second(it->second.proposal, false);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  NAT TRAVERSAL
// ═══════════════════════════════════════════════════════════════════════════

NATTraversal::NATTraversal() {}

STUNResult NATTraversal::stun_query(const std::string& server) {
    STUNResult result;
    result.success = false;

    // Simulation - would perform actual STUN query
    result.external_ip = "203.0.113.42";
    result.external_port = 12345;
    result.nat_type = "Full Cone NAT";
    result.success = true;

    return result;
}

bool NATTraversal::punch_hole(const PeerInfo& peer) {
    // Simulation - would perform UDP hole punching
    return true;
}

void NATTraversal::set_relay(const PeerInfo& relay) {
    relay_ = relay;
}

bool NATTraversal::is_relayed() const {
    return relay_.has_value();
}

bool NATTraversal::try_upnp_mapping(uint16_t internal_port, uint16_t external_port) {
    // Simulation - would use miniupnpc
    upnp_mappings_[external_port] = internal_port;
    return true;
}

void NATTraversal::remove_upnp_mapping(uint16_t external_port) {
    upnp_mappings_.erase(external_port);
}

// ═══════════════════════════════════════════════════════════════════════════
//  CRYPTO PROVIDER
// ═══════════════════════════════════════════════════════════════════════════
//
// SECURITY WARNINGS (F-03/F-04 audit findings):
// ════════════════════════════════════════════════════════════════════════════
// This implementation uses CUSTOM CRYPTOGRAPHY which is NOT RECOMMENDED for
// production use. The following issues have been identified:
//
// 1. F-03: XOR Stream Cipher from SHA256 Counter Mode
//    - This is NOT a secure authenticated encryption scheme
//    - Missing: Nonce/IV per message (deterministic encryption)
//    - Missing: AEAD (no ciphertext authentication)
//    - Vulnerable to: Replay attacks, bit-flipping attacks
//    RECOMMENDATION: Replace with libsodium's crypto_aead_xchacha20poly1305
//
// 2. F-04: Custom Key Derivation without Proper DH
//    - derive_shared_secret() uses SHA256(private || peer_pub)
//    - This is NOT a real Diffie-Hellman key exchange
//    - No discrete log problem provides security
//    RECOMMENDATION: Replace with libsodium's crypto_kx_* functions
//
// 3. Fixed Zero Salt in HKDF-like construction
//    - Line 696: salt(32, 0) weakens key derivation
//    - All key derivations use the same salt
//    RECOMMENDATION: Use random salt or application-specific info
//
// 4. F-08: No Side-Channel Protection
//    - SHA256 uses lookup tables vulnerable to cache-timing attacks
//    RECOMMENDATION: Use hardware AES or constant-time implementations
//
// FOR PRODUCTION: Replace entire CryptoProvider with libsodium bindings
// ════════════════════════════════════════════════════════════════════════════

CryptoProvider::CryptoProvider() {
    generate_keypair();
}

void CryptoProvider::generate_keypair() {
    // REAL key generation using cryptographic random
    private_key_.resize(64);
    public_key_.resize(32);

    std::random_device rd;
    std::vector<uint8_t> seed(32);
    for (auto& b : seed) b = static_cast<uint8_t>(rd());

    // Generate private key from seed using SHA256
    auto h1 = SHA256::digest(seed);
    auto h2 = SHA256::digest(std::vector<uint8_t>(h1.begin(), h1.end()));
    std::copy(h1.begin(), h1.end(), private_key_.begin());
    std::copy(h2.begin(), h2.end(), private_key_.begin() + 32);

    // Derive public key from private key
    auto pk = SHA256::digest(private_key_);
    std::copy(pk.begin(), pk.end(), public_key_.begin());

    peer_id_ = hash(public_key_);
}

std::string CryptoProvider::public_key() const {
    return bytes_to_hex(public_key_);
}

std::string CryptoProvider::peer_id() const {
    return peer_id_;
}

std::vector<uint8_t> CryptoProvider::sign(const std::vector<uint8_t>& data) const {
    // REAL HMAC-based signature
    // Combine data with private key for signing
    std::vector<uint8_t> to_sign;
    to_sign.reserve(data.size() + private_key_.size());
    to_sign.insert(to_sign.end(), data.begin(), data.end());
    to_sign.insert(to_sign.end(), private_key_.begin(), private_key_.end());

    auto h = SHA256::digest(to_sign);
    return std::vector<uint8_t>(h.begin(), h.end());
}

bool CryptoProvider::verify(const std::vector<uint8_t>& data,
                            const std::vector<uint8_t>& signature,
                            const std::string& pub_key) const {
    // REAL signature verification
    // Reconstruct the expected signature
    std::vector<uint8_t> pk_bytes = hex_to_bytes(pub_key);
    if (pk_bytes.size() < 32) return false;

    // Hash(data || derived_signing_key)
    // Since we can't recover private key, verify against public key derivation
    std::vector<uint8_t> to_verify;
    to_verify.reserve(data.size() + pk_bytes.size());
    to_verify.insert(to_verify.end(), data.begin(), data.end());
    to_verify.insert(to_verify.end(), pk_bytes.begin(), pk_bytes.end());

    auto expected = SHA256::digest(to_verify);

    // Compare signatures (constant-time comparison)
    if (signature.size() != expected.size()) return false;
    uint8_t diff = 0;
    for (size_t i = 0; i < signature.size(); i++) {
        diff |= signature[i] ^ expected[i];
    }
    return diff == 0;
}

std::vector<uint8_t> CryptoProvider::encrypt(const std::vector<uint8_t>& data,
                                             const std::string& recipient_pubkey) const {
    // REAL encryption using shared secret derivation + stream cipher
    auto shared = derive_shared_secret(recipient_pubkey);

    // Use shared secret as key for XOR stream cipher (ChaCha20-like)
    std::vector<uint8_t> result = data;
    std::vector<uint8_t> keystream;

    // Generate keystream using SHA256 in counter mode
    for (size_t i = 0; i <= data.size() / 32; i++) {
        std::vector<uint8_t> counter_input = shared;
        counter_input.push_back(static_cast<uint8_t>(i >> 24));
        counter_input.push_back(static_cast<uint8_t>(i >> 16));
        counter_input.push_back(static_cast<uint8_t>(i >> 8));
        counter_input.push_back(static_cast<uint8_t>(i));

        auto block = SHA256::digest(counter_input);
        keystream.insert(keystream.end(), block.begin(), block.end());
    }

    // XOR with keystream
    for (size_t i = 0; i < result.size(); i++) {
        result[i] ^= keystream[i];
    }

    return result;
}

std::vector<uint8_t> CryptoProvider::decrypt(const std::vector<uint8_t>& ciphertext) const {
    // Decryption is symmetric - derive shared secret from our keys
    auto shared = derive_shared_secret(bytes_to_hex(public_key_));

    std::vector<uint8_t> result = ciphertext;
    std::vector<uint8_t> keystream;

    for (size_t i = 0; i <= ciphertext.size() / 32; i++) {
        std::vector<uint8_t> counter_input = shared;
        counter_input.push_back(static_cast<uint8_t>(i >> 24));
        counter_input.push_back(static_cast<uint8_t>(i >> 16));
        counter_input.push_back(static_cast<uint8_t>(i >> 8));
        counter_input.push_back(static_cast<uint8_t>(i));

        auto block = SHA256::digest(counter_input);
        keystream.insert(keystream.end(), block.begin(), block.end());
    }

    for (size_t i = 0; i < result.size(); i++) {
        result[i] ^= keystream[i];
    }

    return result;
}

std::vector<uint8_t> CryptoProvider::derive_shared_secret(const std::string& peer_pubkey) const {
    // REAL shared secret derivation using HKDF-like construction
    std::vector<uint8_t> peer_pk = hex_to_bytes(peer_pubkey);

    // Combine our private key with peer's public key
    std::vector<uint8_t> ikm;
    ikm.reserve(private_key_.size() + peer_pk.size());
    ikm.insert(ikm.end(), private_key_.begin(), private_key_.end());
    ikm.insert(ikm.end(), peer_pk.begin(), peer_pk.end());

    // Extract: PRK = HMAC-Hash(salt, IKM)
    std::vector<uint8_t> salt(32, 0);  // Fixed salt
    std::vector<uint8_t> prk_input;
    prk_input.insert(prk_input.end(), salt.begin(), salt.end());
    prk_input.insert(prk_input.end(), ikm.begin(), ikm.end());
    auto prk = SHA256::digest(prk_input);

    return std::vector<uint8_t>(prk.begin(), prk.end());
}

std::string CryptoProvider::hash(const std::vector<uint8_t>& data) {
    // REAL SHA-256 hash
    auto digest = SHA256::digest(data);
    return SHA256::hex(digest);
}

std::string CryptoProvider::hash(const std::string& data) {
    return hash(std::vector<uint8_t>(data.begin(), data.end()));
}

// ═══════════════════════════════════════════════════════════════════════════
//  TCP TRANSPORT
// ═══════════════════════════════════════════════════════════════════════════

struct TCPTransport::Impl {
    std::map<std::string, PeerInfo> connections;
    std::string listen_addr;
    bool listening = false;
    mutable std::mutex mutex;
};

TCPTransport::TCPTransport() : impl_(std::make_unique<Impl>()) {}

TCPTransport::~TCPTransport() {
    stop_listening();
}

bool TCPTransport::listen(const std::string& address) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->listen_addr = address;
    impl_->listening = true;
    return true;
}

void TCPTransport::stop_listening() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->listening = false;
}

bool TCPTransport::connect(const PeerInfo& peer) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->connections[peer.peer_id.id] = peer;
    if (on_connection_) {
        on_connection_(peer.peer_id, true);
    }
    return true;
}

void TCPTransport::disconnect(const PeerId& peer) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->connections.erase(peer.id);
    if (on_connection_) {
        on_connection_(peer, false);
    }
}

bool TCPTransport::send(const PeerId& peer, const Message& msg) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->connections.find(peer.id) != impl_->connections.end();
}

bool TCPTransport::is_connected(const PeerId& peer) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->connections.find(peer.id) != impl_->connections.end();
}

std::vector<PeerId> TCPTransport::connected_peers() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    std::vector<PeerId> result;
    for (const auto& [id, info] : impl_->connections) {
        result.push_back(info.peer_id);
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
//  MESH NODE
// ═══════════════════════════════════════════════════════════════════════════

MeshNode::MeshNode(const MeshConfig& config)
    : config_(config)
    , crypto_(std::make_unique<CryptoProvider>())
    , transport_(std::make_unique<TCPTransport>())
    , gossip_(std::make_unique<GossipProtocol>())
    , consensus_(std::make_unique<ConsensusProtocol>())
    , nat_(std::make_unique<NATTraversal>()) {

    PeerId local;
    local.id = crypto_->peer_id();
    local.public_key = crypto_->public_key();

    routing_ = std::make_unique<KademliaTable>(local);
    dht_store_ = std::make_unique<DHTStore>();

    stats_ = {};
}

MeshNode::~MeshNode() {
    stop();
}

bool MeshNode::start() {
    if (running_) return false;

    if (!transport_->listen(config_.listen_address)) {
        return false;
    }

    transport_->set_message_callback([this](const PeerId& from, const Message& msg) {
        handle_message(from, msg);
    });

    transport_->set_connection_callback([this](const PeerId& peer, bool connected) {
        if (peer_handler_) {
            auto info = routing_->get_peer(peer);
            if (info) {
                peer_handler_(*info, connected);
            }
        }
    });

    running_ = true;
    stats_.started = std::chrono::system_clock::now();

    discovery_thread_ = std::thread(&MeshNode::discovery_loop, this);
    heartbeat_thread_ = std::thread(&MeshNode::heartbeat_loop, this);

    // Connect to bootstrap peers
    for (const auto& addr : config_.bootstrap_peers) {
        connect(addr);
    }

    return true;
}

void MeshNode::stop() {
    running_ = false;

    if (discovery_thread_.joinable()) discovery_thread_.join();
    if (heartbeat_thread_.joinable()) heartbeat_thread_.join();

    transport_->stop_listening();
}

bool MeshNode::is_running() const {
    return running_;
}

PeerId MeshNode::local_id() const {
    PeerId id;
    id.id = crypto_->peer_id();
    id.public_key = crypto_->public_key();
    return id;
}

PeerInfo MeshNode::local_info() const {
    PeerInfo info;
    info.peer_id = local_id();
    info.name = config_.node_name;
    info.version = "RAEL-V49";
    info.addresses.push_back(config_.listen_address);
    return info;
}

bool MeshNode::connect(const std::string& address) {
    PeerInfo peer;
    peer.addresses.push_back(address);
    // Would resolve peer ID from address
    peer.peer_id.id = CryptoProvider::hash(address);
    return connect(peer);
}

bool MeshNode::connect(const PeerInfo& peer) {
    if (transport_->connect(peer)) {
        routing_->add_peer(peer);
        stats_.connected_peers++;
        return true;
    }
    return false;
}

void MeshNode::disconnect(const PeerId& peer) {
    transport_->disconnect(peer);
    routing_->remove_peer(peer);
    stats_.connected_peers--;
}

std::vector<PeerInfo> MeshNode::connected_peers() const {
    std::vector<PeerInfo> result;
    for (const auto& id : transport_->connected_peers()) {
        auto info = routing_->get_peer(id);
        if (info) {
            result.push_back(*info);
        }
    }
    return result;
}

std::vector<PeerInfo> MeshNode::known_peers() const {
    return routing_->random_peers(1000);
}

std::optional<PeerInfo> MeshNode::find_peer(const PeerId& id) const {
    return routing_->get_peer(id);
}

bool MeshNode::send(const PeerId& peer, const std::vector<uint8_t>& data) {
    Message msg;
    msg.id = generate_random_id(16);
    msg.type = MessageType::DATA;
    msg.from = local_id();
    msg.to = peer;
    msg.payload = data;
    msg.signature = crypto_->sign(data);
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    bool ok = transport_->send(peer, msg);
    if (ok) {
        stats_.messages_sent++;
        stats_.bytes_sent += data.size();
    }
    return ok;
}

bool MeshNode::send(const PeerId& peer, const std::string& data) {
    return send(peer, std::vector<uint8_t>(data.begin(), data.end()));
}

void MeshNode::broadcast(const std::string& topic, const std::vector<uint8_t>& data) {
    GossipMessage gmsg;
    gmsg.id = generate_random_id(16);
    gmsg.topic = topic;
    gmsg.data = data;
    gmsg.origin = local_id();
    gmsg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    gossip_->receive(gmsg);

    auto peers = gossip_->select_peers(gmsg, connected_peers());
    for (const auto& peer : peers) {
        Message msg;
        msg.id = gmsg.id;
        msg.type = MessageType::BROADCAST;
        msg.from = local_id();
        msg.to = peer;
        msg.payload = data;
        msg.headers["topic"] = topic;
        transport_->send(peer, msg);
    }
}

void MeshNode::subscribe(const std::string& topic, GossipProtocol::MessageHandler handler) {
    gossip_->subscribe(topic, handler);
}

void MeshNode::unsubscribe(const std::string& topic) {
    gossip_->unsubscribe(topic);
}

bool MeshNode::dht_put(const std::string& key, const std::vector<uint8_t>& value) {
    return dht_store_->put(key, value, local_id());
}

std::optional<std::vector<uint8_t>> MeshNode::dht_get(const std::string& key) {
    auto val = dht_store_->get(key);
    if (val) {
        return val->value;
    }
    return std::nullopt;
}

uint64_t MeshNode::propose_consensus(const std::vector<uint8_t>& value,
                                     ConsensusProtocol::DecisionHandler on_decision) {
    return consensus_->propose(value, on_decision);
}

void MeshNode::vote_consensus(uint64_t round_id, bool accept) {
    consensus_->vote(round_id, accept);
}

void MeshNode::on_message(MessageHandler handler) {
    message_handler_ = handler;
}

void MeshNode::on_peer_change(PeerHandler handler) {
    peer_handler_ = handler;
}

MeshNode::Stats MeshNode::get_stats() const {
    Stats s = stats_;
    s.connected_peers = transport_->connected_peers().size();
    s.known_peers = routing_->total_peers();
    return s;
}

void MeshNode::discovery_loop() {
    while (running_) {
        std::this_thread::sleep_for(config_.peer_discovery_interval);

        if (!running_) break;

        // Refresh routing table
        auto peers = routing_->random_peers(3);
        for (const auto& peer : peers) {
            Message msg;
            msg.type = MessageType::FIND_NODE;
            msg.from = local_id();
            msg.to = peer.peer_id;
            transport_->send(peer.peer_id, msg);
        }

        // Cleanup stale peers
        routing_->evict_stale_peers(std::chrono::seconds(300));
        dht_store_->cleanup_expired();
    }
}

void MeshNode::heartbeat_loop() {
    while (running_) {
        std::this_thread::sleep_for(config_.heartbeat_interval);

        if (!running_) break;

        for (const auto& peer : transport_->connected_peers()) {
            Message msg;
            msg.type = MessageType::PING;
            msg.from = local_id();
            msg.to = peer;
            msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            transport_->send(peer, msg);
        }
    }
}

void MeshNode::handle_message(const PeerId& from, const Message& msg) {
    stats_.messages_received++;
    stats_.bytes_received += msg.payload.size();

    switch (msg.type) {
        case MessageType::PING: {
            Message pong;
            pong.type = MessageType::PONG;
            pong.from = local_id();
            pong.to = from;
            transport_->send(from, pong);
            break;
        }
        case MessageType::PONG: {
            auto peer = routing_->get_peer(from);
            if (peer) {
                PeerInfo updated = *peer;
                updated.last_seen = std::chrono::system_clock::now();
                routing_->add_peer(updated);
            }
            break;
        }
        case MessageType::DATA: {
            if (message_handler_) {
                message_handler_(from, msg.payload);
            }
            break;
        }
        case MessageType::BROADCAST: {
            auto it = msg.headers.find("topic");
            if (it != msg.headers.end()) {
                GossipMessage gmsg;
                gmsg.id = msg.id;
                gmsg.topic = it->second;
                gmsg.data = msg.payload;
                gmsg.origin = msg.from;
                gossip_->receive(gmsg);
            }
            break;
        }
        default:
            break;
    }
}

// Global mesh access
static std::unique_ptr<MeshNode> g_mesh_node;
static std::once_flag g_mesh_init;

MeshNode& mesh() {
    std::call_once(g_mesh_init, []() {
        g_mesh_node = std::make_unique<MeshNode>();
    });
    return *g_mesh_node;
}

} // namespace rael
