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
#ifndef PXRUSDMAYA_REGISTRYHELPER_H
#define PXRUSDMAYA_REGISTRYHELPER_H

#include "pxr/base/tf/token.h"

/// private helper so that both reader/writer registries can share the same
/// plugin discovery/load mechanism.
struct PxrUsdMaya_RegistryHelper 
{
    /// searches plugInfo's for \p value at the specified \p scope.  
    ///
    /// The scope are the nested keys to search through in the plugInfo (for
    /// example, ["UsdMaya", "PrimReader"].
    ///
    /// {
    ///   'UsdMaya': {
    ///     'PrimReader': {
    ///       'providesTranslator': [ ... ],
    ///       'mayaPlugin': "px_..."
    ///     }
    ///   }
    /// }
    ///
    /// At that scope, it expects a dictionary that has two keys:
    /// "providesTranslator" and "mayaPlugin".  If \p value matches the
    /// something in the "providesTranslator" list, it will proceed to try to
    /// load the "mayaPlugin".
    static void 
        FindAndLoadMayaPlug(
                const std::vector<TfToken>& scope,
                const std::string& value);

};
#endif // PXRUSDMAYA_REGISTRYHELPER_H
