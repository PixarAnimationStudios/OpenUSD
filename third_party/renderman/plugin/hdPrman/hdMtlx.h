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

#if PXR_VERSION >= 2205
#include "pxr/imaging/hd/materialNetwork2Interface.h"
#else
#include "hdPrman/hdMaterialNetwork2Interface.h"
#endif

#if PXR_VERSION >= 2205
#include <MaterialXCore/Library.h>

MATERIALX_NAMESPACE_BEGIN
    class FileSearchPath;
    using DocumentPtr = std::shared_ptr<class Document>;
MATERIALX_NAMESPACE_END
#else
namespace MaterialX {
    class FileSearchPath;
    using DocumentPtr = std::shared_ptr<class Document>;
    using StringMap = std::unordered_map<std::string, std::string>;
}
#endif

PXR_NAMESPACE_OPEN_SCOPE

#define HdMtlxCreateNameFromPath  HdMtlxPrmanCreateNameFromPath
#define HdMtlxStdLibraries HdMtlxPrmanStdLibraries
#define HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface HdMtlxPrmanCreateMtlxDocumentFromHdMaterialNetworkInterface
#define HdMtlxSearchPaths HdMtlxPrmanSearchPaths

class SdfPath;
class VtValue;
struct HdMaterialNetwork2;
struct HdMaterialNode2;

/// Return the MaterialX search paths. In order, this includes:
/// - Paths set in the environment variable 'PXR_MTLX_PLUGIN_SEARCH_PATHS'
/// - Paths set in the environment variable 'PXR_MTLX_STDLIB_SEARCH_PATHS'
/// - Path to the MaterialX standard library discovered at build time.
HDMTLX_API
const MaterialX::FileSearchPath&
HdMtlxPrmanSearchPaths();

/// Return a MaterialX document with the stdlibraries loaded using the above 
/// search paths.
HDMTLX_API
const MaterialX::DocumentPtr&
HdMtlxPrmanStdLibraries();

/// Converts the HdParameterValue to a string MaterialX can understand
HDMTLX_API
std::string
HdMtlxPrmanConvertToString(VtValue const& hdParameterValue);

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
HdMtlxPrmanCreateNameFromPath(SdfPath const& path);

/// Creates and returns a MaterialX Document from the given HdMaterialNetwork2 
/// Collecting the hdTextureNodes and hdPrimvarNodes as the network is 
/// traversed as well as the Texture name mapping between MaterialX and Hydra.
HDMTLX_API
MaterialX::DocumentPtr
HdMtlxPrmanCreateMtlxDocumentFromHdNetwork(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& hdMaterialXNode,
    SdfPath const& hdMaterialXNodePath,
    SdfPath const& materialPath,
    MaterialX::DocumentPtr const& libraries,
    HdMtlxTexturePrimvarData *mxHdData = nullptr);

/// Implementation that uses the material network interface.
HDMTLX_API
MaterialX::DocumentPtr
HdMtlxPrmanCreateMtlxDocumentFromHdMaterialNetworkInterface(
    HdMaterialNetworkInterface *interface,
    TfToken const& terminalNodeName,
    TfTokenVector const& terminalNodeConnectionNames,
    MaterialX::DocumentPtr const& libraries,
    HdMtlxTexturePrimvarData *mxHdData = nullptr);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
