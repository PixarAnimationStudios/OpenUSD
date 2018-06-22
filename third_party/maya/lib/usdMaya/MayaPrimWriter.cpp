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
#include "pxr/pxr.h"
#include "usdMaya/MayaPrimWriter.h"

#include "usdMaya/adaptor.h"
#include "usdMaya/primWriterArgs.h"
#include "usdMaya/primWriterContext.h"
#include "usdMaya/translatorGprim.h"
#include "usdMaya/usdWriteJobCtx.h"
#include "usdMaya/util.h"
#include "usdMaya/writeUtil.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usd/inherits.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MObjectArray.h>
#include <maya/MColorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MPlugArray.h>
#include <maya/MUintArray.h>
#include <maya/MColor.h>
#include <maya/MFnSet.h>

PXR_NAMESPACE_OPEN_SCOPE


PXRUSDMAYA_REGISTER_ADAPTOR_ATTRIBUTE_ALIAS(
        UsdGeomTokens->purpose, "USD_purpose");

TF_DEFINE_PRIVATE_TOKENS(
        _tokens, 

        (USD_inheritClassNames)
);


MayaPrimWriter::MayaPrimWriter(const MDagPath& iDag,
                               const SdfPath& uPath,
                               usdWriteJobCtx& jobCtx) :
    _writeJobCtx(jobCtx),
    _dagPath(iDag),
    _usdPath(uPath),
    _isValid(true),
    _exportVisibility(jobCtx.getArgs().exportVisibility)
{
}

MayaPrimWriter::~MayaPrimWriter()
{
}

// In the future, we'd like to make this a plugin point.
static bool 
_GetClassNamesToWrite(
        MObject mObj,
        const UsdPrim& usdPrim,
        std::vector<std::string>* outClassNames)
{
    std::vector<std::string> ret;
    if (PxrUsdMayaWriteUtil::ReadMayaAttribute(
            MFnDependencyNode(mObj), 
            MString(_tokens->USD_inheritClassNames.GetText()),
            outClassNames)) {
        return true;
    }

    return false;
}

bool
MayaPrimWriter::_WriteImageableAttrs(const MDagPath &dagT, const UsdTimeCode &usdTime, UsdGeomImageable &primSchema) 
{
    MStatus status;
    MFnDependencyNode depFn(GetDagPath().node());
    MFnDependencyNode depFnT(dagT.node()); // optionally also scan a shape's transform if merging transforms

    if (_exportVisibility) {
        bool isVisible  = true;   // if BOTH shape or xform is animated, then visible
        bool isAnimated = false;  // if either shape or xform is animated, then animated

        PxrUsdMayaUtil::getPlugValue(depFn, "visibility", &isVisible, &isAnimated);

        if ( dagT.isValid() ) {
            bool isVis, isAnim;
            if (PxrUsdMayaUtil::getPlugValue(depFnT, "visibility", &isVis, &isAnim)){
                isVisible = isVisible && isVis;
                isAnimated = isAnimated || isAnim;
            }
        }

        TfToken const &visibilityTok = (isVisible ? UsdGeomTokens->inherited : 
                                        UsdGeomTokens->invisible);
        if (usdTime.IsDefault() != isAnimated ) {
            _SetAttribute(primSchema.CreateVisibilityAttr(VtValue(), true), 
                          visibilityTok, 
                          usdTime);
        }
    }

    UsdPrim usdPrim = primSchema.GetPrim();

    // There is no Gprim abstraction in this module, so process the few
    // gprim attrs here.
    UsdGeomGprim gprim = UsdGeomGprim(usdPrim);
    if (gprim && usdTime.IsDefault()){

        PxrUsdMayaPrimWriterContext* unused = NULL;
        PxrUsdMayaTranslatorGprim::Write(
                GetDagPath().node(),
                gprim,
                unused);

    }

    std::vector<std::string> classNames;
    if (_GetClassNamesToWrite(
            GetDagPath().node(),
            usdPrim,
            &classNames)) {
        PxrUsdMayaWriteUtil::WriteClassInherits(usdPrim, classNames);
    }

    // Write UsdGeomImageable typed schema attributes.
    // Currently only purpose, which is uniform, so only export at default time.
    if (usdTime.IsDefault()) {
        PxrUsdMayaWriteUtil::WriteSchemaAttributesToPrim<UsdGeomImageable>(
                GetDagPath().node(),
                dagT.node(),
                usdPrim,
                {UsdGeomTokens->purpose},
                usdTime,
                &_valueWriter);
    }

    // Write API schema attributes, strongly-typed metadata, and user-tagged
    // export attributes.
    // Write attributes on the transform first, and then attributes on the shape
    // node. This means that attribute name collisions will always be handled by
    // taking the shape node's value if we're merging transforms and shapes.
    if (dagT.isValid() && !(dagT == GetDagPath())) {
        if (usdTime.IsDefault()) {
            PxrUsdMayaWriteUtil::WriteMetadataToPrim(dagT.node(), usdPrim);
            PxrUsdMayaWriteUtil::WriteAPISchemaAttributesToPrim(
                    dagT.node(), usdPrim, _GetSparseValueWriter());
        }
        PxrUsdMayaWriteUtil::WriteUserExportedAttributes(dagT, usdPrim, usdTime,
                _GetSparseValueWriter());
    }

    if (usdTime.IsDefault()) {
        PxrUsdMayaWriteUtil::WriteMetadataToPrim(GetDagPath().node(), usdPrim);
        PxrUsdMayaWriteUtil::WriteAPISchemaAttributesToPrim(
                GetDagPath().node(), usdPrim, _GetSparseValueWriter());
    }
    PxrUsdMayaWriteUtil::WriteUserExportedAttributes(GetDagPath(), usdPrim, 
            usdTime, _GetSparseValueWriter());

    return true;
}

bool
MayaPrimWriter::ExportsGprims() const
{
    return false;
}
    
bool
MayaPrimWriter::ExportsReferences() const
{
    return false;
}

bool
MayaPrimWriter::ShouldPruneChildren() const
{
    return false;
}

void
MayaPrimWriter::PostExport()
{ 
}

void
MayaPrimWriter::SetExportVisibility(bool exportVis)
{
    _exportVisibility = exportVis;
}

bool
MayaPrimWriter::GetAllAuthoredUsdPaths(SdfPathVector* outPaths) const
{
    if (!GetUsdPath().IsEmpty()) {
        outPaths->push_back(GetUsdPath());
        return true;
    }
    return false;
}

bool
MayaPrimWriter::GetExportVisibility() const
{
    return _exportVisibility;
}

const MDagPath&
MayaPrimWriter::GetDagPath() const
{
    return _dagPath;
}

const SdfPath&
MayaPrimWriter::GetUsdPath() const
{
    return _usdPath;
}

void
MayaPrimWriter::_SetUsdPath(const SdfPath &newPath)
{
    _usdPath = newPath;
};

const UsdPrim&
MayaPrimWriter::GetUsdPrim() const
{
    return _usdPrim;
}

const UsdStageRefPtr&
MayaPrimWriter::GetUsdStage() const
{
    return _writeJobCtx.getUsdStage();
}

bool
MayaPrimWriter::IsValid() const
{
    return _isValid;
}

void
MayaPrimWriter::_SetValid(bool isValid)
{
    _isValid = isValid;
};

const PxrUsdMayaJobExportArgs&
MayaPrimWriter::_GetExportArgs() const
{
    return _writeJobCtx.getArgs();
}

UsdUtilsSparseValueWriter*
MayaPrimWriter::_GetSparseValueWriter()
{
    return &_valueWriter;
}

PXR_NAMESPACE_CLOSE_SCOPE

