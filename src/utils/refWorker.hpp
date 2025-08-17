#pragma once 
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <fstream>
#include "../commands/init.hpp"

namespace refRelated{
   // resolve reference and return a sha utimately
   std::string refRes(std::string refName, gitDirectory *gd);

   // recursively list refs under base dir (flattened path -> resolved sha)
   std::map<std::string,std::string> ref_list(const std::string &base, gitDirectory *gd);

   // print refs
   void show_ref(gitDirectory *gd, const std::map<std::string,std::string> &refs,
                 bool with_hash = true, const std::string &prefix = "");
}