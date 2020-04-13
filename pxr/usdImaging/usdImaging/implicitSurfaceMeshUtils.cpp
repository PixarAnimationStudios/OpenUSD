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
#include "pxr/usdImaging/usdImaging/implicitSurfaceMeshUtils.h"

#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/tokens.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/vt/array.h"

PXR_NAMESPACE_OPEN_SCOPE

// Sphere ----------------------------------------------------------------------

const PxOsdMeshTopology&
UsdImagingGetUnitSphereMeshTopology()
{
    static const VtIntArray numVerts{
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
    static const VtIntArray verts{
        // Quads
         0,  1, 11, 10,    1,  2, 12, 11,    2,  3, 13, 12,    3,  4, 14, 13,
         4,  5, 15, 14,    5,  6, 16, 15,    6,  7, 17, 16,    7,  8, 18, 17,
         8,  9, 19, 18,    9,  0, 10, 19,   10, 11, 21, 20,   11, 12, 22, 21,
        12, 13, 23, 22,   13, 14, 24, 23,   14, 15, 25, 24,   15, 16, 26, 25,
        16, 17, 27, 26,   17, 18, 28, 27,   18, 19, 29, 28,   19, 10, 20, 29,
        20, 21, 31, 30,   21, 22, 32, 31,   22, 23, 33, 32,   23, 24, 34, 33,
        24, 25, 35, 34,   25, 26, 36, 35,   26, 27, 37, 36,   27, 28, 38, 37,
        28, 29, 39, 38,   29, 20, 30, 39,   30, 31, 41, 40,   31, 32, 42, 41,
        32, 33, 43, 42,   33, 34, 44, 43,   34, 35, 45, 44,   35, 36, 46, 45,
        36, 37, 47, 46,   37, 38, 48, 47,   38, 39, 49, 48,   39, 30, 40, 49,
        40, 41, 51, 50,   41, 42, 52, 51,   42, 43, 53, 52,   43, 44, 54, 53,
        44, 45, 55, 54,   45, 46, 56, 55,   46, 47, 57, 56,   47, 48, 58, 57,
        48, 49, 59, 58,   49, 40, 50, 59,   50, 51, 61, 60,   51, 52, 62, 61,
        52, 53, 63, 62,   53, 54, 64, 63,   54, 55, 65, 64,   55, 56, 66, 65,
        56, 57, 67, 66,   57, 58, 68, 67,   58, 59, 69, 68,   59, 50, 60, 69,
        60, 61, 71, 70,   61, 62, 72, 71,   62, 63, 73, 72,   63, 64, 74, 73,
        64, 65, 75, 74,   65, 66, 76, 75,   66, 67, 77, 76,   67, 68, 78, 77,
        68, 69, 79, 78,   69, 60, 70, 79,   70, 71, 81, 80,   71, 72, 82, 81,
        72, 73, 83, 82,   73, 74, 84, 83,   74, 75, 85, 84,   75, 76, 86, 85,
        76, 77, 87, 86,   77, 78, 88, 87,   78, 79, 89, 88,   79, 70, 80, 89,
        // Tris
         1,  0, 90,    2,  1, 90,    3,  2, 90,    4,  3, 90,    5,  4, 90,
         6,  5, 90,    7,  6, 90,    8,  7, 90,    9,  8, 90,    0,  9, 90,
        80, 81, 91,   81, 82, 91,   82, 83, 91,   83, 84, 91,   84, 85, 91,
        85, 86, 91,   86, 87, 91,   87, 88, 91,   88, 89, 91,   89, 80, 91 };
    static const PxOsdMeshTopology sphereTopo(
        PxOsdOpenSubdivTokens->catmullClark,
        PxOsdOpenSubdivTokens->rightHanded,
        numVerts, verts);

    return sphereTopo;
}

const VtVec3fArray&
UsdImagingGetUnitSphereMeshPoints()
{
    static const VtVec3fArray points{
        GfVec3f( 0.1250,  0.0908, -0.4755), GfVec3f( 0.0477,  0.1469, -0.4755),
        GfVec3f(-0.0477,  0.1469, -0.4755), GfVec3f(-0.1250,  0.0908, -0.4755),
        GfVec3f(-0.1545, -0.0000, -0.4755), GfVec3f(-0.1250, -0.0908, -0.4755),
        GfVec3f(-0.0477, -0.1469, -0.4755), GfVec3f( 0.0477, -0.1469, -0.4755),
        GfVec3f( 0.1250, -0.0908, -0.4755), GfVec3f( 0.1545, -0.0000, -0.4755),
        GfVec3f( 0.2378,  0.1727, -0.4045), GfVec3f( 0.0908,  0.2795, -0.4045),
        GfVec3f(-0.0908,  0.2795, -0.4045), GfVec3f(-0.2378,  0.1727, -0.4045),
        GfVec3f(-0.2939, -0.0000, -0.4045), GfVec3f(-0.2378, -0.1727, -0.4045),
        GfVec3f(-0.0908, -0.2795, -0.4045), GfVec3f( 0.0908, -0.2795, -0.4045),
        GfVec3f( 0.2378, -0.1727, -0.4045), GfVec3f( 0.2939, -0.0000, -0.4045),
        GfVec3f( 0.3273,  0.2378, -0.2939), GfVec3f( 0.1250,  0.3847, -0.2939),
        GfVec3f(-0.1250,  0.3847, -0.2939), GfVec3f(-0.3273,  0.2378, -0.2939),
        GfVec3f(-0.4045, -0.0000, -0.2939), GfVec3f(-0.3273, -0.2378, -0.2939),
        GfVec3f(-0.1250, -0.3847, -0.2939), GfVec3f( 0.1250, -0.3847, -0.2939),
        GfVec3f( 0.3273, -0.2378, -0.2939), GfVec3f( 0.4045, -0.0000, -0.2939),
        GfVec3f( 0.3847,  0.2795, -0.1545), GfVec3f( 0.1469,  0.4523, -0.1545),
        GfVec3f(-0.1469,  0.4523, -0.1545), GfVec3f(-0.3847,  0.2795, -0.1545),
        GfVec3f(-0.4755, -0.0000, -0.1545), GfVec3f(-0.3847, -0.2795, -0.1545),
        GfVec3f(-0.1469, -0.4523, -0.1545), GfVec3f( 0.1469, -0.4523, -0.1545),
        GfVec3f( 0.3847, -0.2795, -0.1545), GfVec3f( 0.4755, -0.0000, -0.1545),
        GfVec3f( 0.4045,  0.2939, -0.0000), GfVec3f( 0.1545,  0.4755, -0.0000),
        GfVec3f(-0.1545,  0.4755, -0.0000), GfVec3f(-0.4045,  0.2939, -0.0000),
        GfVec3f(-0.5000, -0.0000,  0.0000), GfVec3f(-0.4045, -0.2939,  0.0000),
        GfVec3f(-0.1545, -0.4755,  0.0000), GfVec3f( 0.1545, -0.4755,  0.0000),
        GfVec3f( 0.4045, -0.2939,  0.0000), GfVec3f( 0.5000,  0.0000,  0.0000),
        GfVec3f( 0.3847,  0.2795,  0.1545), GfVec3f( 0.1469,  0.4523,  0.1545),
        GfVec3f(-0.1469,  0.4523,  0.1545), GfVec3f(-0.3847,  0.2795,  0.1545),
        GfVec3f(-0.4755, -0.0000,  0.1545), GfVec3f(-0.3847, -0.2795,  0.1545),
        GfVec3f(-0.1469, -0.4523,  0.1545), GfVec3f( 0.1469, -0.4523,  0.1545),
        GfVec3f( 0.3847, -0.2795,  0.1545), GfVec3f( 0.4755,  0.0000,  0.1545),
        GfVec3f( 0.3273,  0.2378,  0.2939), GfVec3f( 0.1250,  0.3847,  0.2939),
        GfVec3f(-0.1250,  0.3847,  0.2939), GfVec3f(-0.3273,  0.2378,  0.2939),
        GfVec3f(-0.4045, -0.0000,  0.2939), GfVec3f(-0.3273, -0.2378,  0.2939),
        GfVec3f(-0.1250, -0.3847,  0.2939), GfVec3f( 0.1250, -0.3847,  0.2939),
        GfVec3f( 0.3273, -0.2378,  0.2939), GfVec3f( 0.4045,  0.0000,  0.2939),
        GfVec3f( 0.2378,  0.1727,  0.4045), GfVec3f( 0.0908,  0.2795,  0.4045),
        GfVec3f(-0.0908,  0.2795,  0.4045), GfVec3f(-0.2378,  0.1727,  0.4045),
        GfVec3f(-0.2939, -0.0000,  0.4045), GfVec3f(-0.2378, -0.1727,  0.4045),
        GfVec3f(-0.0908, -0.2795,  0.4045), GfVec3f( 0.0908, -0.2795,  0.4045),
        GfVec3f( 0.2378, -0.1727,  0.4045), GfVec3f( 0.2939,  0.0000,  0.4045),
        GfVec3f( 0.1250,  0.0908,  0.4755), GfVec3f( 0.0477,  0.1469,  0.4755),
        GfVec3f(-0.0477,  0.1469,  0.4755), GfVec3f(-0.1250,  0.0908,  0.4755),
        GfVec3f(-0.1545, -0.0000,  0.4755), GfVec3f(-0.1250, -0.0908,  0.4755),
        GfVec3f(-0.0477, -0.1469,  0.4755), GfVec3f( 0.0477, -0.1469,  0.4755),
        GfVec3f( 0.1250, -0.0908,  0.4755), GfVec3f( 0.1545,  0.0000,  0.4755),
        GfVec3f( 0.0000, -0.0000, -0.5000), GfVec3f( 0.0000,  0.0000,  0.5000)};
        
    return points;
}

// Cube ------------------------------------------------------------------------

const PxOsdMeshTopology&
UsdImagingGetUnitCubeMeshTopology()
{
    static const VtIntArray numVerts{ 4, 4, 4, 4, 4, 4 };
    static const VtIntArray verts{ 0, 1, 2, 3,
                                   4, 5, 6, 7,
                                   0, 6, 5, 1,
                                   4, 7, 3, 2,
                                   0, 3, 7, 6,
                                   4, 2, 1, 5 };
    static const PxOsdMeshTopology cubeTopo(
        PxOsdOpenSubdivTokens->bilinear,
        PxOsdOpenSubdivTokens->rightHanded,
        numVerts, verts);

    return cubeTopo;
}

const VtVec3fArray&
UsdImagingGetUnitCubeMeshPoints()
{
    static const VtVec3fArray points{ GfVec3f( 0.5f,  0.5f,  0.5f),
                                      GfVec3f(-0.5f,  0.5f,  0.5f),
                                      GfVec3f(-0.5f, -0.5f,  0.5f),
                                      GfVec3f( 0.5f, -0.5f,  0.5f),
                                      GfVec3f(-0.5f, -0.5f, -0.5f),
                                      GfVec3f(-0.5f,  0.5f, -0.5f),
                                      GfVec3f( 0.5f,  0.5f, -0.5f),
                                      GfVec3f( 0.5f, -0.5f, -0.5f) };

    return points;
}

// Cone ------------------------------------------------------------------------

const PxOsdMeshTopology&
UsdImagingGetUnitConeMeshTopology()
{
    static const VtIntArray numVerts{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                                      4, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
    static const VtIntArray verts{
        // Tris
         2,  1,  0,    3,  2,  0,    4,  3,  0,    5,  4,  0,    6,  5,  0,
         7,  6,  0,    8,  7,  0,    9,  8,  0,   10,  9,  0,    1, 10,  0,
        // Quads
        11, 12, 22, 21,   12, 13, 23, 22,   13, 14, 24, 23,   14, 15, 25, 24,
        15, 16, 26, 25,   16, 17, 27, 26,   17, 18, 28, 27,   18, 19, 29, 28,
        19, 20, 30, 29,   20, 11, 21, 30 };
    static const PxOsdMeshTopology coneTopo(
        PxOsdOpenSubdivTokens->catmullClark,
        PxOsdOpenSubdivTokens->rightHanded,
        numVerts, verts);

    return coneTopo;
}

const VtVec3fArray&
UsdImagingGetUnitConeMeshPoints()
{
    // Note: This is a faithful capture of what was being procedurally generated
    // previously, but it certainly appears it (and the topology) could stand to
    // be optimized a bit to remove redundant points.
    static const VtVec3fArray points{
        GfVec3f( 0.0000,  0.0000, -0.5000), GfVec3f( 0.5000,  0.0000, -0.5000),
        GfVec3f( 0.4045,  0.2939, -0.5000), GfVec3f( 0.1545,  0.4755, -0.5000),
        GfVec3f(-0.1545,  0.4755, -0.5000), GfVec3f(-0.4045,  0.2939, -0.5000),
        GfVec3f(-0.5000,  0.0000, -0.5000), GfVec3f(-0.4045, -0.2939, -0.5000),
        GfVec3f(-0.1545, -0.4755, -0.5000), GfVec3f( 0.1545, -0.4755, -0.5000),
        GfVec3f( 0.4045, -0.2939, -0.5000), GfVec3f( 0.5000,  0.0000, -0.5000),
        GfVec3f( 0.4045,  0.2939, -0.5000), GfVec3f( 0.1545,  0.4755, -0.5000),
        GfVec3f(-0.1545,  0.4755, -0.5000), GfVec3f(-0.4045,  0.2939, -0.5000),
        GfVec3f(-0.5000,  0.0000, -0.5000), GfVec3f(-0.4045, -0.2939, -0.5000),
        GfVec3f(-0.1545, -0.4755, -0.5000), GfVec3f( 0.1545, -0.4755, -0.5000),
        GfVec3f( 0.4045, -0.2939, -0.5000), GfVec3f( 0.0000,  0.0000,  0.5000),
        GfVec3f( 0.0000,  0.0000,  0.5000), GfVec3f( 0.0000,  0.0000,  0.5000),
        GfVec3f( 0.0000,  0.0000,  0.5000), GfVec3f( 0.0000,  0.0000,  0.5000),
        GfVec3f( 0.0000,  0.0000,  0.5000), GfVec3f( 0.0000,  0.0000,  0.5000),
        GfVec3f( 0.0000,  0.0000,  0.5000), GfVec3f( 0.0000,  0.0000,  0.5000),
        GfVec3f( 0.0000,  0.0000,  0.5000) };

    return points;
}

// Cylinder --------------------------------------------------------------------

const PxOsdMeshTopology&
UsdImagingGetUnitCylinderMeshTopology()
{
    static const VtIntArray numVerts{ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                                      4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
                                      3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
    static const VtIntArray verts{
        // Tris
         2,  1,  0,    3,  2,  0,    4,  3,  0,    5,  4,  0,    6,  5,  0,
         7,  6,  0,    8,  7,  0,    9,  8,  0,   10,  9,  0,    1, 10,  0,
        // Quads
        11, 12, 22, 21,   12, 13, 23, 22,   13, 14, 24, 23,   14, 15, 25, 24,
        15, 16, 26, 25,   16, 17, 27, 26,   17, 18, 28, 27,   18, 19, 29, 28,
        19, 20, 30, 29,   20, 11, 21, 30,
        // Tris
        31, 32, 41,   32, 33, 41,   33, 34, 41,   34, 35, 41,   35, 36, 41,
        36, 37, 41,   37, 38, 41,   38, 39, 41,   39, 40, 41,   40, 31, 41 };
    static const PxOsdMeshTopology cylinderTopo(
        PxOsdOpenSubdivTokens->catmullClark,
        PxOsdOpenSubdivTokens->rightHanded,
        numVerts, verts);

    return cylinderTopo;
}

const VtVec3fArray&
UsdImagingGetUnitCylinderMeshPoints()
{
    static const VtVec3fArray points{
        GfVec3f( 0.0000,  0.0000, -0.5000), GfVec3f( 0.5000,  0.0000, -0.5000),
        GfVec3f( 0.4045,  0.2939, -0.5000), GfVec3f( 0.1545,  0.4755, -0.5000),
        GfVec3f(-0.1545,  0.4755, -0.5000), GfVec3f(-0.4045,  0.2939, -0.5000),
        GfVec3f(-0.5000,  0.0000, -0.5000), GfVec3f(-0.4045, -0.2939, -0.5000),
        GfVec3f(-0.1545, -0.4755, -0.5000), GfVec3f( 0.1545, -0.4755, -0.5000),
        GfVec3f( 0.4045, -0.2939, -0.5000), GfVec3f( 0.5000,  0.0000, -0.5000),
        GfVec3f( 0.4045,  0.2939, -0.5000), GfVec3f( 0.1545,  0.4755, -0.5000),
        GfVec3f(-0.1545,  0.4755, -0.5000), GfVec3f(-0.4045,  0.2939, -0.5000),
        GfVec3f(-0.5000,  0.0000, -0.5000), GfVec3f(-0.4045, -0.2939, -0.5000),
        GfVec3f(-0.1545, -0.4755, -0.5000), GfVec3f( 0.1545, -0.4755, -0.5000),
        GfVec3f( 0.4045, -0.2939, -0.5000), GfVec3f( 0.5000,  0.0000,  0.5000),
        GfVec3f( 0.4045,  0.2939,  0.5000), GfVec3f( 0.1545,  0.4755,  0.5000),
        GfVec3f(-0.1545,  0.4755,  0.5000), GfVec3f(-0.4045,  0.2939,  0.5000),
        GfVec3f(-0.5000,  0.0000,  0.5000), GfVec3f(-0.4045, -0.2939,  0.5000),
        GfVec3f(-0.1545, -0.4755,  0.5000), GfVec3f( 0.1545, -0.4755,  0.5000),
        GfVec3f( 0.4045, -0.2939,  0.5000), GfVec3f( 0.5000,  0.0000,  0.5000),
        GfVec3f( 0.4045,  0.2939,  0.5000), GfVec3f( 0.1545,  0.4755,  0.5000),
        GfVec3f(-0.1545,  0.4755,  0.5000), GfVec3f(-0.4045,  0.2939,  0.5000),
        GfVec3f(-0.5000,  0.0000,  0.5000), GfVec3f(-0.4045, -0.2939,  0.5000),
        GfVec3f(-0.1545, -0.4755,  0.5000), GfVec3f( 0.1545, -0.4755,  0.5000),
        GfVec3f( 0.4045, -0.2939,  0.5000), GfVec3f( 0.0000,  0.0000,  0.5000)};   

    return points;
}

// Capsule ---------------------------------------------------------------------

// slices are segments around the mesh
static constexpr int _capsuleSlices = 10;

// stacks are segments along the spine axis
static constexpr int _capsuleStacks = 1;

// capsules have additional stacks along the spine for each capping hemisphere
static constexpr int _capsuleCapStacks = 4;

const PxOsdMeshTopology&
UsdImagingGetCapsuleMeshTopology()
{
    // Note: This could technically be boiled down to immediate data like the
    // other primitives, but it's not a bad idea to keep the code around as long
    // as we also have to generate the points dynamically.
    static const PxOsdMeshTopology capsuleTopo = []() {
        const int numCounts =
            _capsuleSlices * (_capsuleStacks + 2 * _capsuleCapStacks);
        const int numIndices =
            4 * _capsuleSlices * _capsuleStacks             // cylinder quads
          + 4 * 2 * _capsuleSlices * (_capsuleCapStacks-1)  // hemisphere quads
          + 3 * 2 * _capsuleSlices;                         // end cap tris

        VtIntArray countsArray(numCounts);
        int * counts = countsArray.data();

        VtIntArray indicesArray(numIndices);
        int * indices = indicesArray.data();

        // populate face counts and face indices
        int face = 0, index = 0, p = 0;

        // base hemisphere end cap triangles
        int base = p++;
        for (int i=0; i<_capsuleSlices; ++i) {
            counts[face++] = 3;
            indices[index++] = p + (i+1)%_capsuleSlices;
            indices[index++] = p + i;
            indices[index++] = base;
        }

        // middle and hemisphere quads
        for (int i=0; i<_capsuleStacks+2*(_capsuleCapStacks-1); ++i) {
            for (int j=0; j<_capsuleSlices; ++j) {
                float x0 = 0;
                float x1 = x0 + _capsuleSlices;
                float y0 = j;
                float y1 = (j + 1) % _capsuleSlices;
                counts[face++] = 4;
                indices[index++] = p + x0 + y0;
                indices[index++] = p + x0 + y1;
                indices[index++] = p + x1 + y1;
                indices[index++] = p + x1 + y0;
            }
            p += _capsuleSlices;
        }

        // top hemisphere end cap triangles
        int top = p + _capsuleSlices;
        for (int i=0; i<_capsuleSlices; ++i) {
            counts[face++] = 3;
            indices[index++] = p + i;
            indices[index++] = p + (i+1)%_capsuleSlices;
            indices[index++] = top;
        }

        TF_VERIFY(face == numCounts && index == numIndices);

        return PxOsdMeshTopology(PxOsdOpenSubdivTokens->catmullClark,
                              PxOsdOpenSubdivTokens->rightHanded,
                              countsArray, indicesArray);
    }();

    return capsuleTopo;
}

VtVec3fArray
UsdImagingGenerateCapsuleMeshPoints(
    const double height,
    const double radius,
    const TfToken& axis)
{
    // The inputs, as the prim attributes, are doubles; but the points are float
    // precision (as is the machinery to produce them).
    const float radiusf = float(radius);
    const float heightf = float(height);

    // choose basis vectors aligned with the spine axis
    GfVec3f u, v, spine;
    if (axis == UsdGeomTokens->x) {
        u = GfVec3f::YAxis();
        v = GfVec3f::ZAxis();
        spine = GfVec3f::XAxis();
    } else if (axis == UsdGeomTokens->y) {
        u = GfVec3f::ZAxis();
        v = GfVec3f::XAxis();
        spine = GfVec3f::YAxis();
    } else { // (axis == UsdGeomTokens->z)
        u = GfVec3f::XAxis();
        v = GfVec3f::YAxis();
        spine = GfVec3f::ZAxis();
    }

    // compute a ring of points with unit radius in the uv plane
    std::vector<GfVec3f> ring(_capsuleSlices);
    for (int i=0; i<_capsuleSlices; ++i) {
        float a = float(2 * M_PI * i) / _capsuleSlices;
        ring[i] = u * cosf(a) + v * sinf(a);
    }

    const int numPoints =
        _capsuleSlices * (_capsuleStacks + 1)       // cylinder
      + 2 * _capsuleSlices * (_capsuleCapStacks-1)  // hemispheres
      + 2;                                          // end points

    // populate points
    VtVec3fArray pointsArray(numPoints);
    GfVec3f * p = pointsArray.data();

    // base hemisphere
    *p++ = spine * (-heightf/2-radiusf);
    for (int i=0; i<_capsuleCapStacks-1; ++i) {
        float a = float(M_PI / 2) * (1.0f - float(i+1) / _capsuleCapStacks);
        float r = radiusf * cosf(a);
        float w = radiusf * sinf(a);

        for (int j=0; j<_capsuleSlices; ++j) {
            *p++ = r * ring[j] + spine * (-heightf/2-w);
        }
    }

    // middle
    for (int i=0; i<=_capsuleStacks; ++i) {
        float t = float(i) / _capsuleStacks;
        float w = heightf * (t - 0.5f);

        for (int j=0; j<_capsuleSlices; ++j) {
            *p++ = radiusf * ring[j] + spine * w;
        }
    }

    // top hemisphere
    for (int i=0; i<_capsuleCapStacks-1; ++i) {
        float a = float(M_PI / 2) * (float(i+1) / _capsuleCapStacks);
        float r = radiusf * cosf(a);
        float w = radiusf * sinf(a);

        for (int j=0; j<_capsuleSlices; ++j) {
            *p++ = r *  ring[j] + spine * (heightf/2+w);
        }
    }
    *p++ = spine * (heightf/2.0f+radiusf);

    TF_VERIFY(p - pointsArray.data() == numPoints);

    return pointsArray;
}

// Transforms ------------------------------------------------------------------

GfMatrix4d
UsdImagingGenerateSphereOrCubeTransform(
    const double size)
{
    return GfMatrix4d(size,  0.0,  0.0, 0.0,
                       0.0, size,  0.0, 0.0,
                       0.0,  0.0, size, 0.0,
                       0.0,  0.0,  0.0, 1.0);
}

GfMatrix4d
UsdImagingGenerateConeOrCylinderTransform(
    const double height,
    const double radius,
    const TfToken& axis)
{
    const double diameter = 2.0 * radius;
    if (axis == UsdGeomTokens->x) {
        return GfMatrix4d(     0.0, diameter,      0.0, 0.0,
                               0.0,      0.0, diameter, 0.0,
                            height,      0.0,      0.0, 0.0,
                               0.0,      0.0,      0.0, 1.0);
    }
    else if (axis == UsdGeomTokens->y) {
        return GfMatrix4d(     0.0,      0.0, diameter, 0.0,
                          diameter,      0.0,      0.0, 0.0,
                               0.0,   height,      0.0, 0.0,
                               0.0,      0.0,      0.0, 1.0);
    }
    else { // (axis == UsdGeomTokens->z)
        return GfMatrix4d(diameter,      0.0,      0.0, 0.0,
                               0.0, diameter,      0.0, 0.0,
                               0.0,      0.0,   height, 0.0,
                               0.0,      0.0,      0.0, 1.0);
    }
}

// -----------------------------------------------------------------------------

PXR_NAMESPACE_CLOSE_SCOPE
