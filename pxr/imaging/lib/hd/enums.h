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
#ifndef HD_ENUMS_H
#define HD_ENUMS_H

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

/// \enum HdGeomStyle
///
/// Defines geometric styles for how each polygon/triangle
/// of a gprim is to be rendered.
///
/// Unspecified indicates this gprim does not indicate how it should be drawn
/// (ie, it will always be overridden by another opinion).
/// The actual geomstyle must come from somewhere else, such as the viewer.
///
/// The polygons/triangles of a gprim can be drawn as Lines or Polygons.
/// The HiddenLine, FeyRay, and Sheer styles are combinations
/// of these styles:
/// <ul>
///  <li> HiddenLine draws both lines and polygons, so the object has outline but
///       also occludes those objects behind it.</li>
///  <li> FeyRay is the effect you get when you peel the skin
///       off the front of the object: you see line style on the front half,
///       but the backfacing half remains polygon and solid.</li>
///  <li> Sheer draws lines and polygons but with the polygons mostly
///       transparent.</li>
/// </ul>
///
enum HdGeomStyle
{
    HdGeomStyleUnspecified,
    HdGeomStyleLines,
    HdGeomStylePolygons,
    HdGeomStyleHiddenLine,
    HdGeomStyleFeyRay,
    HdGeomStyleSheer,
    HdGeomStyleOutline
};

/// \enum HdComplexity
///
/// Defines the display complexity for primitives that support refinement.
///
/// <ul>
///     <li>\b BoundingBoxComplexity:  Complexity is bounding box.</li>
///     <li>\b VeryLowComplexity:      Complexity is very low.</li>
///     <li>\b LowComplexity:          Complexity is low.</li>
///     <li>\b MediumComplexity:       Complexity is medium.</li>
///     <li>\b HighComplexity:         Complexity is high.</li>
///     <li>\b VeryHighComplexity:     Complexity is very high.</li>
///     <li>\b NumComplexities:        Number of distinct complexity values.</li>
/// </ul>
///
enum HdComplexity
{
    HdComplexityBoundingBox,
    HdComplexityVeryLow,
    HdComplexityLow,
    HdComplexityMedium,
    HdComplexityHigh,
    HdComplexityVeryHigh,
};

/// \enum HdWrap
///
/// Enumerates wrapping attributes type values.
///
/// <ul>
///     <li>\b HdWrapClamp               Clamp coordinate to range [1/(2N),1-1/(2N)] where N is the size of the texture in the direction of clamping</li>
///     <li>\b HdWrapRepeat              Creates a repeating pattern</li>
///     <li>\b HdWrapBlack</c></b>       Clamp coordinate to range [-1/(2N),1+1/(2N)] where N is the size of the texture in the direction of clamping</li>
///     <li>\b HdWrapUseMetaDict</c></b> Texture can define its own wrap mode, if not defined by the texture it will use HdWrapRepeat</li>
/// </ul>
///
enum HdWrap 
{
    HdWrapClamp,
    HdWrapRepeat,
    HdWrapBlack,
    HdWrapUseMetaDict,
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

/// \enum HdFormat
///
/// Enumerates formats to be used when creating buffers.
///
/// Format names follow the general pattern:
///
///   Channel identifier, bit precision, type.
///
/// with the channel in the lowest bit coming first. This is the same general
/// naming convention as Vulkan and DXGI
///
enum HdFormat
{
    HdFormatR8UNorm,
    HdFormatR8SNorm,

    HdFormatR8G8UNorm,
    HdFormatR8G8SNorm,

    HdFormatR8G8B8UNorm,
    HdFormatR8G8B8SNorm,

    HdFormatR8G8B8A8UNorm,
    HdFormatR8G8B8A8SNorm,

    HdFormatR32Float,

    HdFormatR32G32Float,

    HdFormatR32G32B32Float,

    HdFormatR32G32B32A32Float,

    HdFormatCount,
    HdFormatUnknown = -1
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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_ENUMS_H
