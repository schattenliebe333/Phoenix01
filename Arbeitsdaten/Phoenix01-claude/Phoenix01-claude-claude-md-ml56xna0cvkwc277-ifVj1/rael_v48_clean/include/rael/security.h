// RAEL V49 - Zero-Trust Security (#30) & Capability-Based Access (#31) & Audit Trail (#32)
// Comprehensive security framework
#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <optional>
#include <mutex>
#include <chrono>
#include <variant>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  CAPABILITY TOKENS (#31)
// ═══════════════════════════════════════════════════════════════════════════

enum class Permission {
    READ,
    WRITE,
    EXECUTE,
    DELETE,
    ADMIN,
    NETWORK,
    FILESYSTEM,
    PROCESS,
    MEMORY,
    SYSCALL
};

struct Capability {
    std::string id;
    std::string resource;
    std::set<Permission> permissions;
    std::chrono::system_clock::time_point issued;
    std::chrono::system_clock::time_point expires;
    std::string issuer;
    std::string subject;
    std::map<std::string, std::string> constraints;
    std::string signature;

    bool is_valid() const;
    bool has_permission(Permission p) const;
    bool allows_resource(const std::string& res) const;
    std::string to_token() const;
    static Capability from_token(const std::string& token);
};

class CapabilityManager {
public:
    CapabilityManager();

    // Issue capabilities
    Capability issue(const std::string& subject, const std::string& resource,
                     const std::set<Permission>& perms,
                     std::chrono::seconds ttl = std::chrono::hours(1));
    Capability delegate(const Capability& parent, const std::string& new_subject,
                        const std::set<Permission>& subset);

    // Verify
    bool verify(const Capability& cap);
    bool check_access(const Capability& cap, const std::string& resource, Permission perm);

    // Revoke
    void revoke(const std::string& capability_id);
    void revoke_all(const std::string& subject);
    bool is_revoked(const std::string& capability_id) const;

    // List
    std::vector<Capability> list_capabilities(const std::string& subject);

private:
    std::string signing_key_;
    std::set<std::string> revoked_;
    std::map<std::string, std::vector<Capability>> by_subject_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  ZERO-TRUST FRAMEWORK (#30)
// ═══════════════════════════════════════════════════════════════════════════

struct Identity {
    std::string id;
    std::string type;  // user, service, device
    std::string name;
    std::map<std::string, std::string> attributes;
    std::vector<std::string> groups;
    int trust_level = 0;  // 0-100
    std::chrono::system_clock::time_point last_verified;
};

struct AuthContext {
    Identity identity;
    std::string session_id;
    std::string source_ip;
    std::string device_id;
    std::map<std::string, std::string> metadata;
    std::chrono::system_clock::time_point created;
    int risk_score = 0;
};

enum class AuthMethod {
    PASSWORD,
    TOKEN,
    CERTIFICATE,
    MFA,
    BIOMETRIC,
    HARDWARE_KEY
};

struct AuthResult {
    bool success;
    std::string session_id;
    std::string error;
    int risk_score;
    std::vector<std::string> required_factors;
};

class ZeroTrustEngine {
public:
    ZeroTrustEngine();

    // Authentication
    AuthResult authenticate(const std::string& identity_id,
                            AuthMethod method,
                            const std::map<std::string, std::string>& credentials);
    bool verify_session(const std::string& session_id);
    void invalidate_session(const std::string& session_id);

    // Continuous verification
    bool continuous_verify(const AuthContext& ctx);
    int calculate_risk_score(const AuthContext& ctx);

    // Policy
    struct Policy {
        std::string name;
        std::function<bool(const AuthContext&)> condition;
        std::set<Permission> grants;
        int min_trust_level = 0;
    };
    void add_policy(const Policy& policy);
    void remove_policy(const std::string& name);
    std::vector<Permission> evaluate_policies(const AuthContext& ctx);

    // Identity management
    void register_identity(const Identity& id);
    void update_trust_level(const std::string& identity_id, int delta);
    std::optional<Identity> get_identity(const std::string& id);

    // MFA
    std::string generate_mfa_challenge(const std::string& identity_id);
    bool verify_mfa(const std::string& identity_id, const std::string& code);

private:
    std::map<std::string, Identity> identities_;
    std::map<std::string, AuthContext> sessions_;
    std::vector<Policy> policies_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  AUDIT TRAIL (#32)
// ═══════════════════════════════════════════════════════════════════════════

enum class AuditEventType {
    AUTH_SUCCESS,
    AUTH_FAILURE,
    ACCESS_GRANTED,
    ACCESS_DENIED,
    RESOURCE_CREATE,
    RESOURCE_READ,
    RESOURCE_UPDATE,
    RESOURCE_DELETE,
    POLICY_CHANGE,
    CONFIG_CHANGE,
    SECURITY_ALERT,
    SYSTEM_EVENT
};

using AuditData = std::variant<std::string, int64_t, double, bool,
                                std::vector<std::string>, std::map<std::string, std::string>>;

struct AuditEvent {
    std::string id;
    AuditEventType type;
    std::chrono::system_clock::time_point timestamp;
    std::string actor_id;
    std::string actor_type;
    std::string action;
    std::string resource;
    std::string resource_type;
    std::map<std::string, AuditData> details;
    std::string outcome;
    std::string source_ip;
    std::string session_id;
    std::string correlation_id;
    int severity = 0;  // 0-10
    std::string signature;  // Tamper-proof
};

struct AuditQuery {
    std::optional<AuditEventType> type;
    std::optional<std::string> actor_id;
    std::optional<std::string> resource;
    std::optional<std::chrono::system_clock::time_point> start_time;
    std::optional<std::chrono::system_clock::time_point> end_time;
    std::optional<int> min_severity;
    int limit = 100;
    int offset = 0;
};

class AuditLogger {
public:
    AuditLogger();

    // Logging
    void log(const AuditEvent& event);
    void log(AuditEventType type, const std::string& actor, const std::string& action,
             const std::string& resource, const std::map<std::string, AuditData>& details = {});

    // Convenience methods
    void log_auth(const std::string& actor, bool success, const std::string& method);
    void log_access(const std::string& actor, const std::string& resource,
                    Permission perm, bool granted);
    void log_change(const std::string& actor, const std::string& resource,
                    const std::string& old_value, const std::string& new_value);
    void log_alert(const std::string& message, int severity);

    // Query
    std::vector<AuditEvent> query(const AuditQuery& q);
    std::vector<AuditEvent> get_recent(int count = 100);
    std::vector<AuditEvent> get_by_actor(const std::string& actor_id, int limit = 100);
    std::vector<AuditEvent> get_by_resource(const std::string& resource, int limit = 100);

    // Integrity
    bool verify_integrity();
    std::string compute_chain_hash();

    // Export
    std::string export_json(const std::vector<AuditEvent>& events);
    std::string export_csv(const std::vector<AuditEvent>& events);
    bool export_to_file(const std::string& path, const AuditQuery& q);

    // Retention (using hours for C++17 compatibility)
    void set_retention_days(int days);
    void cleanup_old_events();
    size_t event_count() const;

private:
    std::string generate_id();
    std::string sign_event(const AuditEvent& event);

    std::vector<AuditEvent> events_;
    std::string chain_hash_;
    std::chrono::hours retention_{365 * 24}; // 365 days in hours
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  ENCRYPTION & CRYPTO
// ═══════════════════════════════════════════════════════════════════════════

class CryptoProvider {
public:
    // Symmetric encryption
    static std::vector<uint8_t> encrypt_aes(const std::vector<uint8_t>& data,
                                             const std::vector<uint8_t>& key);
    static std::vector<uint8_t> decrypt_aes(const std::vector<uint8_t>& data,
                                             const std::vector<uint8_t>& key);

    // Asymmetric encryption
    static std::pair<std::string, std::string> generate_keypair();
    static std::vector<uint8_t> encrypt_rsa(const std::vector<uint8_t>& data,
                                             const std::string& public_key);
    static std::vector<uint8_t> decrypt_rsa(const std::vector<uint8_t>& data,
                                             const std::string& private_key);

    // Signing
    static std::string sign(const std::string& data, const std::string& private_key);
    static bool verify_signature(const std::string& data, const std::string& signature,
                                  const std::string& public_key);

    // Hashing
    static std::string hash_sha256(const std::string& data);
    static std::string hash_sha512(const std::string& data);
    static std::string hmac_sha256(const std::string& data, const std::string& key);

    // Random
    static std::vector<uint8_t> random_bytes(size_t count);
    static std::string random_string(size_t length);
};

// ═══════════════════════════════════════════════════════════════════════════
//  SECURITY MANAGER
// ═══════════════════════════════════════════════════════════════════════════

struct SecurityConfig {
    bool enable_zero_trust = true;
    bool enable_audit = true;
    bool require_mfa = false;
    int min_trust_level = 50;
    std::chrono::seconds session_timeout{3600};
    int max_auth_attempts = 5;
    std::chrono::seconds lockout_duration{300};
};

class SecurityManager {
public:
    explicit SecurityManager(const SecurityConfig& config = {});
    ~SecurityManager();

    static SecurityManager& instance();

    // Components
    ZeroTrustEngine& zero_trust();
    CapabilityManager& capabilities();
    AuditLogger& audit();

    // High-level API
    AuthResult login(const std::string& identity_id,
                     const std::map<std::string, std::string>& credentials);
    void logout(const std::string& session_id);
    bool authorize(const std::string& session_id, const std::string& resource, Permission perm);

    // Quick checks
    bool is_authenticated(const std::string& session_id);
    bool has_permission(const std::string& session_id, Permission perm);

    // Configuration
    void set_config(const SecurityConfig& config);
    SecurityConfig get_config() const;

private:
    SecurityConfig config_;
    std::unique_ptr<ZeroTrustEngine> zero_trust_;
    std::unique_ptr<CapabilityManager> capabilities_;
    std::unique_ptr<AuditLogger> audit_;
    std::map<std::string, int> auth_attempts_;
    std::map<std::string, std::chrono::system_clock::time_point> lockouts_;
    mutable std::mutex mutex_;
};

SecurityManager& security();

} // namespace rael
