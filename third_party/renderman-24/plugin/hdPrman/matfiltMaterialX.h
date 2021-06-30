//
// Copyright 2021 Pixar
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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_MATERIALX_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MATFILT_MATERIALX_H

#include "pxr/pxr.h"
#include "hdPrman/matfiltFilterChain.h"

PXR_NAMESPACE_OPEN_SCOPE

/// MatfiltFilterChain::FilterFnc implementation which supports
/// MaterialX shading node graphs.
/// 
/// The terminal nodes are converted to PxrSurface, PxrDisplacement,
/// and PxrVolume respectively, and any input graphs use MaterialX
/// shader code-generation, are compiled, and replaced with a single
/// node.
void MatfiltMaterialX(const SdfPath & networkId,
                      HdMaterialNetwork2 & network,
                      const std::map<TfToken, VtValue> & contextValues,
                      const NdrTokenVec & shaderTypePriority,
                      std::vector<std::string> * outputErrorMessages);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
