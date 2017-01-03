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

#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/staticTokens.h"
            
#define HD_TOKENS                               \
    (adjacency)                                 \
    (bboxLocalMin)                              \
    (bboxLocalMax)                              \
    (bbox)                                      \
    (bezier)                                    \
    (bSpline)                                   \
    (camera)                                    \
    (catmullRom)                                \
    (children)                                  \
    (color)                                     \
    (collection)                                \
    (computeShader)                             \
    (constantPrimVars)                          \
    (cubic)                                     \
    (culledInstanceIndices)                     \
    (cullStyle)                                 \
    (doubleSided)                               \
    (dispatchBuffer)                            \
    (drawDispatch)                              \
    (drawCommandIndex)                          \
    (drawIndirect)                              \
    (drawIndirectCull)                          \
    (drawIndirectResult)                        \
    (drawingShader)                             \
    (drawingCoord0)                             \
    (drawingCoord1)                             \
    (drawingCoordI)                             \
    (extent)                                    \
    (faceColors)                                \
    (geometry)                                  \
    (hull)                                      \
    (hullIndices)                               \
    (indices)                                   \
    (instancer)                                 \
    (instancerTransform)                        \
    (instancerTransformInverse)                 \
    (instancePrimVars)                          \
    (instanceCountInput)                        \
    (instanceIndices)                           \
    (instanceIndexBase)                         \
    (instanceTransform)                         \
    (isFlipped)                                 \
    (itemsDrawn)                                \
    (layout)                                    \
    (leftHanded)                                \
    (linear)                                    \
    (nonperiodic)                               \
    (normals)                                   \
    (packedNormals)                             \
    (params)                                    \
    (patchParam)                                \
    (periodic)                                  \
    (points)                                    \
    (pointsIndices)                             \
    (primVar)                                   \
    (primID)                                    \
    (primitiveParam)                            \
    (quadInfo)                                  \
    (refineLevel)                               \
    (refined)                                   \
    (refinedWire)                               \
    (refinedWireOnSurf)                         \
    (ulocDrawCommandNumUints)                   \
    (ulocResetPass)                             \
    (ulocCullMatrix)                            \
    (ulocDrawRangeNDC)                          \
    (rightHanded)                               \
    (segmented)                                 \
    (smoothHull)                                \
    (surfaceShaderParams)                       \
    (subdivTags)                                \
    (taskState)                                 \
    (taskParams)                                \
    (topology)                                  \
    (totalItemCount)                            \
    (transform)                                 \
    (transformInverse)                          \
    (visibility)                                \
    (widths)                                    \
    (wire)                                      \
    (wireOnSurf)

#define HD_PERF_TOKENS                          \
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
    (instMeshTopology)                          \
    (instMeshTopologyRange)                     \
    (instVertexAdjacency)                       \
    (meshTopology)                              \
    (quadrangulateCPU)                          \
    (quadrangulateGPU)                          \
    (quadrangulateFaceVarying)                  \
    (quadrangulatedVerts)                       \
    (rebuildBatches)                            \
    (subdivisionRefineCPU)                      \
    (subdivisionRefineGPU)                      \
    (triangulateFaceVarying)                    \
    (vboRelocated)

#define HD_GLSL_PROGRAM_TOKENS                  \
    (smoothNormalsFloat)                        \
    (smoothNormalsDouble)                       \
    (quadrangulateFloat)                        \
    (quadrangulateDouble)

#define HD_SHADER_TOKENS                        \
    (alphaThreshold)                            \
    (clipPlanes)                                \
    (commonShaderSource)                        \
    (cullStyle)                                 \
    (drawRange)                                 \
    (environmentMap)                            \
    (fragmentShader)                            \
    (geometryShader)                            \
    (lightingBlendAmount)                       \
    (overrideColor)                             \
    (projectionMatrix)                          \
    (surfaceShader)                             \
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

#define HD_DELEGATE_TOKENS                      \
    (none)

#define HD_PRIMTYPE_TOKENS                      \
    (mesh)                                      \
    (basisCurves)                               \
    (points)


TF_DECLARE_PUBLIC_TOKENS(HdTokens, HD_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdPerfTokens, HD_PERF_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdGLSLProgramTokens, HD_GLSL_PROGRAM_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdShaderTokens, HD_SHADER_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdOptionTokens, HD_OPTION_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdDelegateTokens, HD_DELEGATE_TOKENS);
TF_DECLARE_PUBLIC_TOKENS(HdPrimTypeTokens, HD_PRIMTYPE_TOKENS);


#endif //HD_TOKENS_H
