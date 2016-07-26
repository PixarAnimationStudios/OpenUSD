//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/plug/info.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Custom get/set paths.  We don't link against libplug so we need to provide
// Plug_SetPaths().
std::vector<std::string>&
Plug_GetPaths()
{
    static std::vector<std::string> paths;
    return paths;
}

void
Plug_SetPaths(const std::vector<std::string>& paths)
{
    Plug_GetPaths() = paths;
}

/// Check to see if this is a path we care about checking.
/// return true if it is, false if not
bool IsPathToCheck(string path)
{
    const char* libs[2] = {"bedrock", "amber"};
    for (int i = 0; i < 2; ++i) {
        if (path.find(libs[i]) != string::npos) {
            return true;
        }
    }
    return false;
}

int main(int argc, char* argv[])
{
    vector<string> paths = Plug_GetPaths();

    vector<string> filtered_paths;

    // Print out the paths so we can compare runs when something fails
    cout << "==== paths ====" << endl;
    sort(paths.begin(), paths.end());
    TF_FOR_ALL(it, paths) {
        cout << *it << endl;
        if (IsPathToCheck(*it)) {
            filtered_paths.push_back(*it);
        }
    }
    cout << endl;

    // Can't check to see if paths are correct since the paths are unique to
    // the tree's config. We should at least get amber and bedrock.
    TF_VERIFY(filtered_paths.size() == 4, "Couldn't find amber or bedrock path");

    return 0;
}

