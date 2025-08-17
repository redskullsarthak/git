#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "../utils/objectClasses.hpp"
#include "../utils/fileProd.hpp"
using namespace std;
// skirk 2 -w -t type filepath
string hashfile(vector<string> &args,string &path){
     if(args.size()!=6) {
        cout<<"error: Bad Input "<<endl;
        return "" ;
     }     
     if(!filesystem::exists(path)){
        cout<<"error in parsing file in bridge fn "<<endl;
        return "";
     } 
     ifstream f(path,ios::binary);
     if(!f.is_open()){
        cout<<"error in Opening File"<<endl;
        return "";
     }
     vector<unsigned char> content((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
     unique_ptr<gitObject> u = nullptr;
     if(args[4]=="blob") u=make_unique<Blob>(content);
     else if(args[4]=="tree")u=make_unique<Tree>(content);
     else if(args[4]=="commit")u=make_unique<Commit>(content);
     else if(args[4]=="tag")u=make_unique<Tag>(content);
     else {
        cout<<"error : wrong type in hash-file"<<endl;
        return "";
     }
     //path should be the current working directory carry the gd* with u replace this path with that 
     string sha=fileFunctions::writeObject(move(u), path);
     cout<<"file hashing done here is the sha : " <<sha<<endl;
     return sha;
}
