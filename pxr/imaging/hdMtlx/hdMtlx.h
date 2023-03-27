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
#ifndef PXR_IMAGING_HD_MTLX_HDMTLX_H
#define PXR_IMAGING_HD_MTLX_HDMTLX_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hdMtlx/api.h"
#include <memory>
#include <set>
#include <unordered_map>

#include <MaterialXCore/Library.h>

MATERIALX_NAMESPACE_BEGIN
    class FileSearchPath;
    using DocumentPtr = std::shared_ptr<class Document>;
MATERIALX_NAMESPACE_END

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class VtValue;
struct HdMaterialNetwork2;
struct HdMaterialNode2;
class HdMaterialNetworkInterface;

/// Return the MaterialX search paths. In order, this includes:
/// - Paths set in the environment variable 'PXR_MTLX_PLUGIN_SEARCH_PATHS'
/// - Paths set in the environment variable 'PXR_MTLX_STDLIB_SEARCH_PATHS'
/// - Path to the MaterialX standard library discovered at build time.
HDMTLX_API
const MaterialX::FileSearchPath&
HdMtlxSearchPaths();

/// Converts the HdParameterValue to a string MaterialX can understand
HDMTLX_API
std::string
HdMtlxConvertToString(VtValue const& hdParameterValue);

// Storing MaterialX-Hydra texture and primvar information
struct HdMtlxTexturePrimvarData {
    HdMtlxTexturePrimvarData() 
        : mxHdTextureMap(MaterialX::StringMap()), // Mx-Hd texture name mapping
          hdTextureNodes(std::set<SdfPath>()),    // Paths to HdTexture Nodes
          hdPrimvarNodes(std::set<SdfPath>()) {}  // Paths to HdPrimvar nodes
    MaterialX::StringMap mxHdTextureMap;
    std::set<SdfPath> hdTextureNodes;
    std::set<SdfPath> hdPrimvarNodes;
};

/// Creates and returns a MaterialX Document from the given HdMaterialNetwork2 
/// Collecting the hdTextureNodes and hdPrimvarNodes as the network is 
/// traversed as well as the Texture name mapping between MaterialX and Hydra.
HDMTLX_API
MaterialX::DocumentPtr
HdMtlxCreateMtlxDocumentFromHdNetwork(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& hdMaterialXNode,
    SdfPath const& hdMaterialXNodePath,
    SdfPath const& materialPath,
    MaterialX::DocumentPtr const& libraries,
    HdMtlxTexturePrimvarData *mxHdData = nullptr);

/// Implementation that uses the material network interface.
HDMTLX_API
MaterialX::DocumentPtr
HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface(
    HdMaterialNetworkInterface *netInterface,
    TfToken const& terminalNodeName,
    TfTokenVector const& terminalNodeConnectionNames,
    MaterialX::DocumentPtr const& libraries,
    HdMtlxTexturePrimvarData *mxHdData = nullptr);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
