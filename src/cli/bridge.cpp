#include <cstddef>
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <cstring>
#include "../commands/init.hpp"
#include "../commands/cat-file.hpp"
#include"../commands/hash-file.hpp"
#include"../commands/ls-tree.hpp"
#include "../commands/checkout.hpp"
#include"../commands/show-ref.hpp"
#include"../commands/tag.hpp"
#include"../utils/index.hpp"
using namespace std;
    vector<string> args;
    void fn(int argc, char* arg[]){
        for(int i=0;i<argc;i++){
            string ParticularArg=arg[i];
            args.push_back(ParticularArg);
        }
        string currpath=std::filesystem::current_path();// root of the project 
        if(argc<2){
            cout<<"Bad usage enter commands skirk -h to see the list of commands"<<endl;
            return ;
        }
        gitDirectory *gd=nullptr;
        if(strcmp(arg[1],"init")!=0)
        {        string worktree = "."; // or get from args/environment
        string path = std::filesystem::current_path().string();
        gd = new gitDirectory(worktree, path);
        if (!std::filesystem::exists(worktree + "/.mygit")) {
            cout << "Error: run git init first" << endl;
            return;
        }}
        if(strcmp(arg[1],"init") == 0) gd = createInit(argc,args,currpath); 
        else if(strcmp(arg[1],"cat-file")==0) catfile(currpath,args);
        else if(strcmp(arg[1],"hash")==0) hashfile(args,currpath);
        else if(strcmp(arg[1],"ls-tree")==0) {
            if (gd == nullptr) {
                cout << "Error: git directory is not initialized. Run 'init' first." << endl;
                return;
            }
            ls_tree_cmd(args,currpath,gd);
        }
        else if(strcmp(arg[1],"checkout")==0){
            if(gd==nullptr) {
                cerr<< "Error : git directory not initialised . run init first"<<endl;
                return ;
            }
            checkout(args,gd);
        }
        else if(strcmp(arg[1],"show-ref")==0){
            if(gd==nullptr){
                cerr<<"initailise init first"<<endl;
                return ;
            }
            string netPath=gd->netpath+"/.mygit";// ref maybe in head 
            show_ref_command(gd,netPath);// base on which show will be called 
        }
        else if(strcmp(arg[1],"tag")==0){
            if(gd==nullptr){
                cerr<<"initialise init first"<<endl;
                return ;
            }
            tag_cmd(args,gd);  
        }
        else if(strcmp(arg[1],"status")==0){
            if(gd==nullptr){
                cerr<<"initialise init first"<<endl;
                return ;
            }
            indexFunctions::status_cmd(gd);
        }
        else if(strcmp(arg[1],"rm")==0){
            if(gd==nullptr) {
                cerr<<"no git directory initialised as of yet "<<endl;
                return ;
            }
            //[skirk rm (-d) (-f) f1 f2 f3 ....]
            bool d=false,f=false;
            vector<string> paths;
            if(args[3]=="-d") d=true;
            if(args[4]=="-f") f=true;
            for(auto el :args){
               if(el=="-d"||el=="-f"||el=="skirk"||el=="rm") continue;
               paths.push_back(el);
            }
            indexFunctions::rm(gd,paths,d,f);
        }
        else if(strcmp(arg[1],"add")==0){
            if(gd==nullptr) {
                cerr<<"no git directory initialised as of yet "<<endl;
                return ;
            }
            //[skirk add (-d) (-f) f1 f2 f3 ....]
            bool d=false,f=false;
            vector<string> paths;
            if(args[3]=="-d") d=true;
            if(args[4]=="-f") f=true;
            for(auto el :args){
//               cout<<el<<endl;
               if(el=="-d"||el=="-f"||el=="skirk"||el=="rm"||el=="./skirk"||el=="/skirk"||el=="../skirk"||el=="add") continue;
               paths.push_back(el);
            }
            indexFunctions::add(gd,paths,d,f);
        }
        else if(strcmp(arg[1],"commit")==0){
            if(gd==nullptr){
                cerr<<"initialise init first"<<endl;
                return;
            }
            // parse -m "message"
            string message;
            for (size_t i = 2; i < args.size(); ++i) {
                if (args[i] == "-m" && i + 1 < args.size()) {
                    message = args[i + 1];
                    break;
                }
            }
            if (message.empty()) {
                cerr << "usage: skirk commit -m \"message\"" << endl;
                return;
            }
            string sha = indexFunctions::commit_cmd(gd, message);
            if (!sha.empty()) {
                cout << "[" << sha.substr(0,7) << "] " << message << endl;
            }
        }
    }
