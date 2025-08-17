#include <iostream>
#include <filesystem>
#include "../utils/fileProd.hpp"
#include "../utils/objectClasses.hpp"
#include "ls-tree.hpp"
#include "init.hpp"
using namespace std;

static void print_tree(Tree* tree, string &prefix, gitDirectory *gd, bool r) {
    for (auto &el : tree->leafNodes) {
        string tp = (el->mode.size() >= 2 ? el->mode.substr(0,2) : el->mode);
        string type;
        if (tp == "04") type = "tree";
        else if (tp == "10" || tp == "12") type = "blob";
        else if (tp == "16") type = "commit";
        else type = "unknown";

        string print_path = prefix.empty() ? el->path : prefix + "/" + el->path;

        if (r && type == "tree") {
            unique_ptr<gitObject> child = fileFunctions::readObject(el->sha, gd->netpath);
            Tree* childTree = dynamic_cast<Tree*>(child.get());
            if (!childTree) {
                cout << "Error: subtree object not a tree: " << el->sha << endl;
                continue;
            }
            string new_prefix = print_path;
            print_tree(childTree, new_prefix, gd, r);
        } else {
            cout << el->mode << " " << type << " " << el->sha << "\t" << print_path << endl;
        }
    }
}

void ls_tree(string &name, string &prefix, gitDirectory *gd, bool r) {
    string fmt = string("tree");
    string net = gd->netpath;
    string sha = fileFunctions::objectFind(net, name, fmt, true, gd);
    if (sha.empty()) {
        cout << "Could not resolve treeish: " << name << endl;
        return;
    }
    unique_ptr<gitObject> obj = fileFunctions::readObject(sha, gd->netpath);
    Tree* tree = dynamic_cast<Tree*>(obj.get());
    if (!tree) {
        cout << "Resolved object is not a tree: " << sha << endl;
        return;
    }
    print_tree(tree, prefix, gd, r);
}

void ls_tree_cmd(vector<string> &args,string &path,gitDirectory * gd){
    if(args.size()<3) {
        cout<<"Error: must contain 3 or 4 arguments for running ls-tree command"<<endl;
        return ;
    }
    bool r=false;
    if(args[2]=="-r") r=true;
    string TreeishName=(args.size()==3?args[2]:args[3]);
    string prefix="";
    ls_tree(TreeishName,prefix,gd,r);
}