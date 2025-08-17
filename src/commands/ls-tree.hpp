#pragma once
#include <string>
#include <vector>

class gitDirectory;

void ls_tree_cmd(std::vector<std::string> &args, std::string &path, gitDirectory *gd);
void ls_tree(std::string &name, std::string &prefix, gitDirectory *gd, bool r);

