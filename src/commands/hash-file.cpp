#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "../utils/objectClasses.hpp"
#include "../utils/fileProd.hpp"
using namespace std;

// Debug log helper
static inline void debug_log(const char* tag, const std::string& msg) {
    std::cerr << "[DEBUG][" << tag << "] " << msg << std::endl;
}

// skirk 2 -w -t type filepath
string hashfile(vector<string> &args,string &path){
     debug_log("hashfile", "Called with args=[" + (args.empty() ? "" : args[0]) + "...], path=" + path);

     if(args.size()!=6) {
        debug_log("hashfile", "error: Bad Input, args.size()=" + std::to_string(args.size()));
        cout<<"error: Bad Input "<<endl;
        return "" ;
     }     
     string path_here=path+args.back();
     if(!filesystem::exists(path_here)){
        debug_log("hashfile", "error: file does not exist at path_here=" + path_here);
        cout<<"error in parsing file in bridge fn "<<endl;
        return "";
     } 
     if (!filesystem::is_regular_file(path_here)) {
         debug_log("hashfile", "error: path_here is not a regular file: " + path_here);
         cout << "error: path_here is not a regular file" << endl;
         return "";
     }
     ifstream f(path_here,ios::binary);
     if(!f.is_open()){
        debug_log("hashfile", "error: could not open file at path_here=" + path_here);
        cout<<"error in Opening File"<<endl;
        return "";
     }
     vector<unsigned char> content((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
     debug_log("hashfile", "Read " + std::to_string(content.size()) + " bytes from file");

     unique_ptr<gitObject> u = nullptr;
     if(args[4]=="blob") {
         debug_log("hashfile", "Creating Blob object");
         u=make_unique<Blob>(content);
     }
     else if(args[4]=="tree") {
         debug_log("hashfile", "Creating Tree object");
         u=make_unique<Tree>(content);
     }
     else if(args[4]=="commit") {
         debug_log("hashfile", "Creating Commit object");
         u=make_unique<Commit>(content);
     }
     else if(args[4]=="tag") {
         debug_log("hashfile", "Creating Tag object");
         u=make_unique<Tag>(content);
     }
     else {
        debug_log("hashfile", "error: wrong type in hash-file: " + args[4]);
        cout<<"error : wrong type in hash-file"<<endl;
        return "";
     }
     //path_here should be the current working directory carry the gd* with u replace this path_here with that 
     string sha=fileFunctions::writeObject(move(u),path);
     debug_log("hashfile", "file hashing done, sha=" + sha);
     cout<<"file hashing done here is the sha : " <<sha<<endl;
     return sha;
}
