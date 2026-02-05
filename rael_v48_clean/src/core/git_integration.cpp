// RAEL V48 - Git/VCS Integration (#14)
// Implementation of version control operations
// SECURITY: Hardened against command injection

#include "rael/git_integration.h"
#include "rael/events.h"
#include <sstream>
#include <fstream>
#include <cstdio>
#include <array>
#include <algorithm>
#include <regex>
#include <chrono>
#include <iomanip>
#include <random>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

namespace fs = std::filesystem;

namespace rael {

// ═══════════════════════════════════════════════════════════════════
//  GITCLIENT IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

GitClient::GitClient(const std::string& repo_path) : repo_path_(repo_path) {
    if (repo_path_.empty()) {
        repo_path_ = ".";
    }
    // SECURITY: Find git binary once at construction
    git_binary_ = find_git_binary();
}

// ============================================================================
// SECURITY: Find git binary (absolute path)
// ============================================================================
std::string GitClient::find_git_binary() const {
#ifdef _WIN32
    // Check common Windows locations
    std::vector<std::string> paths = {
        "C:\\Program Files\\Git\\bin\\git.exe",
        "C:\\Program Files (x86)\\Git\\bin\\git.exe"
    };
    for (const auto& p : paths) {
        if (fs::exists(p)) return p;
    }
    // Fall back to PATH search
    return "git";
#else
    // Check common Unix locations
    std::vector<std::string> paths = {
        "/usr/bin/git",
        "/usr/local/bin/git",
        "/opt/homebrew/bin/git"
    };
    for (const auto& p : paths) {
        if (fs::exists(p)) return p;
    }
    return "/usr/bin/git";  // Default
#endif
}

// ============================================================================
// SECURITY: Execute git without shell (no command injection)
// ============================================================================
std::string GitClient::run_git(const std::vector<std::string>& args) const {
    // SECURITY: Build argument vector (no shell interpretation)
    std::vector<std::string> full_args;
    full_args.push_back(git_binary_);
    full_args.push_back("-C");
    full_args.push_back(repo_path_);
    for (const auto& arg : args) {
        full_args.push_back(arg);
    }

    EventBus::push("GIT_CMD", "git " + (args.empty() ? "" : args[0]));

#ifdef _WIN32
    // Windows: Use CreateProcess without shell
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE stdout_read, stdout_write;
    if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0)) {
        last_error_ = "Failed to create pipe";
        return "";
    }
    SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(STARTUPINFOA);
    si.hStdOutput = stdout_write;
    si.hStdError = stdout_write;
    si.dwFlags |= STARTF_USESTDHANDLES;

    // Build command line (proper escaping for Windows)
    std::string cmd_line;
    for (size_t i = 0; i < full_args.size(); ++i) {
        if (i > 0) cmd_line += " ";
        // Quote if contains spaces
        if (full_args[i].find(' ') != std::string::npos) {
            cmd_line += "\"" + full_args[i] + "\"";
        } else {
            cmd_line += full_args[i];
        }
    }

    PROCESS_INFORMATION pi{};
    if (!CreateProcessA(nullptr, const_cast<char*>(cmd_line.c_str()),
                        nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                        nullptr, &si, &pi)) {
        CloseHandle(stdout_read);
        CloseHandle(stdout_write);
        last_error_ = "Failed to create process";
        return "";
    }
    CloseHandle(stdout_write);

    std::string result;
    char buffer[4096];
    DWORD bytes_read;
    while (ReadFile(stdout_read, buffer, sizeof(buffer) - 1, &bytes_read, nullptr) && bytes_read > 0) {
        buffer[bytes_read] = '\0';
        result += buffer;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    if (exit_code != 0) {
        last_error_ = result;
    }

    CloseHandle(stdout_read);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return result;

#else
    // POSIX: Use fork/execvp (no shell)
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        last_error_ = "Failed to create pipe";
        return "";
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        last_error_ = "Fork failed";
        return "";
    }

    if (pid == 0) {
        // Child process
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        // Build argv for execvp
        std::vector<const char*> argv;
        for (const auto& arg : full_args) {
            argv.push_back(arg.c_str());
        }
        argv.push_back(nullptr);

        // SECURITY: execvp with argument array (no shell)
        execvp(argv[0], const_cast<char* const*>(argv.data()));
        _exit(127);
    }

    // Parent process
    close(pipefd[1]);

    std::string result;
    char buffer[4096];
    ssize_t n;
    while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        result += buffer;
    }
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        last_error_ = result;
    }

    return result;
#endif
}

bool GitClient::run_git_bool(const std::vector<std::string>& args) const {
    // SECURITY: Use the same secure execution path
    std::string result = run_git(args);
    return last_error_.empty();
}

bool GitClient::execute_git(const std::vector<std::string>& args) const {
    return run_git_bool(args);
}

std::vector<std::string> GitClient::split_lines(const std::string& str) const {
    std::vector<std::string> lines;
    std::istringstream iss(str);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

bool GitClient::is_git_repo() const {
    return run_git_bool({"rev-parse", "--git-dir"});
}

std::string GitClient::get_repo_root() const {
    std::string result = run_git({"rev-parse", "--show-toplevel"});
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

std::string GitClient::get_current_branch() const {
    std::string result = run_git({"rev-parse", "--abbrev-ref", "HEAD"});
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

std::string GitClient::get_head_commit() const {
    std::string result = run_git({"rev-parse", "HEAD"});
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

GitStatus GitClient::status() const {
    GitStatus st;
    st.branch = get_current_branch();

    // Get upstream info
    std::string upstream = run_git({"rev-parse", "--abbrev-ref", "@{upstream}"});
    if (!upstream.empty() && upstream.back() == '\n') {
        upstream.pop_back();
    }
    if (upstream.find("fatal:") == std::string::npos) {
        st.upstream = upstream;

        // Get ahead/behind
        std::string ab = run_git({"rev-list", "--left-right", "--count", "HEAD...@{upstream}"});
        std::istringstream iss(ab);
        iss >> st.ahead >> st.behind;
    }

    // Parse porcelain v2 status
    std::string output = run_git({"status", "--porcelain=v2", "-uall"});
    auto lines = split_lines(output);

    for (const auto& line : lines) {
        if (line.empty()) continue;

        char type = line[0];
        if (type == '1' || type == '2') {
            // Changed entry
            // Format: 1 XY sub mH mI mW hH hI path
            // or:     2 XY sub mH mI mW hH hI X path\torigPath
            std::istringstream iss(line);
            std::string t, xy, sub, rest;
            iss >> t >> xy >> sub;

            // Get path (skip other fields)
            std::string path;
            for (int i = 0; i < 5; ++i) iss >> rest;
            std::getline(iss >> std::ws, path);

            // Handle tab-separated rename
            size_t tab = path.find('\t');
            if (tab != std::string::npos) {
                path = path.substr(0, tab);
            }

            char staged = xy[0];
            char working = xy[1];

            if (staged != '.') {
                if (staged == 'R') st.renamed.push_back(path);
                else if (staged == 'D') st.deleted.push_back(path);
                else st.staged.push_back(path);
            }
            if (working == 'M') st.modified.push_back(path);
            else if (working == 'D') st.deleted.push_back(path);

        } else if (type == 'u') {
            // Unmerged entry (conflict)
            std::istringstream iss(line);
            std::string t, xy, sub, rest;
            iss >> t >> xy >> sub;
            for (int i = 0; i < 7; ++i) iss >> rest;
            std::string path;
            std::getline(iss >> std::ws, path);
            st.conflicts.push_back(path);

        } else if (type == '?') {
            // Untracked
            std::string path = line.substr(2);
            st.untracked.push_back(path);
        }
    }

    return st;
}

std::vector<GitFileDiff> GitClient::diff(const std::string& ref1, const std::string& ref2) const {
    std::vector<std::string> args = {"diff", "--numstat"};
    if (!ref1.empty()) {
        args.push_back(ref1);
        if (!ref2.empty()) {
            args.push_back(ref2);
        }
    }

    std::string output = run_git(args);
    std::vector<GitFileDiff> diffs;
    auto lines = split_lines(output);

    for (const auto& line : lines) {
        std::istringstream iss(line);
        GitFileDiff fd;
        std::string adds, dels;
        iss >> adds >> dels >> fd.path;

        if (adds != "-") fd.additions = std::stoi(adds);
        if (dels != "-") fd.deletions = std::stoi(dels);
        fd.status = "M";

        diffs.push_back(fd);
    }

    return diffs;
}

std::vector<GitFileDiff> GitClient::diff_staged() const {
    return diff("--cached", "");
}

std::vector<GitFileDiff> GitClient::diff_file(const std::string& path) const {
    std::string output = run_git({"diff", "--numstat", "--", path});
    std::vector<GitFileDiff> diffs;

    if (!output.empty()) {
        std::istringstream iss(output);
        GitFileDiff fd;
        std::string adds, dels;
        iss >> adds >> dels >> fd.path;

        if (adds != "-") fd.additions = std::stoi(adds);
        if (dels != "-") fd.deletions = std::stoi(dels);
        fd.status = "M";

        diffs.push_back(fd);
    }

    return diffs;
}

std::vector<GitCommit> GitClient::log(int count, const std::string& branch) const {
    std::vector<std::string> args = {
        "log",
        "--format=%H|%h|%an|%ae|%s|%ci|%P",
        "-n", std::to_string(count)
    };
    if (!branch.empty()) {
        args.push_back(branch);
    }

    std::string output = run_git(args);
    std::vector<GitCommit> commits;
    auto lines = split_lines(output);

    for (const auto& line : lines) {
        GitCommit c;
        std::istringstream iss(line);
        std::string part;

        std::getline(iss, c.hash, '|');
        std::getline(iss, c.short_hash, '|');
        std::getline(iss, c.author, '|');
        std::getline(iss, c.email, '|');
        std::getline(iss, c.message, '|');
        std::getline(iss, c.date, '|');

        std::string parents;
        std::getline(iss, parents);
        std::istringstream piss(parents);
        std::string parent;
        while (piss >> parent) {
            c.parents.push_back(parent);
        }

        commits.push_back(c);
    }

    return commits;
}

GitCommit GitClient::get_commit(const std::string& ref) const {
    auto commits = log(1, ref);
    if (!commits.empty()) {
        return commits[0];
    }
    return GitCommit{};
}

std::string GitClient::commit(const std::string& message, bool amend) {
    std::vector<std::string> args = {"commit", "-m", message};
    if (amend) {
        args.push_back("--amend");
    }

    std::string output = run_git(args);

    // Extract commit hash from output
    std::regex re(R"(\[[\w/-]+ ([a-f0-9]+)\])");
    std::smatch match;
    if (std::regex_search(output, match, re)) {
        return match[1].str();
    }

    return "";
}

bool GitClient::add(const std::string& path) {
    return run_git_bool({"add", path});
}

bool GitClient::add_all() {
    return run_git_bool({"add", "-A"});
}

bool GitClient::reset(const std::string& path) {
    if (path.empty()) {
        return run_git_bool({"reset"});
    }
    return run_git_bool({"reset", "--", path});
}

bool GitClient::reset_hard(const std::string& ref) {
    return run_git_bool({"reset", "--hard", ref});
}

std::vector<GitBranch> GitClient::branches(bool include_remote) const {
    std::vector<std::string> args = {"branch", "-v", "--format=%(refname:short)|%(upstream:short)|%(objectname:short)|%(HEAD)"};
    if (include_remote) {
        args.push_back("-a");
    }

    std::string output = run_git(args);
    std::vector<GitBranch> result;
    auto lines = split_lines(output);

    for (const auto& line : lines) {
        GitBranch b;
        std::istringstream iss(line);
        std::string head;

        std::getline(iss, b.name, '|');
        std::getline(iss, b.upstream, '|');
        std::getline(iss, b.last_commit, '|');
        std::getline(iss, head);

        b.is_current = (head == "*");
        b.is_remote = (b.name.find("remotes/") == 0 || b.name.find("origin/") == 0);

        if (b.is_remote && b.name.find('/') != std::string::npos) {
            b.remote = b.name.substr(0, b.name.find('/'));
        }

        result.push_back(b);
    }

    return result;
}

bool GitClient::create_branch(const std::string& name, const std::string& start_point) {
    std::vector<std::string> args = {"branch", name};
    if (!start_point.empty()) {
        args.push_back(start_point);
    }
    return run_git_bool(args);
}

bool GitClient::checkout(const std::string& branch_or_ref) {
    return run_git_bool({"checkout", branch_or_ref});
}

bool GitClient::delete_branch(const std::string& name, bool force) {
    std::string flag = force ? "-D" : "-d";
    return run_git_bool({"branch", flag, name});
}

MergeResult GitClient::merge(const std::string& branch, bool no_ff) {
    MergeResult result;
    std::vector<std::string> args = {"merge", branch};
    if (no_ff) {
        args.push_back("--no-ff");
    }

    std::string output = run_git(args);

    if (output.find("CONFLICT") != std::string::npos) {
        result.has_conflicts = true;
        result.success = false;

        // Get conflicted files
        auto st = status();
        result.conflicted_files = st.conflicts;
        result.error = "Merge conflicts detected";
    } else if (output.find("Already up to date") != std::string::npos ||
               output.find("Merge made") != std::string::npos ||
               output.find("Fast-forward") != std::string::npos) {
        result.success = true;
        result.merged_commit = get_head_commit();
    } else {
        result.success = false;
        result.error = output;
    }

    return result;
}

bool GitClient::rebase(const std::string& upstream) {
    return run_git_bool({"rebase", upstream});
}

std::vector<GitRemote> GitClient::remotes() const {
    std::string output = run_git({"remote", "-v"});
    std::vector<GitRemote> result;
    std::map<std::string, GitRemote> rmap;

    auto lines = split_lines(output);
    for (const auto& line : lines) {
        std::istringstream iss(line);
        std::string name, url, type;
        iss >> name >> url >> type;

        auto& r = rmap[name];
        r.name = name;
        if (type == "(fetch)") {
            r.fetch_url = url;
        } else if (type == "(push)") {
            r.push_url = url;
        }
    }

    for (auto& [name, remote] : rmap) {
        result.push_back(remote);
    }

    return result;
}

bool GitClient::fetch(const std::string& remote) {
    return run_git_bool({"fetch", remote});
}

bool GitClient::pull(const std::string& remote, const std::string& branch) {
    std::vector<std::string> args = {"pull", remote};
    if (!branch.empty()) {
        args.push_back(branch);
    }
    return run_git_bool(args);
}

bool GitClient::push(const std::string& remote, const std::string& branch, bool force) {
    std::vector<std::string> args = {"push", remote};
    if (!branch.empty()) {
        args.push_back(branch);
    }
    if (force) {
        args.push_back("--force");
    }
    return run_git_bool(args);
}

std::vector<GitTag> GitClient::tags() const {
    std::string output = run_git({"tag", "-l", "--format=%(refname:short)|%(objectname:short)|%(contents:subject)|%(objecttype)"});
    std::vector<GitTag> result;

    auto lines = split_lines(output);
    for (const auto& line : lines) {
        GitTag t;
        std::istringstream iss(line);
        std::string type;

        std::getline(iss, t.name, '|');
        std::getline(iss, t.commit, '|');
        std::getline(iss, t.message, '|');
        std::getline(iss, type);

        t.is_annotated = (type == "tag");
        result.push_back(t);
    }

    return result;
}

bool GitClient::create_tag(const std::string& name, const std::string& message, const std::string& ref) {
    std::vector<std::string> args = {"tag"};
    if (!message.empty()) {
        args.push_back("-a");
        args.push_back(name);
        args.push_back("-m");
        args.push_back(message);
    } else {
        args.push_back(name);
    }
    args.push_back(ref);
    return run_git_bool(args);
}

bool GitClient::delete_tag(const std::string& name) {
    return run_git_bool({"tag", "-d", name});
}

std::vector<GitStash> GitClient::stash_list() const {
    std::string output = run_git({"stash", "list", "--format=%gd|%s|%ci"});
    std::vector<GitStash> result;

    auto lines = split_lines(output);
    int index = 0;
    for (const auto& line : lines) {
        GitStash s;
        s.index = index++;

        std::istringstream iss(line);
        std::getline(iss, s.ref, '|');
        std::getline(iss, s.message, '|');
        std::getline(iss, s.date);

        result.push_back(s);
    }

    return result;
}

bool GitClient::stash_push(const std::string& message) {
    std::vector<std::string> args = {"stash", "push"};
    if (!message.empty()) {
        args.push_back("-m");
        args.push_back(message);
    }
    return run_git_bool(args);
}

bool GitClient::stash_pop(int index) {
    return run_git_bool({"stash", "pop", "stash@{" + std::to_string(index) + "}"});
}

bool GitClient::stash_drop(int index) {
    return run_git_bool({"stash", "drop", "stash@{" + std::to_string(index) + "}"});
}

std::string GitClient::show_file(const std::string& ref, const std::string& path) const {
    return run_git({"show", ref + ":" + path});
}

std::vector<std::string> GitClient::ls_files(const std::string& pattern) const {
    std::vector<std::string> args = {"ls-files"};
    if (!pattern.empty()) {
        args.push_back(pattern);
    }
    std::string output = run_git(args);
    return split_lines(output);
}

std::vector<GitClient::BlameLine> GitClient::blame(const std::string& path) const {
    std::string output = run_git({"blame", "--line-porcelain", path});
    std::vector<BlameLine> result;

    auto lines = split_lines(output);
    BlameLine current;
    bool in_header = true;

    for (const auto& line : lines) {
        if (line[0] == '\t') {
            // Content line
            current.content = line.substr(1);
            result.push_back(current);
            current = BlameLine{};
            in_header = true;
        } else if (in_header) {
            // Header lines
            if (line.length() >= 40 && std::isxdigit(line[0])) {
                // Commit line
                std::istringstream iss(line);
                iss >> current.commit;
                int orig_line, final_line;
                iss >> orig_line >> final_line;
                current.line_number = final_line;
            } else if (line.find("author ") == 0) {
                current.author = line.substr(7);
            } else if (line.find("author-time ") == 0) {
                // Convert timestamp to readable date
                long ts = std::stol(line.substr(12));
                auto tp = std::chrono::system_clock::from_time_t(ts);
                auto t = std::chrono::system_clock::to_time_t(tp);
                char buf[32];
                std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&t));
                current.date = buf;
            }
        }
    }

    return result;
}

std::string GitClient::get_config(const std::string& key) const {
    std::string result = run_git({"config", "--get", key});
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

bool GitClient::set_config(const std::string& key, const std::string& value, bool global) {
    std::vector<std::string> args = {"config"};
    if (global) {
        args.push_back("--global");
    }
    args.push_back(key);
    args.push_back(value);
    return run_git_bool(args);
}

// ═══════════════════════════════════════════════════════════════════
//  CHANGESET TRACKER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

ChangeSetTracker::ChangeSetTracker(GitClient& git) : git_(git) {}

std::string ChangeSetTracker::generate_id() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    const char* hex = "0123456789abcdef";
    std::string id;
    for (int i = 0; i < 8; ++i) {
        id += hex[dis(gen)];
    }
    return id;
}

ChangeSet ChangeSetTracker::create_changeset(const std::string& description) {
    ChangeSet cs;
    cs.id = generate_id();
    cs.description = description;
    cs.base_commit = git_.get_head_commit();
    cs.changes = git_.diff();
    cs.created = std::chrono::system_clock::now();

    changesets_[cs.id] = cs;
    return cs;
}

std::optional<ChangeSet> ChangeSetTracker::get_changeset(const std::string& id) const {
    auto it = changesets_.find(id);
    if (it != changesets_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<ChangeSet> ChangeSetTracker::list_changesets() const {
    std::vector<ChangeSet> result;
    for (const auto& [id, cs] : changesets_) {
        result.push_back(cs);
    }
    return result;
}

bool ChangeSetTracker::apply_changeset(const std::string& id) {
    auto cs = get_changeset(id);
    if (!cs) return false;

    // Cherry-pick if we have a head commit
    if (!cs->head_commit.empty()) {
        return git_.execute_git({"cherry-pick", cs->head_commit});
    }
    return false;
}

bool ChangeSetTracker::revert_changeset(const std::string& id) {
    auto cs = get_changeset(id);
    if (!cs) return false;

    if (!cs->head_commit.empty()) {
        return git_.execute_git({"revert", "--no-commit", cs->head_commit});
    }
    return false;
}

void ChangeSetTracker::cleanup(int max_age_days) {
    auto now = std::chrono::system_clock::now();
    auto max_age = std::chrono::hours(24 * max_age_days);

    std::vector<std::string> to_remove;
    for (const auto& [id, cs] : changesets_) {
        if (now - cs.created > max_age) {
            to_remove.push_back(id);
        }
    }

    for (const auto& id : to_remove) {
        changesets_.erase(id);
    }
}

// ═══════════════════════════════════════════════════════════════════
//  CONFLICT RESOLVER IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

std::vector<ConflictMarker> ConflictResolver::parse_conflicts(const std::string& content) {
    std::vector<ConflictMarker> conflicts;
    std::istringstream iss(content);
    std::string line;
    int line_num = 0;

    ConflictMarker current;
    bool in_ours = false;
    bool in_theirs = false;

    while (std::getline(iss, line)) {
        ++line_num;

        if (line.find("<<<<<<<") == 0) {
            current = ConflictMarker{};
            current.start_line = line_num;
            in_ours = true;
            in_theirs = false;
        } else if (line.find("=======") == 0) {
            current.separator_line = line_num;
            in_ours = false;
            in_theirs = true;
        } else if (line.find(">>>>>>>") == 0) {
            current.end_line = line_num;
            in_ours = false;
            in_theirs = false;
            conflicts.push_back(current);
        } else if (in_ours) {
            current.ours.push_back(line);
        } else if (in_theirs) {
            current.theirs.push_back(line);
        }
    }

    return conflicts;
}

bool ConflictResolver::has_conflicts(const std::string& content) {
    return content.find("<<<<<<<") != std::string::npos &&
           content.find("=======") != std::string::npos &&
           content.find(">>>>>>>") != std::string::npos;
}

std::string ConflictResolver::resolve_conflict(const ConflictMarker& conflict, Strategy strategy) {
    std::string result;

    switch (strategy) {
        case Strategy::OURS:
            for (const auto& line : conflict.ours) {
                result += line + "\n";
            }
            break;
        case Strategy::THEIRS:
            for (const auto& line : conflict.theirs) {
                result += line + "\n";
            }
            break;
        case Strategy::BOTH:
            for (const auto& line : conflict.ours) {
                result += line + "\n";
            }
            for (const auto& line : conflict.theirs) {
                result += line + "\n";
            }
            break;
        case Strategy::MANUAL:
            // Return original conflict markers
            result = "<<<<<<< OURS\n";
            for (const auto& line : conflict.ours) {
                result += line + "\n";
            }
            result += "=======\n";
            for (const auto& line : conflict.theirs) {
                result += line + "\n";
            }
            result += ">>>>>>> THEIRS\n";
            break;
    }

    return result;
}

std::string ConflictResolver::resolve_all(const std::string& content, Strategy strategy) {
    if (!has_conflicts(content)) {
        return content;
    }

    std::string result;
    std::istringstream iss(content);
    std::string line;

    ConflictMarker current;
    bool in_conflict = false;
    bool in_ours = false;
    bool in_theirs = false;

    while (std::getline(iss, line)) {
        if (line.find("<<<<<<<") == 0) {
            in_conflict = true;
            in_ours = true;
            in_theirs = false;
            current = ConflictMarker{};
        } else if (line.find("=======") == 0 && in_conflict) {
            in_ours = false;
            in_theirs = true;
        } else if (line.find(">>>>>>>") == 0 && in_conflict) {
            // End of conflict - resolve and add
            result += resolve_conflict(current, strategy);
            in_conflict = false;
            in_ours = false;
            in_theirs = false;
        } else if (in_ours) {
            current.ours.push_back(line);
        } else if (in_theirs) {
            current.theirs.push_back(line);
        } else {
            result += line + "\n";
        }
    }

    return result;
}

ConflictResolver::Strategy ConflictResolver::suggest_resolution(const ConflictMarker& conflict) {
    // Simple heuristic: if one side is empty, use the other
    if (conflict.ours.empty() && !conflict.theirs.empty()) {
        return Strategy::THEIRS;
    }
    if (conflict.theirs.empty() && !conflict.ours.empty()) {
        return Strategy::OURS;
    }

    // If sides are identical, use either
    if (conflict.ours == conflict.theirs) {
        return Strategy::OURS;
    }

    // Default to manual resolution
    return Strategy::MANUAL;
}

// ═══════════════════════════════════════════════════════════════════
//  GIT HOOKS IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

GitHooks::GitHooks(const std::string& repo_path) : repo_path_(repo_path) {}

std::string GitHooks::get_hooks_dir() const {
    return repo_path_ + "/.git/hooks";
}

std::string GitHooks::hook_type_to_name(HookType type) const {
    switch (type) {
        case HookType::PRE_COMMIT: return "pre-commit";
        case HookType::PREPARE_COMMIT_MSG: return "prepare-commit-msg";
        case HookType::COMMIT_MSG: return "commit-msg";
        case HookType::POST_COMMIT: return "post-commit";
        case HookType::PRE_PUSH: return "pre-push";
        case HookType::POST_MERGE: return "post-merge";
        case HookType::PRE_REBASE: return "pre-rebase";
    }
    return "";
}

bool GitHooks::install_hook(HookType type, const std::string& script) {
    std::string path = get_hooks_dir() + "/" + hook_type_to_name(type);
    std::ofstream ofs(path);
    if (!ofs) return false;

    ofs << "#!/bin/sh\n" << script;
    ofs.close();

    // Make executable
    return system(("chmod +x \"" + path + "\"").c_str()) == 0;
}

bool GitHooks::remove_hook(HookType type) {
    std::string path = get_hooks_dir() + "/" + hook_type_to_name(type);
    return std::remove(path.c_str()) == 0;
}

bool GitHooks::hook_exists(HookType type) const {
    std::string path = get_hooks_dir() + "/" + hook_type_to_name(type);
    std::ifstream ifs(path);
    return ifs.good();
}

std::string GitHooks::get_hook(HookType type) const {
    std::string path = get_hooks_dir() + "/" + hook_type_to_name(type);
    std::ifstream ifs(path);
    if (!ifs) return "";

    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

bool GitHooks::run_hook(HookType type, const std::vector<std::string>& args) {
    std::string path = get_hooks_dir() + "/" + hook_type_to_name(type);
    if (!hook_exists(type)) return true;  // No hook = success

    std::string cmd = "\"" + path + "\"";
    for (const auto& arg : args) {
        cmd += " \"" + arg + "\"";
    }

    return system(cmd.c_str()) == 0;
}

} // namespace rael
