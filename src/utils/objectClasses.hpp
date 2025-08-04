#pragma once
#include <vector>
#include <string>

class gitObject {
public:
    std::vector<unsigned char> content;
    std::string fmt;
    virtual std::vector<unsigned char> serialize()=0;
    virtual std::vector<unsigned char> deserialize()=0;
    virtual ~gitObject();
};

class Tree : public gitObject {
public:
vector<unique_ptr<treeLeaf>> leafNodes;
string raw;
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
    Commit(const std::vector<unsigned char>&content);
    std::vector<unsigned char> serialize() override;
    std::vector<unsigned char> deserialize() override;
};

class Tag : public gitObject {
public:
    Tag(const std::vector<unsigned char>&content);
    std::vector<unsigned char> serialize() override;
    std::vector<unsigned char> deserialize() override;
};


class treeLeaf{
    public:
    std::string mode;
    std::string sha;
    std::string path;
    treeLeaf(std::string &mode,std::string &sha, std::string &path);
};