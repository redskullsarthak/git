#pragma once
#include <string>
#include <vector>
#include <memory>

class gitDirectory;
class gitObject;

// Checkout a commit (or tree) into a destination directory
void checkout(std::vector<std::string> &args, gitDirectory *gd);

// Recursively checkout a tree object into a directory
void tree_checkout(gitDirectory* gd, std::unique_ptr<gitObject>& treeObj, const std:: string &destPath);
