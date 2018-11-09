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
#ifndef PXRUSDMAYA_SHADING_MODE_EXPORTER_CONTEXT_H
#define PXRUSDMAYA_SHADING_MODE_EXPORTER_CONTEXT_H

/// \file usdMaya/shadingModeExporterContext.h

#include "usdMaya/api.h"
#include "usdMaya/util.h"

#include "pxr/pxr.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/types.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include <maya/MObject.h>
#include <maya/MPlug.h>

#include <string>
#include <utility>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


struct UsdMayaExportParams
{
    /// Whether the transform node and the shape node must be merged into
    /// a single node in the output USD.
    bool mergeTransformAndShape=true;

    /// If set to false, then direct per-gprim bindings are exported.
    /// If set to true and if \p materialCollectionsPath is non-empty, then
    /// material-collections are created and bindings are made to the
    /// collections at \p materialCollectionsPath, instead of direct
    /// per-gprim bindings.
    bool exportCollectionBasedBindings=false;

    /// Stripping namespaces.
    bool stripNamespaces=false;

    /// Modified root path.
    SdfPath overrideRootPath;

    /// This is the name of the USD prim under which material prims will be
    /// authored.
    TfToken materialsScopeName;

    /// If this is not empty, then a set of collections are exported on the
    /// prim pointed to by the path, each representing the collection of
    /// geometry that's bound to the various shading group sets in maya.
    SdfPath materialCollectionsPath;

    /// Shaders that are bound to prims under \p bindableRoot paths will get
    /// exported.  If \p bindableRoots is empty, it will export all.
    UsdMayaUtil::MDagPathSet bindableRoots;

    /// This is the path of the USD prim under which *all* prims will be
    /// authored.
    SdfPath parentScope;
};


class UsdMayaShadingModeExportContext
{
public:
    void SetShadingEngine(const MObject& shadingEngine) { _shadingEngine = shadingEngine; }
    MObject GetShadingEngine() const { return _shadingEngine; }
    const UsdStageRefPtr& GetUsdStage() const { return _stage; }
    bool GetMergeTransformAndShape() const {
        return _exportParams.mergeTransformAndShape;
    }
    const SdfPath& GetOverrideRootPath() const {
        return _exportParams.overrideRootPath;
    }
    const SdfPathSet& GetBindableRoots() const {
        return _bindableRoots;
    }

    PXRUSDMAYA_API
    void SetSurfaceShaderPlugName(const TfToken& surfaceShaderPlugName);
    PXRUSDMAYA_API
    void SetVolumeShaderPlugName(const TfToken& volumeShaderPlugName);
    PXRUSDMAYA_API
    void SetDisplacementShaderPlugName(const TfToken& displacementShaderPlugName);

    const UsdMayaUtil::MDagPathMap<SdfPath>& GetDagPathToUsdMap() const
    { return _dagPathToUsdMap; }

    PXRUSDMAYA_API
    MObject GetSurfaceShader() const;
    PXRUSDMAYA_API
    MObject GetVolumeShader() const;
    PXRUSDMAYA_API
    MObject GetDisplacementShader() const;

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
    PXRUSDMAYA_API
    AssignmentVector GetAssignments() const;

    /// Use this function to create a UsdShadeMaterial prim at the "standard"
    /// location.  The "standard" location may change depending on arguments
    /// that are passed to the export script.
    ///
    /// If \p boundPrimPaths is not NULL, it is populated with the set of
    /// prim paths that were bound to the created material prim, based on the
    /// given \p assignmentsToBind.
    PXRUSDMAYA_API
    UsdPrim MakeStandardMaterialPrim(
            const AssignmentVector& assignmentsToBind,
            const std::string& name=std::string(),
            SdfPathSet * const boundPrimPaths=nullptr) const;

    /// Use this function to get a "standard" usd attr name for \p attrPlug.
    /// The definition of "standard" may depend on arguments passed to the
    /// script (i.e. stripping namespaces, etc.).
    ///
    /// If attrPlug is an element in an array and if \p allowMultiElementArrays
    /// is true, this will <attrName>_<idx>.
    ///
    /// If it's false, this will return <attrName> if it's the 0-th logical
    /// element and an empty token otherwise.
    PXRUSDMAYA_API
    std::string GetStandardAttrName(
            const MPlug& attrPlug,
            bool allowMultiElementArrays) const;

    PXRUSDMAYA_API
    UsdMayaShadingModeExportContext(
            const MObject& shadingEngine,
            const UsdStageRefPtr& stage,
            const UsdMayaUtil::MDagPathMap<SdfPath>& dagPathToUsdMap,
            const UsdMayaExportParams &exportParams);
private:
    MObject _shadingEngine;
    const UsdStageRefPtr& _stage;
    const UsdMayaUtil::MDagPathMap<SdfPath>& _dagPathToUsdMap;
    const UsdMayaExportParams &_exportParams;
    TfToken _surfaceShaderPlugName;
    TfToken _volumeShaderPlugName;
    TfToken _displacementShaderPlugName;

    // Bindable roots stored as an SdfPathSet.
    SdfPathSet _bindableRoots;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
