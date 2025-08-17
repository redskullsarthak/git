#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include "../utils/fileProd.hpp"
#include "../utils/objectClasses.hpp"
#include "init.hpp"

using namespace std;
namespace fs = std::filesystem;

// Forward declaration of tree_checkout
void tree_checkout(gitDirectory* gd, unique_ptr<gitObject>& treeObj, const string &destPath);

// Command to checkout a commit into a destination directory.
// Expected arguments: args[2] is the commit (or tree) SHA, args[3] is the destination path.
void checkout(vector<string> &args, gitDirectory *gd) {
    if(args.size() < 4) {
        cout << "Error: checkout requires commit SHA and destination path." << endl;
        return;
    }
    string commitSha = args[2];
    string destPath = args[3];

    // Read the object from the repository. It may be a commit.
    unique_ptr<gitObject> obj = fileFunctions::readObject(commitSha, gd->netpath);
    if(!obj) {
        cout << "Error: Could not read object with SHA: " << commitSha << endl;
        return;
    }

    // If the object is a commit, get its tree.
    if(dynamic_cast<Commit*>(obj.get())) {
        Commit* commitPtr = dynamic_cast<Commit*>(obj.get());
        // Retrieve the tree SHA stored in the commit's key-value map.
        string treeSha = commitPtr->kvlm["tree"];
        obj = fileFunctions::readObject(treeSha, gd->netpath);
        if(!obj) {
            cout << "Error: Could not read tree object with SHA: " << treeSha << endl;
            return;
        }
    }
    // At this point, obj should be a tree.
    if (!fs::exists(destPath)) {
        fs::create_directories(destPath);
    } else {
        if (!fs::is_directory(destPath)) {
            cout << "Error: Destination " << destPath << " is not a directory!" << endl;
            return;
        }
        if (!fs::is_empty(destPath)) {
            cout << "Error: Destination directory " << destPath << " is not empty!" << endl;
            return;
        }
    }

    // Get the absolute destination path.
    string absDest = fs::absolute(destPath).string();
    tree_checkout(gd, obj, absDest);
}

// Recursively checks out a tree object into a directory.
// For each leaf in the tree, if it represents a tree then a corresponding subdirectory is created and tree_checkout is called recursively.
// Otherwise, if it is a blob, the file is written.
void tree_checkout(gitDirectory* gd, unique_ptr<gitObject>& treeObj, const string &destPath) {
    // Try to cast the object to a Tree.
    Tree* tree = dynamic_cast<Tree*>(treeObj.get());
    if (!tree) {
        cout << "Error: Provided object is not a tree." << endl;
        return;
    }

    // Iterate over each leaf in the tree.
    for (auto &leaf : tree->leafNodes) {
        // Build the destination file/dir path.
        string childPath = destPath + "/" + leaf->path;

        // Infer the type from the tree leaf's mode.
        // (Here we assume if the mode starts with "04", it represents a tree.)
        bool isTree = false;
        if (leaf->mode.length() >= 2 && leaf->mode.substr(0,2) == "04") {
            isTree = true;
        }

        if (isTree) {
            // Create the directory.
            fs::create_directory(childPath);
            // Read the tree object from the repository using the leaf's SHA.
            unique_ptr<gitObject> childObj = fileFunctions::readObject(leaf->sha, gd->netpath);
            if (!childObj) {
                cout << "Error: Could not read tree object " << leaf->sha << endl;
                continue;
            }
            // Recursively check out this subtree.
            tree_checkout(gd, childObj, childPath);
        } else {
            // Otherwise, treat it as a blob.
            unique_ptr<gitObject> childObj = fileFunctions::readObject(leaf->sha, gd->netpath);
            if (!childObj) {
                cout << "Error: Could not read blob object " << leaf->sha << endl;
                continue;
            }
            // Get blob data.
            vector<unsigned char> blobData = childObj->serialize();
            ofstream ofs(childPath, ios::binary);
            if (!ofs.is_open()) {
                cout << "Error: Could not create file " << childPath << endl;
                continue;
            }
            ofs.write(reinterpret_cast<const char*>(blobData.data()), blobData.size());
            ofs.close();
        }
    }
}
