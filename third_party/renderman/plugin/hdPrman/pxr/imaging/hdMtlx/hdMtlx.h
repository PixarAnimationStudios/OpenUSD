//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_IMAGING_HD_MTLX_HDMTLX_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_IMAGING_HD_MTLX_HDMTLX_H

// XXX: Delete this file after hdPrman drops support for USD versions
// older than 2608. There are bugfixes in 2608 which we want in our
// build.

#include "pxr/pxr.h"
#if PXR_VERSION >= 2608
#include <pxr/imaging/hdMtlx/hdMtlx.h> // IWYU pragma: export
#else

#include "pxr/base/tf/token.h"
#include "pxr/imaging/hdMtlx/api.h"
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

#include <MaterialXCore/Library.h>

MATERIALX_NAMESPACE_BEGIN
    class FileSearchPath;
    using DocumentPtr = std::shared_ptr<class Document>;
    using NodeDefPtr = std::shared_ptr<class NodeDef>;
MATERIALX_NAMESPACE_END

PXR_NAMESPACE_OPEN_SCOPE

#define HdMtlxSearchPaths HdMtlxPrmanSearchPaths
#define HdMtlxStdLibraries HdMtlxPrmanStdLibraries
#define HdMtlxCreateNameFromPath HdMtlxPrmanCreateNameFromPath
#define HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface HdMtlxPrmanCreateMtlxDocumentFromHdMaterialNetworkInterface
#define HdMtlxGetNodeDef HdMtlxPrmanGetNodeDef
#define HdMtlxGetNodeDefName HdMtlxPrmanGetNodeDefName
#define HdMtlxGetMxTerminalName HdMtlxPrmanGetMxTerminalName

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

/// Custom nodes may have their nodeDefs in a separate mtlx file, this function
/// additional looks to the ImplementationURI stored on the sdr to find this 
/// mtlx file and nodeDef. 
/// This method is preferred over mxDoc->getNodeDef() to catch this custom 
/// node case.
HDMTLX_API
MaterialX::NodeDefPtr
HdMtlxPrmanGetNodeDef(
    TfToken const& hdNodeType,
    MaterialX::DocumentPtr const& mxDoc=nullptr);

/// NodeDef names may change between MaterialX versions, this function returns
/// the nodeDef name appropriate for the version of MaterialX being used. 
HDMTLX_API
std::string
HdMtlxPrmanGetNodeDefName(std::string const& prevMxNodeDefName);

/// Get the terminal name used in the MaterialX document based on the Hydra
/// terminal node.
HDMTLX_API
std::string
HdMtlxPrmanGetMxTerminalName(
    HdMaterialNetworkInterface *netInterface,
    TfToken const& hdTerminalNodeName);

/// Get the terminal name used in the MaterialX document based on the MaterialX
/// terminal node type.
HDMTLX_API
std::string
HdMtlxPrmanGetMxTerminalName(std::string const& mxTerminalType);

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

#endif // PXR_VERSION >= 2608

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_PXR_IMAGING_HD_MTLX_HDMTLX_H