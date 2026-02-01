// RAEL V49 - Local LLM Runtime Implementation
#include "rael/llm_runtime.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <regex>
#include <set>

namespace fs = std::filesystem;

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  UTILITY FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

std::string quant_type_to_string(QuantizationType qt) {
    switch (qt) {
        case QuantizationType::F32:    return "F32";
        case QuantizationType::F16:    return "F16";
        case QuantizationType::Q8_0:   return "Q8_0";
        case QuantizationType::Q5_1:   return "Q5_1";
        case QuantizationType::Q5_0:   return "Q5_0";
        case QuantizationType::Q4_1:   return "Q4_1";
        case QuantizationType::Q4_0:   return "Q4_0";
        case QuantizationType::Q3_K_M: return "Q3_K_M";
        case QuantizationType::Q4_K_M: return "Q4_K_M";
        case QuantizationType::Q5_K_M: return "Q5_K_M";
        case QuantizationType::Q6_K:   return "Q6_K";
        default: return "UNKNOWN";
    }
}

QuantizationType string_to_quant_type(const std::string& s) {
    if (s == "F32")    return QuantizationType::F32;
    if (s == "F16")    return QuantizationType::F16;
    if (s == "Q8_0")   return QuantizationType::Q8_0;
    if (s == "Q5_1")   return QuantizationType::Q5_1;
    if (s == "Q5_0")   return QuantizationType::Q5_0;
    if (s == "Q4_1")   return QuantizationType::Q4_1;
    if (s == "Q4_0")   return QuantizationType::Q4_0;
    if (s == "Q3_K_M") return QuantizationType::Q3_K_M;
    if (s == "Q4_K_M") return QuantizationType::Q4_K_M;
    if (s == "Q5_K_M") return QuantizationType::Q5_K_M;
    if (s == "Q6_K")   return QuantizationType::Q6_K;
    return QuantizationType::Q4_0;
}

size_t estimate_memory_mb(size_t parameters_b, QuantizationType qt) {
    double bits_per_param = 4.0;  // Default Q4
    switch (qt) {
        case QuantizationType::F32:    bits_per_param = 32.0; break;
        case QuantizationType::F16:    bits_per_param = 16.0; break;
        case QuantizationType::Q8_0:   bits_per_param = 8.5;  break;
        case QuantizationType::Q5_1:   bits_per_param = 5.5;  break;
        case QuantizationType::Q5_0:   bits_per_param = 5.0;  break;
        case QuantizationType::Q4_1:   bits_per_param = 4.5;  break;
        case QuantizationType::Q4_0:   bits_per_param = 4.0;  break;
        case QuantizationType::Q3_K_M: bits_per_param = 3.5;  break;
        case QuantizationType::Q4_K_M: bits_per_param = 4.5;  break;
        case QuantizationType::Q5_K_M: bits_per_param = 5.5;  break;
        case QuantizationType::Q6_K:   bits_per_param = 6.5;  break;
    }
    // parameters_b is in billions
    double bytes = (parameters_b * 1e9 * bits_per_param) / 8.0;
    return static_cast<size_t>(bytes / (1024.0 * 1024.0));
}

std::string build_prompt(const std::string& system, const std::string& user) {
    std::ostringstream oss;
    if (!system.empty()) {
        oss << "System: " << system << "\n\n";
    }
    oss << "User: " << user << "\n\nAssistant:";
    return oss.str();
}

std::string truncate_to_context(const std::string& text, int max_tokens, LLMBackend* backend) {
    if (!backend) return text;
    auto tokens = backend->tokenize(text);
    if (static_cast<int>(tokens.size()) <= max_tokens) return text;
    tokens.resize(max_tokens);
    return backend->detokenize(tokens);
}

// ═══════════════════════════════════════════════════════════════════════════
//  GGML BACKEND IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════════════

struct GGMLBackend::Impl {
    ModelConfig config;
    ModelInfo info;
    bool loaded = false;
    std::map<std::string, float> lora_adapters;

    // Simulated tokenizer (in real impl, would use llama.cpp)
    std::map<std::string, Token> vocab;
    std::map<Token, std::string> reverse_vocab;
    Token next_token_id = 0;

    void build_simple_vocab() {
        // Simple character-level tokenizer for simulation
        for (int c = 0; c < 256; c++) {
            std::string s(1, static_cast<char>(c));
            vocab[s] = next_token_id;
            reverse_vocab[next_token_id] = s;
            next_token_id++;
        }
        // Add some common tokens
        std::vector<std::string> common = {
            "the", "and", "is", "in", "to", "of", "a", "that",
            "it", "for", "as", "was", "with", "be", "by", "on",
            "not", "he", "this", "are", "or", "his", "from", "at",
            "which", "but", "have", "an", "had", "they", "you", "were",
            " ", "\n", "\t", ".", ",", "!", "?", ":", ";", "'", "\""
        };
        for (const auto& tok : common) {
            if (vocab.find(tok) == vocab.end()) {
                vocab[tok] = next_token_id;
                reverse_vocab[next_token_id] = tok;
                next_token_id++;
            }
        }
    }
};

GGMLBackend::GGMLBackend() : impl_(std::make_unique<Impl>()) {
    impl_->build_simple_vocab();
}

GGMLBackend::~GGMLBackend() {
    unload_model();
}

bool GGMLBackend::load_model(const ModelConfig& config) {
    std::lock_guard<std::mutex> lock(std::mutex{});

    // Check if file exists
    if (!fs::exists(config.model_path)) {
        std::cerr << "[LLM] Model file not found: " << config.model_path << std::endl;
        return false;
    }

    impl_->config = config;

    // Parse model info from filename (simulation)
    std::string filename = fs::path(config.model_path).stem().string();

    impl_->info.name = config.model_name.empty() ? filename : config.model_name;
    impl_->info.architecture = "llama";  // Assume llama by default
    impl_->info.version = "1.0";
    impl_->info.quant_type = config.quant_type;
    impl_->info.context_length = config.context_length;
    impl_->info.file_size_mb = fs::file_size(config.model_path) / (1024 * 1024);

    // Estimate parameters from file size and quantization
    double bits_per_param = 4.0;
    switch (config.quant_type) {
        case QuantizationType::Q4_0: bits_per_param = 4.0; break;
        case QuantizationType::Q4_K_M: bits_per_param = 4.5; break;
        case QuantizationType::Q5_K_M: bits_per_param = 5.5; break;
        case QuantizationType::Q8_0: bits_per_param = 8.5; break;
        default: break;
    }
    impl_->info.parameters = static_cast<size_t>(
        (impl_->info.file_size_mb * 1024.0 * 1024.0 * 8.0) / (bits_per_param * 1e9)
    );

    impl_->info.vocab_size = impl_->next_token_id;
    impl_->info.embedding_dim = 4096;
    impl_->info.num_layers = 32;
    impl_->info.num_heads = 32;

    impl_->loaded = true;

    std::cout << "[LLM] Loaded model: " << impl_->info.name
              << " (~" << impl_->info.parameters << "B params, "
              << quant_type_to_string(impl_->info.quant_type) << ")" << std::endl;

    return true;
}

void GGMLBackend::unload_model() {
    impl_->loaded = false;
    impl_->lora_adapters.clear();
}

bool GGMLBackend::is_loaded() const {
    return impl_->loaded;
}

ModelInfo GGMLBackend::get_model_info() const {
    return impl_->info;
}

TokenSequence GGMLBackend::tokenize(const std::string& text) const {
    TokenSequence tokens;
    // Simple greedy tokenization
    size_t i = 0;
    while (i < text.size()) {
        // Try to match longest token
        bool found = false;
        for (size_t len = std::min(size_t(10), text.size() - i); len > 0; len--) {
            std::string sub = text.substr(i, len);
            auto it = impl_->vocab.find(sub);
            if (it != impl_->vocab.end()) {
                tokens.push_back(it->second);
                i += len;
                found = true;
                break;
            }
        }
        if (!found) {
            // Unknown char, use byte fallback
            tokens.push_back(static_cast<Token>(static_cast<unsigned char>(text[i])));
            i++;
        }
    }
    return tokens;
}

std::string GGMLBackend::detokenize(const TokenSequence& tokens) const {
    std::ostringstream oss;
    for (Token t : tokens) {
        auto it = impl_->reverse_vocab.find(t);
        if (it != impl_->reverse_vocab.end()) {
            oss << it->second;
        } else if (t >= 0 && t < 256) {
            oss << static_cast<char>(t);
        }
    }
    return oss.str();
}

size_t GGMLBackend::vocab_size() const {
    return impl_->vocab.size();
}

GenerationResult GGMLBackend::generate(const std::string& prompt,
                                       const GenerationConfig& config,
                                       StreamCallback on_token) {
    GenerationResult result;
    auto start = std::chrono::high_resolution_clock::now();

    // Tokenize prompt
    auto prompt_tokens = tokenize(prompt);
    result.prompt_tokens = static_cast<int>(prompt_tokens.size());

    // Simulate generation (in real impl, would run inference)
    std::string response;

    // Simple simulation: generate based on prompt patterns
    if (prompt.find("Hello") != std::string::npos ||
        prompt.find("Hi") != std::string::npos) {
        response = "Hello! I'm RAEL's local LLM runtime. How can I assist you today?";
    } else if (prompt.find("code") != std::string::npos ||
               prompt.find("function") != std::string::npos) {
        response = "Here's an example implementation:\n\n```cpp\nvoid example() {\n    // Your code here\n}\n```";
    } else if (prompt.find("explain") != std::string::npos) {
        response = "Let me explain: The concept involves multiple interconnected components "
                   "that work together to achieve the desired outcome.";
    } else {
        response = "I understand your request. Based on the context provided, "
                   "I can help you with this task. Let me elaborate on the key points...";
    }

    // Apply max_tokens limit
    auto response_tokens = tokenize(response);
    if (static_cast<int>(response_tokens.size()) > config.max_tokens) {
        response_tokens.resize(config.max_tokens);
        response = detokenize(response_tokens);
    }

    // Stream tokens if callback provided
    if (on_token) {
        for (size_t i = 0; i < response.size(); i++) {
            std::string tok(1, response[i]);
            if (!on_token(tok)) {
                response = response.substr(0, i);
                result.stop_reason = "cancelled";
                break;
            }
            // Simulate generation delay
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    result.text = response;
    result.tokens = tokenize(response);
    result.completion_tokens = static_cast<int>(result.tokens.size());
    result.stopped_by_eos = true;
    if (result.stop_reason.empty()) {
        result.stop_reason = "eos";
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.generation_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    return result;
}

Embedding GGMLBackend::embed(const std::string& text) const {
    // Simulate embedding generation
    auto tokens = tokenize(text);
    int dim = impl_->info.embedding_dim > 0 ? impl_->info.embedding_dim : 4096;
    Embedding emb(dim, 0.0f);

    // Simple hash-based embedding simulation
    for (size_t i = 0; i < tokens.size(); i++) {
        for (int d = 0; d < dim; d++) {
            emb[d] += std::sin(tokens[i] * 0.1 + d * 0.01 + i * 0.001);
        }
    }

    // Normalize
    float norm = 0.0f;
    for (float v : emb) norm += v * v;
    norm = std::sqrt(norm);
    if (norm > 0) {
        for (float& v : emb) v /= norm;
    }

    return emb;
}

std::vector<Embedding> GGMLBackend::embed_batch(const std::vector<std::string>& texts) const {
    std::vector<Embedding> results;
    results.reserve(texts.size());
    for (const auto& text : texts) {
        results.push_back(embed(text));
    }
    return results;
}

bool GGMLBackend::load_lora(const std::string& path, float scale) {
    if (!fs::exists(path)) return false;
    impl_->lora_adapters[path] = scale;
    std::cout << "[LLM] Loaded LoRA adapter: " << path << " (scale=" << scale << ")" << std::endl;
    return true;
}

void GGMLBackend::unload_lora(const std::string& path) {
    impl_->lora_adapters.erase(path);
}

size_t GGMLBackend::memory_usage() const {
    if (!impl_->loaded) return 0;
    return estimate_memory_mb(impl_->info.parameters, impl_->info.quant_type);
}

void GGMLBackend::clear_kv_cache() {
    // Simulation - nothing to clear
}

// ═══════════════════════════════════════════════════════════════════════════
//  CHAT TEMPLATES
// ═══════════════════════════════════════════════════════════════════════════

std::string ChatMLTemplate::format(const ChatHistory& history) const {
    std::ostringstream oss;
    for (const auto& msg : history) {
        switch (msg.role) {
            case ChatRole::SYSTEM:
                oss << "<|im_start|>system\n" << msg.content << "<|im_end|>\n";
                break;
            case ChatRole::USER:
                oss << "<|im_start|>user\n" << msg.content << "<|im_end|>\n";
                break;
            case ChatRole::ASSISTANT:
                oss << "<|im_start|>assistant\n" << msg.content << "<|im_end|>\n";
                break;
            case ChatRole::FUNCTION:
                oss << "<|im_start|>function\n" << msg.content << "<|im_end|>\n";
                break;
        }
    }
    oss << "<|im_start|>assistant\n";
    return oss.str();
}

std::string Llama2Template::format(const ChatHistory& history) const {
    std::ostringstream oss;
    bool has_system = false;
    std::string system_msg;

    for (const auto& msg : history) {
        if (msg.role == ChatRole::SYSTEM) {
            system_msg = msg.content;
            has_system = true;
        }
    }

    bool first_user = true;
    for (const auto& msg : history) {
        if (msg.role == ChatRole::USER) {
            oss << "[INST] ";
            if (first_user && has_system) {
                oss << "<<SYS>>\n" << system_msg << "\n<</SYS>>\n\n";
                first_user = false;
            }
            oss << msg.content << " [/INST]";
        } else if (msg.role == ChatRole::ASSISTANT) {
            oss << " " << msg.content << " </s>";
        }
    }

    return oss.str();
}

std::string MistralTemplate::format(const ChatHistory& history) const {
    std::ostringstream oss;
    for (const auto& msg : history) {
        if (msg.role == ChatRole::USER) {
            oss << "[INST] " << msg.content << " [/INST]";
        } else if (msg.role == ChatRole::ASSISTANT) {
            oss << msg.content << "</s> ";
        }
    }
    return oss.str();
}

std::string AlpacaTemplate::format(const ChatHistory& history) const {
    std::ostringstream oss;
    for (const auto& msg : history) {
        if (msg.role == ChatRole::SYSTEM) {
            oss << "### Instruction:\n" << msg.content << "\n\n";
        } else if (msg.role == ChatRole::USER) {
            oss << "### Input:\n" << msg.content << "\n\n";
        } else if (msg.role == ChatRole::ASSISTANT) {
            oss << "### Response:\n" << msg.content << "\n\n";
        }
    }
    oss << "### Response:\n";
    return oss.str();
}

// ═══════════════════════════════════════════════════════════════════════════
//  MODEL ZOO
// ═══════════════════════════════════════════════════════════════════════════

ModelZoo::ModelZoo() {
    models_dir_ = "./models";

    // Default catalog of popular models
    catalog_ = {
        {"llama-2-7b-q4", "Llama 2 7B", "Meta's Llama 2 7B model, Q4 quantized",
         "https://huggingface.co/TheBloke/Llama-2-7B-GGUF", "", 3800, "llama",
         QuantizationType::Q4_0, {"general", "chat", "7b"}},
        {"llama-2-13b-q4", "Llama 2 13B", "Meta's Llama 2 13B model, Q4 quantized",
         "https://huggingface.co/TheBloke/Llama-2-13B-GGUF", "", 7300, "llama",
         QuantizationType::Q4_0, {"general", "chat", "13b"}},
        {"mistral-7b-q4", "Mistral 7B", "Mistral AI 7B model, Q4 quantized",
         "https://huggingface.co/TheBloke/Mistral-7B-GGUF", "", 3800, "mistral",
         QuantizationType::Q4_0, {"general", "chat", "7b", "fast"}},
        {"codellama-7b-q4", "Code Llama 7B", "Meta's Code Llama 7B for coding tasks",
         "https://huggingface.co/TheBloke/CodeLlama-7B-GGUF", "", 3800, "llama",
         QuantizationType::Q4_0, {"code", "programming", "7b"}},
        {"phi-2-q4", "Phi-2", "Microsoft's Phi-2 2.7B model",
         "https://huggingface.co/TheBloke/phi-2-GGUF", "", 1500, "phi",
         QuantizationType::Q4_0, {"small", "efficient", "2b"}},
        {"tinyllama-1b-q4", "TinyLlama 1B", "TinyLlama 1.1B model",
         "https://huggingface.co/TheBloke/TinyLlama-1.1B-GGUF", "", 600, "llama",
         QuantizationType::Q4_0, {"tiny", "fast", "1b"}},
    };
}

std::vector<ModelEntry> ModelZoo::list_models() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return catalog_;
}

std::vector<ModelEntry> ModelZoo::search(const std::string& query) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ModelEntry> results;
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    for (const auto& entry : catalog_) {
        std::string name = entry.name;
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        std::string desc = entry.description;
        std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);

        if (name.find(q) != std::string::npos || desc.find(q) != std::string::npos) {
            results.push_back(entry);
            continue;
        }
        for (const auto& tag : entry.tags) {
            if (tag.find(q) != std::string::npos) {
                results.push_back(entry);
                break;
            }
        }
    }
    return results;
}

std::vector<ModelEntry> ModelZoo::by_architecture(const std::string& arch) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ModelEntry> results;
    for (const auto& entry : catalog_) {
        if (entry.architecture == arch) {
            results.push_back(entry);
        }
    }
    return results;
}

std::vector<ModelEntry> ModelZoo::by_size(size_t max_mb) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ModelEntry> results;
    for (const auto& entry : catalog_) {
        if (entry.size_mb <= max_mb) {
            results.push_back(entry);
        }
    }
    return results;
}

std::optional<ModelEntry> ModelZoo::get(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : catalog_) {
        if (entry.id == id) {
            return entry;
        }
    }
    return std::nullopt;
}

bool ModelZoo::download(const std::string& id, ProgressCallback on_progress) {
    auto entry = get(id);
    if (!entry) return false;

    // Simulation of download
    if (on_progress) {
        for (int i = 0; i <= 100; i += 10) {
            on_progress(i / 100.0f, "Downloading " + entry->name + "...");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    // Create placeholder file
    fs::create_directories(models_dir_);
    std::string path = models_dir_ + "/" + id + ".gguf";
    std::ofstream f(path);
    f << "GGUF_PLACEHOLDER_" << id << std::endl;
    f.close();

    return true;
}

bool ModelZoo::is_downloaded(const std::string& id) const {
    return fs::exists(models_dir_ + "/" + id + ".gguf");
}

std::string ModelZoo::get_local_path(const std::string& id) const {
    return models_dir_ + "/" + id + ".gguf";
}

bool ModelZoo::delete_model(const std::string& id) {
    std::string path = get_local_path(id);
    if (fs::exists(path)) {
        fs::remove(path);
        return true;
    }
    return false;
}

void ModelZoo::add_custom(const ModelEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    catalog_.push_back(entry);
}

void ModelZoo::refresh_catalog() {
    // Would fetch updated catalog from remote
}

void ModelZoo::set_models_dir(const std::string& path) {
    models_dir_ = path;
    fs::create_directories(models_dir_);
}

std::string ModelZoo::get_models_dir() const {
    return models_dir_;
}

size_t ModelZoo::total_storage_used() const {
    size_t total = 0;
    if (fs::exists(models_dir_)) {
        for (const auto& entry : fs::directory_iterator(models_dir_)) {
            if (fs::is_regular_file(entry)) {
                total += fs::file_size(entry);
            }
        }
    }
    return total / (1024 * 1024);  // Return MB
}

// ═══════════════════════════════════════════════════════════════════════════
//  INFERENCE QUEUE
// ═══════════════════════════════════════════════════════════════════════════

InferenceQueue::InferenceQueue(size_t max_concurrent)
    : queue_([](const InferenceRequest& a, const InferenceRequest& b) {
        return a.priority < b.priority;
    }) {
    workers_.reserve(max_concurrent);
}

InferenceQueue::~InferenceQueue() {
    stop();
}

uint64_t InferenceQueue::submit(const InferenceRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    InferenceRequest req = request;
    req.id = next_id_++;
    queue_.push(req);
    cv_.notify_one();
    return req.id;
}

bool InferenceQueue::cancel(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Can't easily cancel from priority_queue, mark as cancelled
    return active_requests_.find(id) != active_requests_.end();
}

size_t InferenceQueue::pending_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

size_t InferenceQueue::active_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_requests_.size();
}

bool InferenceQueue::is_processing(uint64_t id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_requests_.find(id) != active_requests_.end();
}

void InferenceQueue::start() {
    running_ = true;
    paused_ = false;
    workers_.emplace_back(&InferenceQueue::worker_loop, this);
}

void InferenceQueue::stop() {
    running_ = false;
    cv_.notify_all();
    for (auto& w : workers_) {
        if (w.joinable()) w.join();
    }
    workers_.clear();
}

void InferenceQueue::pause() {
    paused_ = true;
}

void InferenceQueue::resume() {
    paused_ = false;
    cv_.notify_all();
}

void InferenceQueue::set_backend(std::shared_ptr<LLMBackend> backend) {
    backend_ = backend;
}

void InferenceQueue::worker_loop() {
    while (running_) {
        InferenceRequest req;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] {
                return !running_ || (!paused_ && !queue_.empty());
            });

            if (!running_) break;
            if (paused_ || queue_.empty()) continue;

            req = queue_.top();
            queue_.pop();
            active_requests_.insert(req.id);
        }

        // Process request
        if (backend_) {
            try {
                auto result = backend_->generate(req.prompt, req.config, req.on_token);
                if (req.on_complete) {
                    req.on_complete(result);
                }
            } catch (const std::exception& e) {
                if (req.on_error) {
                    req.on_error(e.what());
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            active_requests_.erase(req.id);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  LLM RUNTIME
// ═══════════════════════════════════════════════════════════════════════════

LLMRuntime::LLMRuntime()
    : backend_(std::make_shared<GGMLBackend>())
    , chat_template_(std::make_shared<ChatMLTemplate>())
    , queue_(std::make_unique<InferenceQueue>())
    , zoo_(std::make_unique<ModelZoo>()) {
    queue_->set_backend(backend_);
    queue_->start();
    stats_ = {};
}

LLMRuntime::~LLMRuntime() {
    queue_->stop();
}

LLMRuntime& LLMRuntime::instance() {
    static LLMRuntime instance;
    return instance;
}

bool LLMRuntime::load_model(const std::string& model_id_or_path, const ModelConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    ModelConfig cfg = config;
    if (cfg.model_path.empty()) {
        cfg.model_path = model_id_or_path;
    }

    return backend_->load_model(cfg);
}

bool LLMRuntime::load_from_zoo(const std::string& model_id, ProgressCallback on_progress) {
    if (!zoo_->is_downloaded(model_id)) {
        if (!zoo_->download(model_id, on_progress)) {
            return false;
        }
    }

    ModelConfig config;
    config.model_path = zoo_->get_local_path(model_id);
    config.model_name = model_id;

    auto entry = zoo_->get(model_id);
    if (entry) {
        config.quant_type = entry->quant_type;
    }

    return load_model(config.model_path, config);
}

void LLMRuntime::unload_model() {
    std::lock_guard<std::mutex> lock(mutex_);
    backend_->unload_model();
}

bool LLMRuntime::is_model_loaded() const {
    return backend_->is_loaded();
}

ModelInfo LLMRuntime::current_model() const {
    return backend_->get_model_info();
}

std::string LLMRuntime::generate(const std::string& prompt, const GenerationConfig& config) {
    auto result = backend_->generate(prompt, config);
    stats_.total_requests++;
    stats_.total_tokens_generated += result.completion_tokens;
    stats_.total_generation_time_ms += result.generation_time_ms;
    if (result.generation_time_ms > 0) {
        stats_.avg_tokens_per_second = (stats_.total_tokens_generated * 1000.0) /
                                       stats_.total_generation_time_ms;
    }
    return result.text;
}

void LLMRuntime::generate_stream(const std::string& prompt, StreamCallback on_token,
                                 const GenerationConfig& config) {
    backend_->generate(prompt, config, on_token);
}

std::string LLMRuntime::chat(const ChatHistory& history, const GenerationConfig& config) {
    std::string prompt = chat_template_->format(history);
    return generate(prompt, config);
}

void LLMRuntime::chat_stream(const ChatHistory& history, StreamCallback on_token,
                             const GenerationConfig& config) {
    std::string prompt = chat_template_->format(history);
    generate_stream(prompt, on_token, config);
}

Embedding LLMRuntime::embed(const std::string& text) {
    return backend_->embed(text);
}

std::vector<Embedding> LLMRuntime::embed_batch(const std::vector<std::string>& texts) {
    return backend_->embed_batch(texts);
}

float LLMRuntime::similarity(const std::string& a, const std::string& b) {
    auto emb_a = embed(a);
    auto emb_b = embed(b);

    // Cosine similarity
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < emb_a.size() && i < emb_b.size(); i++) {
        dot += emb_a[i] * emb_b[i];
        norm_a += emb_a[i] * emb_a[i];
        norm_b += emb_b[i] * emb_b[i];
    }

    if (norm_a > 0 && norm_b > 0) {
        return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
    return 0.0f;
}

uint64_t LLMRuntime::generate_async(const std::string& prompt,
                                    std::function<void(const GenerationResult&)> on_complete,
                                    const GenerationConfig& config) {
    InferenceRequest req;
    req.prompt = prompt;
    req.config = config;
    req.on_complete = on_complete;
    return queue_->submit(req);
}

bool LLMRuntime::cancel_generation(uint64_t request_id) {
    return queue_->cancel(request_id);
}

void LLMRuntime::set_chat_template(std::shared_ptr<ChatTemplate> tmpl) {
    chat_template_ = tmpl;
}

std::shared_ptr<ChatTemplate> LLMRuntime::get_chat_template() const {
    return chat_template_;
}

void LLMRuntime::auto_detect_template() {
    auto info = backend_->get_model_info();
    std::string arch = info.architecture;
    std::string name = info.name;

    std::transform(arch.begin(), arch.end(), arch.begin(), ::tolower);
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    if (arch == "mistral" || name.find("mistral") != std::string::npos) {
        chat_template_ = std::make_shared<MistralTemplate>();
    } else if (name.find("llama-2") != std::string::npos ||
               name.find("llama2") != std::string::npos) {
        chat_template_ = std::make_shared<Llama2Template>();
    } else if (name.find("alpaca") != std::string::npos) {
        chat_template_ = std::make_shared<AlpacaTemplate>();
    } else {
        chat_template_ = std::make_shared<ChatMLTemplate>();
    }
}

ModelZoo& LLMRuntime::zoo() {
    return *zoo_;
}

void LLMRuntime::set_backend(std::shared_ptr<LLMBackend> backend) {
    std::lock_guard<std::mutex> lock(mutex_);
    backend_ = backend;
    queue_->set_backend(backend);
}

std::shared_ptr<LLMBackend> LLMRuntime::get_backend() const {
    return backend_;
}

LLMRuntime::Stats LLMRuntime::get_stats() const {
    return stats_;
}

void LLMRuntime::reset_stats() {
    stats_ = {};
}

// Global access
LLMRuntime& llm() {
    return LLMRuntime::instance();
}

} // namespace rael
