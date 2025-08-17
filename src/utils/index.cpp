#include "index.hpp"
#include <string>
#include <utility>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <filesystem>
#include "../commands/init.hpp"
#include <iomanip>
#include <pwd.h>
#include <grp.h>
#include <ctime>
#include "fileProd.hpp"
#include"objectClasses.hpp"
#include <iterator>
#include <sys/stat.h>
#include <set>
#include <map>
#include "../commands/hash-file.hpp"
#include <sstream>
#include <iostream>
// ...existing includes...
static inline void debug_log(const char* tag, const std::string& msg) {
    std::cerr << "[DEBUG][" << tag << "] " << msg << std::endl;
}
using namespace std;
namespace fs = std::filesystem;

namespace indexFunctions {

//--- GitIndexEntry constructor ---
GitIndexEntry::GitIndexEntry(
    std::pair<int64_t, int64_t> ctime,
    std::pair<int64_t, int64_t> mtime,
    uint32_t dev,
    uint32_t ino,
    uint32_t mode_type,
    uint32_t mode_perms,
    uint32_t uid,
    uint32_t gid,
    uint32_t fsize,
    const std::string& sha,
    bool flag_assume_valid,
    uint16_t flag_stage,
    const std::string& name
)
    : ctime(ctime), mtime(mtime), dev(dev), ino(ino), mode_type(mode_type),
      mode_perms(mode_perms), uid(uid), gid(gid), fsize(fsize), sha(sha),
      flag_assume_valid(flag_assume_valid), flag_stage(flag_stage), name(name)
{
    debug_log("GitIndexEntry", "Constructed for " + name);
}

// --- GitIndex member functions ---
void GitIndex::load(gitDirectory *gd) {
    debug_log("GitIndex::load", "Loading index from " + gd->netpath + "/.mygit/index");
    string index_file = gd->netpath + "/.mygit/index";
    if (!fs::exists(index_file)) {
        debug_log("GitIndex::load", "Index file not found; initializing empty index (v2)");
        version = 2;
        entries.clear();
        return;
    }
    ifstream f(index_file, ios::binary);
    if (!f) {
        debug_log("GitIndex::load", "Could not open index file: " + index_file);
        version = 2;
        entries.clear();
        return;
    }
    vector<uint8_t> raw((istreambuf_iterator<char>(f)), {});
    debug_log("GitIndex::load", "Read " + std::to_string(raw.size()) + " bytes from index");
    if (raw.size() < 12) {
        debug_log("GitIndex::load", "Invalid index file (too small)");
        return;
    }
    string signature(raw.begin(), raw.begin() + 4);
    if (signature != "DIRC") {
        debug_log("GitIndex::load", "Invalid index signature: " + signature);
        return;
    }
    version = (raw[4] << 24) | (raw[5] << 16) | (raw[6] << 8) | raw[7];
    uint32_t count = (raw[8] << 24) | (raw[9] << 16) | (raw[10] << 8) | raw[11];
    debug_log("GitIndex::load", "version=" + std::to_string(version) + " entries=" + std::to_string(count));

    size_t idx = 12;
    entries.clear();

    for (uint32_t i = 0; i < count; i++) {
        GitIndexEntry entry;
        entry.ctime.first  = (raw[idx] << 24) | (raw[idx + 1] << 16) | (raw[idx + 2] << 8) | raw[idx + 3];
        entry.ctime.second = (raw[idx + 4] << 24) | (raw[idx + 5] << 16) | (raw[idx + 6] << 8) | raw[idx + 7];
        entry.mtime.first  = (raw[idx + 8] << 24) | (raw[idx + 9] << 16) | (raw[idx + 10] << 8) | raw[idx + 11];
        entry.mtime.second = (raw[idx + 12] << 24) | (raw[idx + 13] << 16) | (raw[idx + 14] << 8) | raw[idx + 15];
        entry.dev          = (raw[idx + 16] << 24) | (raw[idx + 17] << 16) | (raw[idx + 18] << 8) | raw[idx + 19];
        entry.ino          = (raw[idx + 20] << 24) | (raw[idx + 21] << 16) | (raw[idx + 22] << 8) | raw[idx + 23];
        uint16_t unused    = (raw[idx + 24] << 8) | raw[idx + 25];
        uint16_t mode      = (raw[idx + 26] << 8) | raw[idx + 27];
        entry.mode_type    = mode >> 12;
        entry.mode_perms   = mode & 0b0000000111111111;
        entry.uid          = (raw[idx + 28] << 24) | (raw[idx + 29] << 16) | (raw[idx + 30] << 8) | raw[idx + 31];
        entry.gid          = (raw[idx + 32] << 24) | (raw[idx + 33] << 16) | (raw[idx + 34] << 8) | raw[idx + 35];
        entry.fsize        = (raw[idx + 36] << 24) | (raw[idx + 37] << 16) | (raw[idx + 38] << 8) | raw[idx + 39];
        // SHA1
        char hex[41];
        for (int j = 0; j < 20; ++j)
            sprintf(hex + j * 2, "%02x", raw[idx + 40 + j]);
        hex[40] = '\0';
        entry.sha = string(hex);
        uint16_t flags = (raw[idx + 60] << 8) | raw[idx + 61];
        entry.flag_assume_valid = (flags & 0b1000000000000000) != 0;
        entry.flag_stage        = flags & 0b0011000000000000;
        uint16_t name_length    = flags & 0b0000111111111111;

        idx += 62;
        if (name_length < 0xFFF) {
            entry.name.assign(raw.begin() + idx, raw.begin() + idx + name_length);
            idx += name_length + 1;
        } else {
            auto null_pos = find(raw.begin() + idx + 0xFFF, raw.end(), '\0');
            entry.name.assign(raw.begin() + idx, null_pos);
            idx = (null_pos - raw.begin()) + 1;
        }
        idx = ((idx + 7) / 8) * 8;
        entries.push_back(entry);
    }
    debug_log("GitIndex::load", "Loaded entries=" + std::to_string(entries.size()));
}

void GitIndex::print() const {
    debug_log("GitIndex::print", "Printing index");
    cout << "Index version: " << version << endl;
    cout << "Entries: " << entries.size() << endl;
    for (const auto &e : entries) {
        cout << e.name << "  (" << e.sha << ")" << endl;
    }
}

// Print a single index entry, verbose or not
void print_index_entry(const GitIndexEntry &e, bool verbose) {
    debug_log("print_index_entry", "Printing entry " + e.name);
    cout << e.name << endl;
    if (!verbose) return;

    string entry_type;
    switch (e.mode_type) {
        case 0b1000: entry_type = "regular file"; break;
        case 0b1010: entry_type = "symlink"; break;
        case 0b1110: entry_type = "git link"; break;
        default: entry_type = "unknown"; break;
    }
    cout << "  " << entry_type << " with perms: " << oct << e.mode_perms << dec << endl;
    cout << "  on blob: " << e.sha << endl;

    cout << "  created: " << put_time(localtime(reinterpret_cast<const time_t*>(&e.ctime.first)), "%Y-%m-%d %H:%M:%S") << "." << e.ctime.second
              << ", modified: " << put_time(localtime(reinterpret_cast<const time_t*>(&e.mtime.first)), "%Y-%m-%d %H:%M:%S") << "." << e.mtime.second << endl;

    cout << "  device: " << e.dev << ", inode: " << e.ino << endl;

    // User and group names (POSIX only)
    struct passwd *pw = getpwuid(e.uid);
    struct group  *gr = getgrgid(e.gid);
    string user = pw ? pw->pw_name : to_string(e.uid);
    string group = gr ? gr->gr_name : to_string(e.gid);

    cout << "  user: " << user << " (" << e.uid << ")  group: " << group << " (" << e.gid << ")" << endl;
    cout << "  flags: stage=" << e.flag_stage << " assume_valid=" << (e.flag_assume_valid ? "true" : "false") << endl;
}

// API function to load and print index entries, verbose or not
void ls_files(gitDirectory *gd, bool verbose) {
    debug_log("ls_files", "Listing files, verbose=" + std::string(verbose ? "true" : "false"));
    GitIndex idx;
    idx.load(gd);
    if (verbose) {
        std::cout << "Index file format v" << idx.version << ", containing " << idx.entries.size() << " entries." << std::endl;
    }
    for (const auto &e : idx.entries) {
        print_index_entry(e, verbose);
    }
}


// status command implementation 

string branch_get_active(gitDirectory *gd) {
    debug_log("branch_get_active", "Reading HEAD");
    ifstream f(gd->netpath + "/.mygit/HEAD");
    if (!f.is_open()) {
        cerr << "Error: Could not open HEAD file." << endl;
        return "";
    }

    string content((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    if (!content.empty() && content.back() == '\n') {
        content.pop_back();
    }

    if (content.rfind("ref: refs/heads/") == 0) {
        debug_log("branch_get_active", "Active branch: " + content.substr(16));
        return content.substr(16);
    } else {
        debug_log("branch_get_active", "Detached HEAD");
        return "";
    }
}

void cmd_status_branch(gitDirectory *gd){
    debug_log("cmd_status_branch", "Checking branch status");
    string branch=branch_get_active(gd);
    if(branch==""){
        // head literal'
        string head="HEAD";
        string fmt="";
        string path=gd->netpath+"/.mygit/HEAD";
        string sha=fileFunctions::objectFind(path,head,fmt);
        cout<<"head detached at "<<sha<<endl;
        return;
    }
    else {
       cout<<branch<<endl;
       return;
    }
}

void tree_to_dict(gitDirectory *gd,string &name , string &prefix, unordered_map<string,string> &mp){
    debug_log("tree_to_dict", "Building tree dict for " + name + " with prefix " + prefix);
    string fmt="tree";
    string sha=fileFunctions::objectFind(gd->netpath,name,fmt,gd);
    string root=gd->netpath;
    Tree* v=dynamic_cast<Tree*>(fileFunctions::readObject(sha,root).get());

    v->deserialize();
    for(const auto& el: v->leafNodes){
        string full_path=prefix+'/'+el->path;
         if(el->mode.rfind("04")==0){
            tree_to_dict(gd,el->sha,full_path,mp);
         }   
         else {
            mp[full_path]=el->sha;
         }    
    }
}

void cmd_status_head_index(gitDirectory *gd , GitIndex *gi){
    debug_log("cmd_status_head_index", "Comparing HEAD and index");
    cout<<"Changes to be Commited"<<endl;
    unordered_map<string ,string> tree;
    string prefix="";
    string name="HEAD";
    tree_to_dict(gd,name,prefix,tree);
    for(auto el:gi->entries){
       if(tree.find(el.name)!=tree.end())
       {
        if(tree[el.name]!=el.sha){
            cout<<" modified : "<<el.name<<endl;
            tree.erase(el.name);
        }
       }
       else{
            cout<<" added : "<<el.name<<endl;
       }
    }
    for(auto el:gi->entries){
        cout<<" deleted : "<<el.name<<endl;
    }
}

void status_cmd(gitDirectory *gd){
    debug_log("status_cmd", "Running status command");
    string root=gd->netpath;
    GitIndex* index=new GitIndex();
    index->load(gd);
    cmd_status_branch(gd);
    cmd_status_head_index(gd,index);
    cout<<endl;
}

// Write index to .mygit/index (Big Endian), mirroring wyag's index_write
void write_index(gitDirectory *gd, const GitIndex &index) {
    debug_log("write_index", "Writing index with " + std::to_string(index.entries.size()) + " entries");
    const std::string index_file = gd->netpath + "/.mygit/index";
    debug_log("write_index","name of index "+index_file);
    std::filesystem::create_directories(std::filesystem::path(index_file).parent_path());
    std::ofstream f(index_file, std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "Could not open index file for writing: " << index_file << std::endl;
        return;
    }

    auto write_be32 = [&](uint32_t v) {// read how this functions wroks 
        unsigned char b[4] = {
            static_cast<unsigned char>((v >> 24) & 0xFF),
            static_cast<unsigned char>((v >> 16) & 0xFF),
            static_cast<unsigned char>((v >> 8) & 0xFF),
            static_cast<unsigned char>(v & 0xFF)
        };
        f.write(reinterpret_cast<const char*>(b), 4);
    };
    auto write_be16 = [&](uint16_t v) {
        unsigned char b[2] = {
            static_cast<unsigned char>((v >> 8) & 0xFF),
            static_cast<unsigned char>(v & 0xFF)
        };
        f.write(reinterpret_cast<const char*>(b), 2);
    };

    // HEADER
    f.write("DIRC", 4);
    write_be32(index.version);
    write_be32(static_cast<uint32_t>(index.entries.size()));

    // ENTRIES
    size_t idx = 0;
    for (const auto &e : index.entries) {
        // ctime (sec, nsec)
        write_be32(static_cast<uint32_t>(e.ctime.first));
        write_be32(static_cast<uint32_t>(e.ctime.second));
        // mtime (sec, nsec)
        write_be32(static_cast<uint32_t>(e.mtime.first));
        write_be32(static_cast<uint32_t>(e.mtime.second));

        // dev, ino
        write_be32(e.dev);
        write_be32(e.ino);

        // mode: 16-bit unused + 16-bit mode ((type << 12) | perms)
        uint16_t mode16 = static_cast<uint16_t>(((e.mode_type & 0xF) << 12) | (e.mode_perms & 0x01FF));
        write_be16(0);          // unused
        write_be16(mode16);     // mode

        // uid, gid, fsize
        write_be32(e.uid);
        write_be32(e.gid);
        write_be32(e.fsize);

        // sha (20 raw bytes from hex)
        for (size_t i = 0; i < 40; i += 2) {
            const std::string byteStr = e.sha.substr(i, 2);
            unsigned char byte = static_cast<unsigned char>(std::strtoul(byteStr.c_str(), nullptr, 16));
            f.put(static_cast<char>(byte));
        }

        // flags: assume_valid bit 15, stage bits 12–13, name length low 12 bits
        uint16_t name_length;
        const std::string name_bytes = e.name; // already UTF-8 in std::string
        const uint16_t bytes_len = static_cast<uint16_t>(name_bytes.size());
        if (bytes_len >= 0x0FFF) name_length = 0x0FFF;
        else name_length = bytes_len;

        const uint16_t assume_bit = e.flag_assume_valid ? static_cast<uint16_t>(0x8000) : 0;
        const uint16_t stage_bits = static_cast<uint16_t>(e.flag_stage & 0x3000);
        const uint16_t flags = static_cast<uint16_t>(assume_bit | stage_bits | (name_length & 0x0FFF));
        write_be16(flags);

        // name + NUL
        f.write(name_bytes.data(), name_bytes.size());
        f.put('\0');

        // update running size of this entry (62 fixed + name + NUL)
        idx += 62 + name_bytes.size() + 1;

        // pad to 8-byte boundary
        if (idx % 8 != 0) {
            size_t pad = 8 - (idx % 8);
            static const char zeros[8] = {0};
            f.write(zeros, static_cast<std::streamsize>(pad));
            idx += pad;
        }
    }

    // Note: We intentionally do not write the trailing file checksum (like wyag).
    f.flush();
    debug_log("write_index", "Index write complete");
}


// writing indexes 


void rm(gitDirectory *gd,const vector<string> &paths,bool delete_files,bool skip_missing) {
    debug_log("rm", "Removing paths: " + std::to_string(paths.size()) + ", delete_files=" + std::string(delete_files ? "true" : "false"));
    GitIndex idx;
    idx.load(gd);

    // 2) Worktree root
    fs::path worktree = fs::path(gd->netpath);
    cout<<gd->netpath<<endl;
    string worktree_prefix = worktree.lexically_normal().string();
    if (!worktree_prefix.empty() && worktree_prefix.back() != fs::path::preferred_separator)
        worktree_prefix.push_back(fs::path::preferred_separator);

    // 3) Build absolute path list (and ensure inside worktree)
    vector<string> abspaths;
    abspaths.reserve(paths.size());
    for (const auto &p : paths) {
        fs::path ap = fs::absolute(p).lexically_normal();
        string aps = ap.string();
        debug_log("rm", "Checking path: " + p + " abs=" + aps);
        if (aps.rfind(worktree_prefix, 0) == 0) {
            abspaths.push_back(aps);
        } else {
            debug_log("rm", "Cannot remove paths outside of worktree: " + p);
            return;
        }
    }

    auto contains = [&](const string &s) {
        return find(abspaths.begin(), abspaths.end(), s) != abspaths.end();
    };
    auto erase_one = [&](const string &s) {
        auto it = find(abspaths.begin(), abspaths.end(), s);
        if (it != abspaths.end()) abspaths.erase(it);
    };

    // 4) Partition entries into kept vs removed
    vector<GitIndexEntry> kept_entries;
    kept_entries.reserve(idx.entries.size());
    vector<string> remove_paths;

    for (const auto &e : idx.entries) {
        string full_path = (worktree / e.name).lexically_normal().string();
        if (contains(full_path)) {
            debug_log("rm", "Removing from index: " + e.name + " (" + full_path + ")");
            remove_paths.push_back(full_path);
            erase_one(full_path);
        } else {
            kept_entries.push_back(e);
        }
    }

    // 5) Check missing paths
    if (!abspaths.empty() && !skip_missing) {
        debug_log("rm", "Cannot remove paths not in the index: " + std::to_string(abspaths.size()));
        return;
    }

    // 6) Physically delete files if requested
    if (delete_files) {
        for (const auto &p : remove_paths) {
            std::error_code ec;
            fs::remove(p, ec);
            if (ec) {
                debug_log("rm", "Failed to remove: " + p + " (" + ec.message() + ")");
            }
        }
    }

    // 7) Update index and write back
    idx.entries = std::move(kept_entries);
    write_index(gd, idx);
    debug_log("rm", "rm complete");
}

void add(gitDirectory *gd,const vector<string> &paths, bool delete_files , bool skip_missing){
    debug_log("add", "Adding paths: " + std::to_string(paths.size()));
    rm(gd, paths, /*delete_files=*/false, /*skip_missing=*/true);

    // 2) Build (abspath, relpath) set, validating paths
    fs::path worktree = fs::path(gd->netpath); // repo root
    string worktree_prefix = worktree.lexically_normal().string();
    if (!worktree_prefix.empty() && worktree_prefix.back() != fs::path::preferred_separator)
        worktree_prefix.push_back(fs::path::preferred_separator);

    set<pair<string,string>> clean_paths; // (abspath, relpath)
    for (const auto &p : paths) {
        fs::path ap = fs::absolute(p).lexically_normal();
        string abspath = ap.string();

        // Must be inside worktree and a regular file
        if (!(abspath.rfind(worktree_prefix, 0) == 0 && fs::is_regular_file(ap))) {
            debug_log("add", "Not a file, or outside the worktree: " + p);
            return;
        }

        fs::path rel = fs::relative(ap, worktree).lexically_normal();
        string relpath = rel.string();

        clean_paths.insert({abspath, relpath});
    }

    // 3) Reload index (modified by rm)
    GitIndex ind;
    ind.load(gd);

    // 4) For each path: hash blob, stat, append entry
    for (const auto &pr : clean_paths) {
        const string &abspath = pr.first;
        const string &relpath = pr.second;
        debug_log("add", "Hashing " + abspath +" "+relpath + " as blob");
        vector<string> args = {"skirk","2","-w","-t","blob",relpath};
        debug_log("add","gd net path value "+ gd->netpath+" end");

        string sha = hashfile(args, gd->netpath);

        struct stat st{};
        if (stat(abspath.c_str(), &st) != 0) {
            debug_log("add", "stat failed: " + abspath);
            continue;
        }

        int64_t ctime_s = static_cast<int64_t>(st.st_ctime);
        int64_t mtime_s = static_cast<int64_t>(st.st_mtime);
        int64_t ctime_ns = 0, mtime_ns = 0;
        #if defined(__APPLE__)
            ctime_ns = st.st_ctimespec.tv_nsec;
            mtime_ns = st.st_mtimespec.tv_nsec;
        #elif defined(__linux__)
            ctime_ns = st.st_ctim.tv_nsec;
            mtime_ns = st.st_mtim.tv_nsec;
        #endif

        GitIndexEntry entry(
            {ctime_s, ctime_ns},
            {mtime_s, mtime_ns},
            static_cast<uint32_t>(st.st_dev),
            static_cast<uint32_t>(st.st_ino),
            0b1000,
            0644,
            static_cast<uint32_t>(st.st_uid),
            static_cast<uint32_t>(st.st_gid),
            static_cast<uint32_t>(st.st_size),
            sha,
            false,
            0,
            relpath
        );
        debug_log("add", "Appending entry: " + entry.name + " sha=" + entry.sha.substr(0,7));
        ind.entries.push_back(entry);
    }

    // 5) Write index back
    write_index(gd, ind);
    debug_log("add", "add complete");
  }

  // Helper to format mode string like Python: f"{type:02o}{perms:04o}"
  static std::string mode_string(uint32_t mode_type, uint32_t mode_perms) {
      char buf[8] = {0};
      std::snprintf(buf, sizeof(buf), "%02o%04o",
                    static_cast<unsigned>(mode_type & 0xF),
                    static_cast<unsigned>(mode_perms & 0x1FF));
      return std::string(buf);
  }

  // Build a tree object from the index and return its SHA-1
  std::string tree_from_index(gitDirectory *gd, const GitIndex &gi) {
      // Each directory path maps to a list of items. Items can be files (index entries)
      // or subtrees (name, sha) that we add as we go bottom-up.
      struct DirItem {
          bool isTree;                   // false = file entry; true = subtree entry
          std::string name;              // basename within the directory
          std::string sha;               // used when isTree == true
          GitIndexEntry file;            // used when isTree == false
      };

      std::map<std::string, std::vector<DirItem>> contents;
      contents[""] = {}; // root

      // 1) Enumerate index entries into contents by directory
      for (const auto &e : gi.entries) {
          std::string dirname = fs::path(e.name).parent_path().string();
          // Ensure all parents up to root exist in the map
          std::string key = dirname;
          while (true) {
              if (contents.find(key) == contents.end()) contents[key] = {};
              if (key.empty()) break;
              key = fs::path(key).parent_path().string();
          }
          // Push the file entry into its directory with basename
          DirItem item{};
          item.isTree = false;
          item.name = fs::path(e.name).filename().string();
          item.file = e;
          contents[dirname].push_back(std::move(item));
      }

      // 2) Sort directory keys by length descending (process children before parents)
      std::vector<std::string> dirs;
      dirs.reserve(contents.size());
      for (const auto &kv : contents) dirs.push_back(kv.first);
      std::sort(dirs.begin(), dirs.end(), [](const std::string &a, const std::string &b) {
          return a.size() > b.size();
      });

      std::string rootSha;

      // 3) Walk directories bottom-up, building/writing trees and attaching them to parents
      for (const auto &dir : dirs) {
          std::vector<std::unique_ptr<treeLeaf>> leaves;

          for (const auto &it : contents[dir]) {
              if (!it.isTree) {
                  const auto &e = it.file;
                  std::string m = mode_string(e.mode_type, e.mode_perms);
                  auto leaf = std::make_unique<treeLeaf>();
                  leaf->mode = m;
                  leaf->path = it.name;
                  leaf->sha  = e.sha;
                  leaves.push_back(std::move(leaf));
              } else {
                  auto leaf = std::make_unique<treeLeaf>();
                  leaf->mode = "040000";
                  leaf->path = it.name;
                  leaf->sha  = it.sha;
                  leaves.push_back(std::move(leaf));
              }
          }

          // Serialize and write this directory's tree
          std::string raw = fileFunctions::treeSerialize(leaves);
          std::vector<unsigned char> content(raw.begin(), raw.end());
          std::unique_ptr<Tree> t = std::make_unique<Tree>(content);
          std::string sha = fileFunctions::writeObject(std::move(t), gd->netpath);

          if (dir.empty()) {
              rootSha = sha; // root tree
          } else {
              // Attach this subtree to its parent
              std::string parent = fs::path(dir).parent_path().string();
              std::string base = fs::path(dir).filename().string();
              DirItem subtree{};
              subtree.isTree = true;
              subtree.name = base;
              subtree.sha = sha;
              contents[parent].push_back(std::move(subtree));
          }
      }

      return rootSha;
  }

  // Compute timezone offset in +HHMM/-HHMM form
  static std::string tz_offset_string(std::time_t t) {
      std::tm lt = *std::localtime(&t);
      long offset = 0;
      #if defined(__APPLE__) || defined(__unix__) || defined(__linux__)
          // tm_gmtoff is available on BSD/macOS; on glibc with _BSD_SOURCE/_GNU_SOURCE.
          offset = static_cast<long>(lt.tm_gmtoff);
      #else
          // Fallback: no offset information
          offset = 0;
      #endif
      char sign = (offset >= 0) ? '+' : '-';
      long a = std::labs(offset);
      int hh = static_cast<int>(a / 3600);
      int mm = static_cast<int>((a % 3600) / 60);
      char buf[8];
      std::snprintf(buf, sizeof(buf), "%c%02d%02d", sign, hh, mm);
      return std::string(buf);
  }

  // Create a commit object and return its SHA-1
  std::string commit_create(gitDirectory *gd,
                            const std::string &tree,
                            const std::string &parent,
                            const std::string &author,
                            std::time_t timestamp,
                            const std::string &message) {
      // Build raw commit payload (kvlm-style): headers, blank line, message
      std::string tz = tz_offset_string(timestamp);
      std::string ts = std::to_string(static_cast<long long>(timestamp));

      std::string raw;
      raw += "tree " + tree + "\n";
      if (!parent.empty()) raw += "parent " + parent + "\n";
      raw += "author " + author + " " + ts + " " + tz + "\n";
      raw += "committer " + author + " " + ts + " " + tz + "\n";
      raw += "\n";
      // Trim message and ensure trailing newline
      std::string msg = message;
      while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) msg.pop_back();
      raw += msg + "\n";

      std::vector<unsigned char> content(raw.begin(), raw.end());
      std::unique_ptr<Commit> c = std::make_unique<Commit>(content);
      return fileFunctions::writeObject(std::move(c), gd->netpath);
  }

  // Helper: get "Name <email>" from env, otherwise fallback
  static std::string get_author_from_env() {
      const char* name = std::getenv("GIT_AUTHOR_NAME");
      const char* email = std::getenv("GIT_AUTHOR_EMAIL");
      if (name && email) return std::string(name) + " <" + email + ">";
      // Fallback: try git committer env vars
      name = std::getenv("GIT_COMMITTER_NAME");
      email = std::getenv("GIT_COMMITTER_EMAIL");
      if (name && email) return std::string(name) + " <" + email + ">";
      // Last resort
      return "Skirk User <skirk@example.com>";
  }

  // Commit command worker: builds tree from index, creates commit, updates refs
  std::string commit_cmd(gitDirectory *gd, const std::string &message) {
      // Load current index
      GitIndex idx;
      idx.load(gd);

      // Create tree from index
      std::string tree = tree_from_index(gd, idx);

      // Resolve parent commit from HEAD (if any)
      std::string head = std::string("HEAD");
      std::string want = std::string("commit");
      std::string parent = fileFunctions::objectFind(gd->netpath, head, want, true, gd);

      // Author and timestamp
      std::string author = get_author_from_env();
      std::time_t now = std::time(nullptr);

      // Create commit object
      std::string commitSha = commit_create(gd, tree, parent, author, now, message);

      // Update ref: if on a branch, write refs/heads/BRANCH; else write HEAD directly
      std::string branch = branch_get_active(gd);
      if (!branch.empty()) {
          std::string ref = gd->netpath + "/.mygit/refs/heads/" + branch;
          std::filesystem::create_directories(std::filesystem::path(ref).parent_path());
          std::ofstream f(ref);
          if (f) {
              f << commitSha << std::endl;
          } else {
              std::cerr << "Failed to update ref: " << ref << std::endl;
          }
      } else {
          // Detached HEAD: update .mygit/HEAD directly
          std::ofstream f(gd->netpath + "/.mygit/HEAD");
          if (f) {
              f << commitSha << std::endl;
          } else {
              std::cerr << "Failed to update HEAD: " << gd->netpath + "/.mygit/HEAD" << std::endl;
          }
      }

      return commitSha;
  }
} // namespace indexFunctions


