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

/// \enum HdCullStyle
///
/// Face culling options.
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
    HdBasisCurvesGeomStyleLine,
    HdBasisCurvesGeomStyleRefined,
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
/// </ul>
///
enum HdWrap 
{
    HdWrapClamp,
    HdWrapRepeat,
    HdWrapBlack,
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

#endif // HD_ENUMS_H
