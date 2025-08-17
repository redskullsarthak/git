// ref how does it work ?
// .ref/object/main contains some sha -- means when i enter a skirk checkout main i should use that reference to work 
#include <string>
#include "refWorker.hpp"
#include "../commands/init.hpp"
#include "fileProd.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include<optional>
using namespace std;

// Replace macro with a proper alias for consistency
namespace fs = std::filesystem;

namespace refRelated {

    void createDirsForTags(string &name, string &sha, gitDirectory *gd);


    void tagSet(string &ref,gitDirectory *gd,string &tagName,bool makeTagObject){
       // find the object 
       string fmt="commit";
       string sha=fileFunctions::objectFind(gd->netpath,ref,fmt);
       if(makeTagObject){
           // tag objects creation -a flag passed 
           unique_ptr<Tag> tagObj = make_unique<Tag>(std::vector<unsigned char>{});
           tagObj->kvlm["object"]=sha;
           tagObj->kvlm["type"]="commit";
           tagObj->kvlm["tag"]=tagName;
           tagObj->kvlm["tagger"]="someemail@gmail.com";
           tagObj->kvlm[""]="Sorry no messages possible to create in this version";
           // make this object    
           string newObjSha=fileFunctions::writeObject(move(tagObj), gd->netpath);
           createDirsForTags(tagName,newObjSha,gd);
       }
       else {
          //light weight tags 
           createDirsForTags(tagName,sha,gd);
       }
    }

       void createDirsForTags(string &name,string &sha,gitDirectory *gd){
          string netPath=gd->netpath;// till .mygit
          netPath+="/refs";
          netPath+="/tags";
          netPath+="/"+name;
          ofstream f(netPath);
          if(!f.is_open()){
             cout<<"failed to open the file while writing tag :"<<name<<"during writing sha :"<<sha<<endl;
          }
             f<<sha;
             f<<'\n';
             cout<<"wrote in the file for tag :"<<name<<endl;
       }
    // generate  a map under the base string 
    
string refRes(string refName,gitDirectory *gd){
        string workingpath = gd->netpath + '/' + refName;
        if(!filesystem::exists(workingpath)){
           // null string means no likely commits yet 
           return "";
        }
        ifstream f(workingpath);
        if (!f.is_open()) {
            cout << "Error: Could not open ref file: " << workingpath << endl;
            return "";
        }
        string fileContent((istreambuf_iterator<char>(f)),istreambuf_iterator<char>());
        if (fileContent.empty()) {
            cout << "Error: Ref file is empty: " << workingpath << endl;
            return "";
        }
        if(fileContent.rfind("ref: ",0)==0){
           // recurse 
           if(fileContent.back()=='\n') fileContent.pop_back();
           return refRes(fileContent.substr(5),gd);
        }
        else {
           // contains the sha 
           // trim trailing newline if any
           if(!fileContent.empty() && fileContent.back()=='\n') fileContent.pop_back();
           return fileContent;
        }

   }

    map<string, string> ref_list(const string &base, gitDirectory *gd) {
        map<string, string> ret;
        vector<filesystem::directory_entry> entries;
        for (const auto &entry : filesystem::directory_iterator(base))
            entries.push_back(entry);
        sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
            return a.path().filename() < b.path().filename();
        });

        for (const auto &entry : entries) {
            string name = entry.path().filename().string();
            if (entry.is_directory()) {
                map<string, string> subrefs = ref_list(entry.path().string(), gd);
                for (auto &p : subrefs) {
                    ret[name + "/" + p.first] = p.second;
                }
            } else {
                ifstream f(entry.path(), ios::binary);
                if (!f.is_open()) {
                    cerr << "Error: Could not open ref file: " << entry.path() << endl;
                    continue;
                }
                string content((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
                f.close();
                if (content.rfind("ref: ", 0) == 0) {
                    if (!content.empty() && content.back()=='\n') content.pop_back();
                    string refPath = content.substr(5);
                    content = refRes(refPath, gd);
                } else {
                    if (!content.empty() && content.back()=='\n') content.pop_back();
                }
                ret[name] = content;
            }
        }
        return ret;
    }

    void show_ref(gitDirectory *gd, const map<string, string> &refs,bool with_hash, const string &prefix) {
        string pre = prefix;
        if (!pre.empty())
            pre += "/";
        for (const auto &p : refs) {
            if (with_hash) {
                cout << p.second << " " << pre + p.first << endl;
            } else {
                cout << pre + p.first << endl;
            }
        }
    }

}


