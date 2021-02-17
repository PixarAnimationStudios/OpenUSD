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
#ifndef PXR_IMAGING_HD_TOKENS_H
#define PXR_IMAGING_HD_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HD_TOKENS                               \
    (accelerations)                             \
    (adjacency)                                 \
    (bboxLocalMin)                              \
    (bboxLocalMax)                              \
    (bbox)                                      \
    (bezier)                                    \
    (bSpline)                                   \
    (camera)                                    \
    (catmullRom)                                \
    (collection)                                \
    (computeShader)                             \
    (coordSysBindings)                          \
    (cubic)                                     \
    (cullStyle)                                 \
    (doubleSided)                               \
    (dispatchCount)                             \
    (displayColor)                              \
    (displayOpacity)                            \
    (displayStyle)                              \
    (drawDispatch)                              \
    (drawingShader)                             \
    (drawingCoord0)                             \
    (drawingCoord1)                             \
    (drawingCoord2)                             \
    (drawingCoordI)                             \
    (drivers)                                   \
    (edgeIndices)                               \
    (elementCount)                              \
    (elementsVisibility)                        \
    (extent)                                    \
    (faceColors)                                \
    (filters)                                   \
    (full)                                      \
    (geometry)                                  \
    (hermite)                                   \
    (hullIndices)                               \
    (indices)                                   \
    (isFlipped)                                 \
    (itemsDrawn)                                \
    (layout)                                    \
    (leftHanded)                                \
    (linear)                                    \
    (lightLink)                                 \
    (lightFilterLink)                           \
    (materialParams)                            \
    (nonperiodic)                               \
    (normals)                                   \
    (params)                                    \
    (patchParam)                                \
    (periodic)                                  \
    (pinned)                                    \
    (points)                                    \
    (pointsIndices)                             \
    (power)                                     \
    (preview)                                   \
    (pointsVisibility)                          \
    (primvar)                                   \
    (primID)                                    \
    (primitiveParam)                            \
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
    (velocities)                                \
    (visibility)                                \
    (widths)

#define HD_INSTANCER_TOKENS                     \
    (culledInstanceIndices)                     \
    (instancer)                                 \
    (instancerTransform)                        \
    (instancerTransformInverse)                 \
    (instanceIndices)                           \
    (instanceIndexBase)                         \
    (instanceTransform)                         \
    (rotate)                                    \
    (scale)                                     \
    (translate)

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
    (bufferArrayRangeMigrated)                    \
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
    (numCompletedSamples)                       \
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
    (materialTag)                               \
    (tessControlShader)                         \
    (tessEvalShader)                            \
    (tessLevel)                                 \
    (viewport)                                  \
    (vertexShader)                              \
    (wireframeColor)                            \
    (worldToViewMatrix)                         \
    (worldToViewInverseMatrix)

// Deprecated. Use: HdStMaterialTagTokens
#define HD_MATERIALTAG_TOKENS                   \
    (defaultMaterialTag)

/* Terminal keys used in material networks.
 */
#define HD_MATERIAL_TERMINAL_TOKENS             \
    (surface)                                   \
    (displacement)                              \
    (volume)                                    \
    (light)                                     \
    (lightFilter)

#define HD_RENDERTAG_TOKENS                     \
    (geometry)                                  \
    (guide)                                     \
    (hidden)                                    \
    (proxy)                                     \
    (render)

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
    (coordSys)                                  \
    /* Sprims Lights */                         \
    (simpleLight)                               \
    (cylinderLight)                             \
    (diskLight)                                 \
    (distantLight)                              \
    (domeLight)                                 \
    (lightFilter)                               \
    (pluginLight)                               \
    (rectLight)                                 \
    (sphereLight)                               \
    /* Sprims ExtComputations */                \
    (extComputation)                            \
                                                \
    /* Bprims */                                \
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
     * fragment RGBA color. For correct compositing
     * using Hydra, it should have pre-multiplied alpha.
     */                                         \
    (color)                                     \
    /* HdAovTokens->depth represents the clip-space
     * depth of the final fragment.
     */                                         \
    (depth)                                     \
    /* HdAovTokens->cameraDepth represents the camera-space
     * depth of the final fragment.
     */                                         \
    (cameraDepth)                               \
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
    (convergedSamplesPerPixel)                        \
    /* thread limit settings */                       \
    (threadLimit)

#define HD_RESOURCE_TYPE_TOKENS                       \
    (texture)                                         \
    (shaderFile)

TF_DECLARE_PUBLIC_TOKENS(HdTokens, HD_API, HD_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdInstancerTokens, HD_API, HD_INSTANCER_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdReprTokens, HD_API, HD_REPR_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdPerfTokens, HD_API, HD_PERF_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdShaderTokens, HD_API, HD_SHADER_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdMaterialTagTokens, HD_API, HD_MATERIALTAG_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdMaterialTerminalTokens, HD_API,
                         HD_MATERIAL_TERMINAL_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdRenderTagTokens, HD_API, HD_RENDERTAG_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdOptionTokens, HD_API, HD_OPTION_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdPrimTypeTokens, HD_API, HD_PRIMTYPE_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdPrimvarRoleTokens, HD_API, HD_PRIMVAR_ROLE_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdAovTokens, HD_API, HD_AOV_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdRenderSettingsTokens, HD_API, HD_RENDER_SETTINGS_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdResourceTypeTokens, HD_API, HD_RESOURCE_TYPE_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_TOKENS_H
