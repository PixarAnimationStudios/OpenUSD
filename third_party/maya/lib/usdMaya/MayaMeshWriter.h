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
#ifndef PXRUSDMAYA_MAYAMESHWRITER_H
#define PXRUSDMAYA_MAYAMESHWRITER_H

#include "usdMaya/MayaTransformWriter.h"

#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MString.h>

class UsdGeomMesh;
class UsdGeomGprim;
class MString;


// Writes an MFnMesh as a poly mesh OR a subd mesh
class MayaMeshWriter : public MayaTransformWriter
{
  public:
    MayaMeshWriter(MDagPath & iDag, 
            UsdStageRefPtr stage, 
            const JobExportArgs & iArgs);
    virtual ~MayaMeshWriter() {};

    virtual UsdPrim write(const UsdTimeCode &usdTime);
    
    /// \override
    virtual bool exportsGprims() const;

  protected:
    bool writeMeshAttrs(const UsdTimeCode &usdTime, UsdGeomMesh &primSchema);

  private:
    bool isMeshValid();
    void assignSubDivTagsToUSDPrim( MFnMesh &meshFn, UsdGeomMesh &primSchema);

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

    bool _createAlphaPrimVar(UsdGeomGprim &primSchema,
                             const TfToken& name,
                             const VtArray<float>& data,
                             const TfToken& interpolation,
                             const VtArray<int>& assignmentIndices,
                             const int unassignedValueIndex,
                             bool clamped);

    bool _createRGBPrimVar(UsdGeomGprim &primSchema,
                           const TfToken& name,
                           const VtArray<GfVec3f>& data,
                           const TfToken& interpolation,
                           const VtArray<int>& assignmentIndices,
                           const int unassignedValueIndex,
                           bool clamped);

    bool _createRGBAPrimVar(UsdGeomGprim &primSchema,
                            const TfToken& name,
                            const VtArray<GfVec3f>& rgbData,
                            const VtArray<float>& alphaData,
                            const TfToken& interpolation,
                            const VtArray<int>& assignmentIndices,
                            const int unassignedValueIndex,
                            bool clamped);

    bool _createUVPrimVar(UsdGeomGprim &primSchema,
                          const TfToken& name,
                          const VtArray<GfVec2f>& data,
                          const TfToken& interpolation,
                          const VtArray<int>& assignmentIndices,
                          const int unassignedValueIndex);

    /// Adds displayColor and displayOpacity primvars using the given color,
    /// alpha, and assignment data if the \p primSchema does not already have
    /// authored opinions for them.
    bool _addDisplayPrimvars(
        UsdGeomGprim &primSchema,
        const MFnMesh::MColorRepresentation colorRep,
        const VtArray<GfVec3f>& RGBData,
        const VtArray<float>& AlphaData,
        const TfToken& interpolation,
        const VtArray<int>& assignmentIndices,
        const int unassignedValueIndex,
        const bool clamped,
        const bool authored);

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
};

typedef shared_ptr < MayaMeshWriter > MayaMeshWriterPtr;

#endif  // PXRUSDMAYA_MAYAMESHWRITER_H
