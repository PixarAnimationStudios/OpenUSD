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

/// \file meshUtil.h

#ifndef PXRUSDMAYA_MESH_UTIL_H
#define PXRUSDMAYA_MESH_UTIL_H

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

class MFnMesh;
class MString;
class UsdGeomMesh;

#define PXRUSDMAYA_MESH_COLOR_SET_TOKENS \
    (Authored) \
    (Clamped) \
    ((DisplayColorColorSetName, "displayColor")) \
    ((DisplayOpacityColorSetName, "displayOpacity"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaMeshColorSetTokens,
    PXRUSDMAYA_MESH_COLOR_SET_TOKENS);


namespace PxrUsdMayaMeshUtil
{

    bool getEmitNormals(const MFnMesh &mesh);
    TfToken setEmitNormals(const UsdGeomMesh &primSchema, MFnMesh &meshFn, TfToken defaultValue);
    
    TfToken getSubdivScheme(const MFnMesh &mesh, TfToken defaultValue);
    TfToken setSubdivScheme(const UsdGeomMesh &primSchema, MFnMesh &meshFn, TfToken defaultValue);

    TfToken getSubdivInterpBoundary(const MFnMesh &mesh,  TfToken defaultValue);
    TfToken setSubdivInterpBoundary(const UsdGeomMesh &primSchema, MFnMesh &meshFn, TfToken defaultValue);

    TfToken getSubdivFVLinearInterpolation(const MFnMesh& mesh);
    TfToken setSubdivFVLinearInterpolation(const UsdGeomMesh& primSchema, MFnMesh& meshFn);

} // namespace PxrUsdMayaMeshUtil

#endif // PXRUSDMAYA_MESH_UTIL_H
