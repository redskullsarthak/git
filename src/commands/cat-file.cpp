#include "init.hpp"
#include "../utils/fileProd.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

// use sha ,and path to read and print object to stdout 
void catfile(string &path, vector<string> &args){
    //path-- change to worktree directory(root of project that contains .mygit) , send a gd* 
    string fmt=args[2];
    string sha=args[3];
    unique_ptr<gitObject> ptr = fileFunctions::readObject(sha,path);
    if(!ptr) {
        cout<<"error: pointer was not returned by readObject fn"<<endl;
    }
    vector<unsigned char> v=ptr->serialize();
    for(auto el : v) cout<<el<<" ";
}



