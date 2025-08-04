#include <fstream>
#include <string>
#include <filesystem>
#include "fileProd.hpp"
#include<iostream>
#include <zlib.h>
#include <vector>
#include <algorithm>
#include"../commands/init.hpp"
#include"objectClasses.hpp"
#include "sha1.hpp"
using namespace std;
namespace fs = std::filesystem;

namespace fileFunctions{
    //file creation and writing 
    void createfile(string dirPath, string fileName){
       ofstream file(dirPath+"/"+fileName);
    }
    void create(string dirPath){
        try {
            fs::create_directories(dirPath);
        } catch(const std::exception& e) {
            cout << "error while creating directory: " << e.what() << endl;
        }
        return;
    }
    void CreateDirfully(string path, string worktree){
      string netDirPath=path+"/"+worktree+"/.mygit";
      cout<<"executed this "<<endl;
      create(netDirPath);       
    }
    void CreateDirPart(string path , string nextfolder){
      string netDirPath= path+"/"+nextfolder;
      create(netDirPath);
    }
    void CreateDirAndAfile(string path , string worktree,string fileName){
      string netDirPath=path+'/'+worktree;
      create(netDirPath);
      createfile(netDirPath, fileName);
    }
    void WriteDefaultConfig(string path){
        ofstream configFile(path+"/config");
        if(configFile.is_open()){
          configFile<<"[CORE]\n";
          configFile<<"repositoryformatversion = 0 \n";
          configFile<<"filemode=false\n";
          configFile<<"bare=false\n";
          configFile.close();
        }
        else {
          cout<<"error While Opening/Creating config file "<<endl;
          return;
        }
    }
    // compression functions using zlib
    vector<unsigned char> compressZlib(const vector<unsigned char>& data) {
          uLong srcLen = data.size();
          uLong destLen = compressBound(srcLen);  // max required size
          vector<unsigned char> out(destLen);
          int res = compress(out.data(), &destLen, data.data(), srcLen);
          if (res != Z_OK) {
                    cerr << "Compression failed with code: " << res << endl;
                    return {};
          }
          out.resize(destLen); // trim to actual compressed size
          return out;
    }
    vector<unsigned char> deCompressZlib(const vector<unsigned char>& input){
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
       return output;
    }
    //read and write git objects 
    unique_ptr<gitObject> readObject(string sha, string path) {
    string dirname = sha.substr(0, 2);
    string fileName = sha.substr(2);
    path = path + "/.mygit/objects";
    if (!fs::is_directory(path)) {
        cout << "Error: the path does not exist, initialise git directory first " << endl;
        return nullptr;
    }
    path = path + "/" + dirname;
    if (!fs::is_directory(path)) {
        cout << "Wrong Address No Such folder Exists" << endl;
        return nullptr;
    }
    path = path + "/" + fileName;
    if (!fs::exists(path)) {
        cout << "no such file exists  " << endl;
        return nullptr;
    }
    ifstream f(path, ios::binary);
    if (!f.is_open()) {
        cout << "failed to open the binary format of the file" << endl;
        return nullptr;
    }
    vector<unsigned char> compressedData((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    f.close();
    vector<unsigned char> decompressedDataRaw = deCompressZlib(compressedData);
    auto it = find(decompressedDataRaw.begin(), decompressedDataRaw.end(), static_cast<unsigned char>(' '));
    string type(decompressedDataRaw.begin(), it);
    auto nullIt = find(decompressedDataRaw.begin(), decompressedDataRaw.end(), '\0');
    std::string sizeStr(it + 1, nullIt);
    int size = std::stoi(sizeStr);
    std::vector<unsigned char> content(nullIt + 1, decompressedDataRaw.end());
    // content is a byte stream data it is to be converted in a string for ease of use 
    if (type == "blob") return make_unique<Blob>(content);
    else if (type == "tree") return make_unique<Tree>(content);
    else if (type == "tag") return make_unique<Tag>(content);
    else if (type == "commit") return make_unique<Commit>(content);
    else {
        cout << "unknown type :" << type << endl;
        return nullptr;
    }
}
    string writeObject(unique_ptr<gitObject> object,string &general_path){
       string fmt=object->fmt;
       vector<unsigned char> res;
       vector<unsigned char> content=object->serialize();//openup
       res.insert(res.end(),fmt.begin(),fmt.end());
       res.push_back(' ');
       string sz=to_string(content.size());
       res.insert(res.end(),sz.begin(),sz.end());
       res.push_back('\0');
       res.insert(res.end(),content.begin(),content.end());
       SHA1 sha;
       sha.update(reinterpret_cast<const uint8_t*>(res.data()), res.size());
       string shaHash=sha.final();
       string dirname = shaHash.substr(0,2);
       string filename = shaHash.substr(2);
       string object_dir = general_path + "/.mygit/objects/" + dirname;
       fileFunctions::create(object_dir); // create the required directories 
       string object_path = object_dir + "/" + filename;
       ofstream out(object_path, ios::binary);
       res=compressZlib(res);// compress
       if (out.is_open()) {
           out.write(reinterpret_cast<const char*>(res.data()), res.size()); // write to track 
           out.close();
       } else {
           cout << "Failed to write object file: " << object_path << endl;
       }
       return shaHash;
    }
    // parsing commits , parse the unsigned char to string first to reduce complexity here 
    unordered_map<string,string> kvlm_parse(const string &raw_content){
        unordered_map<string,string> kvlm;
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
    return make_unique<treeLeaf>(mode, path, sha);// create leaves for this tree
   }
   // return a list of pointers ,each pointing to the childeren of this tree object 
   vector<unique_ptr<treeLeaf>> treeParse(const string &raw){
        vector<unique_ptr<treeLeaf>> v;
        int pos=0;
        while(pos<raw.size()){
            v.push_back(parseTreeOne(raw,pos));
        }
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
    return raw;
}


}