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

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/dictionary.h"

PXR_NAMESPACE_OPEN_SCOPE


/// private helper so that both reader/writer registries can share the same
/// plugin discovery/load mechanism.
struct UsdMaya_RegistryHelper 
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

    /// Searches the plugInfos and looks for ShadingModePlugin.
    /// 
    /// "UsdMaya" : {
    ///     "ShadingModeExport" : {
    ///         "mayaPlugin" : "arnoldShaderExporter"
    ///     }
    /// }
    ///
    /// At that scope, it expects a dictionary with one key: "mayaPlugin".
    /// usdMaya will try to load the "mayaPlugin" when shading modes are first accessed.
    static void
        LoadShadingModePlugins();

    /// Searches the plugInfos for metadata dictionaries at the given \p scope,
    /// and composes them together. 
    /// The scope are the nested keys to search through in the plugInfo (for
    /// example, ["UsdMaya", "UsdExport"]).
    /// The same key under the \p scope must not be defined in multiple
    /// plugInfo.json files. If this occurs, the key will not be defined in the
    /// composed result, and this function will raise a coding error indicating
    /// where the keys have been multiply-defined.
    /// XXX We might relax the restriction on multiply-defined keys later on
    /// if there is a need to define values at different scopes, e.g.
    /// site-specific, department-specific, show-specific values.
    static VtDictionary GetComposedInfoDictionary(
            const std::vector<TfToken>& scope);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
