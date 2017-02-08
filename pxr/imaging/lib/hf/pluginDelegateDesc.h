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
#ifndef HF_PLUGN_DELEGATE_DESC_H
#define HF_PLUGN_DELEGATE_DESC_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


///
/// Common structure used to report registered delegates in one of the delegate
/// registries.  The id token is used for internal api communication
/// about the name of the delegate.
/// displayName is a human readable name given to the delegate intended
/// to be used in menus.
/// priorty is used to provide an ordering of plugins.  The plugin
/// with the highest priority is determined to be the default delegate (unless
/// overriden by the application).  In the event of a tie
/// the string version of id is used to sort alphabetically ('a' has priority
/// over 'b').
///
struct HfPluginDelegateDesc {
    TfToken     id;
    std::string displayName;
    int         priority;
};

typedef std::vector<HfPluginDelegateDesc> HfPluginDelegateDescVector;


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HF_PLUGN_DELEGATE_DESC_H
