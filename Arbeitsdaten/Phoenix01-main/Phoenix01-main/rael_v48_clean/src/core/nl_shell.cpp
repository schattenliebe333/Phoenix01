// RAEL V49 - Natural Language Shell Implementation (#38)
#include "rael/nl_shell.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <unistd.h>

namespace rael {

// ═══════════════════════════════════════════════════════════════════════════
//  INTENT RECOGNIZER
// ═══════════════════════════════════════════════════════════════════════════

IntentRecognizer::IntentRecognizer() {
    register_file_intents();
    register_navigation_intents();
    register_search_intents();
    register_git_intents();
    register_system_intents();
}

void IntentRecognizer::register_intent(const Intent& intent) {
    std::lock_guard<std::mutex> lock(mutex_);
    intents_[intent.name] = intent;
}

void IntentRecognizer::unregister_intent(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    intents_.erase(name);
}

std::vector<Intent> IntentRecognizer::list_intents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Intent> result;
    for (const auto& [_, intent] : intents_) {
        result.push_back(intent);
    }
    return result;
}

std::vector<std::pair<std::string, double>> IntentRecognizer::recognize(
    const std::string& input) {

    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, double>> scores;
    std::string normalized_input = normalize(input);

    for (const auto& [name, intent] : intents_) {
        double best_score = 0;

        for (const auto& example : intent.examples) {
            double score = compute_similarity(normalized_input, normalize(example));
            best_score = std::max(best_score, score);
        }

        if (best_score > 0.1) {
            scores.push_back({name, best_score});
        }
    }

    std::sort(scores.begin(), scores.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    return scores;
}

std::optional<Intent> IntentRecognizer::get_best_intent(const std::string& input) {
    auto scores = recognize(input);
    if (scores.empty() || scores[0].second < 0.3) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = intents_.find(scores[0].first);
    if (it != intents_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<ParsedSlot> IntentRecognizer::extract_slots(
    const std::string& input, const Intent& intent) {

    std::vector<ParsedSlot> slots;
    auto tokens = tokenize(input);

    // Look for paths
    for (const auto& token : tokens) {
        if (token.find('/') != std::string::npos ||
            token.find('.') != std::string::npos) {
            ParsedSlot slot;
            slot.name = "path";
            slot.value = token;
            slot.type = "path";
            slot.confidence = 0.8;
            slots.push_back(slot);
        }
    }

    // Check required slots
    for (const auto& req : intent.required_slots) {
        bool found = false;
        for (const auto& slot : slots) {
            if (slot.name == req) {
                found = true;
                break;
            }
        }
        if (!found) {
            ParsedSlot slot;
            slot.name = req;
            slot.required = true;
            slot.confidence = 0;
            slots.push_back(slot);
        }
    }

    return slots;
}

void IntentRecognizer::register_file_intents() {
    // Create file
    Intent create_file;
    create_file.name = "create_file";
    create_file.description = "Create a new file";
    create_file.examples = {
        "create a file called test.txt",
        "make a new file named config.json",
        "touch readme.md",
        "create file example.py",
        "new file main.cpp"
    };
    create_file.required_slots = {"filename"};
    create_file.category = CommandCategory::FILE_SYSTEM;
    create_file.generator = [](const ParsedCommand& cmd) {
        for (const auto& slot : cmd.slots) {
            if (slot.name == "path" || slot.name == "filename") {
                return "touch " + slot.value;
            }
        }
        return std::string("touch");
    };
    register_intent(create_file);

    // Delete file
    Intent delete_file;
    delete_file.name = "delete_file";
    delete_file.description = "Delete a file";
    delete_file.examples = {
        "delete the file test.txt",
        "remove config.json",
        "rm old_file.txt",
        "erase temp.log",
        "delete file output.dat"
    };
    delete_file.required_slots = {"filename"};
    delete_file.category = CommandCategory::FILE_SYSTEM;
    delete_file.generator = [](const ParsedCommand& cmd) {
        for (const auto& slot : cmd.slots) {
            if (slot.name == "path" || slot.name == "filename") {
                return "rm " + slot.value;
            }
        }
        return std::string("rm");
    };
    register_intent(delete_file);

    // Copy file
    Intent copy_file;
    copy_file.name = "copy_file";
    copy_file.description = "Copy a file";
    copy_file.examples = {
        "copy file.txt to backup.txt",
        "duplicate main.py as main_backup.py",
        "cp config.json to config.json.bak",
        "make a copy of readme.md"
    };
    copy_file.required_slots = {"source", "destination"};
    copy_file.category = CommandCategory::FILE_SYSTEM;
    register_intent(copy_file);

    // Move file
    Intent move_file;
    move_file.name = "move_file";
    move_file.description = "Move or rename a file";
    move_file.examples = {
        "move file.txt to archive/",
        "rename old.py to new.py",
        "mv config.json to settings/",
        "move the file to another folder"
    };
    move_file.required_slots = {"source", "destination"};
    move_file.category = CommandCategory::FILE_SYSTEM;
    register_intent(move_file);

    // Read file
    Intent read_file;
    read_file.name = "read_file";
    read_file.description = "Display file contents";
    read_file.examples = {
        "show me the file config.json",
        "read readme.md",
        "cat main.py",
        "display the contents of test.txt",
        "what's in the file log.txt"
    };
    read_file.required_slots = {"filename"};
    read_file.category = CommandCategory::FILE_SYSTEM;
    register_intent(read_file);

    // Create directory
    Intent create_dir;
    create_dir.name = "create_directory";
    create_dir.description = "Create a new directory";
    create_dir.examples = {
        "create a folder called src",
        "make directory test",
        "mkdir build",
        "create new folder lib",
        "make a new directory for the project"
    };
    create_dir.required_slots = {"dirname"};
    create_dir.category = CommandCategory::FILE_SYSTEM;
    register_intent(create_dir);
}

void IntentRecognizer::register_navigation_intents() {
    // Change directory
    Intent cd;
    cd.name = "change_directory";
    cd.description = "Change current directory";
    cd.examples = {
        "go to the src folder",
        "cd to home",
        "change directory to /tmp",
        "navigate to the project folder",
        "switch to the parent directory",
        "go up one level",
        "go back"
    };
    cd.optional_slots = {"path"};
    cd.category = CommandCategory::NAVIGATION;
    register_intent(cd);

    // List directory
    Intent ls;
    ls.name = "list_directory";
    ls.description = "List directory contents";
    ls.examples = {
        "list files in current folder",
        "show me all files",
        "what files are here",
        "ls",
        "list everything including hidden files",
        "show all files with details"
    };
    ls.optional_slots = {"path"};
    ls.category = CommandCategory::NAVIGATION;
    register_intent(ls);

    // Print working directory
    Intent pwd;
    pwd.name = "print_directory";
    pwd.description = "Show current directory";
    pwd.examples = {
        "where am I",
        "what directory am I in",
        "show current path",
        "pwd",
        "current folder"
    };
    pwd.category = CommandCategory::NAVIGATION;
    register_intent(pwd);
}

void IntentRecognizer::register_search_intents() {
    // Find files
    Intent find;
    find.name = "find_files";
    find.description = "Find files by name or pattern";
    find.examples = {
        "find all python files",
        "search for files named config",
        "find files with extension .txt",
        "look for test files",
        "find all files containing main"
    };
    find.optional_slots = {"pattern", "path"};
    find.category = CommandCategory::SEARCH;
    register_intent(find);

    // Grep in files
    Intent grep;
    grep.name = "search_content";
    grep.description = "Search for text in files";
    grep.examples = {
        "search for TODO in all files",
        "find lines containing error",
        "grep for function in py files",
        "look for the word config in the code",
        "search for import statements"
    };
    grep.required_slots = {"pattern"};
    grep.optional_slots = {"path", "file_pattern"};
    grep.category = CommandCategory::SEARCH;
    register_intent(grep);
}

void IntentRecognizer::register_git_intents() {
    // Git status
    Intent status;
    status.name = "git_status";
    status.description = "Show git status";
    status.examples = {
        "show git status",
        "what files have changed",
        "git status",
        "check repo status",
        "what's modified"
    };
    status.category = CommandCategory::GIT;
    register_intent(status);

    // Git commit
    Intent commit;
    commit.name = "git_commit";
    commit.description = "Commit changes";
    commit.examples = {
        "commit changes with message fix bug",
        "git commit -m update readme",
        "save my changes as initial commit",
        "commit all changes"
    };
    commit.optional_slots = {"message"};
    commit.category = CommandCategory::GIT;
    register_intent(commit);

    // Git push
    Intent push;
    push.name = "git_push";
    push.description = "Push changes to remote";
    push.examples = {
        "push to origin",
        "git push",
        "push my commits",
        "upload changes to github"
    };
    push.optional_slots = {"remote", "branch"};
    push.category = CommandCategory::GIT;
    register_intent(push);

    // Git pull
    Intent pull;
    pull.name = "git_pull";
    pull.description = "Pull changes from remote";
    pull.examples = {
        "pull latest changes",
        "git pull",
        "update from remote",
        "fetch and merge"
    };
    pull.optional_slots = {"remote", "branch"};
    pull.category = CommandCategory::GIT;
    register_intent(pull);

    // Git add
    Intent add;
    add.name = "git_add";
    add.description = "Stage files for commit";
    add.examples = {
        "add all files to git",
        "stage the changes",
        "git add everything",
        "add file.txt to staging"
    };
    add.optional_slots = {"path"};
    add.category = CommandCategory::GIT;
    register_intent(add);

    // Git branch
    Intent branch;
    branch.name = "git_branch";
    branch.description = "List or create branches";
    branch.examples = {
        "show all branches",
        "list branches",
        "create a new branch called feature",
        "git branch"
    };
    branch.optional_slots = {"branch_name"};
    branch.category = CommandCategory::GIT;
    register_intent(branch);

    // Git checkout
    Intent checkout;
    checkout.name = "git_checkout";
    checkout.description = "Switch branches or restore files";
    checkout.examples = {
        "switch to main branch",
        "checkout develop",
        "go to the feature branch",
        "git checkout master"
    };
    checkout.optional_slots = {"branch_name", "path"};
    checkout.category = CommandCategory::GIT;
    register_intent(checkout);
}

void IntentRecognizer::register_system_intents() {
    // Date/Time
    Intent datetime;
    datetime.name = "show_datetime";
    datetime.description = "Show current date and time";
    datetime.examples = {
        "what time is it",
        "show the date",
        "current time",
        "what's today's date"
    };
    datetime.category = CommandCategory::SYSTEM;
    register_intent(datetime);

    // Environment
    Intent env;
    env.name = "show_environment";
    env.description = "Show environment variables";
    env.examples = {
        "show environment variables",
        "print PATH",
        "what's the value of HOME",
        "env"
    };
    env.optional_slots = {"variable"};
    env.category = CommandCategory::SYSTEM;
    register_intent(env);

    // Process list
    Intent ps;
    ps.name = "list_processes";
    ps.description = "List running processes";
    ps.examples = {
        "show running processes",
        "list all processes",
        "what's running",
        "ps aux"
    };
    ps.category = CommandCategory::PROCESS;
    register_intent(ps);

    // Kill process
    Intent kill;
    kill.name = "kill_process";
    kill.description = "Terminate a process";
    kill.examples = {
        "kill process 1234",
        "stop the server",
        "terminate node",
        "kill all python processes"
    };
    kill.required_slots = {"process"};
    kill.category = CommandCategory::PROCESS;
    register_intent(kill);
}

double IntentRecognizer::compute_similarity(const std::string& input,
                                             const std::string& example) {
    auto tokens1 = tokenize(input);
    auto tokens2 = tokenize(example);

    if (tokens1.empty() || tokens2.empty()) return 0;

    std::set<std::string> set1(tokens1.begin(), tokens1.end());
    std::set<std::string> set2(tokens2.begin(), tokens2.end());

    std::set<std::string> intersection;
    std::set_intersection(set1.begin(), set1.end(),
                          set2.begin(), set2.end(),
                          std::inserter(intersection, intersection.begin()));

    std::set<std::string> union_set;
    std::set_union(set1.begin(), set1.end(),
                   set2.begin(), set2.end(),
                   std::inserter(union_set, union_set.begin()));

    double jaccard = static_cast<double>(intersection.size()) / union_set.size();

    // Bonus for matching key action words
    double action_bonus = 0;
    std::vector<std::string> actions = {"create", "delete", "move", "copy", "find",
                                        "show", "list", "git", "search", "open"};
    for (const auto& action : actions) {
        bool in1 = set1.count(action) > 0;
        bool in2 = set2.count(action) > 0;
        if (in1 && in2) action_bonus += 0.2;
    }

    return std::min(1.0, jaccard + action_bonus);
}

std::vector<std::string> IntentRecognizer::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::istringstream iss(text);
    std::string token;

    while (iss >> token) {
        // Remove punctuation
        token.erase(std::remove_if(token.begin(), token.end(),
                                   [](char c) { return std::ispunct(c) && c != '.' && c != '/'; }),
                    token.end());
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

std::string IntentRecognizer::normalize(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
//  ENTITY EXTRACTOR
// ═══════════════════════════════════════════════════════════════════════════

EntityExtractor::EntityExtractor() {
    // Get home directory
    const char* home = getenv("HOME");
    home_dir_ = home ? home : "/home";
    current_dir_ = ".";

    // Initialize patterns
    patterns_[EntityType::PATH] = {
        std::regex(R"((?:^|[^\w])([/~][\w/.~-]+))"),
        std::regex(R"((?:^|[^\w])(\.\.?(?:/[\w.-]+)*))"),
    };
    patterns_[EntityType::FILENAME] = {
        std::regex(R"([\w.-]+\.[a-zA-Z0-9]+)"),
    };
    patterns_[EntityType::NUMBER] = {
        std::regex(R"(\b\d+\b)"),
    };
    patterns_[EntityType::URL] = {
        std::regex(R"(https?://[^\s]+)"),
    };
    patterns_[EntityType::EMAIL] = {
        std::regex(R"([\w.+-]+@[\w.-]+\.\w+)"),
    };
    patterns_[EntityType::COMMIT_HASH] = {
        std::regex(R"(\b[0-9a-f]{7,40}\b)"),
    };
    patterns_[EntityType::PATTERN] = {
        std::regex(R"(\*[\w.*?]+|\*\*[\w.*?/]+)"),
    };
}

std::vector<Entity> EntityExtractor::extract(const std::string& text) {
    std::vector<Entity> entities;

    for (const auto& [type, patterns] : patterns_) {
        for (const auto& pattern : patterns) {
            std::sregex_iterator it(text.begin(), text.end(), pattern);
            std::sregex_iterator end;

            while (it != end) {
                Entity e;
                e.value = (*it)[0].str();
                e.normalized = e.value;
                e.type = type;
                e.start_pos = it->position();
                e.end_pos = e.start_pos + e.value.length();
                e.confidence = 0.8;

                // Normalize paths
                if (type == EntityType::PATH) {
                    e.normalized = expand_path(e.value);
                }

                entities.push_back(e);
                ++it;
            }
        }
    }

    // Sort by position
    std::sort(entities.begin(), entities.end(),
              [](const Entity& a, const Entity& b) { return a.start_pos < b.start_pos; });

    return entities;
}

std::vector<Entity> EntityExtractor::extract_type(const std::string& text, EntityType type) {
    std::vector<Entity> all = extract(text);
    std::vector<Entity> filtered;

    for (const auto& e : all) {
        if (e.type == type) {
            filtered.push_back(e);
        }
    }

    return filtered;
}

std::vector<Entity> EntityExtractor::extract_paths(const std::string& text) {
    return extract_type(text, EntityType::PATH);
}

std::vector<Entity> EntityExtractor::extract_numbers(const std::string& text) {
    return extract_type(text, EntityType::NUMBER);
}

std::vector<Entity> EntityExtractor::extract_patterns(const std::string& text) {
    return extract_type(text, EntityType::PATTERN);
}

std::vector<Entity> EntityExtractor::extract_urls(const std::string& text) {
    return extract_type(text, EntityType::URL);
}

std::string EntityExtractor::normalize_path(const std::string& path) {
    if (path.empty()) return path;

    std::string result = path;

    // Expand ~
    if (result[0] == '~') {
        result = home_dir_ + result.substr(1);
    }

    return result;
}

std::string EntityExtractor::expand_path(const std::string& path) {
    return normalize_path(path);
}

void EntityExtractor::add_pattern(EntityType type, const std::string& regex) {
    patterns_[type].push_back(std::regex(regex));
}

// ═══════════════════════════════════════════════════════════════════════════
//  COMMAND GENERATOR
// ═══════════════════════════════════════════════════════════════════════════

CommandGenerator::CommandGenerator() {
    // Initialize templates
    templates_["create_file"] = "touch {filename}";
    templates_["delete_file"] = "rm {filename}";
    templates_["copy_file"] = "cp {source} {destination}";
    templates_["move_file"] = "mv {source} {destination}";
    templates_["read_file"] = "cat {filename}";
    templates_["create_directory"] = "mkdir -p {dirname}";
    templates_["change_directory"] = "cd {path}";
    templates_["list_directory"] = "ls -la {path}";
    templates_["find_files"] = "find {path} -name '{pattern}'";
    templates_["search_content"] = "grep -r '{pattern}' {path}";
    templates_["git_status"] = "git status";
    templates_["git_add"] = "git add {path}";
    templates_["git_commit"] = "git commit -m '{message}'";
    templates_["git_push"] = "git push {remote} {branch}";
    templates_["git_pull"] = "git pull {remote} {branch}";

    // Dangerous patterns
    dangerous_patterns_ = {
        std::regex(R"(rm\s+-rf?\s+/)"),          // rm -rf /
        std::regex(R"(rm\s+-rf?\s+\*)"),         // rm -rf *
        std::regex(R"(>\s*/dev/sd)"),            // write to disk device
        std::regex(R"(mkfs)"),                   // format disk
        std::regex(R"(dd\s+if=)"),               // dd command
        std::regex(R"(:\(\)\{\s*:\|:)"),         // fork bomb
        std::regex(R"(chmod\s+-R\s+777\s+/)"),   // chmod 777 /
    };
}

std::string CommandGenerator::generate(const ParsedCommand& cmd) {
    switch (cmd.category) {
        case CommandCategory::FILE_SYSTEM:
            return generate_file_command(cmd);
        case CommandCategory::NAVIGATION:
            return generate_navigation_command(cmd);
        case CommandCategory::SEARCH:
            return generate_search_command(cmd);
        case CommandCategory::GIT:
            return generate_git_command(cmd);
        case CommandCategory::PROCESS:
            return generate_process_command(cmd);
        case CommandCategory::NETWORK:
            return generate_network_command(cmd);
        case CommandCategory::SYSTEM:
            return generate_system_command(cmd);
        default:
            return "";
    }
}

std::string CommandGenerator::generate_file_command(const ParsedCommand& cmd) {
    std::map<std::string, std::string> vars;

    for (const auto& slot : cmd.slots) {
        vars[slot.name] = slot.value;
    }

    // Map action to template
    std::string tmpl;
    if (cmd.action == "create_file") {
        tmpl = "touch";
        if (vars.count("path")) tmpl += " " + vars["path"];
        else if (vars.count("filename")) tmpl += " " + vars["filename"];
    } else if (cmd.action == "delete_file") {
        tmpl = "rm";
        if (vars.count("path")) tmpl += " " + vars["path"];
        else if (vars.count("filename")) tmpl += " " + vars["filename"];
    } else if (cmd.action == "copy_file") {
        tmpl = "cp";
        if (vars.count("source")) tmpl += " " + vars["source"];
        if (vars.count("destination")) tmpl += " " + vars["destination"];
    } else if (cmd.action == "move_file") {
        tmpl = "mv";
        if (vars.count("source")) tmpl += " " + vars["source"];
        if (vars.count("destination")) tmpl += " " + vars["destination"];
    } else if (cmd.action == "read_file") {
        tmpl = "cat";
        if (vars.count("path")) tmpl += " " + vars["path"];
        else if (vars.count("filename")) tmpl += " " + vars["filename"];
    } else if (cmd.action == "create_directory") {
        tmpl = "mkdir -p";
        if (vars.count("path")) tmpl += " " + vars["path"];
        else if (vars.count("dirname")) tmpl += " " + vars["dirname"];
    }

    return tmpl;
}

std::string CommandGenerator::generate_navigation_command(const ParsedCommand& cmd) {
    if (cmd.action == "change_directory") {
        std::string path = ".";
        for (const auto& slot : cmd.slots) {
            if (slot.name == "path") {
                path = slot.value;
                break;
            }
        }

        // Handle special cases
        if (cmd.original_input.find("back") != std::string::npos ||
            cmd.original_input.find("up") != std::string::npos ||
            cmd.original_input.find("parent") != std::string::npos) {
            path = "..";
        } else if (cmd.original_input.find("home") != std::string::npos) {
            path = "~";
        }

        return "cd " + path;
    } else if (cmd.action == "list_directory") {
        std::string path = ".";
        for (const auto& slot : cmd.slots) {
            if (slot.name == "path") {
                path = slot.value;
                break;
            }
        }

        std::string flags = "-la";
        if (cmd.original_input.find("hidden") == std::string::npos &&
            cmd.original_input.find("all") == std::string::npos) {
            flags = "-l";
        }

        return "ls " + flags + " " + path;
    } else if (cmd.action == "print_directory") {
        return "pwd";
    }

    return "";
}

std::string CommandGenerator::generate_search_command(const ParsedCommand& cmd) {
    std::string pattern;
    std::string path = ".";

    for (const auto& slot : cmd.slots) {
        if (slot.name == "pattern") pattern = slot.value;
        if (slot.name == "path") path = slot.value;
    }

    if (cmd.action == "find_files") {
        // Extract file extension or pattern from input
        std::regex ext_regex(R"(\.(\w+)\s+files?)");
        std::smatch match;
        if (std::regex_search(cmd.original_input, match, ext_regex)) {
            pattern = "*." + match[1].str();
        } else if (pattern.empty()) {
            pattern = "*";
        }

        return "find " + path + " -name '" + pattern + "'";
    } else if (cmd.action == "search_content") {
        // Extract search term
        std::regex term_regex(R"((?:for|containing|with)\s+['\"]?(\w+)['\"]?)");
        std::smatch match;
        if (std::regex_search(cmd.original_input, match, term_regex)) {
            pattern = match[1].str();
        }

        if (!pattern.empty()) {
            return "grep -rn '" + pattern + "' " + path;
        }
    }

    return "";
}

std::string CommandGenerator::generate_git_command(const ParsedCommand& cmd) {
    if (cmd.action == "git_status") {
        return "git status";
    } else if (cmd.action == "git_add") {
        std::string path = ".";
        for (const auto& slot : cmd.slots) {
            if (slot.name == "path") {
                path = slot.value;
                break;
            }
        }
        return "git add " + path;
    } else if (cmd.action == "git_commit") {
        std::string message = "update";

        // Extract message from input
        std::regex msg_regex(R"((?:message|msg|-m)\s+['\"]?([^'\"]+)['\"]?)");
        std::smatch match;
        if (std::regex_search(cmd.original_input, match, msg_regex)) {
            message = match[1].str();
        } else {
            // Try to extract after "with message" or "as"
            std::regex alt_regex(R"((?:with message|as)\s+(.+))");
            if (std::regex_search(cmd.original_input, match, alt_regex)) {
                message = match[1].str();
            }
        }

        return "git commit -m '" + message + "'";
    } else if (cmd.action == "git_push") {
        std::string remote = "origin";
        std::string branch;

        for (const auto& slot : cmd.slots) {
            if (slot.name == "remote") remote = slot.value;
            if (slot.name == "branch") branch = slot.value;
        }

        if (branch.empty()) {
            return "git push " + remote;
        }
        return "git push " + remote + " " + branch;
    } else if (cmd.action == "git_pull") {
        std::string remote = "origin";
        std::string branch;

        for (const auto& slot : cmd.slots) {
            if (slot.name == "remote") remote = slot.value;
            if (slot.name == "branch") branch = slot.value;
        }

        if (branch.empty()) {
            return "git pull " + remote;
        }
        return "git pull " + remote + " " + branch;
    } else if (cmd.action == "git_branch") {
        std::string branch_name;
        for (const auto& slot : cmd.slots) {
            if (slot.name == "branch_name") {
                branch_name = slot.value;
                break;
            }
        }

        if (branch_name.empty()) {
            return "git branch -a";
        }
        return "git branch " + branch_name;
    } else if (cmd.action == "git_checkout") {
        std::string branch;
        for (const auto& slot : cmd.slots) {
            if (slot.name == "branch_name") {
                branch = slot.value;
                break;
            }
        }

        // Extract branch from input
        if (branch.empty()) {
            std::regex branch_regex(R"((?:to|branch)\s+(\w+))");
            std::smatch match;
            if (std::regex_search(cmd.original_input, match, branch_regex)) {
                branch = match[1].str();
            }
        }

        if (!branch.empty()) {
            return "git checkout " + branch;
        }
    }

    return "git " + cmd.action.substr(4);  // Remove "git_" prefix
}

std::string CommandGenerator::generate_process_command(const ParsedCommand& cmd) {
    if (cmd.action == "list_processes") {
        return "ps aux";
    } else if (cmd.action == "kill_process") {
        std::string process;
        for (const auto& slot : cmd.slots) {
            if (slot.name == "process") {
                process = slot.value;
                break;
            }
        }

        // Check if it's a PID or process name
        bool is_pid = !process.empty() &&
                      std::all_of(process.begin(), process.end(), ::isdigit);

        if (is_pid) {
            return "kill " + process;
        } else if (!process.empty()) {
            return "pkill " + process;
        }
    }

    return "";
}

std::string CommandGenerator::generate_network_command(const ParsedCommand& cmd) {
    (void)cmd;
    return "";
}

std::string CommandGenerator::generate_system_command(const ParsedCommand& cmd) {
    if (cmd.action == "show_datetime") {
        if (cmd.original_input.find("time") != std::string::npos) {
            return "date +%H:%M:%S";
        } else if (cmd.original_input.find("date") != std::string::npos) {
            return "date +%Y-%m-%d";
        }
        return "date";
    } else if (cmd.action == "show_environment") {
        std::string var;
        for (const auto& slot : cmd.slots) {
            if (slot.name == "variable") {
                var = slot.value;
                break;
            }
        }

        if (!var.empty()) {
            return "echo $" + var;
        }
        return "env";
    }

    return "";
}

void CommandGenerator::add_template(const std::string& action, const std::string& template_str) {
    templates_[action] = template_str;
}

std::string CommandGenerator::expand_template(const std::string& template_str,
                                               const std::map<std::string, std::string>& vars) {
    std::string result = template_str;

    for (const auto& [key, value] : vars) {
        std::string placeholder = "{" + key + "}";
        size_t pos;
        while ((pos = result.find(placeholder)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
        }
    }

    return result;
}

bool CommandGenerator::is_dangerous(const std::string& command) const {
    for (const auto& pattern : dangerous_patterns_) {
        if (std::regex_search(command, pattern)) {
            return true;
        }
    }
    return false;
}

std::string CommandGenerator::sanitize(const std::string& input) const {
    std::string result;
    for (char c : input) {
        // Allow safe characters
        if (std::isalnum(c) || c == '.' || c == '/' || c == '-' ||
            c == '_' || c == ' ' || c == '~') {
            result += c;
        }
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════
//  CONTEXT MANAGER
// ═══════════════════════════════════════════════════════════════════════════

ContextManager::ContextManager() {
    refresh();
}

ConversationContext& ContextManager::context() {
    return context_;
}

const ConversationContext& ContextManager::context() const {
    return context_;
}

void ContextManager::refresh() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Get current directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd))) {
        context_.current_directory = cwd;
    }

    // Check if in git repo
    std::ifstream git_head(".git/HEAD");
    context_.in_git_repo = git_head.good();
    if (context_.in_git_repo) {
        std::string line;
        if (std::getline(git_head, line)) {
            if (line.find("ref: refs/heads/") == 0) {
                context_.git_branch = line.substr(16);
            }
        }
    }
}

void ContextManager::update_from_result(const CommandResult& result) {
    std::lock_guard<std::mutex> lock(mutex_);
    context_.last_output = result.output;
    context_.last_error = result.error;
}

std::string ContextManager::resolve_pronoun(const std::string& pronoun) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string lower = pronoun;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "it" || lower == "that" || lower == "this") {
        // Return most recent file
        if (!context_.recent_files.empty()) {
            return context_.recent_files.back();
        }
    } else if (lower == "them" || lower == "those") {
        // Return recent files
        if (!context_.recent_files.empty()) {
            std::string result;
            for (const auto& f : context_.recent_files) {
                if (!result.empty()) result += " ";
                result += f;
            }
            return result;
        }
    } else if (lower == "here") {
        return context_.current_directory;
    }

    return pronoun;
}

std::string ContextManager::resolve_reference(const std::string& text) {
    std::string result = text;

    // Replace pronouns
    std::vector<std::pair<std::string, std::string>> pronouns = {
        {" it ", " " + resolve_pronoun("it") + " "},
        {" that ", " " + resolve_pronoun("that") + " "},
        {" this ", " " + resolve_pronoun("this") + " "},
        {" here", " " + resolve_pronoun("here")},
    };

    for (const auto& [from, to] : pronouns) {
        size_t pos;
        while ((pos = result.find(from)) != std::string::npos) {
            result.replace(pos, from.length(), to);
        }
    }

    return result;
}

void ContextManager::add_command(const std::string& cmd) {
    std::lock_guard<std::mutex> lock(mutex_);
    context_.recent_commands.push_back(cmd);
    if (context_.recent_commands.size() > 100) {
        context_.recent_commands.erase(context_.recent_commands.begin());
    }
}

std::vector<std::string> ContextManager::get_history(int n) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;

    int start = std::max(0, static_cast<int>(context_.recent_commands.size()) - n);
    for (int i = start; i < static_cast<int>(context_.recent_commands.size()); ++i) {
        result.push_back(context_.recent_commands[i]);
    }

    return result;
}

std::optional<std::string> ContextManager::find_similar_command(const std::string& partial) const {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto it = context_.recent_commands.rbegin();
         it != context_.recent_commands.rend(); ++it) {
        if (it->find(partial) != std::string::npos) {
            return *it;
        }
    }

    return std::nullopt;
}

void ContextManager::set_variable(const std::string& name, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    context_.variables[name] = value;
}

std::optional<std::string> ContextManager::get_variable(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = context_.variables.find(name);
    if (it != context_.variables.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string ContextManager::expand_variables(const std::string& text) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string result = text;

    for (const auto& [name, value] : context_.variables) {
        std::string placeholder = "$" + name;
        size_t pos;
        while ((pos = result.find(placeholder)) != std::string::npos) {
            result.replace(pos, placeholder.length(), value);
        }
    }

    return result;
}

void ContextManager::set_alias(const std::string& name, const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    context_.aliases[name] = command;
}

std::optional<std::string> ContextManager::get_alias(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = context_.aliases.find(name);
    if (it != context_.aliases.end()) {
        return it->second;
    }
    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════════════════════
//  DISAMBIGUATOR
// ═══════════════════════════════════════════════════════════════════════════

Disambiguator::Disambiguator() {}

bool Disambiguator::needs_disambiguation(const ParsedCommand& cmd) const {
    // Check confidence
    if (cmd.confidence < 0.5) return true;

    // Check for missing required slots
    for (const auto& slot : cmd.slots) {
        if (slot.required && slot.value.empty()) {
            return true;
        }
    }

    // Check for multiple alternatives
    if (cmd.alternatives.size() > 1) return true;

    return false;
}

std::vector<DisambiguationOption> Disambiguator::get_options(const ParsedCommand& cmd) {
    std::vector<DisambiguationOption> options;

    for (const auto& alt : cmd.alternatives) {
        DisambiguationOption opt;
        opt.description = alt;
        opt.command = alt;
        opt.score = 0.5;
        options.push_back(opt);
    }

    return options;
}

std::string Disambiguator::generate_question(const ParsedCommand& cmd) {
    std::ostringstream oss;

    // Check for missing slots
    for (const auto& slot : cmd.slots) {
        if (slot.required && slot.value.empty()) {
            oss << "What " << slot.name << " would you like to use?";
            return oss.str();
        }
    }

    // Low confidence
    if (cmd.confidence < 0.5) {
        oss << "Did you mean: " << cmd.canonical_form << "?";
    }

    return oss.str();
}

std::vector<std::string> Disambiguator::suggest_paths(const std::string& partial) {
    std::vector<std::string> suggestions;

    // Get directory to search
    std::string dir = ".";
    std::string prefix;

    size_t last_slash = partial.rfind('/');
    if (last_slash != std::string::npos) {
        dir = partial.substr(0, last_slash);
        if (dir.empty()) dir = "/";
        prefix = partial.substr(last_slash + 1);
    } else {
        prefix = partial;
    }

    auto entries = list_directory(dir);
    for (const auto& entry : entries) {
        if (entry.find(prefix) == 0) {
            if (dir == ".") {
                suggestions.push_back(entry);
            } else {
                suggestions.push_back(dir + "/" + entry);
            }
        }
    }

    return suggestions;
}

std::vector<std::string> Disambiguator::suggest_commands(const std::string& partial) {
    std::vector<std::string> suggestions;

    std::vector<std::string> commands = {
        "ls", "cd", "pwd", "cat", "rm", "cp", "mv", "mkdir", "touch",
        "find", "grep", "git", "make", "npm", "python", "node"
    };

    for (const auto& cmd : commands) {
        if (cmd.find(partial) == 0) {
            suggestions.push_back(cmd);
        }
    }

    return suggestions;
}

std::vector<std::string> Disambiguator::list_directory(const std::string& dir) {
    std::vector<std::string> entries;
    // Would use opendir/readdir in real implementation
    (void)dir;
    return entries;
}

// ═══════════════════════════════════════════════════════════════════════════
//  FEEDBACK LEARNER
// ═══════════════════════════════════════════════════════════════════════════

FeedbackLearner::FeedbackLearner() {}

void FeedbackLearner::record(const std::string& input, const std::string& generated,
                              bool correct, const std::string& correction) {
    std::lock_guard<std::mutex> lock(mutex_);

    FeedbackEntry entry;
    entry.input = input;
    entry.generated_command = generated;
    entry.corrected_command = correct ? generated : correction;
    entry.was_correct = correct;
    entry.timestamp = std::chrono::system_clock::now();

    feedback_.push_back(entry);

    // Learn from correction
    if (!correct && !correction.empty()) {
        learned_mappings_[input] = correction;
    }
}

void FeedbackLearner::learn_from_feedback() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Group feedback by input
    std::map<std::string, std::vector<FeedbackEntry>> by_input;
    for (const auto& entry : feedback_) {
        by_input[entry.input].push_back(entry);
    }

    // Learn common corrections
    for (const auto& [input, entries] : by_input) {
        std::map<std::string, int> command_counts;
        for (const auto& entry : entries) {
            if (entry.was_correct) {
                command_counts[entry.generated_command]++;
            } else if (!entry.corrected_command.empty()) {
                command_counts[entry.corrected_command] += 2;  // Weight corrections more
            }
        }

        // Find most common
        std::string best;
        int best_count = 0;
        for (const auto& [cmd, count] : command_counts) {
            if (count > best_count) {
                best = cmd;
                best_count = count;
            }
        }

        if (!best.empty() && best_count >= 2) {
            learned_mappings_[input] = best;
        }
    }
}

std::optional<std::string> FeedbackLearner::get_learned_mapping(const std::string& input) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = learned_mappings_.find(input);
    if (it != learned_mappings_.end()) {
        return it->second;
    }
    return std::nullopt;
}

double FeedbackLearner::get_accuracy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (feedback_.empty()) return 0;

    int correct = 0;
    for (const auto& entry : feedback_) {
        if (entry.was_correct) correct++;
    }

    return static_cast<double>(correct) / feedback_.size();
}

size_t FeedbackLearner::feedback_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return feedback_.size();
}

bool FeedbackLearner::save(const std::string& path) {
    (void)path;
    return true;
}

bool FeedbackLearner::load(const std::string& path) {
    (void)path;
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  COMMAND EXECUTOR
// ═══════════════════════════════════════════════════════════════════════════

CommandExecutor::CommandExecutor() {}

CommandResult CommandExecutor::execute(const std::string& command,
                                        const ExecutionConfig& config) {
    CommandResult result;
    auto start = std::chrono::high_resolution_clock::now();

    if (config.dry_run) {
        result.success = true;
        result.output = "[DRY RUN] Would execute: " + command;
        return result;
    }

    // Execute command using popen
    std::string cmd = command;
    if (!config.working_directory.empty()) {
        cmd = "cd " + config.working_directory + " && " + cmd;
    }
    cmd += " 2>&1";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        result.success = false;
        result.error = "Failed to execute command";
        result.exit_code = -1;
        return result;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result.output += buffer;
    }

    result.exit_code = pclose(pipe);
    result.success = (result.exit_code == 0);

    auto end = std::chrono::high_resolution_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    return result;
}

CommandResult CommandExecutor::safe_execute(const std::string& command,
                                             std::function<bool(const std::string&)> confirm) {
    CommandGenerator gen;

    if (gen.is_dangerous(command)) {
        if (!confirm("This command may be dangerous: " + command + "\nContinue?")) {
            CommandResult result;
            result.success = false;
            result.error = "Command cancelled by user";
            return result;
        }
    }

    return execute(command);
}

std::string CommandExecutor::execute_background(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::string job_id = "job_" + std::to_string(background_jobs_.size() + 1);

    background_jobs_[job_id] = std::async(std::launch::async, [this, command]() {
        return execute(command);
    });

    return job_id;
}

std::optional<CommandResult> CommandExecutor::get_background_result(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = background_jobs_.find(job_id);
    if (it == background_jobs_.end()) {
        return std::nullopt;
    }

    if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        auto result = it->second.get();
        background_jobs_.erase(it);
        return result;
    }

    return std::nullopt;
}

bool CommandExecutor::cancel_background(const std::string& job_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return background_jobs_.erase(job_id) > 0;
}

bool CommandExecutor::validate_command(const std::string& command) const {
    (void)command;
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//  AUTOCOMPLETE
// ═══════════════════════════════════════════════════════════════════════════

AutoComplete::AutoComplete() {
    // Add common keywords
    keywords_ = {
        {"all", "Match all items"},
        {"recursive", "Apply recursively"},
        {"force", "Force operation"},
        {"verbose", "Show detailed output"},
        {"quiet", "Suppress output"},
        {"help", "Show help"},
    };
}

std::vector<CompletionItem> AutoComplete::complete(const std::string& partial,
                                                    const ContextManager& ctx) {
    std::vector<CompletionItem> items;

    // Path completions
    auto paths = complete_path(partial);
    items.insert(items.end(), paths.begin(), paths.end());

    // Command completions
    auto cmds = complete_command(partial);
    items.insert(items.end(), cmds.begin(), cmds.end());

    // History completions
    auto hist = complete_from_history(partial, ctx.get_history(50));
    items.insert(items.end(), hist.begin(), hist.end());

    // Keyword completions
    for (const auto& [kw, desc] : keywords_) {
        if (kw.find(partial) == 0) {
            CompletionItem item;
            item.text = kw;
            item.display = kw;
            item.description = desc;
            item.score = 0.6;
            item.type = CompletionItem::Type::KEYWORD;
            items.push_back(item);
        }
    }

    // Sort by score
    std::sort(items.begin(), items.end(),
              [](const auto& a, const auto& b) { return a.score > b.score; });

    return items;
}

std::vector<CompletionItem> AutoComplete::complete_path(const std::string& partial) {
    std::vector<CompletionItem> items;
    // Would list directory entries matching partial
    (void)partial;
    return items;
}

std::vector<CompletionItem> AutoComplete::complete_command(const std::string& partial) {
    std::vector<CompletionItem> items;

    std::vector<std::pair<std::string, std::string>> commands = {
        {"ls", "List directory contents"},
        {"cd", "Change directory"},
        {"cat", "Display file contents"},
        {"grep", "Search for pattern"},
        {"find", "Find files"},
        {"git", "Version control"},
        {"make", "Build project"},
        {"npm", "Node package manager"},
        {"python", "Python interpreter"},
    };

    for (const auto& [cmd, desc] : commands) {
        if (cmd.find(partial) == 0) {
            CompletionItem item;
            item.text = cmd;
            item.display = cmd;
            item.description = desc;
            item.score = 0.8;
            item.type = CompletionItem::Type::COMMAND;
            items.push_back(item);
        }
    }

    return items;
}

std::vector<CompletionItem> AutoComplete::complete_from_history(
    const std::string& partial,
    const std::vector<std::string>& history) {

    std::vector<CompletionItem> items;

    for (const auto& cmd : history) {
        if (cmd.find(partial) != std::string::npos) {
            CompletionItem item;
            item.text = cmd;
            item.display = cmd;
            item.description = "From history";
            item.score = 0.7;
            item.type = CompletionItem::Type::HISTORY;
            items.push_back(item);
        }
    }

    return items;
}

void AutoComplete::add_keyword(const std::string& keyword, const std::string& description) {
    keywords_[keyword] = description;
}

void AutoComplete::add_completion_source(
    std::function<std::vector<CompletionItem>(const std::string&)> source) {
    sources_.push_back(source);
}

// ═══════════════════════════════════════════════════════════════════════════
//  NL SHELL
// ═══════════════════════════════════════════════════════════════════════════

NLShell::NLShell(const NLShellConfig& config)
    : config_(config)
    , intents_(std::make_unique<IntentRecognizer>())
    , entities_(std::make_unique<EntityExtractor>())
    , generator_(std::make_unique<CommandGenerator>())
    , context_(std::make_unique<ContextManager>())
    , disambiguator_(std::make_unique<Disambiguator>())
    , executor_(std::make_unique<CommandExecutor>())
    , autocomplete_(std::make_unique<AutoComplete>())
    , learner_(std::make_unique<FeedbackLearner>()) {

    stats_ = {};
}

NLShell::~NLShell() = default;

NLShell& NLShell::instance() {
    static NLShell instance;
    return instance;
}

ParsedCommand NLShell::parse(const std::string& input) {
    ParsedCommand cmd;
    cmd.original_input = input;

    // Resolve references first
    std::string resolved = context_->resolve_reference(input);

    // Check for learned mapping
    if (config_.enable_learning) {
        auto learned = learner_->get_learned_mapping(resolved);
        if (learned) {
            cmd.canonical_form = *learned;
            cmd.confidence = 1.0;
            return cmd;
        }
    }

    // Recognize intent
    auto intent_opt = intents_->get_best_intent(resolved);
    if (intent_opt) {
        cmd.action = intent_opt->name;
        cmd.category = intent_opt->category;
        cmd.slots = intents_->extract_slots(resolved, *intent_opt);

        // Calculate confidence
        auto scores = intents_->recognize(resolved);
        if (!scores.empty()) {
            cmd.confidence = scores[0].second;
        }

        // Generate canonical form
        if (intent_opt->generator) {
            cmd.canonical_form = intent_opt->generator(cmd);
        }
    }

    // Extract entities to fill slots
    auto entities = entities_->extract(resolved);
    for (const auto& entity : entities) {
        ParsedSlot slot;
        slot.value = entity.value;
        slot.confidence = entity.confidence;

        if (entity.type == EntityType::PATH || entity.type == EntityType::FILENAME) {
            slot.name = "path";
            slot.type = "path";
        } else if (entity.type == EntityType::PATTERN) {
            slot.name = "pattern";
            slot.type = "pattern";
        } else if (entity.type == EntityType::NUMBER) {
            slot.name = "number";
            slot.type = "number";
        }

        // Add if not already present
        bool exists = false;
        for (const auto& s : cmd.slots) {
            if (s.name == slot.name && s.value == slot.value) {
                exists = true;
                break;
            }
        }
        if (!exists && !slot.name.empty()) {
            cmd.slots.push_back(slot);
        }
    }

    // Check if confirmation needed
    if (config_.require_confirmation_for_dangerous) {
        std::string generated = generator_->generate(cmd);
        if (generator_->is_dangerous(generated)) {
            cmd.requires_confirmation = true;
            cmd.confirmation_message = "This command may be dangerous. Continue?";
        }
    }

    return cmd;
}

std::string NLShell::translate(const std::string& input) {
    auto cmd = parse(input);
    return generator_->generate(cmd);
}

CommandResult NLShell::interpret(const std::string& input) {
    auto cmd = parse(input);

    // Check disambiguation
    if (disambiguator_->needs_disambiguation(cmd)) {
        CommandResult result;
        result.success = false;
        result.output = disambiguator_->generate_question(cmd);
        stats_.disambiguations++;
        return result;
    }

    std::string shell_cmd = generator_->generate(cmd);

    if (shell_cmd.empty()) {
        CommandResult result;
        result.success = false;
        result.error = "Could not understand command: " + input;
        return result;
    }

    return {true, shell_cmd, "", 0, std::chrono::milliseconds(0), ""};
}

CommandResult NLShell::execute(const std::string& input) {
    auto cmd = parse(input);
    std::string shell_cmd = generator_->generate(cmd);

    if (shell_cmd.empty()) {
        CommandResult result;
        result.success = false;
        result.error = "Could not understand command: " + input;
        stats_.failed_commands++;
        return result;
    }

    // Confirmation if needed
    if (cmd.requires_confirmation && confirm_cb_) {
        if (!confirm_cb_(cmd.confirmation_message)) {
            CommandResult result;
            result.success = false;
            result.error = "Command cancelled";
            return result;
        }
    }

    // Execute
    auto result = executor_->execute(shell_cmd);

    // Update stats
    stats_.total_commands++;
    if (result.success) {
        stats_.successful_commands++;
    } else {
        stats_.failed_commands++;
    }

    // Update context
    context_->update_from_result(result);
    context_->add_command(shell_cmd);

    // Record for learning
    if (config_.enable_learning) {
        learner_->record(input, shell_cmd, result.success);
    }

    return result;
}

void NLShell::run_interactive() {
    if (output_cb_) {
        output_cb_("RAEL Natural Language Shell v1.0\n");
        output_cb_("Type natural language commands or 'exit' to quit.\n\n");
    }

    std::string line;
    while (true) {
        if (output_cb_) {
            output_cb_("nlsh> ");
        }

        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;

        process_line(line);
    }
}

void NLShell::process_line(const std::string& line) {
    if (line.empty()) return;

    auto result = execute(line);

    if (output_cb_) {
        if (result.success) {
            output_cb_(result.output + "\n");
        } else {
            output_cb_("Error: " + result.error + "\n");
        }
    }
}

std::string NLShell::explain(const ParsedCommand& cmd) {
    std::ostringstream oss;

    oss << "Input: \"" << cmd.original_input << "\"\n";
    oss << "Recognized intent: " << cmd.action << "\n";
    oss << "Confidence: " << std::fixed << std::setprecision(2) << cmd.confidence << "\n";
    oss << "Category: ";

    switch (cmd.category) {
        case CommandCategory::FILE_SYSTEM: oss << "File System"; break;
        case CommandCategory::NAVIGATION: oss << "Navigation"; break;
        case CommandCategory::SEARCH: oss << "Search"; break;
        case CommandCategory::GIT: oss << "Git"; break;
        case CommandCategory::PROCESS: oss << "Process"; break;
        case CommandCategory::SYSTEM: oss << "System"; break;
        default: oss << "Unknown"; break;
    }
    oss << "\n";

    if (!cmd.slots.empty()) {
        oss << "Extracted slots:\n";
        for (const auto& slot : cmd.slots) {
            oss << "  " << slot.name << " = \"" << slot.value << "\"\n";
        }
    }

    oss << "Generated command: " << cmd.canonical_form << "\n";

    return oss.str();
}

std::string NLShell::explain_command(const std::string& command) {
    auto cmd = parse(command);
    return explain(cmd);
}

std::vector<std::string> NLShell::suggest(const std::string& partial) {
    std::vector<std::string> suggestions;

    auto completions = autocomplete_->complete(partial, *context_);
    for (const auto& item : completions) {
        suggestions.push_back(item.text);
    }

    return suggestions;
}

std::string NLShell::suggest_correction(const std::string& failed_cmd) {
    // Try to find similar successful commands
    auto history = context_->get_history(100);

    double best_score = 0;
    std::string best_match;

    for (const auto& cmd : history) {
        // Simple similarity
        std::set<char> s1(failed_cmd.begin(), failed_cmd.end());
        std::set<char> s2(cmd.begin(), cmd.end());

        std::set<char> intersection;
        std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(),
                              std::inserter(intersection, intersection.begin()));

        double score = static_cast<double>(intersection.size()) /
                       std::max(s1.size(), s2.size());

        if (score > best_score && score > 0.6) {
            best_score = score;
            best_match = cmd;
        }
    }

    if (!best_match.empty()) {
        return "Did you mean: " + best_match + "?";
    }

    return "";
}

IntentRecognizer& NLShell::intents() { return *intents_; }
EntityExtractor& NLShell::entities() { return *entities_; }
CommandGenerator& NLShell::generator() { return *generator_; }
ContextManager& NLShell::context() { return *context_; }
Disambiguator& NLShell::disambiguator() { return *disambiguator_; }
AutoComplete& NLShell::autocomplete() { return *autocomplete_; }
FeedbackLearner& NLShell::learner() { return *learner_; }

void NLShell::set_config(const NLShellConfig& config) {
    config_ = config;
}

NLShellConfig NLShell::get_config() const {
    return config_;
}

void NLShell::set_confirm_callback(ConfirmCallback cb) {
    confirm_cb_ = cb;
}

void NLShell::set_output_callback(OutputCallback cb) {
    output_cb_ = cb;
}

bool NLShell::save_history(const std::string& path) {
    (void)path;
    return true;
}

bool NLShell::load_history(const std::string& path) {
    (void)path;
    return true;
}

NLShell::Stats NLShell::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

bool NLShell::should_confirm(const ParsedCommand& cmd) const {
    return cmd.requires_confirmation ||
           generator_->is_dangerous(cmd.canonical_form);
}

void NLShell::log_command(const std::string& input, const std::string& generated,
                           const CommandResult& result) {
    (void)input;
    (void)generated;
    (void)result;
}

NLShell& nlsh() {
    return NLShell::instance();
}

} // namespace rael
