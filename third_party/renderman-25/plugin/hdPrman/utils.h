//
// Copyright 2023 Pixar
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
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_UTILS_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_UTILS_H

#include "pxr/pxr.h"
#include "pxr/base/gf/matrix4d.h"

#include "RiTypesHelper.h" // for RtParamList

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;
class SdfAssetPath;
class VtValue;

namespace HdPrman_Utils {

/// Adds (or updates) a VtValue parameter to \p params and returns true if
/// the parameter was set.
bool
SetParamFromVtValue(
    RtUString const& name,
    VtValue const& val,
    TfToken const& role,
    RtParamList *params);

/// Similar to the function above, with the addition of \p detail, which 
/// specifies how array values should be handled across topology.
bool
SetPrimVarFromVtValue(
    RtUString const& name,
    VtValue const& val,
    RtDetailType const& detail,
    TfToken const& role,
    RtPrimVarList *params);

/// Helper to convert matrix types, handling double->float conversion.
inline RtMatrix4x4
GfMatrixToRtMatrix(const GfMatrix4d &m)
{
    const double *d = m.GetArray();
    return RtMatrix4x4(
        d[0], d[1], d[2], d[3],
        d[4], d[5], d[6], d[7],
        d[8], d[9], d[10], d[11],
        d[12], d[13], d[14], d[15]);
}

/// Helper to convert matrix types, handling float->double conversion.
inline GfMatrix4d
RtMatrixToGfMatrix(const RtMatrix4x4 &m)
{
    return GfMatrix4d(
        m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3],
        m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3],
        m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3],
        m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3]);
}


/// Attempt to extract a useful texture identifier from the given \p asset.
/// If \p asset is determined to not be a .tex file, attempt to use the Hio
/// based Rtx plugin to load the texture.  If \p asset is non-empty, we will
/// always return _something_
RtUString
ResolveAssetToRtUString(
    SdfAssetPath const &asset,
    bool flipTexture = true,
    char const *debugNodeType = nullptr);

/// Some quantites previously given as options now need to be provided
/// through different Riley APIs. This method returns a pruned
/// copy of the options, to be provided to SetOptions().
RtParamList
PruneDeprecatedOptions(
    const RtParamList &options);

}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_UTILS_H
