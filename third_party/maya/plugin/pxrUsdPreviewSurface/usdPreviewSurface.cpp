//
// Copyright 2018 Pixar
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
#include "pxrUsdPreviewSurface/usdPreviewSurface.h"

#include "pxr/pxr.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFloatVector.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MVector.h>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(
    PxrMayaUsdPreviewSurfaceTokens,
    PXRUSDPREVIEWSURFACE_USD_PREVIEW_SURFACE_TOKENS);


const MTypeId PxrMayaUsdPreviewSurface::typeId(0x00126403);
const MString PxrMayaUsdPreviewSurface::typeName(
    PxrMayaUsdPreviewSurfaceTokens->MayaTypeName.GetText());

const MString PxrMayaUsdPreviewSurface::drawDbClassification(
    TfStringPrintf(
        "drawdb/shader/surface/%s",
        PxrMayaUsdPreviewSurfaceTokens->MayaTypeName.GetText()).c_str());
const MString PxrMayaUsdPreviewSurface::fullClassification(
    TfStringPrintf(
        "shader/surface:shader/displacement:%s",
        PxrMayaUsdPreviewSurface::drawDbClassification.asChar()).c_str());

// Attributes
MObject PxrMayaUsdPreviewSurface::clearcoatAttr;
MObject PxrMayaUsdPreviewSurface::clearcoatRoughnessAttr;
MObject PxrMayaUsdPreviewSurface::diffuseColorAttr;
MObject PxrMayaUsdPreviewSurface::displacementAttr;
MObject PxrMayaUsdPreviewSurface::emissiveColorAttr;
MObject PxrMayaUsdPreviewSurface::iorAttr;
MObject PxrMayaUsdPreviewSurface::metallicAttr;
MObject PxrMayaUsdPreviewSurface::normalAttr;
MObject PxrMayaUsdPreviewSurface::occlusionAttr;
MObject PxrMayaUsdPreviewSurface::opacityAttr;
MObject PxrMayaUsdPreviewSurface::roughnessAttr;
MObject PxrMayaUsdPreviewSurface::specularColorAttr;
MObject PxrMayaUsdPreviewSurface::useSpecularWorkflowAttr;

// Output Attributes
MObject PxrMayaUsdPreviewSurface::outColorAttr;
MObject PxrMayaUsdPreviewSurface::outTransparencyAttr;


/* static */
void*
PxrMayaUsdPreviewSurface::creator()
{
    return new PxrMayaUsdPreviewSurface();
}

/* static */
MStatus
PxrMayaUsdPreviewSurface::initialize()
{
    MStatus status;

    MFnNumericAttribute numericAttrFn;

    clearcoatAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName.GetText(),
        "cc",
        MFnNumericData::kFloat,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(clearcoatAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    clearcoatRoughnessAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName.GetText(),
        "ccr",
        MFnNumericData::kFloat,
        0.01,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setMin(0.001);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.001);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(clearcoatRoughnessAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    diffuseColorAttr = numericAttrFn.createColor(
        PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName.GetText(),
        "dc",
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setDefault(0.18f, 0.18f, 0.18f);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(diffuseColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    displacementAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName.GetText(),
        "dsp",
        MFnNumericData::kFloat,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(displacementAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    emissiveColorAttr = numericAttrFn.createColor(
        PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName.GetText(),
        "ec",
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(emissiveColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    iorAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->IorAttrName.GetText(),
        "ior",
        MFnNumericData::kFloat,
        1.5,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(iorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    metallicAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName.GetText(),
        "mtl",
        MFnNumericData::kFloat,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(metallicAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    normalAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->NormalAttrName.GetText(),
        "nrm",
        MFnNumericData::k3Float,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    const MVector upAxis = MGlobal::upAxis(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setDefault(upAxis[0], upAxis[1], upAxis[2]);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(normalAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    occlusionAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->OcclusionAttrName.GetText(),
        "ocl",
        MFnNumericData::kFloat,
        1.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(occlusionAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    opacityAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName.GetText(),
        "opc",
        MFnNumericData::kFloat,
        1.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(opacityAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    roughnessAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName.GetText(),
        "rgh",
        MFnNumericData::kFloat,
        0.5,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setMin(0.001);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMin(0.001);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setSoftMax(1.0);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(roughnessAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    specularColorAttr = numericAttrFn.createColor(
        PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName.GetText(),
        "spc",
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(specularColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    useSpecularWorkflowAttr = numericAttrFn.create(
        PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName.GetText(),
        "usw",
        MFnNumericData::kBoolean,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setKeyable(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(useSpecularWorkflowAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    outColorAttr = numericAttrFn.createColor(
        PxrMayaUsdPreviewSurfaceTokens->OutColorAttrName.GetText(),
        "oc",
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setWritable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setStorable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    outTransparencyAttr = numericAttrFn.createColor(
        PxrMayaUsdPreviewSurfaceTokens->OutTransparencyAttrName.GetText(),
        "ot",
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setWritable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setStorable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(outTransparencyAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Note that we make *all* attributes affect "outColor". During export, we
    // use Maya's MItDependencyGraph iterator to traverse connected plugs
    // upstream in the network beginning at the shading engine's shader plugs
    // (e.g. "surfaceShader"). The iterator will not traverse plugs that it
    // does not know affect connections downstream. For example, if this shader
    // has connections for both "diffuseColor" and "roughness", but we only
    // declared the attribute affects relationship for "diffuseColor", then 
    // only "diffuseColor" would be visited and "roughness" would be skipped
    // during the traversal, since the plug upstream of the shading engine's
    // "surfaceShader" plug is this shader's "outColor" attribute, which Maya
    // knows is affected by "diffuseColor".
    status = attributeAffects(clearcoatAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(clearcoatRoughnessAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(diffuseColorAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(displacementAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(emissiveColorAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(iorAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(metallicAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(normalAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(occlusionAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(opacityAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(roughnessAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(specularColorAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(useSpecularWorkflowAttr, outColorAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = attributeAffects(opacityAttr, outTransparencyAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}

/* virtual */
void
PxrMayaUsdPreviewSurface::postConstructor()
{
    setMPSafe(true);
}

/* virtual */
MStatus
PxrMayaUsdPreviewSurface::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    MStatus status = MS::kUnknownParameter;

    // XXX: For now, we simply propagate diffuseColor to outColor and
    // opacity to outTransparency.

    if (plug == outColorAttr) {
        const MDataHandle diffuseColorData =
            dataBlock.inputValue(diffuseColorAttr, &status);
        CHECK_MSTATUS(status);
        const MFloatVector diffuseColor = diffuseColorData.asFloatVector();

        MDataHandle outColorHandle =
            dataBlock.outputValue(outColorAttr, &status);
        CHECK_MSTATUS(status);
        outColorHandle.asFloatVector() = diffuseColor;
        status = dataBlock.setClean(outColorAttr);
        CHECK_MSTATUS(status);
    }
    else if (plug == outTransparencyAttr) {
        const MDataHandle opacityData =
            dataBlock.inputValue(opacityAttr, &status);
        CHECK_MSTATUS(status);
        const float opacity = opacityData.asFloat();

        const float transparency = 1.0f - opacity;
        const MFloatVector transparencyColor(
            transparency,
            transparency,
            transparency);
        MDataHandle outTransparencyHandle =
            dataBlock.outputValue(outTransparencyAttr, &status);
        CHECK_MSTATUS(status);
        outTransparencyHandle.asFloatVector() = transparencyColor;
        status = dataBlock.setClean(outTransparencyAttr);
        CHECK_MSTATUS(status);
    }

    return status;
}

PxrMayaUsdPreviewSurface::PxrMayaUsdPreviewSurface() : MPxNode()
{
}

/* virtual */
PxrMayaUsdPreviewSurface::~PxrMayaUsdPreviewSurface()
{
}


PXR_NAMESPACE_CLOSE_SCOPE
