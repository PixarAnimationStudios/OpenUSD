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
#ifndef PXRUSDMAYA_SHADINGMODEEXPORTER_H
#define PXRUSDMAYA_SHADINGMODEEXPORTER_H

#include "usdMaya/util.h"
#include "pxr/usd/usd/stage.h"


#include <maya/MObject.h>
#include <boost/function.hpp>

class PxrUsdMayaShadingModeExportContext
{
public:
    MObject GetShadingEngine() const { return _shadingEngine; }
    const UsdStageRefPtr& GetUsdStage() const { return _stage; }

    MObject GetSurfaceShader() const;

    /// An assignment contains a bound prim path and a list of faceIndices. 
    /// If the list of faceIndices is non-empty, then it is a partial assignment
    /// targeting a subset of the bound prim's faces. 
    /// If the list of faceIndices is empty, it means the assignment targets 
    /// all the faces in the bound prim or the entire bound prim.
    typedef std::pair<SdfPath, VtIntArray> Assignment;
    
    /// Vector of assignments.
    typedef std::vector<Assignment> AssignmentVector;

    /// Returns a vector of binding assignments associated with the shading 
    /// engine.
    AssignmentVector GetAssignments() const;

    /// Use this function to create a UsdShadeLook prim at the "standard"
    /// location.  The "standard" location may change depending on arguments
    /// that are passed to the export script.
    UsdPrim MakeStandardLookPrim(
            const AssignmentVector& assignmentsToBind,
            const std::string& name=std::string()) const;

    /// Use this function to get a "standard" usd attr name for \p attrPlug.
    /// The definition of "standard" may depend on arguments passed to the
    /// script (i.e. stripping namespaces, etc.).
    std::string GetStandardAttrName(const MPlug& attrPlug) const;

    PxrUsdMayaShadingModeExportContext(
            const MObject& shadingEngine,
            const UsdStageRefPtr& stage,
            bool mergeTransformAndShape,
            const PxrUsdMayaUtil::ShapeSet& bindableRoots,
            SdfPath overrideRootPath); 

private:
    MObject _shadingEngine;
    const UsdStageRefPtr& _stage;
    bool _mergeTransformAndShape;
    SdfPath _overrideRootPath;

    SdfPathSet _bindableRoots;
};

typedef boost::function< void (PxrUsdMayaShadingModeExportContext*) > PxrUsdMayaShadingModeExporter;

#endif // PXRUSDMAYA_SHADINGMODEEXPORTER_H
