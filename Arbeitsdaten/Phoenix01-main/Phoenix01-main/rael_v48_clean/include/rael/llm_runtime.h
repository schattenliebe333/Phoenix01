// RAEL V49 - Local LLM Runtime (#33)
// Offline LLM inference without cloud dependency
#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <optional>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  MODEL CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

enum class QuantizationType {
    F32,        // Full precision (largest)
    F16,        // Half precision
    Q8_0,       // 8-bit quantization
    Q5_1,       // 5-bit quantization
    Q5_0,
    Q4_1,       // 4-bit quantization
    Q4_0,       // 4-bit (smallest, fastest)
    Q3_K_M,     // K-quant variants
    Q4_K_M,
    Q5_K_M,
    Q6_K
};

struct ModelConfig {
    std::string model_path;
    std::string model_name;
    QuantizationType quant_type = QuantizationType::Q4_0;
    int context_length = 4096;
    int batch_size = 512;
    int threads = 4;
    int gpu_layers = 0;          // Layers to offload to GPU
    bool use_mmap = true;
    bool use_mlock = false;
    size_t max_memory_mb = 4096;

    // LoRA adapters
    std::vector<std::string> lora_paths;
    std::vector<float> lora_scales;
};

struct GenerationConfig {
    int max_tokens = 512;
    float temperature = 0.7f;
    float top_p = 0.9f;
    int top_k = 40;
    float repeat_penalty = 1.1f;
    int repeat_last_n = 64;
    float presence_penalty = 0.0f;
    float frequency_penalty = 0.0f;
    std::vector<std::string> stop_sequences;
    bool stream = true;
    uint32_t seed = 0;           // 0 = random
};

// ═══════════════════════════════════════════════════════════════════════════
//  TOKEN & EMBEDDING
// ═══════════════════════════════════════════════════════════════════════════

using Token = int32_t;
using TokenSequence = std::vector<Token>;
using Embedding = std::vector<float>;

struct TokenInfo {
    Token id;
    std::string text;
    float logprob;
    bool is_special;
};

struct GenerationResult {
    std::string text;
    TokenSequence tokens;
    std::vector<float> logprobs;
    int prompt_tokens;
    int completion_tokens;
    double generation_time_ms;
    bool stopped_by_eos;
    std::string stop_reason;
};

// ═══════════════════════════════════════════════════════════════════════════
//  CHAT MESSAGE FORMAT
// ═══════════════════════════════════════════════════════════════════════════

enum class ChatRole {
    SYSTEM,
    USER,
    ASSISTANT,
    FUNCTION
};

struct ChatMessage {
    ChatRole role;
    std::string content;
    std::string name;            // For function calls
    std::map<std::string, std::string> metadata;
};

using ChatHistory = std::vector<ChatMessage>;

// ═══════════════════════════════════════════════════════════════════════════
//  MODEL INFO
// ═══════════════════════════════════════════════════════════════════════════

struct ModelInfo {
    std::string name;
    std::string architecture;    // llama, mistral, phi, etc.
    std::string version;
    size_t parameters;           // In billions
    size_t vocab_size;
    int context_length;
    int embedding_dim;
    int num_layers;
    int num_heads;
    QuantizationType quant_type;
    size_t file_size_mb;
    std::map<std::string, std::string> metadata;
};

// ═══════════════════════════════════════════════════════════════════════════
//  CALLBACKS
// ═══════════════════════════════════════════════════════════════════════════

using StreamCallback = std::function<bool(const std::string& token)>;
using ProgressCallback = std::function<void(float progress, const std::string& status)>;

// ═══════════════════════════════════════════════════════════════════════════
//  LLM BACKEND (Abstract)
// ═══════════════════════════════════════════════════════════════════════════

class LLMBackend {
public:
    virtual ~LLMBackend() = default;

    // Model lifecycle
    virtual bool load_model(const ModelConfig& config) = 0;
    virtual void unload_model() = 0;
    virtual bool is_loaded() const = 0;
    virtual ModelInfo get_model_info() const = 0;

    // Tokenization
    virtual TokenSequence tokenize(const std::string& text) const = 0;
    virtual std::string detokenize(const TokenSequence& tokens) const = 0;
    virtual size_t vocab_size() const = 0;

    // Generation
    virtual GenerationResult generate(const std::string& prompt,
                                      const GenerationConfig& config,
                                      StreamCallback on_token = nullptr) = 0;

    // Embeddings
    virtual Embedding embed(const std::string& text) const = 0;
    virtual std::vector<Embedding> embed_batch(const std::vector<std::string>& texts) const = 0;

    // LoRA
    virtual bool load_lora(const std::string& path, float scale = 1.0f) = 0;
    virtual void unload_lora(const std::string& path) = 0;

    // Memory management
    virtual size_t memory_usage() const = 0;
    virtual void clear_kv_cache() = 0;
};

// ═══════════════════════════════════════════════════════════════════════════
//  GGML BACKEND (llama.cpp compatible)
// ═══════════════════════════════════════════════════════════════════════════

class GGMLBackend : public LLMBackend {
public:
    GGMLBackend();
    ~GGMLBackend() override;

    bool load_model(const ModelConfig& config) override;
    void unload_model() override;
    bool is_loaded() const override;
    ModelInfo get_model_info() const override;

    TokenSequence tokenize(const std::string& text) const override;
    std::string detokenize(const TokenSequence& tokens) const override;
    size_t vocab_size() const override;

    GenerationResult generate(const std::string& prompt,
                             const GenerationConfig& config,
                             StreamCallback on_token = nullptr) override;

    Embedding embed(const std::string& text) const override;
    std::vector<Embedding> embed_batch(const std::vector<std::string>& texts) const override;

    bool load_lora(const std::string& path, float scale = 1.0f) override;
    void unload_lora(const std::string& path) override;

    size_t memory_usage() const override;
    void clear_kv_cache() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  CHAT TEMPLATE
// ═══════════════════════════════════════════════════════════════════════════

class ChatTemplate {
public:
    virtual ~ChatTemplate() = default;
    virtual std::string format(const ChatHistory& history) const = 0;
    virtual std::string get_system_prefix() const = 0;
    virtual std::string get_user_prefix() const = 0;
    virtual std::string get_assistant_prefix() const = 0;
};

class ChatMLTemplate : public ChatTemplate {
public:
    std::string format(const ChatHistory& history) const override;
    std::string get_system_prefix() const override { return "<|im_start|>system\n"; }
    std::string get_user_prefix() const override { return "<|im_start|>user\n"; }
    std::string get_assistant_prefix() const override { return "<|im_start|>assistant\n"; }
};

class Llama2Template : public ChatTemplate {
public:
    std::string format(const ChatHistory& history) const override;
    std::string get_system_prefix() const override { return "[INST] <<SYS>>\n"; }
    std::string get_user_prefix() const override { return "[INST] "; }
    std::string get_assistant_prefix() const override { return " "; }
};

class MistralTemplate : public ChatTemplate {
public:
    std::string format(const ChatHistory& history) const override;
    std::string get_system_prefix() const override { return ""; }
    std::string get_user_prefix() const override { return "[INST] "; }
    std::string get_assistant_prefix() const override { return " "; }
};

class AlpacaTemplate : public ChatTemplate {
public:
    std::string format(const ChatHistory& history) const override;
    std::string get_system_prefix() const override { return "### Instruction:\n"; }
    std::string get_user_prefix() const override { return "### Input:\n"; }
    std::string get_assistant_prefix() const override { return "### Response:\n"; }
};

// ═══════════════════════════════════════════════════════════════════════════
//  MODEL ZOO
// ═══════════════════════════════════════════════════════════════════════════

struct ModelEntry {
    std::string id;
    std::string name;
    std::string description;
    std::string url;
    std::string sha256;
    size_t size_mb;
    std::string architecture;
    QuantizationType quant_type;
    std::vector<std::string> tags;
};

class ModelZoo {
public:
    ModelZoo();

    // Browse available models
    std::vector<ModelEntry> list_models() const;
    std::vector<ModelEntry> search(const std::string& query) const;
    std::vector<ModelEntry> by_architecture(const std::string& arch) const;
    std::vector<ModelEntry> by_size(size_t max_mb) const;
    std::optional<ModelEntry> get(const std::string& id) const;

    // Download management
    bool download(const std::string& id, ProgressCallback on_progress = nullptr);
    bool is_downloaded(const std::string& id) const;
    std::string get_local_path(const std::string& id) const;
    bool delete_model(const std::string& id);

    // Custom models
    void add_custom(const ModelEntry& entry);
    void refresh_catalog();

    // Storage
    void set_models_dir(const std::string& path);
    std::string get_models_dir() const;
    size_t total_storage_used() const;

private:
    std::string models_dir_;
    std::vector<ModelEntry> catalog_;
    mutable std::mutex mutex_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  INFERENCE QUEUE
// ═══════════════════════════════════════════════════════════════════════════

struct InferenceRequest {
    uint64_t id;
    std::string prompt;
    GenerationConfig config;
    StreamCallback on_token;
    std::function<void(const GenerationResult&)> on_complete;
    std::function<void(const std::string&)> on_error;
    int priority = 0;
};

class InferenceQueue {
public:
    InferenceQueue(size_t max_concurrent = 1);
    ~InferenceQueue();

    // Submit request
    uint64_t submit(const InferenceRequest& request);

    // Cancel request
    bool cancel(uint64_t id);

    // Queue status
    size_t pending_count() const;
    size_t active_count() const;
    bool is_processing(uint64_t id) const;

    // Control
    void start();
    void stop();
    void pause();
    void resume();

    // Bind backend
    void set_backend(std::shared_ptr<LLMBackend> backend);

private:
    void worker_loop();

    std::shared_ptr<LLMBackend> backend_;
    std::priority_queue<InferenceRequest, std::vector<InferenceRequest>,
                       std::function<bool(const InferenceRequest&, const InferenceRequest&)>> queue_;
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<uint64_t> next_id_{1};
    std::set<uint64_t> active_requests_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  LLM RUNTIME (Main Interface)
// ═══════════════════════════════════════════════════════════════════════════

class LLMRuntime {
public:
    LLMRuntime();
    ~LLMRuntime();

    // Singleton access
    static LLMRuntime& instance();

    // Model management
    bool load_model(const std::string& model_id_or_path, const ModelConfig& config = {});
    bool load_from_zoo(const std::string& model_id, ProgressCallback on_progress = nullptr);
    void unload_model();
    bool is_model_loaded() const;
    ModelInfo current_model() const;

    // Simple generation
    std::string generate(const std::string& prompt, const GenerationConfig& config = {});
    void generate_stream(const std::string& prompt, StreamCallback on_token,
                        const GenerationConfig& config = {});

    // Chat interface
    std::string chat(const ChatHistory& history, const GenerationConfig& config = {});
    void chat_stream(const ChatHistory& history, StreamCallback on_token,
                    const GenerationConfig& config = {});

    // Embeddings
    Embedding embed(const std::string& text);
    std::vector<Embedding> embed_batch(const std::vector<std::string>& texts);
    float similarity(const std::string& a, const std::string& b);

    // Async operations
    uint64_t generate_async(const std::string& prompt,
                           std::function<void(const GenerationResult&)> on_complete,
                           const GenerationConfig& config = {});
    bool cancel_generation(uint64_t request_id);

    // Chat template
    void set_chat_template(std::shared_ptr<ChatTemplate> tmpl);
    std::shared_ptr<ChatTemplate> get_chat_template() const;
    void auto_detect_template();

    // Model zoo
    ModelZoo& zoo();

    // Backend selection
    void set_backend(std::shared_ptr<LLMBackend> backend);
    std::shared_ptr<LLMBackend> get_backend() const;

    // Statistics
    struct Stats {
        uint64_t total_requests;
        uint64_t total_tokens_generated;
        double total_generation_time_ms;
        double avg_tokens_per_second;
        size_t peak_memory_mb;
    };
    Stats get_stats() const;
    void reset_stats();

private:
    std::shared_ptr<LLMBackend> backend_;
    std::shared_ptr<ChatTemplate> chat_template_;
    std::unique_ptr<InferenceQueue> queue_;
    std::unique_ptr<ModelZoo> zoo_;
    mutable std::mutex mutex_;
    Stats stats_;
};

// ═══════════════════════════════════════════════════════════════════════════
//  UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

// Quantization helpers
std::string quant_type_to_string(QuantizationType qt);
QuantizationType string_to_quant_type(const std::string& s);
size_t estimate_memory_mb(size_t parameters_b, QuantizationType qt);

// Prompt helpers
std::string build_prompt(const std::string& system, const std::string& user);
std::string truncate_to_context(const std::string& text, int max_tokens, LLMBackend* backend);

// Global runtime access
LLMRuntime& llm();

} // namespace rael
