#include <fstream>
#include <string>
#include <filesystem>
#include "fileProd.hpp"
#include<iostream>
#include <zlib.h>
#include <vector>
#include <algorithm>
#include <regex>
#include"../commands/init.hpp"
#include"objectClasses.hpp"
#include "sha1.hpp"
#include "refWorker.hpp"
#include <unordered_map>
using namespace std;
namespace fs = std::filesystem;

namespace fileFunctions{
    //file creation and writing 
    void createfile(string dirPath, string fileName){
             cerr << "[DEBUG][fileProd] createfile: " << dirPath << "/" << fileName << "\n";
             ofstream file(dirPath+"/"+fileName);
    }
    void create(string dirPath){
                cerr << "[DEBUG][fileProd] create: ensuring directory exists: " << dirPath << "\n";
        try {
            fs::create_directories(dirPath);
                        cerr << "[DEBUG][fileProd] create: created/verified directory: " << dirPath << "\n";
        } catch(const std::exception& e) {
                        cerr << "[DEBUG][fileProd] create: error creating directory: " << e.what() << "\n";
                        cout << "error while creating directory: " << e.what() << endl;
        }
        return;
    }
    void CreateDirfully(string path, string worktree){
      string netDirPath=path+"/"+worktree+"/.mygit";
            cerr << "[DEBUG][fileProd] CreateDirfully: creating " << netDirPath << "\n";
            create(netDirPath);       
    }
    void CreateDirPart(string path , string nextfolder){
      string netDirPath= path+"/"+nextfolder;
            cerr << "[DEBUG][fileProd] CreateDirPart: creating part " << netDirPath << "\n";
            create(netDirPath);
    }
    void CreateDirAndAfile(string path , string worktree,string fileName){
      string netDirPath=path+'/'+worktree;
            cerr << "[DEBUG][fileProd] CreateDirAndAfile: creating " << netDirPath << " and file " << fileName << "\n";
            create(netDirPath);
            createfile(netDirPath, fileName);
    }
    void WriteDefaultConfig(string path){
                cerr << "[DEBUG][fileProd] WriteDefaultConfig: " << path << "/config\n";
                ofstream configFile(path+"/config");
                if(configFile.is_open()){
                    configFile<<"[CORE]\n";
                    configFile<<"repositoryformatversion = 0 \n";
                    configFile<<"filemode=false\n";
                    configFile<<"bare=false\n";
                    configFile.close();
                    cerr << "[DEBUG][fileProd] WriteDefaultConfig: wrote config\n";
                }
                else {
                    cerr << "[DEBUG][fileProd] WriteDefaultConfig: failed to open config file for writing\n";
                    cout<<"error While Opening/Creating config file "<<endl;
                    return;
                }
    }
    // compression functions using zlib
    vector<unsigned char> compressZlib(const vector<unsigned char>& data) {
          cerr << "[DEBUG][fileProd] compressZlib: input size=" << data.size() << "\n";
          uLong srcLen = data.size();
          uLong destLen = compressBound(srcLen);  // max required size
          vector<unsigned char> out(destLen);
          int res = compress(out.data(), &destLen, data.data(), srcLen);
          if (res != Z_OK) {
                    cerr << "[DEBUG][fileProd] compressZlib: Compression failed with code: " << res << "\n";
                    return {};
          }
          out.resize(destLen); // trim to actual compressed size
          cerr << "[DEBUG][fileProd] compressZlib: compressed size=" << out.size() << "\n";
          return out;
    }
    vector<unsigned char> deCompressZlib(const vector<unsigned char>& input){
      cerr << "[DEBUG][fileProd] deCompressZlib: input size=" << input.size() << "\n";
      vector<unsigned char> output(1024 * 100);  // 100 KB buffer
      z_stream strm = {};
      strm.total_in = strm.avail_in = input.size();
      strm.next_in = (Bytef *)input.data();
      strm.total_out = strm.avail_out = output.size();
      strm.next_out = (Bytef *)output.data();

       strm.zalloc = Z_NULL;
       strm.zfree = Z_NULL;
       strm.opaque = Z_NULL;

       if (inflateInit(&strm) != Z_OK)
              throw std::runtime_error("Failed to init zlib");

       int res = inflate(&strm, Z_FINISH);
       if (res != Z_STREAM_END) {
              inflateEnd(&strm);
              throw std::runtime_error("Failed to decompress: buffer too small?");
       }
       
       output.resize(strm.total_out);
    inflateEnd(&strm);
    cerr << "[DEBUG][fileProd] deCompressZlib: output size=" << output.size() << "\n";
    return output;
    }
    //read and write git objects 
    unique_ptr<gitObject> readObject(string sha, string path) {
    cerr << "[DEBUG][fileProd] readObject: sha='" << sha << "' path='" << path << "'\n";
    string dirname = sha.substr(0, 2);
    string fileName = sha.substr(2);
    path = path + "/.mygit/objects";
    if (!fs::is_directory(path)) {
        cerr << "[DEBUG][fileProd] readObject: objects dir missing: " << path << "\n";
        cout << "Error: the path does not exist, initialise git directory first " << endl;
        return nullptr;
    }
    path = path + "/" + dirname;
    if (!fs::is_directory(path)) {
        cerr << "[DEBUG][fileProd] readObject: object dir not found: " << path << "\n";
        cout << "Wrong Address No Such folder Exists" << endl;
        return nullptr;
    }
    path = path + "/" + fileName;
    if (!fs::exists(path)) {
        cerr << "[DEBUG][fileProd] readObject: object file missing: " << path << "\n";
        cout << "no such file exists  " << endl;
        return nullptr;
    }
    ifstream f(path, ios::binary);
    if (!f.is_open()) {
        cerr << "[DEBUG][fileProd] readObject: failed to open file: " << path << "\n";
        cout << "failed to open the binary format of the file" << endl;
        return nullptr;
    }
    vector<unsigned char> compressedData((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    f.close();
    cerr << "[DEBUG][fileProd] readObject: read " << compressedData.size() << " bytes (compressed)\n";
    vector<unsigned char> decompressedDataRaw = deCompressZlib(compressedData);
    cerr << "[DEBUG][fileProd] readObject: decompressed size=" << decompressedDataRaw.size() << "\n";
    auto it = find(decompressedDataRaw.begin(), decompressedDataRaw.end(), static_cast<unsigned char>(' '));
    string type(decompressedDataRaw.begin(), it);
    auto nullIt = find(decompressedDataRaw.begin(), decompressedDataRaw.end(), '\0');
    std::string sizeStr(it + 1, nullIt);
    int size = std::stoi(sizeStr);
    std::vector<unsigned char> content(nullIt + 1, decompressedDataRaw.end());
    cerr << "[DEBUG][fileProd] readObject: type='" << type << "' size=" << size << " content_bytes=" << content.size() << "\n";
    // content is a byte stream data it is to be converted in a string for ease of use 
    if (type == "blob") return make_unique<Blob>(content);
    else if (type == "tree") return make_unique<Tree>(content);
    else if (type == "tag") return make_unique<Tag>(content);
    else if (type == "commit") return make_unique<Commit>(content);
    else {
        cerr << "[DEBUG][fileProd] readObject: unknown type: " << type << "\n";
        cout << "unknown type :" << type << endl;
        return nullptr;
    }
}
    string writeObject(unique_ptr<gitObject> object,string &general_path){
       string fmt=object->fmt;
       vector<unsigned char> res;
       vector<unsigned char> content=object->serialize();//openup
       cerr << "[DEBUG][fileProd] writeObject: type='" << fmt << "' content_bytes=" << content.size() << "\n";
       res.insert(res.end(),fmt.begin(),fmt.end());
       res.push_back(' ');
       string sz=to_string(content.size());
       res.insert(res.end(),sz.begin(),sz.end());
       res.push_back('\0');
       res.insert(res.end(),content.begin(),content.end());
       SHA1 sha;
       sha.update(reinterpret_cast<const uint8_t*>(res.data()), res.size());
       string shaHash=sha.final();
       cerr << "[DEBUG][fileProd] writeObject: computed sha=" << shaHash << "\n";
       string dirname = shaHash.substr(0,2);
       string filename = shaHash.substr(2);
       string object_dir = general_path + "/.mygit/objects/" + dirname;
       fileFunctions::create(object_dir); // create the required directories 
       string object_path = object_dir + "/" + filename;
       ofstream out(object_path, ios::binary);
       res=compressZlib(res);// compress
       cerr << "[DEBUG][fileProd] writeObject: compressed bytes=" << res.size() << "\n";
       object->sha=shaHash;
       if (out.is_open()) {
           out.write(reinterpret_cast<const char*>(res.data()), res.size()); // write to track 
           out.close();
           cerr << "[DEBUG][fileProd] writeObject: wrote object to " << object_path << "\n";
       } else {
           cerr << "[DEBUG][fileProd] writeObject: Failed to write object file: " << object_path << "\n";
           cout << "Failed to write object file: " << object_path << endl;
       }
       
       return shaHash;
    }
    // parsing commits , parse the unsigned char to string first to reduce complexity here 
    unordered_map<string,string> kvlm_parse(const string &raw_content){
        unordered_map<string,string> kvlm;
        cerr << "[DEBUG][fileProd] kvlm_parse: raw_content size=" << raw_content.size() << "\n";
        size_t pos=0;
        while(pos<raw_content.size()){
            if(raw_content[pos]=='\n') {
                kvlm[""]=raw_content.substr(pos+1);
                break;
            }
            size_t end_of_line=raw_content.find(' ',pos);
            string key=raw_content.substr(pos,end_of_line-pos);
            pos=end_of_line+1;
            size_t line_end=raw_content.find('\n',pos);
            string value=raw_content.substr(pos,line_end-pos);
            kvlm[key]=value;
            pos=line_end+1;
        }
        cerr << "[DEBUG][fileProd] kvlm_parse: parsed " << kvlm.size() << " keys\n";
        return kvlm;
    }
    
    // make a gitleaf for a particular input string , convert unsigned char to string for easier use
    //tree format mode+space+path+null-termination+sha;
   unique_ptr<treeLeaf> parseTreeOne(const string &raw, int &pos) {
    //  Find space — marks end of mode
    size_t space_pos = raw.find(' ', pos);
    string mode = raw.substr(pos, space_pos - pos);
    if (mode.size() == 5) mode = "0" + mode;  // normalize to 6 digits
    //  Find NULL terminator — marks end of path
    pos = space_pos + 1;
    size_t null_pos = raw.find('\0', pos);
    string path = raw.substr(pos, null_pos - pos);

    //SHA-1 is 20 bytes starting from null_pos + 1
    pos = null_pos + 1;
    //Extract the 20 bytes as hex
    stringstream sha_stream;
    for (int i = 0; i < 20; ++i) {
        unsigned char byte = raw[pos + i];
        sha_stream << hex << setw(2) << setfill('0') << (int)byte;
    }

    string sha = sha_stream.str(); //40-char hex
    pos += 20;
    cerr << "[DEBUG][fileProd] parseTreeOne: mode='" << mode << "' path='" << path << "' sha='" << sha << "'\n";
    return make_unique<treeLeaf>(mode, path, sha);// create leaves for this tree
   }
   // return a list of pointers ,each pointing to the childeren of this tree object 
   vector<unique_ptr<treeLeaf>> treeParse(const string &raw){
        vector<unique_ptr<treeLeaf>> v;
        int pos=0;
        while(pos<raw.size()){
            v.push_back(parseTreeOne(raw,pos));
        }
        cerr << "[DEBUG][fileProd] treeParse: parsed " << v.size() << " leaf nodes\n";
        return v;
   }
   
// string xyz, xyz.c, xyz/ , comparison based on mode , sorting necessary as hash(same objects cannot be diffrent)
   string treeSerialize(vector<unique_ptr<treeLeaf>> &obj) {
    // Sort like Python's tree_leaf_sort_key
    sort(obj.begin(), obj.end(), [](const unique_ptr<treeLeaf> &a, const unique_ptr<treeLeaf> &b) {
        string keyA = a->mode.rfind("10", 0) == 0 ? a->path : a->path + '/';
        string keyB = b->mode.rfind("10", 0) == 0 ? b->path : b->path + '/';
        return keyA < keyB;
    });

    string raw;
    for (auto &el : obj) {
        raw += el->mode;
        raw += ' ';
        raw += el->path;
        
        raw += '\0';
        for (size_t i = 0; i < el->sha.length(); i += 2) {
            string byteStr = el->sha.substr(i, 2);
            char byte = (char)strtol(byteStr.c_str(), nullptr, 16);
            raw += byte;
        }
    }
    cerr << "[DEBUG][fileProd] treeSerialize: serialized size=" << raw.size() << " entries=" << obj.size() << "\n";
    return raw;
}
// return some nice name   
// if given a full hash return a full sha 
// if given a sub hash(a prefix of hash return the hash )
// if(given the head tag or a particular branch name resolve the final hash and return sha)
   
   vector<string> nameResolve(gitDirectory *gd, string &name) {
    vector<string> candidates;
    static const regex hashRE("^[0-9A-Fa-f]{4,40}$");

    // Trim whitespace
    string trimmed = name;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

    if (trimmed.empty()) return {};

    // HEAD
    if (trimmed == "HEAD") {
        string sha = refRelated::refRes("HEAD", gd);
        if (!sha.empty()) candidates.push_back(sha);
        else cout<<"check refres"<<endl;
        return candidates;
    }

    // SHA or SHA prefix
    if (regex_match(trimmed, hashRE)) {
        string lowerName = trimmed;
        for (auto &c : lowerName) c = tolower(c);
        string prefix = lowerName.substr(0, 2);
        string objectsPath = gd->netpath + "/objects/" + prefix;
        if (fs::exists(objectsPath) && fs::is_directory(objectsPath)) {
            for (auto &it : fs::directory_iterator(objectsPath)) {
                string fname = it.path().filename();
                if (fname.rfind(lowerName.substr(2), 0) == 0) {
                    candidates.push_back(prefix + fname);
                }
            }
        }
    }

    // Tag
    {
        string tagSha = refRelated::refRes("refs/tags/" + trimmed, gd);
        if (!tagSha.empty() && find(candidates.begin(), candidates.end(), tagSha) == candidates.end())
            candidates.push_back(tagSha);
    }

    // Local branch
    {
        string branchSha = refRelated::refRes("refs/heads/" + trimmed, gd);
        if (!branchSha.empty() && find(candidates.begin(), candidates.end(), branchSha) == candidates.end())
            candidates.push_back(branchSha);
    }

    // Remote branch
    {
        string remoteSha = refRelated::refRes("refs/remotes/" + trimmed, gd);
        if (!remoteSha.empty() && find(candidates.begin(), candidates.end(), remoteSha) == candidates.end())
            candidates.push_back(remoteSha);
    }

    return candidates;
}
   
   string objectFind( string &netpath, string &name, string &fmt, bool follow, gitDirectory *gd) {
    cerr << "[DEBUG][fileProd] objectFind: name='" << name << "' fmt='" << fmt << "' follow=" << follow << " netpath='" << netpath << "'\n";
    // 1. Resolve name to SHA(s)
    vector<string> shaList = nameResolve(gd, name);
    cerr << "[DEBUG][fileProd] objectFind: nameResolve returned " << shaList.size() << " candidate(s)\n";

    if (shaList.empty()) {
        cerr << "[DEBUG][fileProd] objectFind: No such reference: " << name << "\n";
        cerr << "No such reference: " << name << endl;
        return "";
    }
    if (shaList.size() > 1) {
        cerr << "[DEBUG][fileProd] objectFind: Ambiguous reference " << name << ": Candidates are:\n";
        for (const auto &s : shaList) {
            cerr << "[DEBUG][fileProd]  - " << s << "\n";
        }
        cerr << "Ambiguous reference " << name << ": Candidates are:\n";
        for (const auto &s : shaList) {
            cerr << " - " << s << "\n";
        }
        return "";
    }

    string sha = shaList[0];
    cerr << "[DEBUG][fileProd] objectFind: selected candidate sha='" << sha << "'\n";

    // 2. If no format required, return SHA directly
    if (fmt=="") {
        return sha;
    }

    // 3. Follow objects until format matches or follow == false
    while (true) {
        cerr << "[DEBUG][fileProd] objectFind: attempting to read object '" << sha << "'\n";
        unique_ptr<gitObject> obj = readObject(sha, gd->netpath);
        if (!obj) {
            cerr << "[DEBUG][fileProd] objectFind: Could not read object for SHA: " << sha << "\n";
            cerr << "Error: Could not read object for SHA: " << sha << endl;
            return "";
        }
        cerr << "[DEBUG][fileProd] objectFind: object fmt='" << obj->fmt << "' sha='" << sha << "'\n";

        if (obj->fmt == fmt) {
            cerr << "[DEBUG][fileProd] objectFind: format matches requested fmt='" << fmt << "', returning sha='" << sha << "'\n";
            return sha;
        }
        if (!follow) {
            cerr << "[DEBUG][fileProd] objectFind: format does not match and follow=false, returning empty\n";
            return "";
        }

        // Follow tags
        if (obj->fmt == "tag") {
            cerr << "[DEBUG][fileProd] objectFind: following tag object '" << sha << "'\n";
            Tag* tagObj = dynamic_cast<Tag*>(obj.get());
            if (tagObj && tagObj->kvlm.count("object")) {
                string next = tagObj->kvlm["object"];
                cerr << "[DEBUG][fileProd] objectFind: tag points to '" << next << "'\n";
                sha = next;
                continue;
            }
            cerr << "[DEBUG][fileProd] objectFind: tag object had no 'object' entry\n";
            return "";
        }
        // Follow commit to tree
        else if (obj->fmt == "commit" && fmt == "tree") {
            cerr << "[DEBUG][fileProd] objectFind: following commit to its tree for commit '" << sha << "'\n";
            Commit* commitObj = dynamic_cast<Commit*>(obj.get());
            if (commitObj && commitObj->kvlm.count("tree")) {
                string next = commitObj->kvlm["tree"];
                cerr << "[DEBUG][fileProd] objectFind: commit's tree is '" << next << "'\n";
                sha = next;
                continue;
            }
            cerr << "[DEBUG][fileProd] objectFind: commit object had no 'tree' entry\n";
            return "";
        }
        else {
            cerr << "[DEBUG][fileProd] objectFind: object fmt '" << obj->fmt << "' cannot be followed to requested fmt '" << fmt << "'\n";
            return "";
        }
    }
}
   
   
}