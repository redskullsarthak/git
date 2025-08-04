#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <cstring>
#include "../commands/init.hpp"
#include "../commands/cat-file.hpp"
#include"../commands/hash-file.hpp"
using namespace std;

    vector<string> args;
    void fn(int argc, char* arg[]){
        for(int i=0;i<argc;i++){
            string ParticularArg=arg[i];
            args.push_back(ParticularArg);
        }
        string currpath=std::filesystem::current_path();
        if(argc<2){
            cout<<"Bad usage enter commands skirk -h to see the list of commands"<<endl;
            return ;
        }
        
        if(strcmp(arg[1],"init") == 0) gitDirectory gd=createInit(argc,args,currpath); 
        else if(strcmp(arg[1],"cat-file")==0) catfile(currpath,args);
        else if(strcmp(arg[1],"hash")==0) hashfile(args,currpath);
    }
 