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
#include "pxr/base/plug/info.h"
#include "pxr/base/arch/attributes.h"
#include "pixver.h"
#include <algorithm>

namespace {

ARCH_CONSTRUCTOR(103)
static
void
_PixarInit()
{
    std::vector<std::string> result;

    // Get a list of all the packages we care about. Basically we only care
    // about everything that isn't tools and globaltrees.
    std::vector<std::string> allPackages  = PixverListPackages();
    std::vector<std::string> twoXPackages = PixverListPackageAffects("tools");
    // 'tools' isn't included automatically
    twoXPackages.push_back("tools");

    // set_difference requires the lists to be sorted
    std::sort(allPackages.begin(), allPackages.end());
    std::sort(twoXPackages.begin(), twoXPackages.end());
    std::vector<std::string> ourPackages;
    std::set_difference(allPackages.begin(),
                        allPackages.end(),
                        twoXPackages.begin(),
                        twoXPackages.end(),
                        back_inserter(ourPackages));

    // Construct the list of lib paths for our components.
    for (const auto& pkg: ourPackages) {
        const std::string path = PixverGetPackageLocation(pkg);
        result.push_back(path + "/lib");
        result.push_back(path + "/lib/python");
    }

    Plug_SetPaths(result);
}

}
