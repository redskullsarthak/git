#pragma once
#include <string>
#include <vector>

class gitDirectory {
public:
    std::string worktree;
    std::string path;
    std::string netpath;
    bool force = false;

    gitDirectory(std::string Worktree, std::string path, bool force);
    gitDirectory(std::string worktree, std::string path);

    void CreateGitRepository();
};
gitDirectory createInit(int argc, std::vector<std::string>& args, std::string path);