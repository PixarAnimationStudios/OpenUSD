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
#ifndef _usdExport_MayaMeshWriter_h_
#define _usdExport_MayaMeshWriter_h_

#include "usdMaya/MayaTransformWriter.h"
#include <maya/MFnMesh.h>

class UsdGeomMesh;
class UsdGeomGprim;
class MFnLambertShader;
class MFnMesh;
class MString;
class MColorArray;

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
    MStatus _GetMeshUVSetData( MFnMesh& m,  MString uvSetName,
               VtArray<GfVec2f> *uvArray,
               TfToken *interpolation);
                    
    bool _GetMeshColorSetData(
        MFnMesh& m,
        MString colorSet,
        bool isDisplayColor,
        const VtArray<GfVec3f>& shadersRGBData,
        const VtArray<float>& shadersAlphaData,
        VtArray<GfVec3f> *RGBData, TfToken *RGBInterp,
        VtArray<GfVec4f> *RGBAData, TfToken *RGBAInterp,
        VtArray<float> *AlphaData, TfToken *AlphaInterp,
        MFnMesh::MColorRepresentation *colorSetRep,
        bool *clamped);

    static MStatus
    _CompressUVs(const MFnMesh& m,
                 const MIntArray& uvIds,
                 const MFloatArray& uArray, const MFloatArray& vArray,
                 VtArray<GfVec2f> *uvArray,
                 TfToken *interpolation);

    static MStatus
    _FullUVsFromSparse(const MFnMesh& m,
                       const MIntArray& uvCounts, const MIntArray& uvIds,
                       const MFloatArray& uArray, const MFloatArray& vArray,
                       VtArray<GfVec2f> *uvArray);

    bool _createAlphaPrimVar(  UsdGeomGprim &primSchema, const TfToken name,
                                const VtArray<float>& data, TfToken interpolation,
                                bool clamped);

    bool _createRGBPrimVar(  UsdGeomGprim &primSchema, const TfToken name,
                            const VtArray<GfVec3f>& data, TfToken interpolation,
                            bool clamped);

    bool _createRGBAPrimVar(  UsdGeomGprim &primSchema, const TfToken name,
                            const VtArray<GfVec4f>& RGBAData, TfToken RGBAInterp,
                                bool clamped);

    bool _setDisplayPrimVar( UsdGeomGprim &primSchema,
                            MFnMesh::MColorRepresentation colorRep,
                            VtArray<GfVec3f> RGBData, TfToken RGBInterp,
                            VtArray<float> AlphaData, TfToken AlphaInterp,
                            bool clamped, bool authored);
};

typedef shared_ptr < MayaMeshWriter > MayaMeshWriterPtr;

#endif  // _usdExport_MayaMeshWriter_h_
