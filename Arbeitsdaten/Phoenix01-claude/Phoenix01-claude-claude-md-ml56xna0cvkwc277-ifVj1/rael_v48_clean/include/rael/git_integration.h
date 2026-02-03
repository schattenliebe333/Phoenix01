// RAEL V48 - Git/VCS Integration (#14)
// Provides version control operations for autonomous programming
#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <optional>
#include <functional>

namespace rael {

// ═══════════════════════════════════════════════════════════════════
//  STRUCTS
// ═══════════════════════════════════════════════════════════════════

struct GitCommit {
    std::string hash;
    std::string short_hash;
    std::string author;
    std::string email;
    std::string message;
    std::string date;
    std::vector<std::string> parents;
    std::vector<std::string> changed_files;
};

struct GitBranch {
    std::string name;
    std::string remote;
    std::string upstream;
    bool is_current = false;
    bool is_remote = false;
    std::string last_commit;
};

struct GitDiffHunk {
    int old_start = 0;
    int old_count = 0;
    int new_start = 0;
    int new_count = 0;
    std::string header;
    std::vector<std::string> lines;
};

struct GitFileDiff {
    std::string path;
    std::string old_path;  // For renames
    std::string status;    // A=Added, M=Modified, D=Deleted, R=Renamed
    int additions = 0;
    int deletions = 0;
    std::vector<GitDiffHunk> hunks;
};

struct GitStatus {
    std::string branch;
    std::string upstream;
    int ahead = 0;
    int behind = 0;
    std::vector<std::string> staged;
    std::vector<std::string> modified;
    std::vector<std::string> untracked;
    std::vector<std::string> deleted;
    std::vector<std::string> renamed;
    std::vector<std::string> conflicts;
    bool is_clean() const {
        return staged.empty() && modified.empty() && untracked.empty() &&
               deleted.empty() && conflicts.empty();
    }
};

struct GitStash {
    int index = 0;
    std::string ref;
    std::string message;
    std::string branch;
    std::string date;
};

struct GitRemote {
    std::string name;
    std::string fetch_url;
    std::string push_url;
};

struct GitTag {
    std::string name;
    std::string commit;
    std::string message;
    bool is_annotated = false;
};

struct MergeResult {
    bool success = false;
    bool has_conflicts = false;
    std::vector<std::string> conflicted_files;
    std::string merged_commit;
    std::string error;
};

struct ChangeSet {
    std::string id;
    std::string description;
    std::vector<GitFileDiff> changes;
    std::string base_commit;
    std::string head_commit;
    std::chrono::system_clock::time_point created;
};

// ═══════════════════════════════════════════════════════════════════
//  GIT CLIENT
// ═══════════════════════════════════════════════════════════════════

class GitClient {
public:
    explicit GitClient(const std::string& repo_path = ".");

    // Repository info
    bool is_git_repo() const;
    std::string get_repo_root() const;
    std::string get_current_branch() const;
    std::string get_head_commit() const;

    // Status
    GitStatus status() const;
    std::vector<GitFileDiff> diff(const std::string& ref1 = "", const std::string& ref2 = "") const;
    std::vector<GitFileDiff> diff_staged() const;
    std::vector<GitFileDiff> diff_file(const std::string& path) const;

    // Commits
    std::vector<GitCommit> log(int count = 10, const std::string& branch = "") const;
    GitCommit get_commit(const std::string& ref) const;
    std::string commit(const std::string& message, bool amend = false);

    // Staging
    bool add(const std::string& path);
    bool add_all();
    bool reset(const std::string& path = "");
    bool reset_hard(const std::string& ref = "HEAD");

    // Branches
    std::vector<GitBranch> branches(bool include_remote = false) const;
    bool create_branch(const std::string& name, const std::string& start_point = "");
    bool checkout(const std::string& branch_or_ref);
    bool delete_branch(const std::string& name, bool force = false);
    MergeResult merge(const std::string& branch, bool no_ff = false);
    bool rebase(const std::string& upstream);

    // Remote operations
    std::vector<GitRemote> remotes() const;
    bool fetch(const std::string& remote = "origin");
    bool pull(const std::string& remote = "origin", const std::string& branch = "");
    bool push(const std::string& remote = "origin", const std::string& branch = "", bool force = false);

    // Tags
    std::vector<GitTag> tags() const;
    bool create_tag(const std::string& name, const std::string& message = "", const std::string& ref = "HEAD");
    bool delete_tag(const std::string& name);

    // Stash
    std::vector<GitStash> stash_list() const;
    bool stash_push(const std::string& message = "");
    bool stash_pop(int index = 0);
    bool stash_drop(int index = 0);

    // File operations
    std::string show_file(const std::string& ref, const std::string& path) const;
    std::vector<std::string> ls_files(const std::string& pattern = "") const;

    // Blame
    struct BlameLine {
        std::string commit;
        std::string author;
        std::string date;
        int line_number;
        std::string content;
    };
    std::vector<BlameLine> blame(const std::string& path) const;

    // Configuration
    std::string get_config(const std::string& key) const;
    bool set_config(const std::string& key, const std::string& value, bool global = false);

    // Error handling
    std::string last_error() const { return last_error_; }

    // For internal use by related classes
    bool execute_git(const std::vector<std::string>& args) const;

private:
    std::string repo_path_;
    mutable std::string last_error_;

    std::string run_git(const std::vector<std::string>& args) const;
    bool run_git_bool(const std::vector<std::string>& args) const;
    std::vector<std::string> split_lines(const std::string& str) const;
    GitFileDiff parse_diff_file(const std::string& diff_output) const;
};

// ═══════════════════════════════════════════════════════════════════
//  CHANGESET TRACKER
// ═══════════════════════════════════════════════════════════════════

class ChangeSetTracker {
public:
    explicit ChangeSetTracker(GitClient& git);

    // Create a new changeset from current changes
    ChangeSet create_changeset(const std::string& description);

    // Get changeset by ID
    std::optional<ChangeSet> get_changeset(const std::string& id) const;

    // List all changesets
    std::vector<ChangeSet> list_changesets() const;

    // Apply a changeset (checkout + cherry-pick or patch)
    bool apply_changeset(const std::string& id);

    // Revert a changeset
    bool revert_changeset(const std::string& id);

    // Clear old changesets
    void cleanup(int max_age_days = 30);

private:
    GitClient& git_;
    std::map<std::string, ChangeSet> changesets_;
    std::string generate_id() const;
};

// ═══════════════════════════════════════════════════════════════════
//  CONFLICT RESOLVER
// ═══════════════════════════════════════════════════════════════════

struct ConflictMarker {
    int start_line;
    int separator_line;
    int end_line;
    std::vector<std::string> ours;
    std::vector<std::string> theirs;
    std::string base;  // For 3-way merge
};

class ConflictResolver {
public:
    // Parse conflict markers from file
    static std::vector<ConflictMarker> parse_conflicts(const std::string& content);

    // Check if file has conflicts
    static bool has_conflicts(const std::string& content);

    // Resolution strategies
    enum class Strategy {
        OURS,       // Keep our version
        THEIRS,     // Keep their version
        BOTH,       // Keep both (concatenate)
        MANUAL      // Requires manual resolution
    };

    // Resolve a single conflict
    static std::string resolve_conflict(const ConflictMarker& conflict, Strategy strategy);

    // Resolve all conflicts in file
    static std::string resolve_all(const std::string& content, Strategy strategy);

    // Get suggested resolution based on conflict content
    static Strategy suggest_resolution(const ConflictMarker& conflict);
};

// ═══════════════════════════════════════════════════════════════════
//  GIT HOOKS
// ═══════════════════════════════════════════════════════════════════

class GitHooks {
public:
    explicit GitHooks(const std::string& repo_path);

    enum class HookType {
        PRE_COMMIT,
        PREPARE_COMMIT_MSG,
        COMMIT_MSG,
        POST_COMMIT,
        PRE_PUSH,
        POST_MERGE,
        PRE_REBASE
    };

    // Install a hook
    bool install_hook(HookType type, const std::string& script);

    // Remove a hook
    bool remove_hook(HookType type);

    // Check if hook exists
    bool hook_exists(HookType type) const;

    // Get hook content
    std::string get_hook(HookType type) const;

    // Run a hook manually
    bool run_hook(HookType type, const std::vector<std::string>& args = {});

private:
    std::string repo_path_;
    std::string hook_type_to_name(HookType type) const;
    std::string get_hooks_dir() const;
};

} // namespace rael
