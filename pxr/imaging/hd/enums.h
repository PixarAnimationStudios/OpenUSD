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

/// \enum HdWrap
///
/// Enumerates wrapping attributes type values.
///
/// <ul>
///     <li>\b HdWrapClamp               Clamp coordinate to range [1/(2N),1-1/(2N)] where N is the size of the texture in the direction of clamping</li>
///     <li>\b HdWrapRepeat              Creates a repeating pattern</li>
///     <li>\b HdWrapBlack               Clamp coordinate to range [-1/(2N),1+1/(2N)] where N is the size of the texture in the direction of clamping</li>
///     <li>\b HdWrapMirror              Creates a mirrored repeating pattern.</li>
///     <li>\b HdWrapUseMetadata         Data Texture can define its own wrap mode, if not defined by the texture it will use HdWrapBlack</li>
///     <li>\b HdWrapLegacy              (deprecated) Texture can define its own wrap mode, if not defined by the texture it will use HdWrapRepeat</li>
/// </ul>
///
enum HdWrap 
{
    HdWrapClamp,
    HdWrapRepeat,
    HdWrapBlack,
    HdWrapMirror,
    HdWrapUseMetadata,
    HdWrapLegacy,
};

/// \enum HdMinFilter
///
/// Enumerates minFilter attribute type values.
///
/// <ul>
///     <li>\b HdMinFilterNearest                Nearest to center of the pixel</li>
///     <li>\b HdMinFilterLinear                 Weighted average od the four texture elements closest to the pixel</li>
///     <li>\b HdMinFilterNearestMipmapNearest   Nearest to center of the pixel from the nearest mipmaps</li>
///     <li>\b HdMinFilterLinearMipmapNeares     Weighted average using texture elements from the nearest mipmaps</li>
///     <li>\b HdMinFilterNearestMipmapLinear    Weighted average of the nearest pixels from the two nearest mipmaps</li>
///     <li>\b HdMinFilterLinearMipmapLinear     WeightedAverage of the weighted averages from the nearest mipmaps</li>
/// </ul>
///
enum HdMinFilter 
{
    HdMinFilterNearest,
    HdMinFilterLinear,
    HdMinFilterNearestMipmapNearest,
    HdMinFilterLinearMipmapNearest,
    HdMinFilterNearestMipmapLinear,
    HdMinFilterLinearMipmapLinear,
};

/// \enum HdMagFilter
///
/// Enumerates magFilter attribute type values.
///
/// <ul>
///     <li>HdFilterNearest       Nearest to center of the pixel</li>
///     <li>HdFilterLinear        Weighted average of the four texture elements closest to the pixel</li>
/// </ul>
///
enum HdMagFilter 
{
    HdMagFilterNearest,
    HdMagFilterLinear,
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

///
/// \enum HdTextureType
/// Enumerates Hydra's supported texture types.
///
/// Uv:   Sample the uv coordinates and accesses a single 2d texture.
///
/// Uvw:  Sample the uvw coordinates and accesses a single 3d texture.
///
/// Ptex: Use the ptex connectivity information to sample a ptex texture.
///
/// Udim: Remap the uv coordinates into udim coordinates using a maximum
///       tile width of 10 and sample all the udim tiles found in the
///       file system.
///
enum class HdTextureType
{
    Uv,
    Uvw,
    Ptex,
    Udim
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
