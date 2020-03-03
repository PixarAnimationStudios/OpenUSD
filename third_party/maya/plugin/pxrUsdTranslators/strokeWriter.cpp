//
// Copyright 2019 Pixar
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
#include "pxrUsdTranslators/strokeWriter.h"

#include "usdMaya/primWriter.h"
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/writeJobContext.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/basisCurves.h"
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <maya/MDoubleArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnPfxGeometry.h>
#include <maya/MRenderLine.h>
#include <maya/MRenderLineArray.h>
#include <maya/MStatus.h>
#include <maya/MVectorArray.h>


PXR_NAMESPACE_OPEN_SCOPE


PXRUSDMAYA_REGISTER_WRITER(stroke, PxrUsdTranslators_StrokeWriter);


PxrUsdTranslators_StrokeWriter::PxrUsdTranslators_StrokeWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx) :
    UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    if (!TF_VERIFY(GetDagPath().isValid())) {
        return;
    }

    UsdGeomBasisCurves primSchema =
        UsdGeomBasisCurves::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            primSchema,
            "Could not define UsdGeomBasisCurves at path <%s>\n",
            GetUsdPath().GetText())) {
        return;
    }

    _usdPrim = primSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdGeomBasisCurves at path <%s>\n",
            primSchema.GetPath().GetText())) {
        return;
    }
}

static
void
_CollectCurveVertexData(
        const MRenderLineArray& renderLineArray,
        VtIntArray& curveVertexCounts,
        VtVec3fArray& curvePoints,
        VtFloatArray& curveWidths,
        VtVec3fArray& curveDisplayColors,
        VtFloatArray& curveDisplayOpacities)
{
    const int renderLineArrayLength = renderLineArray.length();
    if (renderLineArrayLength < 0) {
        return;
    }

    const unsigned int numRenderLines =
        static_cast<unsigned int>(renderLineArrayLength);

    MStatus status;

    for (unsigned int lineIndex = 0u;
            lineIndex < numRenderLines;
            ++lineIndex) {
        const MRenderLine renderLine =
            renderLineArray.renderLine(lineIndex, &status);
        if (status != MS::kSuccess) {
            // Render line arrays can be sparse, so some lines may be invalid
            // and should just be skipped.
            continue;
        }

        const MVectorArray linePoints = renderLine.getLine();
        const unsigned int numPoints = linePoints.length();

        const MDoubleArray lineWidths = renderLine.getWidth();
        if (!TF_VERIFY(
                lineWidths.length() == numPoints,
                "Number of line widths (%u) does not match number of line "
                "points (%u).\n",
                lineWidths.length(),
                numPoints)) {
            continue;
        }

        const MVectorArray lineColors = renderLine.getColor();
        if (!TF_VERIFY(
                lineColors.length() == numPoints,
                "Number of line colors (%u) does not match number of line "
                "points (%u).\n",
                lineColors.length(),
                numPoints)) {
            continue;
        }

        const MVectorArray lineTransparencies = renderLine.getTransparency();
        if (!TF_VERIFY(
                lineTransparencies.length() == numPoints,
                "Number of line transparencies (%u) does not match number of "
                "line points (%u).\n",
                lineTransparencies.length(),
                numPoints)) {
            continue;
        }

        curveVertexCounts.push_back(numPoints);

        for (unsigned int vertIndex = 0u; vertIndex < numPoints; ++vertIndex) {
            const MVector& vertPoint = linePoints[vertIndex];
            curvePoints.push_back(
                GfVec3f(vertPoint[0], vertPoint[1], vertPoint[2]));

            curveWidths.push_back(static_cast<float>(lineWidths[vertIndex]));

            const MVector& vertColor = lineColors[vertIndex];
            curveDisplayColors.push_back(
                GfVec3f(vertColor[0], vertColor[1], vertColor[2]));

            // Convert transparency in Maya (a vector with zero as fully opaque
            // and one as fully transparent) into a single float opacity for
            // USD with zero as fully transparent and one as fully opaque. We
            // do this by averaging together the individual components of the
            // Maya transparency.
            const MVector& vertTransparency = lineTransparencies[vertIndex];
            const float vertOpacity = static_cast<float>(
                1.0 - ((vertTransparency[0] +
                        vertTransparency[1] +
                        vertTransparency[2]) / 3.0));
            curveDisplayOpacities.push_back(vertOpacity);
        }
    }
}

/* virtual */
void
PxrUsdTranslators_StrokeWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaPrimWriter::Write(usdTime);

    if (usdTime.IsDefault() == _HasAnimCurves()) {
        return;
    }

    MStatus status;
    MFnPfxGeometry pfxGeomFn(GetDagPath(), &status);
    if (status != MS::kSuccess) {
        TF_RUNTIME_ERROR(
            "MFnPfxGeometry() failed for stroke at DAG path: %s",
            GetDagPath().fullPathName().asChar());
        return;
    }

    MRenderLineArray mainLines;
    MRenderLineArray leafLines;
    MRenderLineArray flowerLines;

    status = pfxGeomFn.getLineData(
        mainLines,
        leafLines,
        flowerLines,
        /* doLines = */ true,
        /* doTwist = */ false,
        /* doWidth = */ true,
        /* doFlatness = */ false,
        /* doParameter = */ false,
        /* doColor = */ true,
        /* doIncandescence = */ false,
        /* doTransparency = */ true,
        /* worldSpace = */ false);
    if (status != MS::kSuccess) {
        TF_RUNTIME_ERROR(
            "Failed to get line data for stroke at DAG path: %s",
            GetDagPath().fullPathName().asChar());
        return;
    }

    VtIntArray curveVertexCounts;

    VtVec3fArray curvePoints;
    VtFloatArray curveWidths;
    VtVec3fArray curveDisplayColors;
    VtFloatArray curveDisplayOpacities;

    _CollectCurveVertexData(
        mainLines,
        curveVertexCounts,
        curvePoints,
        curveWidths,
        curveDisplayColors,
        curveDisplayOpacities);

    _CollectCurveVertexData(
        leafLines,
        curveVertexCounts,
        curvePoints,
        curveWidths,
        curveDisplayColors,
        curveDisplayOpacities);

    _CollectCurveVertexData(
        flowerLines,
        curveVertexCounts,
        curvePoints,
        curveWidths,
        curveDisplayColors,
        curveDisplayOpacities);

    UsdGeomBasisCurves curvesSchema(_usdPrim);

    curvesSchema.CreateTypeAttr(VtValue(UsdGeomTokens->linear));

    UsdAttribute curveVertexCountsAttr =
        curvesSchema.CreateCurveVertexCountsAttr();
    curveVertexCountsAttr.Set(curveVertexCounts, usdTime);

    UsdAttribute curvePointsAttr = curvesSchema.CreatePointsAttr();
    curvePointsAttr.Set(curvePoints, usdTime);

    UsdAttribute curveWidthsAttr = curvesSchema.CreateWidthsAttr();
    curveWidthsAttr.Set(curveWidths, usdTime);

    UsdGeomPrimvar displayColorPrimvar =
        curvesSchema.CreateDisplayColorPrimvar(UsdGeomTokens->vertex);
    displayColorPrimvar.Set(curveDisplayColors, usdTime);

    UsdGeomPrimvar displayOpacityPrimvar =
        curvesSchema.CreateDisplayOpacityPrimvar(UsdGeomTokens->vertex);
    displayOpacityPrimvar.Set(curveDisplayOpacities, usdTime);

    mainLines.deleteArray();
    leafLines.deleteArray();
    flowerLines.deleteArray();
}


PXR_NAMESPACE_CLOSE_SCOPE
