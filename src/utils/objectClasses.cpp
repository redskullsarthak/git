#include "objectClasses.hpp"
#include "fileProd.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
using namespace std;

// gitObject
gitObject::~gitObject() = default;

// Commit
Commit::Commit(const vector<unsigned char>& content) {
    this->fmt = "commit";
    this->content = content;
    deserialize(); // parse kvlm now
}
vector<unsigned char> Commit::serialize() { return content; }
vector<unsigned char> Commit::deserialize() {
    std::cout << std::string(content.begin(), content.end()) << endl;
    const string cnt(content.begin(), content.end());
    kvlm = fileFunctions::kvlm_parse(cnt);
    return content;
}

// Tag
Tag::Tag(const vector<unsigned char>& content) {
    this->fmt = "tag";
    this->content = content;
    deserialize(); // parse kvlm now
}
vector<unsigned char> Tag::serialize() { return content; }
vector<unsigned char> Tag::deserialize() {
    const string cnt(content.begin(), content.end());
    kvlm = fileFunctions::kvlm_parse(cnt);
    return content;
}

// treeLeaf function , represents leaves of the tree being parsed 
treeLeaf::treeLeaf(const string &mode,const string &path,const string &sha){
    this->mode=mode;
    this->path=path;
    this->sha=sha;
}



// Blob
Blob::Blob(const vector<unsigned char>& content) {
    this->fmt = "blob";
    this->content = content;
}
vector<unsigned char> Blob::serialize() {
    return content; // Blob content is already serialized
}
vector<unsigned char> Blob::deserialize() {
    return content; // Blob content doesn't need parsing
}

// Tree
Tree::Tree(const vector<unsigned char>& content) {
    this->fmt = "tree";
    this->content = content;
    deserialize(); // Parse tree entries now
}
vector<unsigned char> Tree::serialize() {
    const string serialized = fileFunctions::treeSerialize(leafNodes); // Serialize tree entries
    return vector<unsigned char>(serialized.begin(), serialized.end());
}
vector<unsigned char> Tree::deserialize() {
    const string raw(content.begin(), content.end());
    leafNodes = fileFunctions::treeParse(raw); // Parse tree entries
    return content;
}