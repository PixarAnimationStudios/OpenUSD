//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

/// Return a MaterialX document with the stdlibraries loaded using the above 
/// search paths.
HDMTLX_API
const MaterialX::DocumentPtr&
HdMtlxStdLibraries();

/// Converts the HdParameterValue to a string MaterialX can understand
HDMTLX_API
std::string
HdMtlxConvertToString(VtValue const& hdParameterValue);

// Storing MaterialX-Hydra texture and primvar information
struct HdMtlxTexturePrimvarData {
    HdMtlxTexturePrimvarData() = default;
    using TextureMap = std::map<std::string, std::set<std::string>>;
    TextureMap mxHdTextureMap; // Mx-Hd texture name mapping
    std::set<SdfPath> hdTextureNodes; // Paths to HdTexture Nodes
    std::set<SdfPath> hdPrimvarNodes; // Paths to HdPrimvar nodes
};

HDMTLX_API
std::string
HdMtlxCreateNameFromPath(SdfPath const& path);

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
