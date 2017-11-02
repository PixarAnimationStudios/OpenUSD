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

/// \file usdGeom/tokens.h

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
// 
// This is an automatically generated file (by usdGenSchema.py).
// Do not hand-edit!
// 
// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \hideinitializer
#define USDGEOM_TOKENS \
    (all) \
    (angularVelocities) \
    (axis) \
    (basis) \
    (bezier) \
    (bilinear) \
    (boundaries) \
    (bounds) \
    (box) \
    (bspline) \
    (card) \
    (cards) \
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
    (cross) \
    (cubic) \
    (curveVertexCounts) \
    ((default_, "default")) \
    (doubleSided) \
    (edgeAndCorner) \
    (edgeOnly) \
    (elementSize) \
    (elementType) \
    (extent) \
    (extentsHint) \
    (face) \
    (faceSet) \
    (faceVarying) \
    (faceVaryingLinearInterpolation) \
    (faceVertexCounts) \
    (faceVertexIndices) \
    (familyName) \
    (focalLength) \
    (focusDistance) \
    (fromTexture) \
    (fStop) \
    (fullGeom) \
    (guide) \
    (height) \
    (hermite) \
    (holeIndices) \
    (horizontalAperture) \
    (horizontalApertureOffset) \
    (ids) \
    (inactiveIds) \
    (indices) \
    (inherited) \
    (interpolateBoundary) \
    (interpolation) \
    (invisible) \
    (invisibleIds) \
    (knots) \
    (left) \
    (leftHanded) \
    (linear) \
    (loop) \
    ((modelApplyDrawMode, "model:applyDrawMode")) \
    ((modelCardGeometry, "model:cardGeometry")) \
    ((modelCardTextureXNeg, "model:cardTextureXNeg")) \
    ((modelCardTextureXPos, "model:cardTextureXPos")) \
    ((modelCardTextureYNeg, "model:cardTextureYNeg")) \
    ((modelCardTextureYPos, "model:cardTextureYPos")) \
    ((modelCardTextureZNeg, "model:cardTextureZNeg")) \
    ((modelCardTextureZPos, "model:cardTextureZPos")) \
    ((modelDrawMode, "model:drawMode")) \
    ((modelDrawModeColor, "model:drawModeColor")) \
    (mono) \
    ((motionVelocityScale, "motion:velocityScale")) \
    (none) \
    (nonOverlapping) \
    (nonperiodic) \
    (normals) \
    (open) \
    (order) \
    (orientation) \
    (orientations) \
    (origin) \
    (orthographic) \
    (partition) \
    (periodic) \
    (perspective) \
    (point) \
    (points) \
    (pointWeights) \
    (positions) \
    (power) \
    ((primvarsDisplayColor, "primvars:displayColor")) \
    ((primvarsDisplayOpacity, "primvars:displayOpacity")) \
    (projection) \
    (protoIndices) \
    (prototypeDrawMode) \
    (prototypes) \
    (proxy) \
    (proxyPrim) \
    (purpose) \
    (radius) \
    (ranges) \
    (render) \
    (right) \
    (rightHanded) \
    (scales) \
    ((shutterClose, "shutter:close")) \
    ((shutterOpen, "shutter:open")) \
    (size) \
    (smooth) \
    (stereoRole) \
    (subdivisionScheme) \
    (triangleSubdivisionRule) \
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
    (unrestricted) \
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
///
/// <b>UsdGeomTokens</b> provides static, efficient TfToken's for
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
/// \li <b>angularVelocities</b> - UsdGeomPointInstancer
/// \li <b>axis</b> - UsdGeomCone, UsdGeomCapsule, UsdGeomCylinder
/// \li <b>basis</b> - UsdGeomBasisCurves
/// \li <b>bezier</b> - Possible value for UsdGeomBasisCurves::GetBasisAttr(), Default value for UsdGeomBasisCurves::GetBasisAttr()
/// \li <b>bilinear</b> - Possible value for UsdGeomMesh::GetSubdivisionSchemeAttr()
/// \li <b>boundaries</b> - Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr()
/// \li <b>bounds</b> - Possible value for UsdGeomModelAPI::GetModelDrawModeAttr()
/// \li <b>box</b> - Possible value for UsdGeomModelAPI::GetModelCardGeometryAttr()
/// \li <b>bspline</b> - Possible value for UsdGeomBasisCurves::GetBasisAttr()
/// \li <b>card</b> - Possible value for UsdGeomPointInstancer::GetPrototypeDrawModeAttr()
/// \li <b>cards</b> - Possible value for UsdGeomModelAPI::GetModelDrawModeAttr()
/// \li <b>catmullClark</b> - Possible value for UsdGeomMesh::GetSubdivisionSchemeAttr(), Default value for UsdGeomMesh::GetSubdivisionSchemeAttr(), Possible value for UsdGeomMesh::GetTriangleSubdivisionRuleAttr(), Default value for UsdGeomMesh::GetTriangleSubdivisionRuleAttr()
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
/// \li <b>cross</b> - Possible value for UsdGeomModelAPI::GetModelCardGeometryAttr()
/// \li <b>cubic</b> - Possible value for UsdGeomBasisCurves::GetTypeAttr(), Default value for UsdGeomBasisCurves::GetTypeAttr()
/// \li <b>curveVertexCounts</b> - UsdGeomCurves
/// \li <b>default_</b> - Possible value for UsdGeomModelAPI::GetModelDrawModeAttr(), Possible value for UsdGeomImageable::GetPurposeAttr(), Default value for UsdGeomImageable::GetPurposeAttr()
/// \li <b>doubleSided</b> - UsdGeomGprim
/// \li <b>edgeAndCorner</b> - Possible value for UsdGeomMesh::GetInterpolateBoundaryAttr(), Default value for UsdGeomMesh::GetInterpolateBoundaryAttr()
/// \li <b>edgeOnly</b> - Possible value for UsdGeomMesh::GetInterpolateBoundaryAttr()
/// \li <b>elementSize</b> - UsdGeomPrimvar - The number of values in the value array that must be aggregated for each element on the  primitive.
/// \li <b>elementType</b> - UsdGeomSubset
/// \li <b>extent</b> - UsdGeomCone, UsdGeomCapsule, UsdGeomCylinder, UsdGeomSphere, UsdGeomCube, UsdGeomBoundable
/// \li <b>extentsHint</b> - Name of the attribute used to author extents hints at the root of leaf models. Extents hints are stored by purpose as a vector of GfVec3f values. They are ordered based on the order of purpose tokens returned by  UsdGeomImageable::GetOrderedPurposeTokens.
/// \li <b>face</b> - Possible value for UsdGeomSubset::GetElementTypeAttr(), Default value for UsdGeomSubset::GetElementTypeAttr()
/// \li <b>faceSet</b> - <Deprecated> This is the namespace prefix used by  UsdGeomFaceSetAPI for authoring faceSet attributes.
/// \li <b>faceVarying</b> - Possible value for UsdGeomPrimVar::SetInterpolation. For polygons and subdivision surfaces, four values are interpolated over each face of the mesh. Bilinear interpolation  is used for interpolation between the four values.
/// \li <b>faceVaryingLinearInterpolation</b> - UsdGeomMesh
/// \li <b>faceVertexCounts</b> - UsdGeomMesh
/// \li <b>faceVertexIndices</b> - UsdGeomMesh
/// \li <b>familyName</b> - UsdGeomSubset
/// \li <b>focalLength</b> - UsdGeomCamera
/// \li <b>focusDistance</b> - UsdGeomCamera
/// \li <b>fromTexture</b> - Possible value for UsdGeomModelAPI::GetModelCardGeometryAttr()
/// \li <b>fStop</b> - UsdGeomCamera
/// \li <b>fullGeom</b> - Possible value for UsdGeomPointInstancer::GetPrototypeDrawModeAttr()
/// \li <b>guide</b> - Possible value for UsdGeomImageable::GetPurposeAttr()
/// \li <b>height</b> - UsdGeomCone, UsdGeomCapsule, UsdGeomCylinder
/// \li <b>hermite</b> - Possible value for UsdGeomBasisCurves::GetBasisAttr()
/// \li <b>holeIndices</b> - UsdGeomMesh
/// \li <b>horizontalAperture</b> - UsdGeomCamera
/// \li <b>horizontalApertureOffset</b> - UsdGeomCamera
/// \li <b>ids</b> - UsdGeomPointInstancer, UsdGeomPoints
/// \li <b>inactiveIds</b> - int64listop prim metadata that specifies the PointInstancer ids that should be masked (unrenderable) over all time.
/// \li <b>indices</b> - UsdGeomSubset
/// \li <b>inherited</b> - Possible value for UsdGeomImageable::GetVisibilityAttr(), Default value for UsdGeomImageable::GetVisibilityAttr()
/// \li <b>interpolateBoundary</b> - UsdGeomMesh
/// \li <b>interpolation</b> - UsdGeomPrimvar - How a Primvar interpolates across a primitive; equivalent to RenderMan's \ref Usd_InterpolationVals "class specifier" 
/// \li <b>invisible</b> - Possible value for UsdGeomImageable::GetVisibilityAttr()
/// \li <b>invisibleIds</b> - UsdGeomPointInstancer
/// \li <b>knots</b> - UsdGeomNurbsCurves
/// \li <b>left</b> - Possible value for UsdGeomCamera::GetStereoRoleAttr()
/// \li <b>leftHanded</b> - Possible value for UsdGeomGprim::GetOrientationAttr()
/// \li <b>linear</b> - Possible value for UsdGeomBasisCurves::GetTypeAttr()
/// \li <b>loop</b> - Possible value for UsdGeomMesh::GetSubdivisionSchemeAttr()
/// \li <b>modelApplyDrawMode</b> - UsdGeomModelAPI
/// \li <b>modelCardGeometry</b> - UsdGeomModelAPI
/// \li <b>modelCardTextureXNeg</b> - UsdGeomModelAPI
/// \li <b>modelCardTextureXPos</b> - UsdGeomModelAPI
/// \li <b>modelCardTextureYNeg</b> - UsdGeomModelAPI
/// \li <b>modelCardTextureYPos</b> - UsdGeomModelAPI
/// \li <b>modelCardTextureZNeg</b> - UsdGeomModelAPI
/// \li <b>modelCardTextureZPos</b> - UsdGeomModelAPI
/// \li <b>modelDrawMode</b> - UsdGeomModelAPI
/// \li <b>modelDrawModeColor</b> - UsdGeomModelAPI
/// \li <b>mono</b> - Possible value for UsdGeomCamera::GetStereoRoleAttr(), Default value for UsdGeomCamera::GetStereoRoleAttr()
/// \li <b>motionVelocityScale</b> - UsdGeomMotionAPI
/// \li <b>none</b> - Possible value for UsdGeomMesh::GetInterpolateBoundaryAttr(), Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr(), Possible value for UsdGeomMesh::GetSubdivisionSchemeAttr()
/// \li <b>nonOverlapping</b> - A type of family of GeomSubsets. It implies that  the elements in the various subsets belonging to the family are  mutually exclusive, i.e., an element that appears in one  subset may not belong to any other subset in the family.
/// \li <b>nonperiodic</b> - Possible value for UsdGeomBasisCurves::GetWrapAttr(), Default value for UsdGeomBasisCurves::GetWrapAttr()
/// \li <b>normals</b> - UsdGeomPointBased
/// \li <b>open</b> - Possible value for UsdGeomNurbsPatch::GetVFormAttr(), Default value for UsdGeomNurbsPatch::GetVFormAttr(), Possible value for UsdGeomNurbsPatch::GetUFormAttr(), Default value for UsdGeomNurbsPatch::GetUFormAttr()
/// \li <b>order</b> - UsdGeomNurbsCurves
/// \li <b>orientation</b> - UsdGeomGprim
/// \li <b>orientations</b> - UsdGeomPointInstancer
/// \li <b>origin</b> - Possible value for UsdGeomModelAPI::GetModelDrawModeAttr()
/// \li <b>orthographic</b> - Possible value for UsdGeomCamera::GetProjectionAttr()
/// \li <b>partition</b> - A type of family of GeomSubsets. It implies  that every element appears exacly once in only one of the  subsets in the family.
/// \li <b>periodic</b> - Possible value for UsdGeomBasisCurves::GetWrapAttr(), Possible value for UsdGeomNurbsPatch::GetVFormAttr(), Possible value for UsdGeomNurbsPatch::GetUFormAttr()
/// \li <b>perspective</b> - Possible value for UsdGeomCamera::GetProjectionAttr(), Default value for UsdGeomCamera::GetProjectionAttr()
/// \li <b>point</b> - Possible value for UsdGeomPointInstancer::GetPrototypeDrawModeAttr()
/// \li <b>points</b> - UsdGeomPointBased
/// \li <b>pointWeights</b> - UsdGeomNurbsPatch
/// \li <b>positions</b> - UsdGeomPointInstancer
/// \li <b>power</b> - Possible value for UsdGeomBasisCurves::GetBasisAttr()
/// \li <b>primvarsDisplayColor</b> - UsdGeomGprim
/// \li <b>primvarsDisplayOpacity</b> - UsdGeomGprim
/// \li <b>projection</b> - UsdGeomCamera
/// \li <b>protoIndices</b> - UsdGeomPointInstancer
/// \li <b>prototypeDrawMode</b> - UsdGeomPointInstancer
/// \li <b>prototypes</b> - UsdGeomPointInstancer
/// \li <b>proxy</b> - Possible value for UsdGeomImageable::GetPurposeAttr()
/// \li <b>proxyPrim</b> - UsdGeomImageable
/// \li <b>purpose</b> - UsdGeomImageable
/// \li <b>radius</b> - UsdGeomCone, UsdGeomCapsule, UsdGeomCylinder, UsdGeomSphere
/// \li <b>ranges</b> - UsdGeomNurbsCurves
/// \li <b>render</b> - Possible value for UsdGeomImageable::GetPurposeAttr()
/// \li <b>right</b> - Possible value for UsdGeomCamera::GetStereoRoleAttr()
/// \li <b>rightHanded</b> - Possible value for UsdGeomGprim::GetOrientationAttr(), Default value for UsdGeomGprim::GetOrientationAttr()
/// \li <b>scales</b> - UsdGeomPointInstancer
/// \li <b>shutterClose</b> - UsdGeomCamera
/// \li <b>shutterOpen</b> - UsdGeomCamera
/// \li <b>size</b> - UsdGeomCube
/// \li <b>smooth</b> - Possible value for UsdGeomMesh::GetTriangleSubdivisionRuleAttr()
/// \li <b>stereoRole</b> - UsdGeomCamera
/// \li <b>subdivisionScheme</b> - UsdGeomMesh
/// \li <b>triangleSubdivisionRule</b> - UsdGeomMesh
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
/// \li <b>unrestricted</b> - A type of family of GeomSubsets. It implies that there are no restrictions w.r.t. the membership of elements in  the subsets. There could be overlapping members in subsets  belonging to the family and the union of all subsets in the  family may not contain all the elements.
/// \li <b>uOrder</b> - UsdGeomNurbsPatch
/// \li <b>upAxis</b> - Stage-level metadata that encodes a scene's orientation as a token whose value can be "Y" or "Z".
/// \li <b>uRange</b> - UsdGeomNurbsPatch
/// \li <b>uVertexCount</b> - UsdGeomNurbsPatch
/// \li <b>varying</b> - Possible value for UsdGeomPrimvar::SetInterpolation. Four values are interpolated over each uv patch segment of the  surface. Bilinear interpolation is used for interpolation  between the four values.
/// \li <b>velocities</b> - UsdGeomPointInstancer, UsdGeomPointBased
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

PXR_NAMESPACE_CLOSE_SCOPE

#endif
