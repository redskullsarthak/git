#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// forward declarations / small PODs
class treeLeaf {
public:
    std::string mode;
    std::string path;
    std::string sha;
    treeLeaf()=default;
    treeLeaf(const std::string &mode, const std::string &path, const std::string &sha);
};

class gitObject {
public:
    std::vector<unsigned char> content;
    std::string fmt;
    std::string sha;
    virtual std::vector<unsigned char> serialize()=0;
    virtual std::vector<unsigned char> deserialize()=0;
    virtual ~gitObject();
};

class Tree : public gitObject {
public:
    std::vector<std::unique_ptr<treeLeaf>> leafNodes;
    std::string raw;
    Tree(const std::vector<unsigned char>&content);
    std::vector<unsigned char> serialize() override;
    std::vector<unsigned char> deserialize() override;
};

class Blob : public gitObject {
public:
    Blob(const std::vector<unsigned char>&content);
    std::vector<unsigned char> serialize() override;
    std::vector<unsigned char> deserialize() override;
};

class Commit : public gitObject {
public:
    std::unordered_map<std::string,std::string> kvlm;
    Commit(const std::vector<unsigned char>&content);
    std::vector<unsigned char> serialize() override;
    std::vector<unsigned char> deserialize() override;
};

class Tag : public gitObject {
public:
    // Tag objects also carry kvlm (object, type, tag, tagger, message)
    std::unordered_map<std::string,std::string> kvlm;
    Tag(const std::vector<unsigned char>&content);
    std::vector<unsigned char> serialize() override;
    std::vector<unsigned char> deserialize() override;
};