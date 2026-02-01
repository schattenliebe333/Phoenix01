// RAEL V49 - REST/GraphQL API Server (#22)
// HTTP API for external integration
#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <optional>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  HTTP TYPES
// ═══════════════════════════════════════════════════════════════════════════

enum class HttpMethod {
    GET,
    POST,
    PUT,
    PATCH,
    DELETE_,
    OPTIONS,
    HEAD
};

enum class HttpStatus {
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    CONFLICT = 409,
    TOO_MANY_REQUESTS = 429,
    INTERNAL_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503
};

struct HttpRequest {
    HttpMethod method = HttpMethod::GET;
    std::string path;
    std::string query_string;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> path_params;
    std::string body;
    std::string client_ip;
    std::string content_type;
    std::string authorization;

    std::optional<std::string> get_header(const std::string& name) const;
    std::optional<std::string> get_param(const std::string& name) const;
};

struct HttpResponse {
    HttpStatus status = HttpStatus::OK;
    std::map<std::string, std::string> headers;
    std::string body;
    std::string content_type = "application/json";

    static HttpResponse json(const std::string& json_body, HttpStatus status = HttpStatus::OK);
    static HttpResponse text(const std::string& text, HttpStatus status = HttpStatus::OK);
    static HttpResponse error(HttpStatus status, const std::string& message);
    static HttpResponse redirect(const std::string& url);
};

// ═══════════════════════════════════════════════════════════════════════════
//  ROUTING
// ═══════════════════════════════════════════════════════════════════════════

using RouteHandler = std::function<HttpResponse(const HttpRequest&)>;
using Middleware = std::function<HttpResponse(const HttpRequest&, RouteHandler)>;

struct Route {
    std::string pattern;
    HttpMethod method;
    RouteHandler handler;
    std::vector<std::string> param_names;
    std::string description;
};

class Router {
public:
    Router();

    // Route registration
    void get(const std::string& path, RouteHandler handler);
    void post(const std::string& path, RouteHandler handler);
    void put(const std::string& path, RouteHandler handler);
    void patch(const std::string& path, RouteHandler handler);
    void del(const std::string& path, RouteHandler handler);
    void route(HttpMethod method, const std::string& path, RouteHandler handler);

    // Group routes
    void group(const std::string& prefix, std::function<void(Router&)> setup);

    // Middleware
    void use(Middleware mw);

    // Route matching
    std::optional<Route> match(HttpMethod method, const std::string& path,
                               std::map<std::string, std::string>& params) const;

    // Handle request
    HttpResponse handle(const HttpRequest& request);

    // List routes
    std::vector<Route> routes() const;

private:
    std::vector<Route> routes_;
    std::vector<Middleware> middleware_;
    std::string prefix_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  AUTHENTICATION
// ═══════════════════════════════════════════════════════════════════════════

struct JWTClaims {
    std::string sub;       // Subject (user id)
    std::string iss;       // Issuer
    std::string aud;       // Audience
    int64_t exp = 0;       // Expiration
    int64_t iat = 0;       // Issued at
    int64_t nbf = 0;       // Not before
    std::string jti;       // JWT ID
    std::map<std::string, std::string> custom;
};

class JWTAuth {
public:
    JWTAuth(const std::string& secret, const std::string& issuer = "rael");

    // Token operations
    std::string generate(const JWTClaims& claims);
    std::optional<JWTClaims> verify(const std::string& token) const;
    bool is_expired(const JWTClaims& claims) const;

    // Middleware
    Middleware middleware();

    // Configuration
    void set_expiry(int seconds);
    void set_secret(const std::string& secret);

private:
    std::string secret_;
    std::string issuer_;
    int expiry_seconds_ = 3600;
};

class APIKeyAuth {
public:
    APIKeyAuth();

    // Key management
    std::string generate_key(const std::string& name);
    bool revoke_key(const std::string& key);
    bool is_valid(const std::string& key) const;
    std::optional<std::string> get_key_name(const std::string& key) const;

    // Middleware
    Middleware middleware(const std::string& header_name = "X-API-Key");

private:
    std::map<std::string, std::string> keys_;  // key -> name
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  RATE LIMITING
// ═══════════════════════════════════════════════════════════════════════════

class RateLimiter {
public:
    RateLimiter(int requests_per_minute = 60, int burst = 10);

    // Check if request is allowed
    bool allow(const std::string& client_id);

    // Get remaining requests
    int remaining(const std::string& client_id) const;

    // Reset time
    std::chrono::system_clock::time_point reset_time(const std::string& client_id) const;

    // Middleware
    Middleware middleware();

private:
    struct Bucket {
        int tokens;
        std::chrono::system_clock::time_point last_refill;
    };

    int rate_;
    int burst_;
    std::map<std::string, Bucket> buckets_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  CORS
// ═══════════════════════════════════════════════════════════════════════════

struct CORSConfig {
    std::vector<std::string> allowed_origins = {"*"};
    std::vector<std::string> allowed_methods = {"GET", "POST", "PUT", "DELETE", "OPTIONS"};
    std::vector<std::string> allowed_headers = {"Content-Type", "Authorization", "X-API-Key"};
    std::vector<std::string> exposed_headers;
    bool allow_credentials = false;
    int max_age = 86400;
};

class CORS {
public:
    CORS(const CORSConfig& config = {});

    Middleware middleware();
    void add_headers(HttpResponse& response, const HttpRequest& request);

private:
    CORSConfig config_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  WEBSOCKET
// ═══════════════════════════════════════════════════════════════════════════

struct WebSocketMessage {
    std::string data;
    bool is_binary = false;
};

class WebSocketConnection {
public:
    virtual ~WebSocketConnection() = default;

    virtual void send(const std::string& data) = 0;
    virtual void send_binary(const std::vector<uint8_t>& data) = 0;
    virtual void close(int code = 1000, const std::string& reason = "") = 0;
    virtual bool is_open() const = 0;
    virtual std::string id() const = 0;
};

class WebSocketHub {
public:
    WebSocketHub();

    // Connection management
    void add_connection(std::shared_ptr<WebSocketConnection> conn);
    void remove_connection(const std::string& id);
    std::shared_ptr<WebSocketConnection> get_connection(const std::string& id) const;
    std::vector<std::string> connection_ids() const;

    // Broadcasting
    void broadcast(const std::string& message);
    void broadcast_to(const std::vector<std::string>& ids, const std::string& message);

    // Rooms/channels
    void join_room(const std::string& conn_id, const std::string& room);
    void leave_room(const std::string& conn_id, const std::string& room);
    void broadcast_to_room(const std::string& room, const std::string& message);

    // Handlers
    using MessageHandler = std::function<void(const std::string& conn_id, const WebSocketMessage&)>;
    using ConnectionHandler = std::function<void(const std::string& conn_id, bool connected)>;

    void on_message(MessageHandler handler);
    void on_connection(ConnectionHandler handler);

    size_t connection_count() const;

private:
    std::map<std::string, std::shared_ptr<WebSocketConnection>> connections_;
    std::map<std::string, std::set<std::string>> rooms_;  // room -> connection ids
    std::map<std::string, std::set<std::string>> conn_rooms_;  // conn -> rooms
    MessageHandler on_message_;
    ConnectionHandler on_connection_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  GRAPHQL
// ═══════════════════════════════════════════════════════════════════════════

struct GraphQLRequest {
    std::string query;
    std::string operation_name;
    std::map<std::string, std::string> variables;
};

struct GraphQLResponse {
    std::string data;
    std::vector<std::string> errors;
};

using GraphQLResolver = std::function<std::string(
    const std::string& field,
    const std::map<std::string, std::string>& args)>;

class GraphQLSchema {
public:
    GraphQLSchema();

    // Type definitions
    void add_type(const std::string& name, const std::string& definition);
    void add_query(const std::string& name, const std::string& return_type,
                   GraphQLResolver resolver);
    void add_mutation(const std::string& name, const std::string& return_type,
                     GraphQLResolver resolver);
    void add_subscription(const std::string& name, const std::string& return_type);

    // Schema generation
    std::string generate_sdl() const;

    // Execution
    GraphQLResponse execute(const GraphQLRequest& request);

private:
    struct Field {
        std::string name;
        std::string return_type;
        std::vector<std::pair<std::string, std::string>> args;
        GraphQLResolver resolver;
    };

    std::map<std::string, std::string> types_;
    std::vector<Field> queries_;
    std::vector<Field> mutations_;
    std::vector<Field> subscriptions_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  OPENAPI / SWAGGER
// ═══════════════════════════════════════════════════════════════════════════

struct OpenAPIInfo {
    std::string title = "RAEL API";
    std::string version = "1.0.0";
    std::string description;
    std::string terms_of_service;
    std::string contact_name;
    std::string contact_email;
    std::string license_name;
    std::string license_url;
};

class OpenAPIGenerator {
public:
    OpenAPIGenerator(const OpenAPIInfo& info = {});

    // From router
    std::string generate(const Router& router);

    // Manual endpoint documentation
    void document_endpoint(const std::string& path, HttpMethod method,
                          const std::string& summary,
                          const std::string& description);

    void add_tag(const std::string& name, const std::string& description);
    void add_security_scheme(const std::string& name, const std::string& type);

    std::string get_spec() const;

private:
    OpenAPIInfo info_;
    std::string spec_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  API SERVER
// ═══════════════════════════════════════════════════════════════════════════

struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    int max_connections = 1000;
    int worker_threads = 4;
    std::chrono::seconds request_timeout{30};
    size_t max_body_size = 10 * 1024 * 1024;  // 10MB
    bool enable_cors = true;
    bool enable_swagger = true;
    std::string swagger_path = "/docs";
    bool enable_websocket = true;
    std::string websocket_path = "/ws";
};

class APIServer {
public:
    APIServer(const ServerConfig& config = {});
    ~APIServer();

    // Singleton
    static APIServer& instance();

    // Lifecycle
    bool start();
    void stop();
    bool is_running() const;

    // Router access
    Router& router();

    // Authentication
    void set_jwt_auth(std::shared_ptr<JWTAuth> auth);
    void set_api_key_auth(std::shared_ptr<APIKeyAuth> auth);
    JWTAuth* jwt_auth();
    APIKeyAuth* api_key_auth();

    // Rate limiting
    void set_rate_limiter(std::shared_ptr<RateLimiter> limiter);
    RateLimiter* rate_limiter();

    // CORS
    void set_cors(const CORSConfig& config);

    // WebSocket
    WebSocketHub& websocket_hub();

    // GraphQL
    GraphQLSchema& graphql_schema();
    void enable_graphql(const std::string& path = "/graphql");

    // OpenAPI
    void set_openapi_info(const OpenAPIInfo& info);
    std::string openapi_spec() const;

    // Stats
    struct Stats {
        uint64_t total_requests;
        uint64_t successful_requests;
        uint64_t failed_requests;
        uint64_t active_connections;
        uint64_t websocket_connections;
        double avg_response_time_ms;
        std::chrono::system_clock::time_point started;
    };
    Stats get_stats() const;

    // Built-in routes
    void register_health_endpoint(const std::string& path = "/health");
    void register_metrics_endpoint(const std::string& path = "/metrics");

private:
    void accept_loop();
    void handle_connection(int client_fd);
    HttpRequest parse_request(const std::string& raw);
    std::string serialize_response(const HttpResponse& response);

    ServerConfig config_;
    std::unique_ptr<Router> router_;
    std::shared_ptr<JWTAuth> jwt_auth_;
    std::shared_ptr<APIKeyAuth> api_key_auth_;
    std::shared_ptr<RateLimiter> rate_limiter_;
    std::unique_ptr<CORS> cors_;
    std::unique_ptr<WebSocketHub> ws_hub_;
    std::unique_ptr<GraphQLSchema> graphql_schema_;
    std::unique_ptr<OpenAPIGenerator> openapi_;

    std::atomic<bool> running_{false};
    std::thread accept_thread_;
    std::vector<std::thread> worker_threads_;
    int server_fd_ = -1;

    mutable std::mutex mutex_;
    Stats stats_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  JSON HELPERS
// ═══════════════════════════════════════════════════════════════════════════

class JSON {
public:
    // Simple JSON builder
    static std::string object(const std::map<std::string, std::string>& fields);
    static std::string array(const std::vector<std::string>& items);
    static std::string string(const std::string& s);
    static std::string number(double n);
    static std::string boolean(bool b);
    static std::string null();

    // Escape string for JSON
    static std::string escape(const std::string& s);

    // Parse simple values
    static std::optional<std::string> get_string(const std::string& json, const std::string& key);
    static std::optional<double> get_number(const std::string& json, const std::string& key);
    static std::optional<bool> get_bool(const std::string& json, const std::string& key);
};

// ═══════════════════════════════════════════════════════════════════════════
//  GLOBAL ACCESS
// ═══════════════════════════════════════════════════════════════════════════

APIServer& api();

} // namespace rael
