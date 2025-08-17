#pragma once
#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <unordered_map>
#include <ctime>
#include "../commands/init.hpp"

namespace indexFunctions {

// Represents an entry in the Git index
struct GitIndexEntry {
    std::pair<int64_t, int64_t> ctime; // Metadata change time (seconds, nanoseconds)
    std::pair<int64_t, int64_t> mtime; // Data change time (seconds, nanoseconds)
    uint32_t dev;                     // Device ID
    uint32_t ino;                     // Inode number
    uint32_t mode_type;               // Object type (e.g., regular file, symlink, gitlink)
    uint32_t mode_perms;              // Object permissions
    uint32_t uid;                     // User ID of owner
    uint32_t gid;                     // Group ID of owner
    uint32_t fsize;                   // File size in bytes
    std::string sha;                  // SHA-1 hash of the object
    bool flag_assume_valid;           // Assume-valid flag
    uint16_t flag_stage;              // Stage flag
    std::string name;                 // Full path of the object

    // Constructor
    GitIndexEntry(
        std::pair<int64_t, int64_t> ctime = {0, 0},
        std::pair<int64_t, int64_t> mtime = {0, 0},
        uint32_t dev = 0,
        uint32_t ino = 0,
        uint32_t mode_type = 0,
        uint32_t mode_perms = 0,
        uint32_t uid = 0,
        uint32_t gid = 0,
        uint32_t fsize = 0,
        const std::string& sha = "",
        bool flag_assume_valid = false,
        uint16_t flag_stage = 0,
        const std::string& name = ""
    );
};

// Represents the Git index
class GitIndex {
public:
    uint32_t version;                 // Index version
    std::vector<GitIndexEntry> entries; // List of index entries

    // Load the index from the repository
    void load(gitDirectory *gd);

    // Print the index entries
    void print() const;
};

// API function to show the index entries
void ls_files(gitDirectory *gd, bool verbose);

// API function to get the active branch
std::string branch_get_active(gitDirectory *gd);

// Command to display the current branch
void cmd_status_branch(gitDirectory *gd);

// Convert a tree object to a dictionary of file paths and their SHAs
void tree_to_dict(gitDirectory *gd, std::string &name, std::string &prefix, std::unordered_map<std::string, std::string> &mp);

// Command to compare HEAD and index for changes
void cmd_status_head_index(gitDirectory *gd, GitIndex *gi);

// Status command to display branch and changes
void status_cmd(gitDirectory *gd);

// rm command to rem files from the local system as well as index system with options , skirk rm (--cache) <files>;
void rm(gitDirectory *gd,const std::vector<std::string> &paths, bool delete_files, bool skip_missing);
void add(gitDirectory *gd,const std::vector<std::string> &paths, bool delete_files, bool skip_missing);

// Build a tree object from the index and return its SHA-1
std::string tree_from_index(gitDirectory *gd, const class GitIndex &gi);

// Create a commit object and return its SHA-1
std::string commit_create(gitDirectory *gd,const std::string &tree,const std::string &parent,const std::string &author,std::time_t timestamp,const std::string &message);

// Commit command worker: builds tree from index, creates commit, updates refs
std::string commit_cmd(gitDirectory *gd, const std::string &message);

} // namespace indexFunctions