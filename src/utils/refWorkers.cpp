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
#include <optional>
#include <memory>
using namespace std;

// Replace macro with a proper alias for consistency
namespace fs = std::filesystem;

namespace refRelated {

    void createDirsForTags(string &name, string &sha, gitDirectory *gd);


    void tagSet(string &ref,gitDirectory *gd,string &tagName,bool makeTagObject){
       cerr << "[DEBUG][refWorkers] tagSet called: ref='" << ref << "' tag='" << tagName << "' makeTagObject=" << makeTagObject << "\n";
       // find the object 
       string fmt="commit";
       string sha=fileFunctions::objectFind(gd->netpath,ref,fmt);
       cerr << "[DEBUG][refWorkers] tagSet: resolved target sha='" << sha << "'\n";
       if(makeTagObject){
           cerr << "[DEBUG][refWorkers] tagSet: creating annotated tag object for '" << tagName << "'\n";
           // tag objects creation -a flag passed 
           unique_ptr<Tag> tagObj = make_unique<Tag>(std::vector<unsigned char>{});
           tagObj->kvlm["object"]=sha;
           tagObj->kvlm["type"]="commit";
           tagObj->kvlm["tag"]=tagName;
           tagObj->kvlm["tagger"]="someemail@gmail.com";
           tagObj->kvlm[""]="Sorry no messages possible to create in this version";
           // make this object    
           string newObjSha=fileFunctions::writeObject(move(tagObj), gd->netpath);
           cerr << "[DEBUG][refWorkers] tagSet: wrote tag object sha='" << newObjSha << "'\n";
           createDirsForTags(tagName,newObjSha,gd);
           cerr << "[DEBUG][refWorkers] tagSet: created annotated tag ref for '" << tagName << "' -> " << newObjSha << "\n";
       }
       else {
          //light weight tags 
           cerr << "[DEBUG][refWorkers] tagSet: creating lightweight tag for '" << tagName << "' -> " << sha << "\n";
           createDirsForTags(tagName,sha,gd);
       }
       cerr << "[DEBUG][refWorkers] tagSet complete for tag='" << tagName << "'\n";
    }

       void createDirsForTags(string &name,string &sha,gitDirectory *gd){
          string netPath=gd->netpath;// till .mygit
          netPath+="/refs";
          netPath+="/tags";
          netPath+="/"+name;
          cerr << "[DEBUG][refWorkers] createDirsForTags: writing tag ref file: " << netPath << " -> " << sha << "\n";
          ofstream f(netPath);
          if(!f.is_open()){
             cerr<<"[DEBUG][refWorkers] failed to open the file while writing tag :"<<name<<" during writing sha :"<<sha<<"\n";
             cout<<"failed to open the file while writing tag :"<<name<<"during writing sha :"<<sha<<endl;
             return;
          }
             f<<sha;
             f<<'\n';
             cerr<<"[DEBUG][refWorkers] wrote tag ref for :"<<name<<"\n";
             cout<<"wrote in the file for tag :"<<name<<endl;
       }
    // generate  a map under the base string 
    
string refRes(string refName,gitDirectory *gd){
        string workingpath = gd->netpath+".mygit/"+refName;
        cerr << "[DEBUG][refWorkers] refRes: resolving refName='" << refName << "' -> path='" << workingpath << "'\n";
        if(!filesystem::exists(workingpath)){
           // null string means no likely commits yet 
           cerr << "[DEBUG][refWorkers] refRes: path does not exist: " << workingpath << "\n";
           return "";
        }
        ifstream f(workingpath);
        if (!f.is_open()) {
            cerr << "[DEBUG][refWorkers] Error: Could not open ref file: " << workingpath << "\n";
            cout << "Error: Could not open ref file: " << workingpath << endl;
            return "";
        }
        string fileContent((istreambuf_iterator<char>(f)),istreambuf_iterator<char>());
        cerr << "[DEBUG][refWorkers] refRes: read " << fileContent.size() << " bytes\n";
        if (fileContent.empty()) {
            cerr << "[DEBUG][refWorkers] Error: Ref file is empty: " << workingpath << "\n";
            cout << "Error: Ref file is empty: " << workingpath << endl;
            return "";
        }
        if(fileContent.rfind("ref: ",0)==0){
           // recurse 
           if(fileContent.back()=='\n') fileContent.pop_back();
           cerr << "[DEBUG][refWorkers] refRes: indirection to '" << fileContent.substr(5) << "'\n";
           return refRes(fileContent.substr(5),gd);
        }
        else {
           // contains the sha 
           // trim trailing newline if any
           if(!fileContent.empty() && fileContent.back()=='\n') fileContent.pop_back();
           cerr << "[DEBUG][refWorkers] refRes: resolved to sha='" << fileContent << "'\n";
           return fileContent;
        }

   }

    map<string, string> ref_list(const string &base, gitDirectory *gd) {
        cerr << "[DEBUG][refWorkers] ref_list: scanning base='" << base << "'\n";
        map<string, string> ret;
        vector<filesystem::directory_entry> entries;
        for (const auto &entry : filesystem::directory_iterator(base))
            entries.push_back(entry);
        sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
            return a.path().filename() < b.path().filename();
        });

        for (const auto &entry : entries) {
            string name = entry.path().filename().string();
            cerr << "[DEBUG][refWorkers] ref_list: found entry='" << entry.path() << "' name='" << name << "'\n";
            if (entry.is_directory()) {
                cerr << "[DEBUG][refWorkers] ref_list: descending into directory '" << entry.path() << "'\n";
                map<string, string> subrefs = ref_list(entry.path().string(), gd);
                for (auto &p : subrefs) {
                    ret[name + "/" + p.first] = p.second;
                }
            } else {
                ifstream f(entry.path(), ios::binary);
                if (!f.is_open()) {
                    cerr << "[DEBUG][refWorkers] Error: Could not open ref file: " << entry.path() << "\n";
                    continue;
                }
                string content((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
                f.close();
                cerr << "[DEBUG][refWorkers] ref_list: read " << content.size() << " bytes from '" << entry.path() << "'\n";
                if (content.rfind("ref: ", 0) == 0) {
                    if (!content.empty() && content.back()=='\n') content.pop_back();
                    string refPath = content.substr(5);
                    cerr << "[DEBUG][refWorkers] ref_list: entry is indirection to '" << refPath << "'\n";
                    content = refRes(refPath, gd);
                    cerr << "[DEBUG][refWorkers] ref_list: indirection resolved to '" << content << "'\n";
                } else {
                    if (!content.empty() && content.back()=='\n') content.pop_back();
                }
                ret[name] = content;
                cerr << "[DEBUG][refWorkers] ref_list: registered ref '" << name << "' -> '" << content << "'\n";
            }
        }
        cerr << "[DEBUG][refWorkers] ref_list: returning " << ret.size() << " refs for base='" << base << "'\n";
        return ret;
    }

    void show_ref(gitDirectory *gd, const map<string, string> &refs,bool with_hash, const string &prefix) {
        cerr << "[DEBUG][refWorkers] show_ref: with_hash=" << with_hash << " prefix='" << prefix << "'\n";
        string pre = prefix;
        if (!pre.empty())
            pre += "/";
        for (const auto &p : refs) {
            if (with_hash) {
                cout << p.second << " " << pre + p.first << endl;
                cerr << "[DEBUG][refWorkers] show_ref: printed '" << p.second << " " << pre + p.first << "'\n";
            } else {
                cout << pre + p.first << endl;
                cerr << "[DEBUG][refWorkers] show_ref: printed '" << pre + p.first << "'\n";
            }
        }
    }

}


