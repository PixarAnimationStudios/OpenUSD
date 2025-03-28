//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdGeomTokensType
///
/// \link UsdGeomTokens \endlink provides static, efficient
/// \link TfToken TfTokens\endlink for use in all public USD API.
///
/// These tokens are auto-generated from the module's schema, representing
/// property names, for when you need to fetch an attribute or relationship
/// directly by name, e.g. UsdPrim::GetAttribute(), in the most efficient
/// manner, and allow the compiler to verify that you spelled the name
/// correctly.
///
/// UsdGeomTokens also contains all of the \em allowedTokens values
/// declared for schema builtin attributes of 'token' scene description type.
/// Use UsdGeomTokens like so:
///
/// \code
///     gprim.GetMyTokenValuedAttr().Set(UsdGeomTokens->accelerations);
/// \endcode
struct UsdGeomTokensType {
    USDGEOM_API UsdGeomTokensType();
    /// \brief "accelerations"
    /// 
    /// UsdGeomPointBased, UsdGeomPointInstancer
    const TfToken accelerations;
    /// \brief "all"
    /// 
    /// Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr()
    const TfToken all;
    /// \brief "angularVelocities"
    /// 
    /// UsdGeomPointInstancer
    const TfToken angularVelocities;
    /// \brief "axis"
    /// 
    /// UsdGeomCylinder, UsdGeomCapsule, UsdGeomCone, UsdGeomCylinder_1, UsdGeomCapsule_1, UsdGeomPlane
    const TfToken axis;
    /// \brief "basis"
    /// 
    /// UsdGeomBasisCurves
    const TfToken basis;
    /// \brief "bezier"
    /// 
    /// Fallback value for UsdGeomBasisCurves::GetBasisAttr()
    const TfToken bezier;
    /// \brief "bilinear"
    /// 
    /// Possible value for UsdGeomMesh::GetSubdivisionSchemeAttr()
    const TfToken bilinear;
    /// \brief "boundaries"
    /// 
    /// Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr()
    const TfToken boundaries;
    /// \brief "bounds"
    /// 
    /// Possible value for UsdGeomModelAPI::GetModelDrawModeAttr()
    const TfToken bounds;
    /// \brief "box"
    /// 
    /// Possible value for UsdGeomModelAPI::GetModelCardGeometryAttr()
    const TfToken box;
    /// \brief "bspline"
    /// 
    /// Possible value for UsdGeomBasisCurves::GetBasisAttr()
    const TfToken bspline;
    /// \brief "cards"
    /// 
    /// Possible value for UsdGeomModelAPI::GetModelDrawModeAttr()
    const TfToken cards;
    /// \brief "catmullClark"
    /// 
    /// Fallback value for UsdGeomMesh::GetSubdivisionSchemeAttr(), Fallback value for UsdGeomMesh::GetTriangleSubdivisionRuleAttr()
    const TfToken catmullClark;
    /// \brief "catmullRom"
    /// 
    /// Possible value for UsdGeomBasisCurves::GetBasisAttr()
    const TfToken catmullRom;
    /// \brief "clippingPlanes"
    /// 
    /// UsdGeomCamera
    const TfToken clippingPlanes;
    /// \brief "clippingRange"
    /// 
    /// UsdGeomCamera
    const TfToken clippingRange;
    /// \brief "closed"
    /// 
    /// Possible value for UsdGeomNurbsPatch::GetUFormAttr(), Possible value for UsdGeomNurbsPatch::GetVFormAttr()
    const TfToken closed;
    /// \brief "constant"
    /// 
    /// Possible value for UsdGeomPrimvar::SetInterpolation. Default value for UsdGeomPrimvar::GetInterpolation. One value remains constant over the entire surface primitive.
    const TfToken constant;
    /// \brief "cornerIndices"
    /// 
    /// UsdGeomMesh
    const TfToken cornerIndices;
    /// \brief "cornerSharpnesses"
    /// 
    /// UsdGeomMesh
    const TfToken cornerSharpnesses;
    /// \brief "cornersOnly"
    /// 
    /// Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr()
    const TfToken cornersOnly;
    /// \brief "cornersPlus1"
    /// 
    /// Fallback value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr()
    const TfToken cornersPlus1;
    /// \brief "cornersPlus2"
    /// 
    /// Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr()
    const TfToken cornersPlus2;
    /// \brief "creaseIndices"
    /// 
    /// UsdGeomMesh
    const TfToken creaseIndices;
    /// \brief "creaseLengths"
    /// 
    /// UsdGeomMesh
    const TfToken creaseLengths;
    /// \brief "creaseSharpnesses"
    /// 
    /// UsdGeomMesh
    const TfToken creaseSharpnesses;
    /// \brief "cross"
    /// 
    /// Fallback value for UsdGeomModelAPI::GetModelCardGeometryAttr()
    const TfToken cross;
    /// \brief "cubic"
    /// 
    /// Fallback value for UsdGeomBasisCurves::GetTypeAttr()
    const TfToken cubic;
    /// \brief "curveVertexCounts"
    /// 
    /// UsdGeomCurves
    const TfToken curveVertexCounts;
    /// \brief "default"
    /// 
    /// Fallback value for UsdGeomImageable::GetPurposeAttr(), Possible value for UsdGeomModelAPI::GetModelDrawModeAttr()
    const TfToken default_;
    /// \brief "doubleSided"
    /// 
    /// UsdGeomGprim, UsdGeomPlane
    const TfToken doubleSided;
    /// \brief "edge"
    /// 
    /// Possible value for UsdGeomSubset::GetElementTypeAttr()
    const TfToken edge;
    /// \brief "edgeAndCorner"
    /// 
    /// Fallback value for UsdGeomMesh::GetInterpolateBoundaryAttr()
    const TfToken edgeAndCorner;
    /// \brief "edgeOnly"
    /// 
    /// Possible value for UsdGeomMesh::GetInterpolateBoundaryAttr()
    const TfToken edgeOnly;
    /// \brief "elementSize"
    /// 
    /// UsdGeomPrimvar - The number of values in the value array that must be aggregated for each element on the  primitive.
    const TfToken elementSize;
    /// \brief "elementType"
    /// 
    /// UsdGeomSubset
    const TfToken elementType;
    /// \brief "exposure"
    /// 
    /// UsdGeomCamera
    const TfToken exposure;
    /// \brief "exposure:fStop"
    /// 
    /// UsdGeomCamera
    const TfToken exposureFStop;
    /// \brief "exposure:iso"
    /// 
    /// UsdGeomCamera
    const TfToken exposureIso;
    /// \brief "exposure:responsivity"
    /// 
    /// UsdGeomCamera
    const TfToken exposureResponsivity;
    /// \brief "exposure:time"
    /// 
    /// UsdGeomCamera
    const TfToken exposureTime;
    /// \brief "extent"
    /// 
    /// UsdGeomBoundable, UsdGeomCube, UsdGeomSphere, UsdGeomCylinder, UsdGeomCapsule, UsdGeomCone, UsdGeomCylinder_1, UsdGeomCapsule_1, UsdGeomPlane
    const TfToken extent;
    /// \brief "extentsHint"
    /// 
    /// Name of the attribute used to author extents hints at the root of leaf models. Extents hints are stored by purpose as a vector of GfVec3f values. They are ordered based on the order of purpose tokens returned by  UsdGeomImageable::GetOrderedPurposeTokens.
    const TfToken extentsHint;
    /// \brief "face"
    /// 
    /// Fallback value for UsdGeomSubset::GetElementTypeAttr()
    const TfToken face;
    /// \brief "faceVarying"
    /// 
    /// Possible value for UsdGeomPrimvar::SetInterpolation. For polygons and subdivision surfaces, four values are interpolated over each face of the mesh. Bilinear interpolation  is used for interpolation between the four values.
    const TfToken faceVarying;
    /// \brief "faceVaryingLinearInterpolation"
    /// 
    /// UsdGeomMesh
    const TfToken faceVaryingLinearInterpolation;
    /// \brief "faceVertexCounts"
    /// 
    /// UsdGeomMesh
    const TfToken faceVertexCounts;
    /// \brief "faceVertexIndices"
    /// 
    /// UsdGeomMesh
    const TfToken faceVertexIndices;
    /// \brief "familyName"
    /// 
    /// UsdGeomSubset
    const TfToken familyName;
    /// \brief "focalLength"
    /// 
    /// UsdGeomCamera
    const TfToken focalLength;
    /// \brief "focusDistance"
    /// 
    /// UsdGeomCamera
    const TfToken focusDistance;
    /// \brief "fromTexture"
    /// 
    /// Possible value for UsdGeomModelAPI::GetModelCardGeometryAttr()
    const TfToken fromTexture;
    /// \brief "fStop"
    /// 
    /// UsdGeomCamera
    const TfToken fStop;
    /// \brief "guide"
    /// 
    /// Possible value for UsdGeomImageable::GetPurposeAttr()
    const TfToken guide;
    /// \brief "guideVisibility"
    /// 
    /// UsdGeomVisibilityAPI
    const TfToken guideVisibility;
    /// \brief "height"
    /// 
    /// UsdGeomCylinder, UsdGeomCapsule, UsdGeomCone, UsdGeomCylinder_1, UsdGeomCapsule_1
    const TfToken height;
    /// \brief "hermite"
    /// 
    /// A deprecated basis token for UsdGeomBasisCurves. Consumers of USD should transition to using the UsdGeomHermiteCurves schema.
    const TfToken hermite;
    /// \brief "holeIndices"
    /// 
    /// UsdGeomMesh
    const TfToken holeIndices;
    /// \brief "horizontalAperture"
    /// 
    /// UsdGeomCamera
    const TfToken horizontalAperture;
    /// \brief "horizontalApertureOffset"
    /// 
    /// UsdGeomCamera
    const TfToken horizontalApertureOffset;
    /// \brief "ids"
    /// 
    /// UsdGeomPoints, UsdGeomPointInstancer
    const TfToken ids;
    /// \brief "inactiveIds"
    /// 
    /// int64listop prim metadata that specifies the PointInstancer ids that should be masked (unrenderable) over all time.
    const TfToken inactiveIds;
    /// \brief "indices"
    /// 
    /// UsdGeomSubset
    const TfToken indices;
    /// \brief "inherited"
    /// 
    /// Fallback value for UsdGeomImageable::GetVisibilityAttr(), Possible value for UsdGeomVisibilityAPI::GetGuideVisibilityAttr(), Fallback value for UsdGeomVisibilityAPI::GetProxyVisibilityAttr(), Fallback value for UsdGeomVisibilityAPI::GetRenderVisibilityAttr(), Fallback value for UsdGeomModelAPI::GetModelDrawModeAttr()
    const TfToken inherited;
    /// \brief "interpolateBoundary"
    /// 
    /// UsdGeomMesh
    const TfToken interpolateBoundary;
    /// \brief "interpolation"
    /// 
    /// UsdGeomPrimvar - How a Primvar interpolates across a primitive; equivalent to RenderMan's \ref Usd_InterpolationVals "class specifier" 
    const TfToken interpolation;
    /// \brief "invisible"
    /// 
    /// Possible value for UsdGeomImageable::GetVisibilityAttr(), Fallback value for UsdGeomVisibilityAPI::GetGuideVisibilityAttr(), Possible value for UsdGeomVisibilityAPI::GetProxyVisibilityAttr(), Possible value for UsdGeomVisibilityAPI::GetRenderVisibilityAttr()
    const TfToken invisible;
    /// \brief "invisibleIds"
    /// 
    /// UsdGeomPointInstancer
    const TfToken invisibleIds;
    /// \brief "knots"
    /// 
    /// UsdGeomNurbsCurves
    const TfToken knots;
    /// \brief "left"
    /// 
    /// Possible value for UsdGeomCamera::GetStereoRoleAttr()
    const TfToken left;
    /// \brief "leftHanded"
    /// 
    /// Possible value for UsdGeomGprim::GetOrientationAttr()
    const TfToken leftHanded;
    /// \brief "length"
    /// 
    /// UsdGeomPlane
    const TfToken length;
    /// \brief "linear"
    /// 
    /// Possible value for UsdGeomBasisCurves::GetTypeAttr()
    const TfToken linear;
    /// \brief "loop"
    /// 
    /// Possible value for UsdGeomMesh::GetSubdivisionSchemeAttr()
    const TfToken loop;
    /// \brief "metersPerUnit"
    /// 
    /// Stage-level metadata that encodes a scene's linear unit of measure as meters per encoded unit.
    const TfToken metersPerUnit;
    /// \brief "model:applyDrawMode"
    /// 
    /// UsdGeomModelAPI
    const TfToken modelApplyDrawMode;
    /// \brief "model:cardGeometry"
    /// 
    /// UsdGeomModelAPI
    const TfToken modelCardGeometry;
    /// \brief "model:cardTextureXNeg"
    /// 
    /// UsdGeomModelAPI
    const TfToken modelCardTextureXNeg;
    /// \brief "model:cardTextureXPos"
    /// 
    /// UsdGeomModelAPI
    const TfToken modelCardTextureXPos;
    /// \brief "model:cardTextureYNeg"
    /// 
    /// UsdGeomModelAPI
    const TfToken modelCardTextureYNeg;
    /// \brief "model:cardTextureYPos"
    /// 
    /// UsdGeomModelAPI
    const TfToken modelCardTextureYPos;
    /// \brief "model:cardTextureZNeg"
    /// 
    /// UsdGeomModelAPI
    const TfToken modelCardTextureZNeg;
    /// \brief "model:cardTextureZPos"
    /// 
    /// UsdGeomModelAPI
    const TfToken modelCardTextureZPos;
    /// \brief "model:drawMode"
    /// 
    /// UsdGeomModelAPI
    const TfToken modelDrawMode;
    /// \brief "model:drawModeColor"
    /// 
    /// UsdGeomModelAPI
    const TfToken modelDrawModeColor;
    /// \brief "mono"
    /// 
    /// Fallback value for UsdGeomCamera::GetStereoRoleAttr()
    const TfToken mono;
    /// \brief "motion:blurScale"
    /// 
    /// UsdGeomMotionAPI
    const TfToken motionBlurScale;
    /// \brief "motion:nonlinearSampleCount"
    /// 
    /// UsdGeomMotionAPI
    const TfToken motionNonlinearSampleCount;
    /// \brief "motion:velocityScale"
    /// 
    /// UsdGeomMotionAPI
    const TfToken motionVelocityScale;
    /// \brief "none"
    /// 
    /// Possible value for UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr(), Possible value for UsdGeomMesh::GetInterpolateBoundaryAttr(), Possible value for UsdGeomMesh::GetSubdivisionSchemeAttr()
    const TfToken none;
    /// \brief "nonOverlapping"
    /// 
    /// A type of family of GeomSubsets. It implies that  the elements in the various subsets belonging to the family are  mutually exclusive, i.e., an element that appears in one  subset may not belong to any other subset in the family.
    const TfToken nonOverlapping;
    /// \brief "nonperiodic"
    /// 
    /// Fallback value for UsdGeomBasisCurves::GetWrapAttr()
    const TfToken nonperiodic;
    /// \brief "normals"
    /// 
    /// UsdGeomPointBased
    const TfToken normals;
    /// \brief "open"
    /// 
    /// Fallback value for UsdGeomNurbsPatch::GetUFormAttr(), Fallback value for UsdGeomNurbsPatch::GetVFormAttr()
    const TfToken open;
    /// \brief "order"
    /// 
    /// UsdGeomNurbsCurves
    const TfToken order;
    /// \brief "orientation"
    /// 
    /// UsdGeomGprim
    const TfToken orientation;
    /// \brief "orientations"
    /// 
    /// UsdGeomPointInstancer
    const TfToken orientations;
    /// \brief "orientationsf"
    /// 
    /// UsdGeomPointInstancer
    const TfToken orientationsf;
    /// \brief "origin"
    /// 
    /// Possible value for UsdGeomModelAPI::GetModelDrawModeAttr()
    const TfToken origin;
    /// \brief "orthographic"
    /// 
    /// Possible value for UsdGeomCamera::GetProjectionAttr()
    const TfToken orthographic;
    /// \brief "partition"
    /// 
    /// A type of family of GeomSubsets. It implies  that every element appears exacly once in only one of the  subsets in the family.
    const TfToken partition;
    /// \brief "periodic"
    /// 
    /// Possible value for UsdGeomNurbsPatch::GetUFormAttr(), Possible value for UsdGeomNurbsPatch::GetVFormAttr(), Possible value for UsdGeomBasisCurves::GetWrapAttr()
    const TfToken periodic;
    /// \brief "perspective"
    /// 
    /// Fallback value for UsdGeomCamera::GetProjectionAttr()
    const TfToken perspective;
    /// \brief "pinned"
    /// 
    /// Possible value for UsdGeomBasisCurves::GetWrapAttr()
    const TfToken pinned;
    /// \brief "pivot"
    /// 
    /// Op suffix for the standard scale-rotate pivot on a UsdGeomXformCommonAPI-compatible prim. 
    const TfToken pivot;
    /// \brief "point"
    /// 
    /// Possible value for UsdGeomSubset::GetElementTypeAttr()
    const TfToken point;
    /// \brief "points"
    /// 
    /// UsdGeomPointBased
    const TfToken points;
    /// \brief "pointWeights"
    /// 
    /// UsdGeomNurbsPatch, UsdGeomNurbsCurves
    const TfToken pointWeights;
    /// \brief "positions"
    /// 
    /// UsdGeomPointInstancer
    const TfToken positions;
    /// \brief "power"
    /// 
    /// A deprecated basis token for UsdGeomBasisCurves.
    const TfToken power;
    /// \brief "primvars:displayColor"
    /// 
    /// UsdGeomGprim
    const TfToken primvarsDisplayColor;
    /// \brief "primvars:displayOpacity"
    /// 
    /// UsdGeomGprim
    const TfToken primvarsDisplayOpacity;
    /// \brief "projection"
    /// 
    /// UsdGeomCamera
    const TfToken projection;
    /// \brief "protoIndices"
    /// 
    /// UsdGeomPointInstancer
    const TfToken protoIndices;
    /// \brief "prototypes"
    /// 
    /// UsdGeomPointInstancer
    const TfToken prototypes;
    /// \brief "proxy"
    /// 
    /// Possible value for UsdGeomImageable::GetPurposeAttr()
    const TfToken proxy;
    /// \brief "proxyPrim"
    /// 
    /// UsdGeomImageable
    const TfToken proxyPrim;
    /// \brief "proxyVisibility"
    /// 
    /// UsdGeomVisibilityAPI
    const TfToken proxyVisibility;
    /// \brief "purpose"
    /// 
    /// UsdGeomImageable
    const TfToken purpose;
    /// \brief "radius"
    /// 
    /// UsdGeomSphere, UsdGeomCylinder, UsdGeomCapsule, UsdGeomCone
    const TfToken radius;
    /// \brief "radiusBottom"
    /// 
    /// UsdGeomCylinder_1, UsdGeomCapsule_1
    const TfToken radiusBottom;
    /// \brief "radiusTop"
    /// 
    /// UsdGeomCylinder_1, UsdGeomCapsule_1
    const TfToken radiusTop;
    /// \brief "ranges"
    /// 
    /// UsdGeomNurbsCurves
    const TfToken ranges;
    /// \brief "render"
    /// 
    /// Possible value for UsdGeomImageable::GetPurposeAttr()
    const TfToken render;
    /// \brief "renderVisibility"
    /// 
    /// UsdGeomVisibilityAPI
    const TfToken renderVisibility;
    /// \brief "right"
    /// 
    /// Possible value for UsdGeomCamera::GetStereoRoleAttr()
    const TfToken right;
    /// \brief "rightHanded"
    /// 
    /// Fallback value for UsdGeomGprim::GetOrientationAttr()
    const TfToken rightHanded;
    /// \brief "scales"
    /// 
    /// UsdGeomPointInstancer
    const TfToken scales;
    /// \brief "segment"
    /// 
    /// Possible value for UsdGeomSubset::GetElementTypeAttr()
    const TfToken segment;
    /// \brief "shutter:close"
    /// 
    /// UsdGeomCamera
    const TfToken shutterClose;
    /// \brief "shutter:open"
    /// 
    /// UsdGeomCamera
    const TfToken shutterOpen;
    /// \brief "size"
    /// 
    /// UsdGeomCube
    const TfToken size;
    /// \brief "smooth"
    /// 
    /// Possible value for UsdGeomMesh::GetTriangleSubdivisionRuleAttr()
    const TfToken smooth;
    /// \brief "stereoRole"
    /// 
    /// UsdGeomCamera
    const TfToken stereoRole;
    /// \brief "subdivisionScheme"
    /// 
    /// UsdGeomMesh
    const TfToken subdivisionScheme;
    /// \brief "surfaceFaceVertexIndices"
    /// 
    /// UsdGeomTetMesh
    const TfToken surfaceFaceVertexIndices;
    /// \brief "tangents"
    /// 
    /// UsdGeomHermiteCurves
    const TfToken tangents;
    /// \brief "tetrahedron"
    /// 
    /// Possible value for UsdGeomSubset::GetElementTypeAttr()
    const TfToken tetrahedron;
    /// \brief "tetVertexIndices"
    /// 
    /// UsdGeomTetMesh
    const TfToken tetVertexIndices;
    /// \brief "triangleSubdivisionRule"
    /// 
    /// UsdGeomMesh
    const TfToken triangleSubdivisionRule;
    /// \brief "trimCurve:counts"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken trimCurveCounts;
    /// \brief "trimCurve:knots"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken trimCurveKnots;
    /// \brief "trimCurve:orders"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken trimCurveOrders;
    /// \brief "trimCurve:points"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken trimCurvePoints;
    /// \brief "trimCurve:ranges"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken trimCurveRanges;
    /// \brief "trimCurve:vertexCounts"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken trimCurveVertexCounts;
    /// \brief "type"
    /// 
    /// UsdGeomBasisCurves
    const TfToken type;
    /// \brief "uForm"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken uForm;
    /// \brief "uKnots"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken uKnots;
    /// \brief "unauthoredValuesIndex"
    /// 
    /// UsdGeomPrimvar - The index that represents  unauthored values in the indices array of an indexed primvar.
    const TfToken unauthoredValuesIndex;
    /// \brief "uniform"
    /// 
    /// Possible value for UsdGeomPrimvar::SetInterpolation. One value remains constant for each uv patch segment of the surface primitive (which is a \em face for meshes).
    const TfToken uniform;
    /// \brief "unrestricted"
    /// 
    /// A type of family of GeomSubsets. It implies that there are no restrictions w.r.t. the membership of elements in  the subsets. There could be overlapping members in subsets  belonging to the family and the union of all subsets in the  family may not contain all the elements.
    const TfToken unrestricted;
    /// \brief "uOrder"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken uOrder;
    /// \brief "upAxis"
    /// 
    /// Stage-level metadata that encodes a scene's orientation as a token whose value can be "Y" or "Z".
    const TfToken upAxis;
    /// \brief "uRange"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken uRange;
    /// \brief "uVertexCount"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken uVertexCount;
    /// \brief "varying"
    /// 
    /// Possible value for UsdGeomPrimvar::SetInterpolation. Four values are interpolated over each uv patch segment of the  surface. Bilinear interpolation is used for interpolation  between the four values.
    const TfToken varying;
    /// \brief "velocities"
    /// 
    /// UsdGeomPointBased, UsdGeomPointInstancer
    const TfToken velocities;
    /// \brief "vertex"
    /// 
    /// Possible value for UsdGeomPrimvar::SetInterpolation. Values are interpolated between each vertex in the surface primitive. The basis function of the surface is used for  interpolation between vertices.
    const TfToken vertex;
    /// \brief "verticalAperture"
    /// 
    /// UsdGeomCamera
    const TfToken verticalAperture;
    /// \brief "verticalApertureOffset"
    /// 
    /// UsdGeomCamera
    const TfToken verticalApertureOffset;
    /// \brief "vForm"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken vForm;
    /// \brief "visibility"
    /// 
    /// UsdGeomImageable
    const TfToken visibility;
    /// \brief "visible"
    /// 
    /// Possible value for UsdGeomVisibilityAPI::GetGuideVisibilityAttr(), Possible value for UsdGeomVisibilityAPI::GetProxyVisibilityAttr(), Possible value for UsdGeomVisibilityAPI::GetRenderVisibilityAttr()
    const TfToken visible;
    /// \brief "vKnots"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken vKnots;
    /// \brief "vOrder"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken vOrder;
    /// \brief "vRange"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken vRange;
    /// \brief "vVertexCount"
    /// 
    /// UsdGeomNurbsPatch
    const TfToken vVertexCount;
    /// \brief "width"
    /// 
    /// UsdGeomPlane
    const TfToken width;
    /// \brief "widths"
    /// 
    /// UsdGeomCurves, UsdGeomPoints
    const TfToken widths;
    /// \brief "wrap"
    /// 
    /// UsdGeomBasisCurves
    const TfToken wrap;
    /// \brief "X"
    /// 
    /// Possible value for UsdGeomCylinder::GetAxisAttr(), Possible value for UsdGeomCapsule::GetAxisAttr(), Possible value for UsdGeomCone::GetAxisAttr(), Possible value for UsdGeomCylinder_1::GetAxisAttr(), Possible value for UsdGeomCapsule_1::GetAxisAttr(), Possible value for UsdGeomPlane::GetAxisAttr()
    const TfToken x;
    /// \brief "xformOpOrder"
    /// 
    /// UsdGeomXformable
    const TfToken xformOpOrder;
    /// \brief "Y"
    /// 
    /// Possible value for UsdGeomCylinder::GetAxisAttr(), Possible value for UsdGeomCapsule::GetAxisAttr(), Possible value for UsdGeomCone::GetAxisAttr(), Possible value for UsdGeomCylinder_1::GetAxisAttr(), Possible value for UsdGeomCapsule_1::GetAxisAttr(), Possible value for UsdGeomPlane::GetAxisAttr()
    const TfToken y;
    /// \brief "Z"
    /// 
    /// Fallback value for UsdGeomCylinder::GetAxisAttr(), Fallback value for UsdGeomCapsule::GetAxisAttr(), Fallback value for UsdGeomCone::GetAxisAttr(), Fallback value for UsdGeomCylinder_1::GetAxisAttr(), Fallback value for UsdGeomCapsule_1::GetAxisAttr(), Fallback value for UsdGeomPlane::GetAxisAttr()
    const TfToken z;
    /// \brief "BasisCurves"
    /// 
    /// Schema identifer and family for UsdGeomBasisCurves
    const TfToken BasisCurves;
    /// \brief "Boundable"
    /// 
    /// Schema identifer and family for UsdGeomBoundable
    const TfToken Boundable;
    /// \brief "Camera"
    /// 
    /// Schema identifer and family for UsdGeomCamera
    const TfToken Camera;
    /// \brief "Capsule"
    /// 
    /// Schema identifer and family for UsdGeomCapsule, Schema family for UsdGeomCapsule_1
    const TfToken Capsule;
    /// \brief "Capsule_1"
    /// 
    /// Schema identifer for UsdGeomCapsule_1
    const TfToken Capsule_1;
    /// \brief "Cone"
    /// 
    /// Schema identifer and family for UsdGeomCone
    const TfToken Cone;
    /// \brief "Cube"
    /// 
    /// Schema identifer and family for UsdGeomCube
    const TfToken Cube;
    /// \brief "Curves"
    /// 
    /// Schema identifer and family for UsdGeomCurves
    const TfToken Curves;
    /// \brief "Cylinder"
    /// 
    /// Schema identifer and family for UsdGeomCylinder, Schema family for UsdGeomCylinder_1
    const TfToken Cylinder;
    /// \brief "Cylinder_1"
    /// 
    /// Schema identifer for UsdGeomCylinder_1
    const TfToken Cylinder_1;
    /// \brief "GeomModelAPI"
    /// 
    /// Schema identifer and family for UsdGeomModelAPI
    const TfToken GeomModelAPI;
    /// \brief "GeomSubset"
    /// 
    /// Schema identifer and family for UsdGeomSubset
    const TfToken GeomSubset;
    /// \brief "Gprim"
    /// 
    /// Schema identifer and family for UsdGeomGprim
    const TfToken Gprim;
    /// \brief "HermiteCurves"
    /// 
    /// Schema identifer and family for UsdGeomHermiteCurves
    const TfToken HermiteCurves;
    /// \brief "Imageable"
    /// 
    /// Schema identifer and family for UsdGeomImageable
    const TfToken Imageable;
    /// \brief "Mesh"
    /// 
    /// Schema identifer and family for UsdGeomMesh
    const TfToken Mesh;
    /// \brief "MotionAPI"
    /// 
    /// Schema identifer and family for UsdGeomMotionAPI
    const TfToken MotionAPI;
    /// \brief "NurbsCurves"
    /// 
    /// Schema identifer and family for UsdGeomNurbsCurves
    const TfToken NurbsCurves;
    /// \brief "NurbsPatch"
    /// 
    /// Schema identifer and family for UsdGeomNurbsPatch
    const TfToken NurbsPatch;
    /// \brief "Plane"
    /// 
    /// Schema identifer and family for UsdGeomPlane
    const TfToken Plane;
    /// \brief "PointBased"
    /// 
    /// Schema identifer and family for UsdGeomPointBased
    const TfToken PointBased;
    /// \brief "PointInstancer"
    /// 
    /// Schema identifer and family for UsdGeomPointInstancer
    const TfToken PointInstancer;
    /// \brief "Points"
    /// 
    /// Schema identifer and family for UsdGeomPoints
    const TfToken Points;
    /// \brief "PrimvarsAPI"
    /// 
    /// Schema identifer and family for UsdGeomPrimvarsAPI
    const TfToken PrimvarsAPI;
    /// \brief "Scope"
    /// 
    /// Schema identifer and family for UsdGeomScope
    const TfToken Scope;
    /// \brief "Sphere"
    /// 
    /// Schema identifer and family for UsdGeomSphere
    const TfToken Sphere;
    /// \brief "TetMesh"
    /// 
    /// Schema identifer and family for UsdGeomTetMesh
    const TfToken TetMesh;
    /// \brief "VisibilityAPI"
    /// 
    /// Schema identifer and family for UsdGeomVisibilityAPI
    const TfToken VisibilityAPI;
    /// \brief "Xform"
    /// 
    /// Schema identifer and family for UsdGeomXform
    const TfToken Xform;
    /// \brief "Xformable"
    /// 
    /// Schema identifer and family for UsdGeomXformable
    const TfToken Xformable;
    /// \brief "XformCommonAPI"
    /// 
    /// Schema identifer and family for UsdGeomXformCommonAPI
    const TfToken XformCommonAPI;
    /// A vector of all of the tokens listed above.
    const std::vector<TfToken> allTokens;
};

/// \var UsdGeomTokens
///
/// A global variable with static, efficient \link TfToken TfTokens\endlink
/// for use in all public USD API.  \sa UsdGeomTokensType
extern USDGEOM_API TfStaticData<UsdGeomTokensType> UsdGeomTokens;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
