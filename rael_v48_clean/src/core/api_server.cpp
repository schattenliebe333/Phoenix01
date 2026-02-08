// RAEL V49 - REST/GraphQL API Server Implementation
#include "rael/api_server.h"
#include "rael/sha256.h"    // → PhiHash backend (V49)
#include "rael/rst_crypto.hpp" // PhiMAC direkt
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <regex>
#include <random>
#include <set>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  HTTP REQUEST/RESPONSE
// ═══════════════════════════════════════════════════════════════════════════

std::optional<std::string> HttpRequest::get_header(const std::string& name) const {
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    for (const auto& [key, value] : headers) {
        std::string lower_key = key;
        std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
        if (lower_key == lower_name) {
            return value;
        }
    }
    return std::nullopt;
}

std::optional<std::string> HttpRequest::get_param(const std::string& name) const {
    auto it = query_params.find(name);
    if (it != query_params.end()) return it->second;

    it = path_params.find(name);
    if (it != path_params.end()) return it->second;

    return std::nullopt;
}

HttpResponse HttpResponse::json(const std::string& json_body, HttpStatus status) {
    HttpResponse resp;
    resp.status = status;
    resp.content_type = "application/json";
    resp.body = json_body;
    return resp;
}

HttpResponse HttpResponse::text(const std::string& text, HttpStatus status) {
    HttpResponse resp;
    resp.status = status;
    resp.content_type = "text/plain";
    resp.body = text;
    return resp;
}

HttpResponse HttpResponse::error(HttpStatus status, const std::string& message) {
    std::ostringstream oss;
    oss << "{\"error\":\"" << JSON::escape(message)
        << "\",\"status\":" << static_cast<int>(status) << "}";
    return json(oss.str(), status);
}

HttpResponse HttpResponse::redirect(const std::string& url) {
    HttpResponse resp;
    resp.status = HttpStatus::OK;
    resp.headers["Location"] = url;
    resp.status = static_cast<HttpStatus>(302);
    return resp;
}

// ═══════════════════════════════════════════════════════════════════════════
//  ROUTER
// ═══════════════════════════════════════════════════════════════════════════

Router::Router() {}

void Router::get(const std::string& path, RouteHandler handler) {
    route(HttpMethod::GET, path, handler);
}

void Router::post(const std::string& path, RouteHandler handler) {
    route(HttpMethod::POST, path, handler);
}

void Router::put(const std::string& path, RouteHandler handler) {
    route(HttpMethod::PUT, path, handler);
}

void Router::patch(const std::string& path, RouteHandler handler) {
    route(HttpMethod::PATCH, path, handler);
}

void Router::del(const std::string& path, RouteHandler handler) {
    route(HttpMethod::DELETE_, path, handler);
}

void Router::route(HttpMethod method, const std::string& path, RouteHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);

    Route r;
    r.pattern = prefix_ + path;
    r.method = method;
    r.handler = handler;

    // Extract parameter names from path (e.g., /users/:id)
    std::regex param_regex(":([a-zA-Z_][a-zA-Z0-9_]*)");
    std::string pattern = r.pattern;
    std::smatch match;

    while (std::regex_search(pattern, match, param_regex)) {
        r.param_names.push_back(match[1].str());
        pattern = match.suffix().str();
    }

    routes_.push_back(r);
}

void Router::group(const std::string& prefix, std::function<void(Router&)> setup) {
    std::string old_prefix = prefix_;
    prefix_ = prefix_ + prefix;
    setup(*this);
    prefix_ = old_prefix;
}

void Router::use(Middleware mw) {
    std::lock_guard<std::mutex> lock(mutex_);
    middleware_.push_back(mw);
}

std::optional<Route> Router::match(HttpMethod method, const std::string& path,
                                   std::map<std::string, std::string>& params) const {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& route : routes_) {
        if (route.method != method) continue;

        // Convert route pattern to regex
        std::string regex_pattern = route.pattern;

        // Escape special regex chars except :param
        std::regex special_chars(R"([\.\+\*\?\^\$\(\)\[\]\{\}\|\\])");
        regex_pattern = std::regex_replace(regex_pattern, special_chars, "\\$&");

        // Replace :param with capturing group
        std::regex param_pattern(":([a-zA-Z_][a-zA-Z0-9_]*)");
        regex_pattern = std::regex_replace(regex_pattern, param_pattern, "([^/]+)");
        regex_pattern = "^" + regex_pattern + "$";

        std::regex route_regex(regex_pattern);
        std::smatch match;

        if (std::regex_match(path, match, route_regex)) {
            params.clear();
            for (size_t i = 0; i < route.param_names.size(); i++) {
                params[route.param_names[i]] = match[i + 1].str();
            }
            return route;
        }
    }

    return std::nullopt;
}

HttpResponse Router::handle(const HttpRequest& request) {
    std::map<std::string, std::string> params;
    auto route = match(request.method, request.path, params);

    if (!route) {
        return HttpResponse::error(HttpStatus::NOT_FOUND, "Route not found");
    }

    // Build request with params
    HttpRequest req = request;
    req.path_params = params;

    // Build handler chain with middleware
    RouteHandler final_handler = route->handler;

    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = middleware_.rbegin(); it != middleware_.rend(); ++it) {
        auto mw = *it;
        auto next = final_handler;
        final_handler = [mw, next](const HttpRequest& r) {
            return mw(r, next);
        };
    }

    try {
        return final_handler(req);
    } catch (const std::exception& e) {
        return HttpResponse::error(HttpStatus::INTERNAL_ERROR, e.what());
    }
}

std::vector<Route> Router::routes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return routes_;
}

// ═══════════════════════════════════════════════════════════════════════════
//  JWT AUTH
// ═══════════════════════════════════════════════════════════════════════════

JWTAuth::JWTAuth(const std::string& secret, const std::string& issuer)
    : secret_(secret), issuer_(issuer) {}

static std::string base64_encode(const std::string& input) {
    static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int i = 0;
    unsigned char arr3[3];
    unsigned char arr4[4];

    for (size_t n = 0; n < input.size(); ) {
        arr3[i++] = input[n++];
        if (i == 3) {
            arr4[0] = (arr3[0] & 0xfc) >> 2;
            arr4[1] = ((arr3[0] & 0x03) << 4) + ((arr3[1] & 0xf0) >> 4);
            arr4[2] = ((arr3[1] & 0x0f) << 2) + ((arr3[2] & 0xc0) >> 6);
            arr4[3] = arr3[2] & 0x3f;
            for (int j = 0; j < 4; j++) result += chars[arr4[j]];
            i = 0;
        }
    }
    if (i) {
        for (int j = i; j < 3; j++) arr3[j] = 0;
        arr4[0] = (arr3[0] & 0xfc) >> 2;
        arr4[1] = ((arr3[0] & 0x03) << 4) + ((arr3[1] & 0xf0) >> 4);
        arr4[2] = ((arr3[1] & 0x0f) << 2) + ((arr3[2] & 0xc0) >> 6);
        for (int j = 0; j < i + 1; j++) result += chars[arr4[j]];
        while (i++ < 3) result += '=';
    }
    return result;
}

std::string JWTAuth::generate(const JWTClaims& claims) {
    // Header
    std::string header = R"({"alg":"HS256","typ":"JWT"})";

    // Payload
    std::ostringstream payload;
    payload << "{";
    payload << "\"sub\":\"" << claims.sub << "\"";
    payload << ",\"iss\":\"" << issuer_ << "\"";

    auto now = std::chrono::system_clock::now();
    auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    payload << ",\"iat\":" << now_sec;
    payload << ",\"exp\":" << (now_sec + expiry_seconds_);

    if (!claims.aud.empty()) payload << ",\"aud\":\"" << claims.aud << "\"";
    if (!claims.jti.empty()) payload << ",\"jti\":\"" << claims.jti << "\"";

    for (const auto& [key, value] : claims.custom) {
        payload << ",\"" << key << "\":\"" << value << "\"";
    }
    payload << "}";

    std::string header_b64 = base64_encode(header);
    std::string payload_b64 = base64_encode(payload.str());

    // V49: PhiMAC Signatur (ersetzt HMAC-SHA256)
    std::string sign_input = header_b64 + "." + payload_b64;
    auto hmac = crypto::PhiMAC::compute(
        reinterpret_cast<const uint8_t*>(secret_.data()), secret_.size(),
        reinterpret_cast<const uint8_t*>(sign_input.data()), sign_input.size());

    // Base64URL encode signature (JWT requires URL-safe base64)
    std::string signature(hmac.begin(), hmac.end());
    std::string sig_b64 = base64_encode(signature);
    // Convert to URL-safe base64
    for (char& c : sig_b64) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    // Remove padding
    while (!sig_b64.empty() && sig_b64.back() == '=') sig_b64.pop_back();

    return header_b64 + "." + payload_b64 + "." + sig_b64;
}

static std::string base64_decode(const std::string& encoded) {
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // Convert URL-safe base64 to standard
    std::string input = encoded;
    for (char& c : input) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    // Add padding if needed
    while (input.size() % 4 != 0) input += '=';

    std::string decoded;
    int val = 0, bits = -8;
    for (unsigned char c : input) {
        if (c == '=') break;
        size_t pos = chars.find(c);
        if (pos == std::string::npos) continue;
        val = (val << 6) + static_cast<int>(pos);
        bits += 6;
        if (bits >= 0) {
            decoded += static_cast<char>((val >> bits) & 0xFF);
            bits -= 8;
        }
    }
    return decoded;
}

std::optional<JWTClaims> JWTAuth::verify(const std::string& token) const {
    // Split token
    std::vector<std::string> parts;
    std::istringstream iss(token);
    std::string part;
    while (std::getline(iss, part, '.')) {
        parts.push_back(part);
    }

    if (parts.size() != 3) return std::nullopt;

    // V49: PhiMAC Verifikation (ersetzt HMAC-SHA256)
    std::string sign_input = parts[0] + "." + parts[1];
    auto expected_hmac = crypto::PhiMAC::compute(
        reinterpret_cast<const uint8_t*>(secret_.data()), secret_.size(),
        reinterpret_cast<const uint8_t*>(sign_input.data()), sign_input.size());

    // Decode provided signature
    std::string provided_sig = base64_decode(parts[2]);
    std::vector<uint8_t> provided_hmac(provided_sig.begin(), provided_sig.end());

    // Constant-time comparison to prevent timing attacks
    if (provided_hmac.size() != expected_hmac.size()) return std::nullopt;
    int diff = 0;
    for (size_t i = 0; i < expected_hmac.size(); i++) {
        diff |= expected_hmac[i] ^ provided_hmac[i];
    }
    if (diff != 0) return std::nullopt;

    // Parse payload claims
    std::string payload = base64_decode(parts[1]);
    JWTClaims claims;

    // Parse sub
    auto sub = JSON::get_string(payload, "sub");
    if (sub) claims.sub = *sub;

    // Parse iss
    auto parsed_iss = JSON::get_string(payload, "iss");
    if (parsed_iss) claims.iss = *parsed_iss;

    // Verify issuer matches
    if (claims.iss != issuer_) return std::nullopt;

    // Parse exp
    auto exp = JSON::get_number(payload, "exp");
    if (exp) claims.exp = static_cast<int64_t>(*exp);

    // Parse iat
    auto iat = JSON::get_number(payload, "iat");
    if (iat) claims.iat = static_cast<int64_t>(*iat);

    // Parse aud
    auto aud = JSON::get_string(payload, "aud");
    if (aud) claims.aud = *aud;

    // Parse jti
    auto jti = JSON::get_string(payload, "jti");
    if (jti) claims.jti = *jti;

    return claims;
}

bool JWTAuth::is_expired(const JWTClaims& claims) const {
    auto now = std::chrono::system_clock::now();
    auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    return claims.exp > 0 && now_sec > claims.exp;
}

Middleware JWTAuth::middleware() {
    return [this](const HttpRequest& req, RouteHandler next) -> HttpResponse {
        auto auth = req.get_header("Authorization");
        if (!auth || auth->substr(0, 7) != "Bearer ") {
            return HttpResponse::error(HttpStatus::UNAUTHORIZED, "Missing or invalid token");
        }

        std::string token = auth->substr(7);
        auto claims = verify(token);
        if (!claims) {
            return HttpResponse::error(HttpStatus::UNAUTHORIZED, "Invalid token");
        }

        if (is_expired(*claims)) {
            return HttpResponse::error(HttpStatus::UNAUTHORIZED, "Token expired");
        }

        // SECURITY (F-05 audit fix): Check if token has been revoked
        if (!claims->jti.empty() && is_revoked(claims->jti)) {
            return HttpResponse::error(HttpStatus::UNAUTHORIZED, "Token has been revoked");
        }

        // SECURITY (F-05 audit fix): Replay protection - check if JTI already used
        if (replay_protection_enabled_ && !claims->jti.empty()) {
            std::lock_guard<std::mutex> lock(revocation_mutex_);
            if (used_jtis_.find(claims->jti) != used_jtis_.end()) {
                return HttpResponse::error(HttpStatus::UNAUTHORIZED, "Token replay detected");
            }
            // Prevent memory exhaustion
            if (used_jtis_.size() < MAX_USED_JTIS) {
                used_jtis_.insert(claims->jti);
            }
        }

        return next(req);
    };
}

void JWTAuth::set_expiry(int seconds) {
    expiry_seconds_ = seconds;
}

void JWTAuth::set_secret(const std::string& secret) {
    secret_ = secret;
}

// SECURITY (F-05 audit fix): Destructor to clear sensitive data
JWTAuth::~JWTAuth() {
    // Zero out the secret before deallocation
    std::fill(secret_.begin(), secret_.end(), '\0');
}

// SECURITY (F-05 audit fix): Token revocation
void JWTAuth::revoke_token(const std::string& jti) {
    std::lock_guard<std::mutex> lock(revocation_mutex_);
    auto now = std::chrono::system_clock::now();
    auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    // Keep the revocation for 2x expiry time to ensure coverage
    revoked_tokens_[jti] = now_sec + (2 * expiry_seconds_);
}

bool JWTAuth::is_revoked(const std::string& jti) const {
    std::lock_guard<std::mutex> lock(revocation_mutex_);
    return revoked_tokens_.find(jti) != revoked_tokens_.end();
}

void JWTAuth::cleanup_expired_revocations() {
    std::lock_guard<std::mutex> lock(revocation_mutex_);
    auto now = std::chrono::system_clock::now();
    auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    // Remove expired revocations
    for (auto it = revoked_tokens_.begin(); it != revoked_tokens_.end(); ) {
        if (it->second < now_sec) {
            it = revoked_tokens_.erase(it);
        } else {
            ++it;
        }
    }

    // Clear old used JTIs (they can't be replayed after token expiry anyway)
    used_jtis_.clear();
}

// ═══════════════════════════════════════════════════════════════════════════
//  API KEY AUTH
// ═══════════════════════════════════════════════════════════════════════════

APIKeyAuth::APIKeyAuth() {}

static std::string generate_api_key() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    std::ostringstream oss;
    oss << "rael_" << std::hex << dis(gen) << dis(gen);
    return oss.str();
}

std::string APIKeyAuth::generate_key(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = generate_api_key();
    keys_[key] = name;
    return key;
}

bool APIKeyAuth::revoke_key(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    return keys_.erase(key) > 0;
}

bool APIKeyAuth::is_valid(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return keys_.find(key) != keys_.end();
}

std::optional<std::string> APIKeyAuth::get_key_name(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = keys_.find(key);
    if (it != keys_.end()) return it->second;
    return std::nullopt;
}

Middleware APIKeyAuth::middleware(const std::string& header_name) {
    return [this, header_name](const HttpRequest& req, RouteHandler next) -> HttpResponse {
        auto key = req.get_header(header_name);
        if (!key || !is_valid(*key)) {
            return HttpResponse::error(HttpStatus::UNAUTHORIZED, "Invalid API key");
        }
        return next(req);
    };
}

// ═══════════════════════════════════════════════════════════════════════════
//  RATE LIMITER
// ═══════════════════════════════════════════════════════════════════════════

RateLimiter::RateLimiter(int requests_per_minute, int burst)
    : rate_(requests_per_minute), burst_(burst) {}

bool RateLimiter::allow(const std::string& client_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    auto& bucket = buckets_[client_id];

    // Refill tokens
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - bucket.last_refill).count();
    int refill = static_cast<int>(elapsed * rate_ / 60);

    if (refill > 0) {
        bucket.tokens = std::min(burst_, bucket.tokens + refill);
        bucket.last_refill = now;
    }

    // Check if request allowed
    if (bucket.tokens > 0) {
        bucket.tokens--;
        return true;
    }
    return false;
}

int RateLimiter::remaining(const std::string& client_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = buckets_.find(client_id);
    if (it != buckets_.end()) {
        return it->second.tokens;
    }
    return burst_;
}

std::chrono::system_clock::time_point RateLimiter::reset_time(
    const std::string& client_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = buckets_.find(client_id);
    if (it != buckets_.end()) {
        return it->second.last_refill + std::chrono::seconds(60);
    }
    return std::chrono::system_clock::now() + std::chrono::seconds(60);
}

Middleware RateLimiter::middleware() {
    return [this](const HttpRequest& req, RouteHandler next) -> HttpResponse {
        if (!allow(req.client_ip)) {
            auto resp = HttpResponse::error(HttpStatus::TOO_MANY_REQUESTS, "Rate limit exceeded");
            resp.headers["Retry-After"] = "60";
            return resp;
        }
        auto resp = next(req);
        resp.headers["X-RateLimit-Remaining"] = std::to_string(remaining(req.client_ip));
        return resp;
    };
}

// ═══════════════════════════════════════════════════════════════════════════
//  CORS
// ═══════════════════════════════════════════════════════════════════════════

CORS::CORS(const CORSConfig& config) : config_(config) {}

Middleware CORS::middleware() {
    return [this](const HttpRequest& req, RouteHandler next) -> HttpResponse {
        if (req.method == HttpMethod::OPTIONS) {
            HttpResponse resp;
            resp.status = HttpStatus::NO_CONTENT;
            add_headers(resp, req);
            return resp;
        }
        auto resp = next(req);
        add_headers(resp, req);
        return resp;
    };
}

void CORS::add_headers(HttpResponse& response, const HttpRequest& request) {
    auto origin = request.get_header("Origin");
    std::string allowed_origin = "*";

    if (origin && config_.allowed_origins[0] != "*") {
        for (const auto& o : config_.allowed_origins) {
            if (o == *origin) {
                allowed_origin = *origin;
                break;
            }
        }
    }

    response.headers["Access-Control-Allow-Origin"] = allowed_origin;

    std::ostringstream methods;
    for (size_t i = 0; i < config_.allowed_methods.size(); i++) {
        if (i > 0) methods << ", ";
        methods << config_.allowed_methods[i];
    }
    response.headers["Access-Control-Allow-Methods"] = methods.str();

    std::ostringstream headers_str;
    for (size_t i = 0; i < config_.allowed_headers.size(); i++) {
        if (i > 0) headers_str << ", ";
        headers_str << config_.allowed_headers[i];
    }
    response.headers["Access-Control-Allow-Headers"] = headers_str.str();

    if (config_.allow_credentials) {
        response.headers["Access-Control-Allow-Credentials"] = "true";
    }

    response.headers["Access-Control-Max-Age"] = std::to_string(config_.max_age);
}

// ═══════════════════════════════════════════════════════════════════════════
//  WEBSOCKET HUB
// ═══════════════════════════════════════════════════════════════════════════

WebSocketHub::WebSocketHub() {}

void WebSocketHub::add_connection(std::shared_ptr<WebSocketConnection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_[conn->id()] = conn;
    if (on_connection_) {
        on_connection_(conn->id(), true);
    }
}

void WebSocketHub::remove_connection(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Leave all rooms
    auto it = conn_rooms_.find(id);
    if (it != conn_rooms_.end()) {
        for (const auto& room : it->second) {
            rooms_[room].erase(id);
        }
        conn_rooms_.erase(it);
    }

    connections_.erase(id);
    if (on_connection_) {
        on_connection_(id, false);
    }
}

std::shared_ptr<WebSocketConnection> WebSocketHub::get_connection(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connections_.find(id);
    return it != connections_.end() ? it->second : nullptr;
}

std::vector<std::string> WebSocketHub::connection_ids() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids;
    for (const auto& [id, _] : connections_) {
        ids.push_back(id);
    }
    return ids;
}

void WebSocketHub::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [_, conn] : connections_) {
        if (conn->is_open()) {
            conn->send(message);
        }
    }
}

void WebSocketHub::broadcast_to(const std::vector<std::string>& ids, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& id : ids) {
        auto it = connections_.find(id);
        if (it != connections_.end() && it->second->is_open()) {
            it->second->send(message);
        }
    }
}

void WebSocketHub::join_room(const std::string& conn_id, const std::string& room) {
    std::lock_guard<std::mutex> lock(mutex_);
    rooms_[room].insert(conn_id);
    conn_rooms_[conn_id].insert(room);
}

void WebSocketHub::leave_room(const std::string& conn_id, const std::string& room) {
    std::lock_guard<std::mutex> lock(mutex_);
    rooms_[room].erase(conn_id);
    conn_rooms_[conn_id].erase(room);
}

void WebSocketHub::broadcast_to_room(const std::string& room, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rooms_.find(room);
    if (it != rooms_.end()) {
        for (const auto& conn_id : it->second) {
            auto conn_it = connections_.find(conn_id);
            if (conn_it != connections_.end() && conn_it->second->is_open()) {
                conn_it->second->send(message);
            }
        }
    }
}

void WebSocketHub::on_message(MessageHandler handler) {
    on_message_ = handler;
}

void WebSocketHub::on_connection(ConnectionHandler handler) {
    on_connection_ = handler;
}

size_t WebSocketHub::connection_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_.size();
}

// ═══════════════════════════════════════════════════════════════════════════
//  GRAPHQL SCHEMA
// ═══════════════════════════════════════════════════════════════════════════

GraphQLSchema::GraphQLSchema() {}

void GraphQLSchema::add_type(const std::string& name, const std::string& definition) {
    std::lock_guard<std::mutex> lock(mutex_);
    types_[name] = definition;
}

void GraphQLSchema::add_query(const std::string& name, const std::string& return_type,
                              GraphQLResolver resolver) {
    std::lock_guard<std::mutex> lock(mutex_);
    Field f;
    f.name = name;
    f.return_type = return_type;
    f.resolver = resolver;
    queries_.push_back(f);
}

void GraphQLSchema::add_mutation(const std::string& name, const std::string& return_type,
                                 GraphQLResolver resolver) {
    std::lock_guard<std::mutex> lock(mutex_);
    Field f;
    f.name = name;
    f.return_type = return_type;
    f.resolver = resolver;
    mutations_.push_back(f);
}

void GraphQLSchema::add_subscription(const std::string& name, const std::string& return_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    Field f;
    f.name = name;
    f.return_type = return_type;
    subscriptions_.push_back(f);
}

std::string GraphQLSchema::generate_sdl() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;

    for (const auto& [name, def] : types_) {
        oss << "type " << name << " " << def << "\n\n";
    }

    if (!queries_.empty()) {
        oss << "type Query {\n";
        for (const auto& q : queries_) {
            oss << "  " << q.name << ": " << q.return_type << "\n";
        }
        oss << "}\n\n";
    }

    if (!mutations_.empty()) {
        oss << "type Mutation {\n";
        for (const auto& m : mutations_) {
            oss << "  " << m.name << ": " << m.return_type << "\n";
        }
        oss << "}\n\n";
    }

    return oss.str();
}

GraphQLResponse GraphQLSchema::execute(const GraphQLRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    GraphQLResponse response;

    // Simple query parsing (would use proper parser in production)
    std::string query_name;
    std::regex query_regex(R"RE(\{\s*(\w+))RE");
    std::smatch match;
    if (std::regex_search(request.query, match, query_regex)) {
        query_name = match[1].str();
    }

    for (const auto& q : queries_) {
        if (q.name == query_name && q.resolver) {
            response.data = q.resolver(query_name, request.variables);
            return response;
        }
    }

    response.errors.push_back("Query not found: " + query_name);
    return response;
}

// ═══════════════════════════════════════════════════════════════════════════
//  OPENAPI GENERATOR
// ═══════════════════════════════════════════════════════════════════════════

OpenAPIGenerator::OpenAPIGenerator(const OpenAPIInfo& info) : info_(info) {}

std::string OpenAPIGenerator::generate(const Router& router) {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"openapi\": \"3.0.3\",\n";
    oss << "  \"info\": {\n";
    oss << "    \"title\": \"" << info_.title << "\",\n";
    oss << "    \"version\": \"" << info_.version << "\"\n";
    oss << "  },\n";
    oss << "  \"paths\": {\n";

    auto routes = router.routes();
    for (size_t i = 0; i < routes.size(); i++) {
        const auto& route = routes[i];
        std::string method;
        switch (route.method) {
            case HttpMethod::GET: method = "get"; break;
            case HttpMethod::POST: method = "post"; break;
            case HttpMethod::PUT: method = "put"; break;
            case HttpMethod::PATCH: method = "patch"; break;
            case HttpMethod::DELETE_: method = "delete"; break;
            default: method = "get"; break;
        }

        oss << "    \"" << route.pattern << "\": {\n";
        oss << "      \"" << method << "\": {\n";
        oss << "        \"summary\": \"" << route.description << "\",\n";
        oss << "        \"responses\": {\n";
        oss << "          \"200\": { \"description\": \"Success\" }\n";
        oss << "        }\n";
        oss << "      }\n";
        oss << "    }";
        if (i < routes.size() - 1) oss << ",";
        oss << "\n";
    }

    oss << "  }\n";
    oss << "}\n";

    spec_ = oss.str();
    return spec_;
}

void OpenAPIGenerator::document_endpoint(const std::string& path, HttpMethod method,
                                         const std::string& summary,
                                         const std::string& description) {
    // Would add to spec
}

void OpenAPIGenerator::add_tag(const std::string& name, const std::string& description) {
    // Would add tag
}

void OpenAPIGenerator::add_security_scheme(const std::string& name, const std::string& type) {
    // Would add security scheme
}

std::string OpenAPIGenerator::get_spec() const {
    return spec_;
}

// ═══════════════════════════════════════════════════════════════════════════
//  API SERVER
// ═══════════════════════════════════════════════════════════════════════════

APIServer::APIServer(const ServerConfig& config)
    : config_(config)
    , router_(std::make_unique<Router>())
    , cors_(std::make_unique<CORS>())
    , ws_hub_(std::make_unique<WebSocketHub>())
    , graphql_schema_(std::make_unique<GraphQLSchema>())
    , openapi_(std::make_unique<OpenAPIGenerator>()) {

    stats_ = {};
}

APIServer::~APIServer() {
    stop();
}

APIServer& APIServer::instance() {
    static APIServer inst;
    return inst;
}

bool APIServer::start() {
    if (running_) return false;

    // In production, would set up actual socket server
    running_ = true;
    stats_.started = std::chrono::system_clock::now();

    // Register default middleware
    if (config_.enable_cors) {
        router_->use(cors_->middleware());
    }

    if (rate_limiter_) {
        router_->use(rate_limiter_->middleware());
    }

    // accept_thread_ = std::thread(&APIServer::accept_loop, this);

    return true;
}

void APIServer::stop() {
    running_ = false;
    // Would close socket
}

bool APIServer::is_running() const {
    return running_;
}

Router& APIServer::router() {
    return *router_;
}

void APIServer::set_jwt_auth(std::shared_ptr<JWTAuth> auth) {
    jwt_auth_ = auth;
}

void APIServer::set_api_key_auth(std::shared_ptr<APIKeyAuth> auth) {
    api_key_auth_ = auth;
}

JWTAuth* APIServer::jwt_auth() {
    return jwt_auth_.get();
}

APIKeyAuth* APIServer::api_key_auth() {
    return api_key_auth_.get();
}

void APIServer::set_rate_limiter(std::shared_ptr<RateLimiter> limiter) {
    rate_limiter_ = limiter;
}

RateLimiter* APIServer::rate_limiter() {
    return rate_limiter_.get();
}

void APIServer::set_cors(const CORSConfig& config) {
    cors_ = std::make_unique<CORS>(config);
}

WebSocketHub& APIServer::websocket_hub() {
    return *ws_hub_;
}

GraphQLSchema& APIServer::graphql_schema() {
    return *graphql_schema_;
}

void APIServer::enable_graphql(const std::string& path) {
    router_->post(path, [this](const HttpRequest& req) -> HttpResponse {
        GraphQLRequest gql;
        gql.query = JSON::get_string(req.body, "query").value_or("");
        gql.operation_name = JSON::get_string(req.body, "operationName").value_or("");

        auto response = graphql_schema_->execute(gql);

        std::ostringstream oss;
        oss << "{\"data\":" << response.data;
        if (!response.errors.empty()) {
            oss << ",\"errors\":[";
            for (size_t i = 0; i < response.errors.size(); i++) {
                if (i > 0) oss << ",";
                oss << "\"" << JSON::escape(response.errors[i]) << "\"";
            }
            oss << "]";
        }
        oss << "}";

        return HttpResponse::json(oss.str());
    });
}

void APIServer::set_openapi_info(const OpenAPIInfo& info) {
    openapi_ = std::make_unique<OpenAPIGenerator>(info);
}

std::string APIServer::openapi_spec() const {
    return openapi_->generate(*router_);
}

APIServer::Stats APIServer::get_stats() const {
    Stats s = stats_;
    s.websocket_connections = ws_hub_->connection_count();
    return s;
}

void APIServer::register_health_endpoint(const std::string& path) {
    router_->get(path, [](const HttpRequest&) -> HttpResponse {
        return HttpResponse::json("{\"status\":\"healthy\"}");
    });
}

void APIServer::register_metrics_endpoint(const std::string& path) {
    router_->get(path, [this](const HttpRequest&) -> HttpResponse {
        auto stats = get_stats();
        std::ostringstream oss;
        oss << "{";
        oss << "\"total_requests\":" << stats.total_requests << ",";
        oss << "\"successful_requests\":" << stats.successful_requests << ",";
        oss << "\"failed_requests\":" << stats.failed_requests << ",";
        oss << "\"active_connections\":" << stats.active_connections << ",";
        oss << "\"websocket_connections\":" << stats.websocket_connections << ",";
        oss << "\"avg_response_time_ms\":" << stats.avg_response_time_ms;
        oss << "}";
        return HttpResponse::json(oss.str());
    });
}

// ═══════════════════════════════════════════════════════════════════════════
//  JSON HELPERS
// ═══════════════════════════════════════════════════════════════════════════

std::string JSON::object(const std::map<std::string, std::string>& fields) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : fields) {
        if (!first) oss << ",";
        oss << "\"" << escape(key) << "\":" << value;
        first = false;
    }
    oss << "}";
    return oss.str();
}

std::string JSON::array(const std::vector<std::string>& items) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < items.size(); i++) {
        if (i > 0) oss << ",";
        oss << items[i];
    }
    oss << "]";
    return oss.str();
}

std::string JSON::string(const std::string& s) {
    return "\"" + escape(s) + "\"";
}

std::string JSON::number(double n) {
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

std::string JSON::boolean(bool b) {
    return b ? "true" : "false";
}

std::string JSON::null() {
    return "null";
}

std::string JSON::escape(const std::string& s) {
    std::ostringstream oss;
    for (char c : s) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b"; break;
            case '\f': oss << "\\f"; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c; break;
        }
    }
    return oss.str();
}

std::optional<std::string> JSON::get_string(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"\\s*:\\s*\"([^\"]*)\"";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(json, match, re)) {
        return match[1].str();
    }
    return std::nullopt;
}

std::optional<double> JSON::get_number(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"\\s*:\\s*(-?[0-9.]+)";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(json, match, re)) {
        return std::stod(match[1].str());
    }
    return std::nullopt;
}

std::optional<bool> JSON::get_bool(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"\\s*:\\s*(true|false)";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(json, match, re)) {
        return match[1].str() == "true";
    }
    return std::nullopt;
}

// Global access
APIServer& api() {
    return APIServer::instance();
}

} // namespace rael
