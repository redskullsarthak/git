#include "objectClasses.hpp"
#include "fileProd.hpp"
#include <iostream>
#include <string>
#include <unordered_map>
using namespace std;


//gitObject is a abstract class , formed using pure virtual functions 


// Tree
Tree::Tree(const vector<unsigned char>& content) {
    this->fmt = "tree";
    this->content = content;
}
vector<unsigned char> Tree::serialize() {
    cout << "Serializing Tree" << endl;
    this->raw=fileFunctions::treeSerialize(leafNodes);
    return {};
}
vector<unsigned char> Tree::deserialize() {
    cout << "Deserializing Tree" << endl;
    string str(content.begin(),content.end());
    this->leafNodes=fileFunctions::treeParse(str);
    return {};
}






// Blob
Blob::Blob(const vector<unsigned char>& content) {
    this->fmt = "blob";
    this->content = content;
}
vector<unsigned char> Blob::serialize() {
    cout << "Serializing Blob" << endl;
    return content;
}
vector<unsigned char> Blob::deserialize() {
    cout << "Deserializing Blob" << endl;
    return {};
}

// Commit
Commit::Commit(const vector<unsigned char>& content) {
    this->fmt = "commit";
    this->content = content;
}
vector<unsigned char> Commit::serialize() {
    return {};
}
vector<unsigned char> Commit::deserialize() {
    
    const string cnt(content.begin(), content.end());
    unordered_map kvlm = fileFunctions::kvlm_parse(cnt);
    return {};
}

// Tag
Tag::Tag(const vector<unsigned char>& content) {
    this->fmt = "tag";
    this->content = content;
}
vector<unsigned char> Tag::serialize() {
    cout << "Serializing Tag" << endl;
    return {};
}
vector<unsigned char> Tag::deserialize() {
    cout << "Deserializing Tag" << endl;
    return {};
}
 
// treeLeaf function , represents leaves of the tree being parsed 
treeLeaf::treeLeaf(string &mode,string &sha,string &path){
         this->mode=mode;
         this->path=path;
         this->sha=sha;
}