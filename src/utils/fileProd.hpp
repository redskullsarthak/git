#include<memory>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "objectClasses.hpp"
#include "../commands/init.hpp"
//path=>path+worktree+.git
// check if path exists or not ?
// already a file called worktree
// check does not contain already containing git repository 

namespace fileFunctions{
    void createfile(std::string dirPath,std:: string fileName);
    void create(std::string path);
    void CreateDirfully(std::string path, std::string worktree);
    void CreateDirPart(std::string path,std::string worktree);
    void CreateDirAndAfile(std::string path,std::string worktree,std::string fileName);
    void WriteDefaultConfig(std::string path);
    std::vector<unsigned char> compressZlib(const std::vector<unsigned char>& data);
    std::vector<unsigned char> deCompressZlib(const std::vector<unsigned char>& input);
    std::unique_ptr<gitObject> readObject(std::string sha, std::string path);
    std::string writeObject(std::unique_ptr<gitObject> object,std::string &general_path);
    std::unordered_map<std::string,std::string> kvlm_parse(const std::string &raw_content);
    std::unique_ptr<treeLeaf> parseTreeOne(const std::string &raw, int &pos);
    std::vector<std::unique_ptr<treeLeaf>> treeParse(const std::string &raw);
    std::string treeSerialize(std::vector<std::unique_ptr<treeLeaf>> &obj);
    std::string objectFind(std::string &netpath,std::string &name, std::string &fmt,bool follow=true,gitDirectory *gd=nullptr);
}
// targets --1.ls-tree
// cv-dpa pointers 
// dsa 

// destructor for git Object class 