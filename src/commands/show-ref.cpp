#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <filesystem>
#include <algorithm>
#include "show-ref.hpp"
#include "../utils/refWorker.hpp" // for refRelated::refRes
#include "../commands/init.hpp"   // for gitDirectory

namespace fs = std::filesystem;

// Recursively collect all refs under a given directory.
// It returns a map that associates the relative path (from "base")
// with the resolved SHA value. If a file’s content begins with "ref: ",
// refRelated::refRes is used to recursively resolve it.
void show_ref_command(gitDirectory *gd, const std::string &base,
                      bool with_hash, const std::string &prefix) {
    auto refs = refRelated::ref_list(base, gd);
    refRelated::show_ref(gd, refs, with_hash, prefix);
}