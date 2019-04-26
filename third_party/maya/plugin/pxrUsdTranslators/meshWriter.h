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
#ifndef PXRUSDTRANSLATORS_MESH_WRITER_H
#define PXRUSDTRANSLATORS_MESH_WRITER_H

/// \file pxrUsdTranslators/meshWriter.h

#include "pxr/pxr.h"
#include "usdMaya/primWriter.h"

#include "usdMaya/writeJobContext.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvar.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MString.h>

#include <set>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE


/// Exports Maya mesh objects (MFnMesh)as UsdGeomMesh prims, taking into account
/// subd/poly, skinning, reference objects, UVs, and color sets.
class PxrUsdTranslators_MeshWriter : public UsdMayaPrimWriter
{
public:
    PxrUsdTranslators_MeshWriter(
            const MFnDependencyNode& depNodeFn,
            const SdfPath& usdPath,
            UsdMayaWriteJobContext& jobCtx);

    void Write(const UsdTimeCode& usdTime) override;
    bool ExportsGprims() const override;

    void PostExport() override;

protected:
    bool writeMeshAttrs(const UsdTimeCode& usdTime, UsdGeomMesh& primSchema);

private:
    bool isMeshValid();
    void assignSubDivTagsToUSDPrim(MFnMesh& meshFn, UsdGeomMesh& primSchema);

    /// Writes skeleton skinning data for the mesh if it has skin clusters.
    /// This method will internally determine, based on the job export args,
    /// whether the prim has skinning data and whether it is eligible for
    /// skinning data export.
    /// If skinning data is successfully exported, then returns the pre-skin
    /// mesh object. Otherwise, if no skeleton data was exported (whether there
    /// was an error, or this mesh had no skinning, or this mesh was skipped),
    /// returns a null MObject.
    /// This should only be called once at the default time.
    MObject writeSkinningData(UsdGeomMesh& primSchema);

    bool _GetMeshUVSetData(
            const MFnMesh& mesh,
            const MString& uvSetName,
            VtArray<GfVec2f>* uvArray,
            TfToken* interpolation,
            VtArray<int>* assignmentIndices);

    bool _GetMeshColorSetData(
            MFnMesh& mesh,
            const MString& colorSet,
            bool isDisplayColor,
            const VtArray<GfVec3f>& shadersRGBData,
            const VtArray<float>& shadersAlphaData,
            const VtArray<int>& shadersAssignmentIndices,
            VtArray<GfVec3f>* colorSetRGBData,
            VtArray<float>* colorSetAlphaData,
            TfToken* interpolation,
            VtArray<int>* colorSetAssignmentIndices,
            MFnMesh::MColorRepresentation* colorSetRep,
            bool* clamped);

    bool _createAlphaPrimVar(
            UsdGeomGprim& primSchema,
            const TfToken& name,
            const UsdTimeCode& usdTime,
            const VtArray<float>& data,
            const TfToken& interpolation,
            const VtArray<int>& assignmentIndices,
            bool clamped);

    bool _createRGBPrimVar(
            UsdGeomGprim& primSchema,
            const TfToken& name,
            const UsdTimeCode& usdTime,
            const VtArray<GfVec3f>& data,
            const TfToken& interpolation,
            const VtArray<int>& assignmentIndices,
            bool clamped);

    bool _createRGBAPrimVar(
            UsdGeomGprim& primSchema,
            const TfToken& name,
            const UsdTimeCode& usdTime,
            const VtArray<GfVec3f>& rgbData,
            const VtArray<float>& alphaData,
            const TfToken& interpolation,
            const VtArray<int>& assignmentIndices,
            bool clamped);

    bool _createUVPrimVar(
            UsdGeomGprim& primSchema,
            const TfToken& name,
            const UsdTimeCode& usdTime,
            const VtArray<GfVec2f>& data,
            const TfToken& interpolation,
            const VtArray<int>& assignmentIndices);

    /// Adds displayColor and displayOpacity primvars using the given color,
    /// alpha, and assignment data if the \p primSchema does not already have
    /// authored opinions for them.
    bool _addDisplayPrimvars(
            UsdGeomGprim& primSchema,
            const UsdTimeCode& usdTime,
            const MFnMesh::MColorRepresentation colorRep,
            const VtArray<GfVec3f>& RGBData,
            const VtArray<float>& AlphaData,
            const TfToken& interpolation,
            const VtArray<int>& assignmentIndices,
            const bool clamped,
            const bool authored);

    /// Sets the primvar \p primvar at time \p usdTime using the given
    /// \p indices (can be empty) and \p values.
    /// The \p defaultValue is used to pad the \p values array in case
    /// \p indices contains unassigned indices (i.e. indices < 0) that need a
    /// corresponding value in the array.
    ///
    /// When authoring values at a non-default time, _SetPrimvar() might
    /// unnecessarily pad \p values with \p defaultValue in order to guarantee
    /// that the primvar remains valid during the export process. In that case,
    /// the expected value of UsdGeomPrimvar::ComputeFlattened() is still
    /// correct (there is just some memory wasted).
    /// In order to cleanup any extra values and reclaim the wasted memory, call
    /// _CleanupPrimvars() at the end of the export process.
    void _SetPrimvar(
            const UsdGeomPrimvar& primvar,
            const VtIntArray& indices,
            const VtValue& values,
            const VtValue& defaultValue,
            const UsdTimeCode& usdTime);

    /// Cleans up any extra data authored by _SetPrimvar().
    void _CleanupPrimvars();

    /// Whether the mesh is animated. For the time being, meshes on which
    /// skinning is being exported are considered to be non-animated.
    /// XXX In theory you could have an animated input mesh before the
    /// skinCluster is applied but we don't support that right now.
    bool _IsMeshAnimated() const;

    /// Default value to use when collecting UVs from a UV set and a component
    /// has no authored value.
    static const GfVec2f _DefaultUV;

    /// Default values to use when collecting colors based on shader values
    /// and an object or component has no assigned shader.
    static const GfVec3f _ShaderDefaultRGB;
    static const float _ShaderDefaultAlpha;

    /// Default values to use when collecting colors from a color set and a
    /// component has no authored value.
    static const GfVec3f _ColorSetDefaultRGB;
    static const float _ColorSetDefaultAlpha;
    static const GfVec4f _ColorSetDefaultRGBA;

    /// Input mesh before any skeletal deformations, cached between iterations.
    MObject _skelInputMesh;

    /// Set of color sets that should be excluded.
    /// Intermediate processes may alter this set prior to writeMeshAttrs().
    std::set<std::string> _excludeColorSets;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
