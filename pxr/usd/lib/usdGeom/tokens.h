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
#ifndef USDGEOM_TOKENS_H
#define USDGEOM_TOKENS_H

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/usd/usdGeom/api.h"
#include "pxr/base/tf/staticTokens.h"

/// \file pxr/usd/usdGeom/tokens.h

/// \hideinitializer
#define USDGEOM_TOKENS \
    (all) \
    (alwaysSharp) \
    (axis) \
    (basis) \
    (bezier) \
    (bilinear) \
    (boundaries) \
    (bspline) \
    (catmullClark) \
    (catmullRom) \
    (clippingPlanes) \
    (clippingRange) \
    (closed) \
    (collection) \
    (constant) \
    (cornerIndices) \
    (cornerSharpnesses) \
    (cornersOnly) \
    (cornersPlus1) \
    (cornersPlus2) \
    (creaseIndices) \
    (creaseLengths) \
    (creaseSharpnesses) \
    (cubic) \
    (curveVertexCounts) \
    ((default_, "default")) \
    (doubleSided) \
    (edgeAndCorner) \
    (edgeOnly) \
    (elementSize) \
    (extent) \
    (extentsHint) \
    (faceSet) \
    (faceVarying) \
    (faceVaryingInterpolateBoundary) \
    (faceVaryingLinearInterpolation) \
    (faceVertexCounts) \
    (faceVertexIndices) \
    (focalLength) \
    (focusDistance) \
    (fStop) \
    (guide) \
    (height) \
    (hermite) \
    (holeIndices) \
    (horizontalAperture) \
    (horizontalApertureOffset) \
    (ids) \
    (inherited) \
    (interpolateBoundary) \
    (interpolation) \
    (invisible) \
    (knots) \
    (left) \
    (leftHanded) \
    (linear) \
    (loop) \
    (mono) \
    (none) \
    (nonperiodic) \
    (normals) \
    (open) \
    (order) \
    (orientation) \
    (orthographic) \
    (periodic) \
    (perspective) \
    (points) \
    (pointWeights) \
    (power) \
    ((primvarsDisplayColor, "primvars:displayColor")) \
    ((primvarsDisplayOpacity, "primvars:displayOpacity")) \
    (projection) \
    (proxy) \
    (purpose) \
    (radius) \
    (ranges) \
    (render) \
    (right) \
    (rightHanded) \
    (size) \
    (stereoRole) \
    (subdivisionScheme) \
    ((trimCurveCounts, "trimCurve:counts")) \
    ((trimCurveKnots, "trimCurve:knots")) \
    ((trimCurveOrders, "trimCurve:orders")) \
    ((trimCurvePoints, "trimCurve:points")) \
    ((trimCurveRanges, "trimCurve:ranges")) \
    ((trimCurveVertexCounts, "trimCurve:vertexCounts")) \
    (type) \
    (uForm) \
    (uKnots) \
    (unauthoredValuesIndex) \
    (uniform) \
    (uOrder) \
    (upAxis) \
    (uRange) \
    (uVertexCount) \
    (varying) \
    (velocities) \
    (vertex) \
    (verticalAperture) \
    (verticalApertureOffset) \
    (vForm) \
    (visibility) \
    (vKnots) \
    (vOrder) \
    (vRange) \
    (vVertexCount) \
    (widths) \
    (wrap) \
    ((x, "X")) \
    (xformOpOrder) \
    ((y, "Y")) \
    ((z, "Z"))

/// \anchor UsdGeomTokens
/// \brief <b>UsdGeomTokens</b> provides static, efficient TfToken's for
/// use in all public USD API
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdGeomTokens also contains all of the \em allowedTokens values declared
/// for schema builtin attributes of 'token' scene description type.
/// Use UsdGeomTokens like so:
///
/// \code
///     gprim.GetVisibilityAttr().Set(UsdGeomTokens->invisible);
/// \endcode
///
/// The tokens are:
/// \li <b>all</b> - Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr()
/// \li <b>alwaysSharp</b> - Legacy token representing a deprecated  faceVaryingInterpolateBoundary state. The modern equivalent is UsdGeomTokens->boundaries.
/// \li <b>axis</b> - UsdGeomCone, UsdGeomCapsule, UsdGeomCylinder
/// \li <b>basis</b> - UsdGeomBasisCurves
/// \li <b>bezier</b> - Possible value for UsdGeomBasisCurves::GetBasisAttr(), Default value for UsdGeomBasisCurves::GetBasisAttr()
/// \li <b>bilinear</b> - Legacy token representing a deprecated  faceVaryingInterpolateBoundary state. The modern equivalent is UsdGeomTokens->all., Possible value for UsdGeomMesh::GetSubdivisionSchemeAttr()
/// \li <b>boundaries</b> - Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr()
/// \li <b>bspline</b> - Possible value for UsdGeomBasisCurves::GetBasisAttr()
/// \li <b>catmullClark</b> - Possible value for UsdGeomMesh::GetSubdivisionSchemeAttr(), Default value for UsdGeomMesh::GetSubdivisionSchemeAttr()
/// \li <b>catmullRom</b> - Possible value for UsdGeomBasisCurves::GetBasisAttr()
/// \li <b>clippingPlanes</b> - UsdGeomCamera
/// \li <b>clippingRange</b> - UsdGeomCamera
/// \li <b>closed</b> - Possible value for UsdGeomNurbsPatch::GetVFormAttr(), Possible value for UsdGeomNurbsPatch::GetUFormAttr()
/// \li <b>collection</b> - This is the namespace prefix used by  UsdGeomCollectionAPI for authoring collections.
/// \li <b>constant</b> - Possible value for UsdGeomPrimvar::SetInterpolation. Default value for UsdGeomPrimvar::GetInterpolation. One value remains constant over the entire surface primitive.
/// \li <b>cornerIndices</b> - UsdGeomMesh
/// \li <b>cornerSharpnesses</b> - UsdGeomMesh
/// \li <b>cornersOnly</b> - Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr()
/// \li <b>cornersPlus1</b> - Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr(), Default value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr()
/// \li <b>cornersPlus2</b> - Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr()
/// \li <b>creaseIndices</b> - UsdGeomMesh
/// \li <b>creaseLengths</b> - UsdGeomMesh
/// \li <b>creaseSharpnesses</b> - UsdGeomMesh
/// \li <b>cubic</b> - Possible value for UsdGeomBasisCurves::GetTypeAttr(), Default value for UsdGeomBasisCurves::GetTypeAttr()
/// \li <b>curveVertexCounts</b> - UsdGeomCurves
/// \li <b>default_</b> - Possible value for UsdGeomImageable::GetPurposeAttr(), Default value for UsdGeomImageable::GetPurposeAttr()
/// \li <b>doubleSided</b> - UsdGeomGprim
/// \li <b>edgeAndCorner</b> - Legacy token representing a deprecated  faceVaryingInterpolateBoundary state. The modern equivalent is UsdGeomTokens->cornersPlus1, Possible value for UsdGeomMesh::GetInterpolateBoundaryAttr(), Default value for UsdGeomMesh::GetInterpolateBoundaryAttr()
/// \li <b>edgeOnly</b> - Legacy token representing a deprecated  faceVaryingInterpolateBoundary state. The modern equivalent is UsdGeomTokens->none., Possible value for UsdGeomMesh::GetInterpolateBoundaryAttr()
/// \li <b>elementSize</b> - UsdGeomPrimvar - The number of values in the value array that must be aggregated for each element on the  primitive.
/// \li <b>extent</b> - UsdGeomCone, UsdGeomCapsule, UsdGeomCylinder, UsdGeomSphere, UsdGeomCube, UsdGeomBoundable
/// \li <b>extentsHint</b> - Name of the attribute used to author extents hints at the root of leaf models. Extents hints are stored by purpose as a vector of GfVec3f values. They are ordered based on the order of purpose tokens returned by  UsdGeomImageable::GetOrderedPurposeTokens.
/// \li <b>faceSet</b> - This is the namespace prefix used by  UsdGeomFaceSetAPI for authoring faceSet attributes.
/// \li <b>faceVarying</b> - Possible value for UsdGeomPrimVar::SetInterpolation. For polygons and subdivision surfaces, four values are interpolated over each face of the mesh. Bilinear interpolation  is used for interpolation between the four values.
/// \li <b>faceVaryingInterpolateBoundary</b> - Legacy token. The modern equivalent is faceVaryingLinearInterpolation.
/// \li <b>faceVaryingLinearInterpolation</b> - UsdGeomMesh
/// \li <b>faceVertexCounts</b> - UsdGeomMesh
/// \li <b>faceVertexIndices</b> - UsdGeomMesh
/// \li <b>focalLength</b> - UsdGeomCamera
/// \li <b>focusDistance</b> - UsdGeomCamera
/// \li <b>fStop</b> - UsdGeomCamera
/// \li <b>guide</b> - Possible value for UsdGeomImageable::GetPurposeAttr()
/// \li <b>height</b> - UsdGeomCone, UsdGeomCapsule, UsdGeomCylinder
/// \li <b>hermite</b> - Possible value for UsdGeomBasisCurves::GetBasisAttr()
/// \li <b>holeIndices</b> - UsdGeomMesh
/// \li <b>horizontalAperture</b> - UsdGeomCamera
/// \li <b>horizontalApertureOffset</b> - UsdGeomCamera
/// \li <b>ids</b> - UsdGeomPoints
/// \li <b>inherited</b> - Possible value for UsdGeomImageable::GetVisibilityAttr(), Default value for UsdGeomImageable::GetVisibilityAttr()
/// \li <b>interpolateBoundary</b> - UsdGeomMesh
/// \li <b>interpolation</b> - UsdGeomPrimvar - How a Primvar interpolates across a primitive; equivalent to RenderMan's \ref Usd_InterpolationVals "class specifier" 
/// \li <b>invisible</b> - Possible value for UsdGeomImageable::GetVisibilityAttr()
/// \li <b>knots</b> - UsdGeomNurbsCurves
/// \li <b>left</b> - Possible value for UsdGeomCamera::GetStereoRoleAttr()
/// \li <b>leftHanded</b> - Possible value for UsdGeomGprim::GetOrientationAttr()
/// \li <b>linear</b> - Possible value for UsdGeomBasisCurves::GetTypeAttr()
/// \li <b>loop</b> - Possible value for UsdGeomMesh::GetSubdivisionSchemeAttr()
/// \li <b>mono</b> - Possible value for UsdGeomCamera::GetStereoRoleAttr(), Default value for UsdGeomCamera::GetStereoRoleAttr()
/// \li <b>none</b> - Possible value for UsdGeomMesh::GetInterpolateBoundaryAttr(), Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr(), Possible value for UsdGeomMesh::GetSubdivisionSchemeAttr()
/// \li <b>nonperiodic</b> - Possible value for UsdGeomBasisCurves::GetWrapAttr(), Default value for UsdGeomBasisCurves::GetWrapAttr()
/// \li <b>normals</b> - UsdGeomPointBased
/// \li <b>open</b> - Possible value for UsdGeomNurbsPatch::GetVFormAttr(), Default value for UsdGeomNurbsPatch::GetVFormAttr(), Possible value for UsdGeomNurbsPatch::GetUFormAttr(), Default value for UsdGeomNurbsPatch::GetUFormAttr()
/// \li <b>order</b> - UsdGeomNurbsCurves
/// \li <b>orientation</b> - UsdGeomGprim
/// \li <b>orthographic</b> - Possible value for UsdGeomCamera::GetProjectionAttr()
/// \li <b>periodic</b> - Possible value for UsdGeomBasisCurves::GetWrapAttr(), Possible value for UsdGeomNurbsPatch::GetVFormAttr(), Possible value for UsdGeomNurbsPatch::GetUFormAttr()
/// \li <b>perspective</b> - Possible value for UsdGeomCamera::GetProjectionAttr(), Default value for UsdGeomCamera::GetProjectionAttr()
/// \li <b>points</b> - UsdGeomPointBased
/// \li <b>pointWeights</b> - UsdGeomNurbsPatch
/// \li <b>power</b> - Possible value for UsdGeomBasisCurves::GetBasisAttr()
/// \li <b>primvarsDisplayColor</b> - UsdGeomGprim
/// \li <b>primvarsDisplayOpacity</b> - UsdGeomGprim
/// \li <b>projection</b> - UsdGeomCamera
/// \li <b>proxy</b> - Possible value for UsdGeomImageable::GetPurposeAttr()
/// \li <b>purpose</b> - UsdGeomImageable
/// \li <b>radius</b> - UsdGeomCone, UsdGeomCapsule, UsdGeomCylinder, UsdGeomSphere
/// \li <b>ranges</b> - UsdGeomNurbsCurves
/// \li <b>render</b> - Possible value for UsdGeomImageable::GetPurposeAttr()
/// \li <b>right</b> - Possible value for UsdGeomCamera::GetStereoRoleAttr()
/// \li <b>rightHanded</b> - Possible value for UsdGeomGprim::GetOrientationAttr(), Default value for UsdGeomGprim::GetOrientationAttr()
/// \li <b>size</b> - UsdGeomCube
/// \li <b>stereoRole</b> - UsdGeomCamera
/// \li <b>subdivisionScheme</b> - UsdGeomMesh
/// \li <b>trimCurveCounts</b> - UsdGeomNurbsPatch
/// \li <b>trimCurveKnots</b> - UsdGeomNurbsPatch
/// \li <b>trimCurveOrders</b> - UsdGeomNurbsPatch
/// \li <b>trimCurvePoints</b> - UsdGeomNurbsPatch
/// \li <b>trimCurveRanges</b> - UsdGeomNurbsPatch
/// \li <b>trimCurveVertexCounts</b> - UsdGeomNurbsPatch
/// \li <b>type</b> - UsdGeomBasisCurves
/// \li <b>uForm</b> - UsdGeomNurbsPatch
/// \li <b>uKnots</b> - UsdGeomNurbsPatch
/// \li <b>unauthoredValuesIndex</b> - UsdGeomPrimvar - The index that represents  unauthored values in the indices array of an indexed primvar.
/// \li <b>uniform</b> - Possible value for UsdGeomPrimvar::SetInterpolation. One value remains constant for each uv patch segment of the surface primitive (which is a \em face for meshes).
/// \li <b>uOrder</b> - UsdGeomNurbsPatch
/// \li <b>upAxis</b> - Stage-level metadata that encodes a scene's orientation as a token whose value can be "Y" or "Z".
/// \li <b>uRange</b> - UsdGeomNurbsPatch
/// \li <b>uVertexCount</b> - UsdGeomNurbsPatch
/// \li <b>varying</b> - Possible value for UsdGeomPrimvar::SetInterpolation. Four values are interpolated over each uv patch segment of the  surface. Bilinear interpolation is used for interpolation  between the four values.
/// \li <b>velocities</b> - UsdGeomPointBased
/// \li <b>vertex</b> - Possible value for UsdGeomPrimvar::SetInterpolation. Values are interpolated between each vertex in the surface primitive. The basis function of the surface is used for  interpolation between vertices.
/// \li <b>verticalAperture</b> - UsdGeomCamera
/// \li <b>verticalApertureOffset</b> - UsdGeomCamera
/// \li <b>vForm</b> - UsdGeomNurbsPatch
/// \li <b>visibility</b> - UsdGeomImageable
/// \li <b>vKnots</b> - UsdGeomNurbsPatch
/// \li <b>vOrder</b> - UsdGeomNurbsPatch
/// \li <b>vRange</b> - UsdGeomNurbsPatch
/// \li <b>vVertexCount</b> - UsdGeomNurbsPatch
/// \li <b>widths</b> - UsdGeomPoints, UsdGeomCurves
/// \li <b>wrap</b> - UsdGeomBasisCurves
/// \li <b>x</b> - Possible value for UsdGeomCone::GetAxisAttr(), Possible value for UsdGeomCapsule::GetAxisAttr(), Possible value for UsdGeomCylinder::GetAxisAttr()
/// \li <b>xformOpOrder</b> - UsdGeomXformable
/// \li <b>y</b> - Possible value for UsdGeomCone::GetAxisAttr(), Possible value for UsdGeomCapsule::GetAxisAttr(), Possible value for UsdGeomCylinder::GetAxisAttr()
/// \li <b>z</b> - Possible value for UsdGeomCone::GetAxisAttr(), Default value for UsdGeomCone::GetAxisAttr(), Possible value for UsdGeomCapsule::GetAxisAttr(), Default value for UsdGeomCapsule::GetAxisAttr(), Possible value for UsdGeomCylinder::GetAxisAttr(), Default value for UsdGeomCylinder::GetAxisAttr()
TF_DECLARE_PUBLIC_TOKENS(UsdGeomTokens, USDGEOM_API, USDGEOM_TOKENS);

#endif
