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
#ifndef HD_VERSION_H
#define HD_VERSION_H

// 18 -> 19: Add support for SceneDelegate surface shaders.
// 19 -> 20: RenderPass constructor takes RenderIndex. RasterState class.
// 20 -> 21: Add HdSceneDelegate::IsEnabled().
// 21 -> 22: split HdRasterState out of HdRenderPass and renamed to HdRenderPassState.
//           HdEngine::Draw API change.
// 22 -> 23: remove ID render API
// 23 -> 24: GetPathForInstanceIndex returns absolute instance index.
// 24 -> 25: move simpleLightingShader to Hdx.
// 25 -> 26: move camera and light to Hdx.
// 26 -> 27: move drawTarget to Hdx.
// 27 -> 28: switch render index Sprim to take a typeId.
// 28 -> 29: cameras only support matrices.
#define HD_API 29

// 1  ->  2: SimpleLighting -> FallbackLighting
#define HD_SHADER_API 2

// 1 -> 2: Changes "doubleShaded" API to "doubleSided".
#define HD_CHANGETRACKER_API_VERSION 2
#define HD_ENGINE_API_VERSION 1
#define HD_RPRIMCOLLECTION_API_VERSION 1

// 2 -> 3: Changes "doubleShaded" API to "doubleSided".
#define HD_TOKENS_API_VERSION  3

#endif // HD_VERSION_H
