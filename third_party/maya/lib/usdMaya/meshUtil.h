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

/// \file usdMaya/meshUtil.h

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

TF_DECLARE_PUBLIC_TOKENS(UsdMayaMeshColorSetTokens,
    PXRUSDMAYA_API,
    PXRUSDMAYA_MESH_COLOR_SET_TOKENS);

/// Utilities for dealing with USD and RenderMan for Maya mesh/subdiv tags.
namespace UsdMayaMeshUtil
{
    /// Gets the internal emit-normals tag on the Maya \p mesh, placing it in
    /// \p value. Returns true if the tag exists on the mesh, and false if not.
    PXRUSDMAYA_API
    bool GetEmitNormalsTag(const MFnMesh &mesh, bool* value);

    /// Sets the internal emit-normals tag on the Maya \p mesh.
    /// This value indicates to the exporter whether it should write out the
    /// normals for the mesh to USD.
    PXRUSDMAYA_API
    void SetEmitNormalsTag(MFnMesh &meshFn, const bool emitNormals);

    /// Helper method for getting Maya mesh normals as a VtVec3fArray.
    PXRUSDMAYA_API
    bool GetMeshNormals(
        const MFnMesh& mesh,
        VtArray<GfVec3f>* normalsArray,
        TfToken* interpolation);

    /// Gets the subdivision scheme tagged for the Maya mesh by consulting the
    /// adaptor for \c UsdGeomMesh.subdivisionSurface, and then falling back to
    /// the RenderMan for Maya attribute.
    PXRUSDMAYA_API
    TfToken GetSubdivScheme(const MFnMesh &mesh);

    /// Gets the subdivision interpolate boundary tagged for the Maya mesh by
    /// consulting the adaptor for \c UsdGeomMesh.interpolateBoundary, and then
    /// falling back to the RenderMan for Maya attribute.
    PXRUSDMAYA_API
    TfToken GetSubdivInterpBoundary(const MFnMesh &mesh);

    /// Gets the subdivision face-varying linear interpolation tagged for the
    /// Maya mesh by consulting the adaptor for
    /// \c UsdGeomMesh.faceVaryingLinearInterpolation, and then falling back to
    /// the OpenSubdiv2-style tagging.
    PXRUSDMAYA_API
    TfToken GetSubdivFVLinearInterpolation(const MFnMesh& mesh);

} // namespace UsdMayaMeshUtil


PXR_NAMESPACE_CLOSE_SCOPE

#endif
