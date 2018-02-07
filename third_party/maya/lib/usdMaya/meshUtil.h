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

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"

#include <maya/MFnMesh.h>
#include <maya/MString.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdGeomMesh;

#define PXRUSDMAYA_MESH_COLOR_SET_TOKENS \
    ((DisplayColorColorSetName, "displayColor")) \
    ((DisplayOpacityColorSetName, "displayOpacity"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaMeshColorSetTokens,
    PXRUSDMAYA_MESH_COLOR_SET_TOKENS);


namespace PxrUsdMayaMeshUtil
{

    PXRUSDMAYA_API
    bool getEmitNormals(const MFnMesh &mesh, const TfToken& subdivScheme);
    PXRUSDMAYA_API
    TfToken setEmitNormals(const UsdGeomMesh &primSchema, MFnMesh &meshFn, TfToken defaultValue);
    PXRUSDMAYA_API
    bool GetMeshNormals(
        const MFnMesh& mesh,
        VtArray<GfVec3f>* normalsArray,
        TfToken* interpolation);
    
    PXRUSDMAYA_API
    TfToken getSubdivScheme(const MFnMesh &mesh, const TfToken& defaultValue);
    PXRUSDMAYA_API
    TfToken setSubdivScheme(const UsdGeomMesh &primSchema, MFnMesh &meshFn, TfToken defaultValue);

    PXRUSDMAYA_API
    TfToken getSubdivInterpBoundary(const MFnMesh &mesh, const TfToken& defaultValue);
    PXRUSDMAYA_API
    TfToken setSubdivInterpBoundary(const UsdGeomMesh &primSchema, MFnMesh &meshFn, TfToken defaultValue);

    PXRUSDMAYA_API
    TfToken getSubdivFVLinearInterpolation(const MFnMesh& mesh);
    PXRUSDMAYA_API
    TfToken setSubdivFVLinearInterpolation(const UsdGeomMesh& primSchema, MFnMesh& meshFn);

} // namespace PxrUsdMayaMeshUtil


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_MESH_UTIL_H
