#pragma once
#include <string>
#include "../commands/init.hpp"

// bridge/entry for show-ref
void show_ref_command(gitDirectory *gd, const std::string &base,
                      bool with_hash = true, const std::string &prefix = "");