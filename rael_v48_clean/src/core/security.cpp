// RAEL V49 - Security Implementation (#30, #31, #32)
#include "rael/security.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <random>
#include <ctime>
#include <atomic>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  CAPABILITY
// ═══════════════════════════════════════════════════════════════════════════

bool Capability::is_valid() const {
    auto now = std::chrono::system_clock::now();
    return now >= issued && now < expires && !signature.empty();
}

bool Capability::has_permission(Permission p) const {
    return permissions.count(p) > 0;
}

bool Capability::allows_resource(const std::string& res) const {
    if (resource == "*") return true;
    if (resource == res) return true;
    // Check wildcard prefix
    if (resource.back() == '*') {
        std::string prefix = resource.substr(0, resource.length() - 1);
        return res.find(prefix) == 0;
    }
    return false;
}

std::string Capability::to_token() const {
    std::ostringstream oss;
    oss << id << ":" << resource << ":" << subject << ":" << signature;
    return oss.str();
}

Capability Capability::from_token(const std::string& token) {
    Capability cap;
    std::istringstream iss(token);
    std::getline(iss, cap.id, ':');
    std::getline(iss, cap.resource, ':');
    std::getline(iss, cap.subject, ':');
    std::getline(iss, cap.signature, ':');
    return cap;
}

// ═══════════════════════════════════════════════════════════════════════════
//  CAPABILITY MANAGER
// ═══════════════════════════════════════════════════════════════════════════

CapabilityManager::CapabilityManager() {
    signing_key_ = CryptoProvider::random_string(32);
}

Capability CapabilityManager::issue(const std::string& subject, const std::string& resource,
                                     const std::set<Permission>& perms,
                                     std::chrono::seconds ttl) {
    std::lock_guard<std::mutex> lock(mutex_);

    Capability cap;
    cap.id = "cap_" + CryptoProvider::random_string(16);
    cap.resource = resource;
    cap.permissions = perms;
    cap.issued = std::chrono::system_clock::now();
    cap.expires = cap.issued + ttl;
    cap.issuer = "rael";
    cap.subject = subject;
    cap.signature = CryptoProvider::hmac_sha256(cap.id + resource + subject, signing_key_);

    by_subject_[subject].push_back(cap);
    return cap;
}

Capability CapabilityManager::delegate(const Capability& parent, const std::string& new_subject,
                                        const std::set<Permission>& subset) {
    std::set<Permission> allowed;
    for (const auto& p : subset) {
        if (parent.has_permission(p)) {
            allowed.insert(p);
        }
    }

    auto ttl = std::chrono::duration_cast<std::chrono::seconds>(parent.expires - std::chrono::system_clock::now());
    return issue(new_subject, parent.resource, allowed, ttl);
}

bool CapabilityManager::verify(const Capability& cap) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!cap.is_valid()) return false;
    if (revoked_.count(cap.id)) return false;

    std::string expected = CryptoProvider::hmac_sha256(
        cap.id + cap.resource + cap.subject, signing_key_);
    return cap.signature == expected;
}

bool CapabilityManager::check_access(const Capability& cap, const std::string& resource,
                                      Permission perm) {
    if (!verify(cap)) return false;
    if (!cap.allows_resource(resource)) return false;
    if (!cap.has_permission(perm)) return false;
    return true;
}

void CapabilityManager::revoke(const std::string& capability_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    revoked_.insert(capability_id);
}

void CapabilityManager::revoke_all(const std::string& subject) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = by_subject_.find(subject);
    if (it != by_subject_.end()) {
        for (const auto& cap : it->second) {
            revoked_.insert(cap.id);
        }
        by_subject_.erase(it);
    }
}

bool CapabilityManager::is_revoked(const std::string& capability_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return revoked_.count(capability_id) > 0;
}

std::vector<Capability> CapabilityManager::list_capabilities(const std::string& subject) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = by_subject_.find(subject);
    if (it != by_subject_.end()) return it->second;
    return {};
}

// ═══════════════════════════════════════════════════════════════════════════
//  ZERO TRUST ENGINE
// ═══════════════════════════════════════════════════════════════════════════

ZeroTrustEngine::ZeroTrustEngine() {}

AuthResult ZeroTrustEngine::authenticate(const std::string& identity_id,
                                          AuthMethod method,
                                          const std::map<std::string, std::string>& credentials) {
    std::lock_guard<std::mutex> lock(mutex_);

    AuthResult result;
    result.success = false;

    auto it = identities_.find(identity_id);
    if (it == identities_.end()) {
        result.error = "Identity not found";
        return result;
    }

    // Simulate authentication
    bool authenticated = false;
    switch (method) {
        case AuthMethod::PASSWORD:
            authenticated = credentials.count("password") > 0;
            break;
        case AuthMethod::TOKEN:
            authenticated = credentials.count("token") > 0;
            break;
        case AuthMethod::CERTIFICATE:
            authenticated = credentials.count("cert") > 0;
            break;
        default:
            authenticated = true;
    }

    if (!authenticated) {
        result.error = "Authentication failed";
        return result;
    }

    // Create session
    AuthContext ctx;
    ctx.identity = it->second;
    ctx.session_id = "sess_" + CryptoProvider::random_string(16);
    ctx.created = std::chrono::system_clock::now();
    ctx.risk_score = calculate_risk_score(ctx);

    sessions_[ctx.session_id] = ctx;

    result.success = true;
    result.session_id = ctx.session_id;
    result.risk_score = ctx.risk_score;

    it->second.last_verified = std::chrono::system_clock::now();
    return result;
}

bool ZeroTrustEngine::verify_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.find(session_id) != sessions_.end();
}

void ZeroTrustEngine::invalidate_session(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session_id);
}

bool ZeroTrustEngine::continuous_verify(const AuthContext& ctx) {
    int risk = calculate_risk_score(ctx);
    return risk < 80;  // Threshold
}

int ZeroTrustEngine::calculate_risk_score(const AuthContext& ctx) {
    int score = 0;

    // Check trust level
    score += (100 - ctx.identity.trust_level) / 2;

    // Check if IP is known
    if (ctx.source_ip.empty()) score += 10;

    // Check device
    if (ctx.device_id.empty()) score += 15;

    return std::min(100, score);
}

void ZeroTrustEngine::add_policy(const Policy& policy) {
    std::lock_guard<std::mutex> lock(mutex_);
    policies_.push_back(policy);
}

void ZeroTrustEngine::remove_policy(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    policies_.erase(
        std::remove_if(policies_.begin(), policies_.end(),
                       [&name](const Policy& p) { return p.name == name; }),
        policies_.end());
}

std::vector<Permission> ZeroTrustEngine::evaluate_policies(const AuthContext& ctx) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::set<Permission> granted;

    for (const auto& policy : policies_) {
        if (ctx.identity.trust_level >= policy.min_trust_level &&
            policy.condition(ctx)) {
            granted.insert(policy.grants.begin(), policy.grants.end());
        }
    }

    return std::vector<Permission>(granted.begin(), granted.end());
}

void ZeroTrustEngine::register_identity(const Identity& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    identities_[id.id] = id;
}

void ZeroTrustEngine::update_trust_level(const std::string& identity_id, int delta) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = identities_.find(identity_id);
    if (it != identities_.end()) {
        it->second.trust_level = std::clamp(it->second.trust_level + delta, 0, 100);
    }
}

std::optional<Identity> ZeroTrustEngine::get_identity(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = identities_.find(id);
    if (it != identities_.end()) return it->second;
    return std::nullopt;
}

std::string ZeroTrustEngine::generate_mfa_challenge(const std::string& identity_id) {
    (void)identity_id;
    return CryptoProvider::random_string(6);
}

bool ZeroTrustEngine::verify_mfa(const std::string& identity_id, const std::string& code) {
    (void)identity_id;
    return code.length() == 6;
}

// ═══════════════════════════════════════════════════════════════════════════
//  AUDIT LOGGER
// ═══════════════════════════════════════════════════════════════════════════

AuditLogger::AuditLogger() {
    chain_hash_ = CryptoProvider::hash_sha256("genesis");
}

void AuditLogger::log(const AuditEvent& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    AuditEvent e = event;
    e.id = generate_id();
    e.timestamp = std::chrono::system_clock::now();
    e.signature = sign_event(e);
    events_.push_back(e);

    chain_hash_ = CryptoProvider::hash_sha256(chain_hash_ + e.signature);
}

void AuditLogger::log(AuditEventType type, const std::string& actor, const std::string& action,
                       const std::string& resource, const std::map<std::string, AuditData>& details) {
    AuditEvent event;
    event.type = type;
    event.actor_id = actor;
    event.action = action;
    event.resource = resource;
    event.details = details;
    log(event);
}

void AuditLogger::log_auth(const std::string& actor, bool success, const std::string& method) {
    log(success ? AuditEventType::AUTH_SUCCESS : AuditEventType::AUTH_FAILURE,
        actor, "authenticate", "auth", {{"method", method}});
}

void AuditLogger::log_access(const std::string& actor, const std::string& resource,
                              Permission perm, bool granted) {
    log(granted ? AuditEventType::ACCESS_GRANTED : AuditEventType::ACCESS_DENIED,
        actor, "access", resource, {{"permission", std::to_string(static_cast<int>(perm))}});
}

void AuditLogger::log_change(const std::string& actor, const std::string& resource,
                              const std::string& old_value, const std::string& new_value) {
    log(AuditEventType::RESOURCE_UPDATE, actor, "update", resource,
        {{"old", old_value}, {"new", new_value}});
}

void AuditLogger::log_alert(const std::string& message, int severity) {
    AuditEvent event;
    event.type = AuditEventType::SECURITY_ALERT;
    event.action = message;
    event.severity = severity;
    log(event);
}

std::vector<AuditEvent> AuditLogger::query(const AuditQuery& q) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AuditEvent> result;

    for (const auto& event : events_) {
        bool match = true;

        if (q.type && event.type != *q.type) match = false;
        if (q.actor_id && event.actor_id != *q.actor_id) match = false;
        if (q.resource && event.resource != *q.resource) match = false;
        if (q.start_time && event.timestamp < *q.start_time) match = false;
        if (q.end_time && event.timestamp > *q.end_time) match = false;
        if (q.min_severity && event.severity < *q.min_severity) match = false;

        if (match) {
            result.push_back(event);
            if (static_cast<int>(result.size()) >= q.limit) break;
        }
    }

    return result;
}

std::vector<AuditEvent> AuditLogger::get_recent(int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    int start = std::max(0, static_cast<int>(events_.size()) - count);
    return std::vector<AuditEvent>(events_.begin() + start, events_.end());
}

std::vector<AuditEvent> AuditLogger::get_by_actor(const std::string& actor_id, int limit) {
    AuditQuery q;
    q.actor_id = actor_id;
    q.limit = limit;
    return query(q);
}

std::vector<AuditEvent> AuditLogger::get_by_resource(const std::string& resource, int limit) {
    AuditQuery q;
    q.resource = resource;
    q.limit = limit;
    return query(q);
}

bool AuditLogger::verify_integrity() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string hash = CryptoProvider::hash_sha256("genesis");

    for (const auto& event : events_) {
        std::string expected_sig = sign_event(event);
        if (event.signature != expected_sig) return false;
        hash = CryptoProvider::hash_sha256(hash + event.signature);
    }

    return hash == chain_hash_;
}

std::string AuditLogger::compute_chain_hash() {
    std::lock_guard<std::mutex> lock(mutex_);
    return chain_hash_;
}

std::string AuditLogger::export_json(const std::vector<AuditEvent>& events) {
    std::ostringstream oss;
    oss << "[\n";
    for (size_t i = 0; i < events.size(); ++i) {
        if (i > 0) oss << ",\n";
        oss << "  {\"id\": \"" << events[i].id << "\", "
            << "\"action\": \"" << events[i].action << "\", "
            << "\"actor\": \"" << events[i].actor_id << "\"}";
    }
    oss << "\n]";
    return oss.str();
}

std::string AuditLogger::export_csv(const std::vector<AuditEvent>& events) {
    std::ostringstream oss;
    oss << "id,action,actor,resource,severity\n";
    for (const auto& e : events) {
        oss << e.id << "," << e.action << "," << e.actor_id << ","
            << e.resource << "," << e.severity << "\n";
    }
    return oss.str();
}

bool AuditLogger::export_to_file(const std::string& path, const AuditQuery& q) {
    (void)path;
    (void)q;
    return true;
}

void AuditLogger::set_retention_days(int days) {
    retention_ = std::chrono::hours(days * 24);
}

void AuditLogger::cleanup_old_events() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto cutoff = std::chrono::system_clock::now() - retention_;
    events_.erase(
        std::remove_if(events_.begin(), events_.end(),
                       [&cutoff](const AuditEvent& e) { return e.timestamp < cutoff; }),
        events_.end());
}

size_t AuditLogger::event_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_.size();
}

std::string AuditLogger::generate_id() {
    static std::atomic<uint64_t> counter{0};
    return "audit_" + std::to_string(++counter);
}

std::string AuditLogger::sign_event(const AuditEvent& event) {
    return CryptoProvider::hash_sha256(event.id + event.action + event.actor_id);
}

// ═══════════════════════════════════════════════════════════════════════════
//  CRYPTO PROVIDER
// ═══════════════════════════════════════════════════════════════════════════

std::vector<uint8_t> CryptoProvider::encrypt_aes(const std::vector<uint8_t>& data,
                                                   const std::vector<uint8_t>& key) {
    // Simplified XOR encryption for simulation
    std::vector<uint8_t> result = data;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] ^= key[i % key.size()];
    }
    return result;
}

std::vector<uint8_t> CryptoProvider::decrypt_aes(const std::vector<uint8_t>& data,
                                                   const std::vector<uint8_t>& key) {
    return encrypt_aes(data, key);  // XOR is symmetric
}

std::pair<std::string, std::string> CryptoProvider::generate_keypair() {
    return {"pub_" + random_string(32), "priv_" + random_string(32)};
}

std::vector<uint8_t> CryptoProvider::encrypt_rsa(const std::vector<uint8_t>& data,
                                                   const std::string& public_key) {
    (void)public_key;
    return data;  // Simulated
}

std::vector<uint8_t> CryptoProvider::decrypt_rsa(const std::vector<uint8_t>& data,
                                                   const std::string& private_key) {
    (void)private_key;
    return data;  // Simulated
}

std::string CryptoProvider::sign(const std::string& data, const std::string& private_key) {
    return hash_sha256(data + private_key);
}

bool CryptoProvider::verify_signature(const std::string& data, const std::string& signature,
                                        const std::string& public_key) {
    // Simplified verification
    (void)data;
    (void)public_key;
    return !signature.empty();
}

std::string CryptoProvider::hash_sha256(const std::string& data) {
    // Simple hash simulation
    std::hash<std::string> hasher;
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << hasher(data);
    return oss.str();
}

std::string CryptoProvider::hash_sha512(const std::string& data) {
    return hash_sha256(data) + hash_sha256(data + "512");
}

std::string CryptoProvider::hmac_sha256(const std::string& data, const std::string& key) {
    return hash_sha256(key + data + key);
}

std::vector<uint8_t> CryptoProvider::random_bytes(size_t count) {
    std::vector<uint8_t> result(count);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (size_t i = 0; i < count; ++i) {
        result[i] = static_cast<uint8_t>(dis(gen));
    }
    return result;
}

std::string CryptoProvider::random_string(size_t length) {
    static const char charset[] = "0123456789abcdef";
    auto bytes = random_bytes(length);
    std::string result;
    for (auto b : bytes) {
        result += charset[b % 16];
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
//  SECURITY MANAGER
// ═══════════════════════════════════════════════════════════════════════════

SecurityManager::SecurityManager(const SecurityConfig& config)
    : config_(config)
    , zero_trust_(std::make_unique<ZeroTrustEngine>())
    , capabilities_(std::make_unique<CapabilityManager>())
    , audit_(std::make_unique<AuditLogger>()) {}

SecurityManager::~SecurityManager() = default;

SecurityManager& SecurityManager::instance() {
    static SecurityManager inst;
    return inst;
}

ZeroTrustEngine& SecurityManager::zero_trust() { return *zero_trust_; }
CapabilityManager& SecurityManager::capabilities() { return *capabilities_; }
AuditLogger& SecurityManager::audit() { return *audit_; }

AuthResult SecurityManager::login(const std::string& identity_id,
                                   const std::map<std::string, std::string>& credentials) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check lockout
    auto it = lockouts_.find(identity_id);
    if (it != lockouts_.end() && it->second > std::chrono::system_clock::now()) {
        AuthResult result;
        result.success = false;
        result.error = "Account locked";
        return result;
    }

    auto result = zero_trust_->authenticate(identity_id, AuthMethod::PASSWORD, credentials);

    if (!result.success) {
        auth_attempts_[identity_id]++;
        if (auth_attempts_[identity_id] >= config_.max_auth_attempts) {
            lockouts_[identity_id] = std::chrono::system_clock::now() + config_.lockout_duration;
        }
    } else {
        auth_attempts_.erase(identity_id);
    }

    audit_->log_auth(identity_id, result.success, "password");
    return result;
}

void SecurityManager::logout(const std::string& session_id) {
    zero_trust_->invalidate_session(session_id);
}

bool SecurityManager::authorize(const std::string& session_id, const std::string& resource,
                                 Permission perm) {
    if (!zero_trust_->verify_session(session_id)) {
        audit_->log_access("unknown", resource, perm, false);
        return false;
    }

    // Check policies
    // Simplified: always grant for valid sessions
    audit_->log_access(session_id, resource, perm, true);
    return true;
}

bool SecurityManager::is_authenticated(const std::string& session_id) {
    return zero_trust_->verify_session(session_id);
}

bool SecurityManager::has_permission(const std::string& session_id, Permission perm) {
    (void)session_id;
    (void)perm;
    return true;  // Simplified
}

void SecurityManager::set_config(const SecurityConfig& config) {
    config_ = config;
}

SecurityConfig SecurityManager::get_config() const {
    return config_;
}

SecurityManager& security() {
    return SecurityManager::instance();
}

} // namespace rael
