#include "init.hpp"
#include "../utils/fileProd.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

gitDirectory::gitDirectory(string Worktree, string path, bool force) {
    this->worktree = Worktree;
    this->path = path;
    this->force = force;
    this->netpath = path + '/' + worktree;
}

gitDirectory::gitDirectory(string worktree, string path) {
    if(std::filesystem::exists(worktree+'/'+".mygit")){
    this->worktree = worktree;
    this->path = path;
    this->netpath = path + '/' + worktree;
   }
   else cout<<"Error : run git init first"<<endl;
}

void gitDirectory::CreateGitRepository() {
    if (force || !std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    } else {
        cout << "error: The Path is Invalid. Use force flag to create directories, or provide a valid path.\n";
        return;
    }

    if (!std::filesystem::exists(netpath)) {
        fileFunctions::CreateDirfully(path, worktree);  // Creates path/worktree/.mygit
    } else {
        if (std::filesystem::exists(netpath + "/.mygit")) {
            cout << "error: Repo already initialized.\n";
            return;
        } else {
            fileFunctions::CreateDirPart(netpath, ".mygit");
        }
    }

    string gitDir = netpath + "/.mygit";

    // Create subdirectories
    fileFunctions::CreateDirPart(gitDir, "objects");
    fileFunctions::CreateDirPart(gitDir + "/refs", "heads");
    fileFunctions::CreateDirPart(gitDir + "/refs", "tags");

    // Create metadata files
    fileFunctions::createfile(gitDir, "HEAD");
    fileFunctions::createfile(gitDir, "config");
    fileFunctions::createfile(gitDir, "description");

    // Write basic content to the HEAD file and config file
    ofstream headFile(gitDir + "/HEAD");
    headFile << "ref: refs/heads/master\n";
    headFile.close();
    
    fileFunctions::WriteDefaultConfig(gitDir);

    cout << "Repository initialized successfully at: " << netpath << endl;
}

gitDirectory* createInit(int argc, vector<string>& args, string path) {
    if (argc <= 2) {
        cout << "error : add a directory name" << endl;
        throw std::exception();
    }
    string worktree = args[2];
    bool force;
    if (argc == 4 && args[3] == "force") force = true;
    else force = false;
    gitDirectory* gd = new gitDirectory(worktree, path, force);
    gd->CreateGitRepository();
    return gd;
}