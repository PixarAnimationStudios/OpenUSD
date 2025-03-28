//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ENUMS_H
#define PXR_IMAGING_HD_ENUMS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \enum HdCompareFunction
///
/// Abstraction of the Graphics compare functions.
///
enum HdCompareFunction
{
    HdCmpFuncNever,
    HdCmpFuncLess,
    HdCmpFuncEqual,
    HdCmpFuncLEqual,
    HdCmpFuncGreater,
    HdCmpFuncNotEqual,
    HdCmpFuncGEqual,
    HdCmpFuncAlways,

    HdCmpFuncLast
};

/// \enum HdStencilOp
///
/// Abstraction of the Graphics stencil test operations.
///
enum HdStencilOp
{
    HdStencilOpKeep,
    HdStencilOpZero,
    HdStencilOpReplace,
    HdStencilOpIncrement,
    HdStencilOpIncrementWrap,
    HdStencilOpDecrement,
    HdStencilOpDecrementWrap,
    HdStencilOpInvert,

    HdStencilOpLast
};

/// \enum HdBlendOp
///
/// Abstraction of the Graphics blend operations.
///
enum HdBlendOp
{
    HdBlendOpAdd,
    HdBlendOpSubtract,
    HdBlendOpReverseSubtract,
    HdBlendOpMin,
    HdBlendOpMax,

    HdBlendOpLast
};

/// \enum HdBlendFactor
///
/// Abstraction of the Graphics blend factors.
///
enum HdBlendFactor
{
    HdBlendFactorZero,
    HdBlendFactorOne,
    HdBlendFactorSrcColor,
    HdBlendFactorOneMinusSrcColor,
    HdBlendFactorDstColor,
    HdBlendFactorOneMinusDstColor,
    HdBlendFactorSrcAlpha,
    HdBlendFactorOneMinusSrcAlpha,
    HdBlendFactorDstAlpha,
    HdBlendFactorOneMinusDstAlpha,
    HdBlendFactorConstantColor,
    HdBlendFactorOneMinusConstantColor,
    HdBlendFactorConstantAlpha,
    HdBlendFactorOneMinusConstantAlpha,
    HdBlendFactorSrcAlphaSaturate,
    HdBlendFactorSrc1Color,
    HdBlendFactorOneMinusSrc1Color,
    HdBlendFactorSrc1Alpha,
    HdBlendFactorOneMinusSrc1Alpha,

    HdBlendFactorLast
};

/// \enum HdCullStyle
///
/// Face culling options.
///
/// DontCare indicates this prim doesn't determine what should be culled.
/// Any other CullStyle opinion will override this (such as from the viewer).
///
/// BackUnlessDoubleSided and FrontUnlessDoubleSided will only cull back or
/// front faces if prim isn't marked as doubleSided.
///
enum HdCullStyle
{
    HdCullStyleDontCare,
    HdCullStyleNothing,
    HdCullStyleBack,
    HdCullStyleFront,
    HdCullStyleBackUnlessDoubleSided,
    HdCullStyleFrontUnlessDoubleSided
};

/// Returns the opposite of the given cullstyle; backface culling becomes
/// frontface and vice versa.
HD_API
HdCullStyle HdInvertCullStyle(HdCullStyle cs);

enum HdPolygonMode
{
    HdPolygonModeFill,
    HdPolygonModeLine
};

/// \enum HdMeshGeomStyle
///
/// Hydra native geom styles.
///
enum HdMeshGeomStyle {
    HdMeshGeomStyleInvalid,
    HdMeshGeomStyleSurf,
    HdMeshGeomStyleEdgeOnly,
    HdMeshGeomStyleEdgeOnSurf,
    HdMeshGeomStyleHull,
    HdMeshGeomStyleHullEdgeOnly,
    HdMeshGeomStyleHullEdgeOnSurf,
    HdMeshGeomStylePoints
};

enum HdBasisCurvesGeomStyle {
    HdBasisCurvesGeomStyleInvalid,
    HdBasisCurvesGeomStyleWire,
    HdBasisCurvesGeomStylePatch,
    HdBasisCurvesGeomStylePoints
};

enum HdPointsGeomStyle {
    HdPointsGeomStyleInvalid,
    HdPointsGeomStylePoints
};

///
/// \enum HdInterpolation
///
/// Enumerates Hydra's primvar interpolation modes.
///
/// Constant:    One value remains constant over the entire surface primitive.
///
/// Uniform:     One value remains constant for each uv patch segment of the
///              surface primitive.
///
/// Varying:     Four values are interpolated over each uv patch segment of
///              the surface. Bilinear interpolation is used for interpolation
///              between the four values.
///
/// Vertex:      Values are interpolated between each vertex in the surface
///              primitive. The basis function of the surface is used for
///              interpolation between vertices.
///
/// Facevarying: For polygons and subdivision surfaces, four values are
///              interpolated over each face of the mesh. Bilinear interpolation
///              is used for interpolation between the four values.
///
/// Instance:    One value remains constant across each instance.
///
enum HdInterpolation
{
    HdInterpolationConstant = 0,
    HdInterpolationUniform,
    HdInterpolationVarying,
    HdInterpolationVertex,
    HdInterpolationFaceVarying,
    HdInterpolationInstance,

    HdInterpolationCount
};

/// \enum HdDepthPriority
/// Sets the priorities for a depth based operation
///
/// <ul>
///     <li>HdDepthPriorityNearest     Prioritize objects nearest to the camera</li>
///     <li>HdDepthPriorityFarthest    Prioritize objects farthest from the camera</li>
/// </ul>
///
enum HdDepthPriority
{
    HdDepthPriorityNearest = 0,
    HdDepthPriorityFarthest,

    HdDepthPriorityCount
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ENUMS_H
