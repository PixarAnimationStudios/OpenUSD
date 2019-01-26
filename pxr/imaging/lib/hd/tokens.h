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
#ifndef HD_TOKENS_H
#define HD_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HD_TOKENS                               \
    (adjacency)                                 \
    (bboxLocalMin)                              \
    (bboxLocalMax)                              \
    (bbox)                                      \
    (bezier)                                    \
    (bSpline)                                   \
    (camera)                                    \
    (catmullRom)                                \
    (color)                                     \
    (collection)                                \
    (computeShader)                             \
    (cubic)                                     \
    (culledInstanceIndices)                     \
    (cullStyle)                                 \
    (doubleSided)                               \
    (dispatchCount)                             \
    (displayStyle)                              \
    (drawDispatch)                              \
    (drawingShader)                             \
    (drawingCoord0)                             \
    (drawingCoord1)                             \
    (drawingCoord2)                             \
    (drawingCoordI)                             \
    (edgeIndices)                               \
    (elementCount)                              \
    (elementsVisibility)                        \
    (extent)                                    \
    (faceColors)                                \
    (full)                                      \
    (geometry)                                  \
    (guide)                                     \
    (hermite)                                   \
    (hidden)                                    \
    (hullIndices)                               \
    (indices)                                   \
    (instancer)                                 \
    (instancerTransform)                        \
    (instancerTransformInverse)                 \
    (instanceIndices)                           \
    (instanceIndexBase)                         \
    (instanceTransform)                         \
    (isFlipped)                                 \
    (itemsDrawn)                                \
    (layout)                                    \
    (leftHanded)                                \
    (linear)                                    \
    (lightLink)                                 \
    (materialParams)                            \
    (nonperiodic)                               \
    (normals)                                   \
    (params)                                    \
    (patchParam)                                \
    (periodic)                                  \
    (points)                                    \
    (pointsIndices)                             \
    (power)                                     \
    (preview)                                   \
    (pointsVisibility)                          \
    (primvar)                                   \
    (primID)                                    \
    (primitiveParam)                            \
    (proxy)                                     \
    (quadInfo)                                  \
    (renderTags)                                \
    (rightHanded)                               \
    (segmented)                                 \
    (shadowLink)                                \
    (subdivTags)                                \
    (taskState)                                 \
    (taskParams)                                \
    (topology)                                  \
    (topologyVisibility)                        \
    (totalItemCount)                            \
    (transform)                                 \
    (transformInverse)                          \
    (visibility)                                \
    (widths)

#define HD_REPR_TOKENS                          \
    (disabled)                                  \
    (hull)                                      \
    (points)                                    \
    (smoothHull)                                \
    (refined)                                   \
    (refinedWire)                               \
    (refinedWireOnSurf)                         \
    (wire)                                      \
    (wireOnSurf)

#define HD_PERF_TOKENS                          \
    (adjacencyBufSize)                          \
    (basisCurvesTopology)                       \
    (bufferSourcesResolved)                     \
    (bufferArrayRangeMerged)                    \
    (bufferArrayRangeContainerResized)          \
    (collectionsRefreshed)                      \
    (computationsCommited)                      \
    (drawBatches)                               \
    (drawCalls)                                 \
    (dirtyLists)                                \
    (dirtyListsRebuilt)                         \
    (garbageCollected)                          \
    (garbageCollectedSsbo)                      \
    (garbageCollectedUbo)                       \
    (garbageCollectedVbo)                       \
    (glBufferSubData)                           \
    (glCopyBufferSubData)                       \
    (gpuMemoryUsed)                             \
    (instBasisCurvesTopology)                   \
    (instBasisCurvesTopologyRange)              \
    (instExtComputationDataRange)               \
    (instMeshTopology)                          \
    (instMeshTopologyRange)                     \
    (instPrimvarRange)                          \
    (instVertexAdjacency)                       \
    (meshTopology)                              \
    (nonUniformSize)                            \
    (quadrangulateCPU)                          \
    (quadrangulateGPU)                          \
    (quadrangulateFaceVarying)                  \
    (quadrangulatedVerts)                       \
    (rebuildBatches)                            \
    (singleBufferSize)                          \
    (ssboSize)                                  \
    (skipInvisibleRprimSync)                    \
    (subdivisionRefineCPU)                      \
    (subdivisionRefineGPU)                      \
    (textureResourceMemory)                     \
    (textureMemory)                             \
    (triangulateFaceVarying)                    \
    (uboSize)                                   \
    (vboRelocated)

#define HD_SHADER_TOKENS                        \
    (alphaThreshold)                            \
    (clipPlanes)                                \
    (commonShaderSource)                        \
    (computeShader)                             \
    (cullStyle)                                 \
    (drawRange)                                 \
    (environmentMap)                            \
    (fragmentShader)                            \
    (geometryShader)                            \
    (indicatorColor)                            \
    (lightingBlendAmount)                       \
    (overrideColor)                             \
    (maskColor)                                 \
    (projectionMatrix)                          \
    (pointColor)                                \
    (pointSize)                                 \
    (pointSelectedSize)                         \
    (tessControlShader)                         \
    (tessEvalShader)                            \
    (tessLevel)                                 \
    (viewport)                                  \
    (vertexShader)                              \
    (wireframeColor)                            \
    (worldToViewMatrix)                         \
    (worldToViewInverseMatrix)

#define HD_OPTION_TOKENS                        \
    (parallelRprimSync)                        

#define HD_PRIMTYPE_TOKENS                      \
    /* Rprims */                                \
    (mesh)                                      \
    (basisCurves)                               \
    (points)                                    \
    (volume)                                    \
                                                \
    /* Sprims */                                \
    (camera)                                    \
    (drawTarget)                                \
    (material)                                  \
    /* Sprims Lights */                         \
    (simpleLight)                               \
    (cylinderLight)                             \
    (diskLight)                                 \
    (distantLight)                              \
    (domeLight)                                 \
    (rectLight)                                 \
    (sphereLight)                               \
    /* Sprims ExtComputations */                \
    (extComputation)                            \
                                                \
    /* Bprims */                                \
    (texture)                                   \
    (renderBuffer)

#define HD_PRIMVAR_ROLE_TOKENS                  \
    ((none, ""))                                \
    (color)                                     \
    (vector)                                    \
    (normal)                                    \
    (point)                                     \
    (textureCoordinate)

/* Schema for "Alternate Output Values" rendering,
 * describing which values a renderpass should
 * compute and write at render time.
 */
#define HD_AOV_TOKENS                           \
    /* Standard rendering outputs */            \
                                                \
    /* HdAovTokens->color represents the final
     * fragment RGBA color.
     */                                         \
    (color)                                     \
    /* HdAovTokens->depth represents the clip-space
     * depth of the final fragment.
     */                                         \
    (depth)                                     \
    /* HdAovTokens->linearDepth represents the camera-space
     * depth of the final fragment.
     */                                         \
    (linearDepth)                               \
    /* ID rendering - these tokens represent the
     * prim, instance, and subprim ids of the final
     * fragment.
     */                                         \
    (primId)                                    \
    (instanceId)                                \
    (elementId)                                 \
    (edgeId)                                    \
    (pointId)                                   \
    /* Geometric data */                        \
    (Peye)                                      \
    (Neye)                                      \
    (patchCoord)                                \
    (primitiveParam)                            \
    (normal)                                    \
    /* Others we might want to add:
     * https://rmanwiki.pixar.com/display/REN/Arbitrary+Output+Variables
     * - curvature
     * - tangent
     * - velocity
     */                                         \
    /* Primvars:
     *   The tokens don't try to enumerate primvars,
     *   but instead provide an identifying namespace.
     *   The "color" primvar is addressable as "primvars:color".
     */                                         \
    ((primvars, "primvars:"))                   \
    /* Light path expressions:
     *   Applicable only to raytracers, these tell
     *   the renderer to output specific shading
     *   components for specific classes of lightpath.
     *
     *   Lightpath syntax is defined here:
     *   https://rmanwiki.pixar.com/display/REN/Light+Path+Expressions
     *   ... so for example, you could specify
     *   "lpe:CD[<L.>O]"
     */                                         \
    ((lpe, "lpe:"))                             \
    /* Shader signals:
     *   This tells the renderer to output a partial shading signal,
     *   whether from the BXDF (e.g. bxdf.diffuse) or from an intermediate
     *   shading node (e.g. fractal.rgb).
     *   XXX: The exact format is TBD.
     */                                         \
    ((shader, "shader:"))

HD_API
TfToken HdAovTokensMakePrimvar(TfToken const& primvar);

HD_API
TfToken HdAovTokensMakeLpe(TfToken const& lpe);

HD_API
TfToken HdAovTokensMakeShader(TfToken const& shader);

/* Schema for application-configurable render settings. */
#define HD_RENDER_SETTINGS_TOKENS                     \
    /* General graphical options */                   \
    (enableShadows)                                   \
    (enableSceneMaterials)                            \
    (enableSceneLights)                               \
    /* Raytracer sampling settings */                 \
    (convergedVariance)                               \
    (convergedSamplesPerPixel)

#define HD_COLOR_CORRECTION_TOKENS              \
    (disabled)                                  \
    (sRGB)                                      \
    (openColorIO)

TF_DECLARE_PUBLIC_TOKENS(HdTokens, HD_API, HD_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdReprTokens, HD_API, HD_REPR_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdPerfTokens, HD_API, HD_PERF_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdShaderTokens, HD_API, HD_SHADER_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdOptionTokens, HD_API, HD_OPTION_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdPrimTypeTokens, HD_API, HD_PRIMTYPE_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdPrimvarRoleTokens, HD_API, HD_PRIMVAR_ROLE_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdAovTokens, HD_API, HD_AOV_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdRenderSettingsTokens, HD_API, HD_RENDER_SETTINGS_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdColorCorrectionTokens, HD_API, 
                         HD_COLOR_CORRECTION_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_TOKENS_H
