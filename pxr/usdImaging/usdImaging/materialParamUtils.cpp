//
// Copyright 2020 Pixar
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
#include "pxr/usdImaging/usdImaging/materialParamUtils.h"

#include "pxr/base/tf/pathUtils.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/nodeDefAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

static const char UDIM_PATTERN[] = "<UDIM>";
static const int UDIM_START_TILE = 1001;
static const int UDIM_END_TILE = 1100;
static const std::string::size_type UDIM_TILE_NUMBER_LENGTH = 4;

// We need to find the first layer that changes the value
// of the parameter so that we anchor relative paths to that.
static
SdfLayerHandle 
_FindLayerHandle(const UsdAttribute& attr, const UsdTimeCode& time) {
    for (const auto& spec: attr.GetPropertyStack(time)) {
        if (spec->HasDefaultValue() ||
            spec->GetLayer()->GetNumTimeSamplesForPath(
                spec->GetPath()) > 0) {
            return spec->GetLayer();
        }
    }
    return TfNullPtr;
}

// Resolve symlinks for string path.
// Resolving symlinks can reduce the number of unique textures added into the
// texture registry since it may use the asset path as hash.
static bool
_ResolveSymlinks(
    const std::string& srcPath,
    std::string* outPath)
{
    std::string error;
    *outPath = TfRealPath(srcPath, false, &error);

    if (outPath->empty() || !error.empty()) {
        return false;
    }

    return true;
}

// Resolve symlinks for asset path.
// Resolving symlinks can reduce the number of unique textures added into the
// texture registry since it may use the asset path as hash.
static SdfAssetPath
_ResolveAssetSymlinks(const SdfAssetPath& assetPath)
{
    std::string p = assetPath.GetResolvedPath();
    if (p.empty()) {
        p = assetPath.GetAssetPath();
    }

    if (_ResolveSymlinks(p, &p)) {
        return SdfAssetPath(assetPath.GetAssetPath(), p);
    } else {
        return assetPath;
    }
}

// Given the prefix (e.g., /someDir/myImage.) and suffix (e.g., .exr),
// add integer between them and try to resolve. Iterate until
// resolution succeeded.
static
std::string
_ResolvedPathForFirstTile(
    const std::pair<std::string, std::string> &splitPath,
    SdfLayerHandle const &layer)
{
    TRACE_FUNCTION();

    ArResolver& resolver = ArGetResolver();
    
    for (int i = UDIM_START_TILE; i < UDIM_END_TILE; i++) {
        // Fill in integer
        std::string path =
            splitPath.first + std::to_string(i) + splitPath.second;
        if (layer) {
            // Deal with layer-relative paths.
            path = SdfComputeAssetPathRelativeToLayer(layer, path);
        }
        // Resolve. Unlike the non-UDIM case, we do not resolve symlinks
        // here to handle the case where the symlinks follow the UDIM
        // naming pattern but the files that are linked do not. We'll
        // let whoever consumes the pattern determine if they want to
        // resolve symlinks themselves.
        return resolver.Resolve(path);
    }
    return std::string();
}

// Split a udim file path such as /someDir/myFile.<UDIM>.exr into a
// prefix (/someDir/myFile.) and suffix (.exr).
//
// We might support other patterns such as /someDir/myFile._MAPID_.exr
// in the future.
static
std::pair<std::string, std::string>
_SplitUdimPattern(const std::string &path)
{
    static const std::vector<std::string> patterns = { UDIM_PATTERN };

    for (const std::string &pattern : patterns) {
        const std::string::size_type pos = path.find(pattern);
        if (pos != std::string::npos) {
            return { path.substr(0, pos), path.substr(pos + pattern.size()) };
        }
    }

    return { std::string(), std::string() };
}

// If given assetPath contains UDIM pattern, resolve the UDIM pattern.
// Otherwise, leave assetPath untouched.
static
SdfAssetPath
_ResolveAssetAttribute(
    const SdfAssetPath& assetPath,
    const UsdAttribute& attr,
    const UsdTimeCode& time)
{
    TRACE_FUNCTION();

    // See whether the asset path contains UDIM pattern.
    const std::pair<std::string, std::string>
        splitPath = _SplitUdimPattern(assetPath.GetAssetPath());

    if (splitPath.first.empty() && splitPath.second.empty()) {
        // Not a UDIM, resolve symlinks and exit.
        return _ResolveAssetSymlinks(assetPath);
    }

    // Find first tile.
    std::string firstTilePackage;
    std::string firstTilePath =
        _ResolvedPathForFirstTile(splitPath, _FindLayerHandle(attr, time));

    if (firstTilePath.empty()) {
        return assetPath;
    }

    // If the resolved path of the first tile is located in a packaged asset,
    // like /foo/bar/baz.usdz[myImage.0001.exr], we need to separate the
    // paths to restore the "<UDIM>" prefix to the image filename in the
    // code below, then join the path back togther before we return.
    if (ArIsPackageRelativePath(firstTilePath)) {
        std::tie(firstTilePackage, firstTilePath) =
            ArSplitPackageRelativePathInner(firstTilePath);
    }

    // Construct the file path /filePath/myImage.<UDIM>.exr by using
    // the first part from the first resolved tile, "<UDIM>" and the
    // suffix.

    const std::string &suffix = splitPath.second;

    // Sanity check that the part after <UDIM> did not change.
    if (!TfStringEndsWith(firstTilePath, suffix)) {
        TF_WARN(
            "Resolution of first udim tile gave ambigious result. "
            "First tile for '%s' is '%s'.",
            assetPath.GetAssetPath().c_str(), firstTilePath.c_str());
        return assetPath;
    }

    // Length of the part /filePath/myImage.<UDIM>.exr.
    const std::string::size_type prefixLength =
        firstTilePath.size() - suffix.size() - UDIM_TILE_NUMBER_LENGTH;

    firstTilePath = 
        firstTilePath.substr(0, prefixLength) + UDIM_PATTERN + suffix;

    return SdfAssetPath( 
        assetPath.GetAssetPath(),
        firstTilePackage.empty() ? 
            firstTilePath : 
            ArJoinPackageRelativePath(firstTilePackage, firstTilePath));
}

VtValue
UsdImaging_ResolveMaterialParamValue(
    const UsdAttribute& attr,
    const UsdTimeCode& time)
{
    TRACE_FUNCTION();

    VtValue value;

    if (!attr.Get(&value, time)) {
        return value;
    }
    
    if (!value.IsHolding<SdfAssetPath>()) {
        return value;
    }

    return VtValue(
        _ResolveAssetAttribute(
            value.UncheckedGet<SdfAssetPath>(), attr, time));
}

static TfToken
_GetPrimvarNameAttributeValue(
    SdrShaderNodeConstPtr const& sdrNode,
    HdMaterialNode const& node,
    TfToken const& propName)
{
    VtValue vtName;

    // If the name of the primvar was authored in parameters list.
    // The authored value is the strongest opinion

    auto const& paramIt = node.parameters.find(propName);
    if (paramIt != node.parameters.end()) {
        vtName = paramIt->second;
    }

    // If we didn't find an authored value consult Sdr for the default value.

    if (vtName.IsEmpty() && sdrNode) {
        if (SdrShaderPropertyConstPtr sdrPrimvarInput = 
                sdrNode->GetShaderInput(propName)) {
            vtName = sdrPrimvarInput->GetDefaultValue();
        }
    }

    if (vtName.IsHolding<TfToken>()) {
        return vtName.UncheckedGet<TfToken>();
    } else if (vtName.IsHolding<std::string>()) {
        return TfToken(vtName.UncheckedGet<std::string>());
    }

    return TfToken();
}

static void
_ExtractPrimvarsFromNode(
    HdMaterialNode const& node,
    HdMaterialNetwork *materialNetwork,
    TfTokenVector const & shaderSourceTypes)
{
    SdrRegistry &shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifier(
        node.identifier, shaderSourceTypes);

    if (sdrNode) {
        // GetPrimvars and GetAdditionalPrimvarProperties together give us the
        // complete set of primvars needed by this shader node.
        NdrTokenVec const& primvars = sdrNode->GetPrimvars();
        materialNetwork->primvars.insert( 
            materialNetwork->primvars.end(), primvars.begin(), primvars.end());

        for (TfToken const& p : sdrNode->GetAdditionalPrimvarProperties()) {
            TfToken name = _GetPrimvarNameAttributeValue(sdrNode, node, p);
            materialNetwork->primvars.push_back(name);
        }
    }
}

static
TfToken _GetNodeId(UsdShadeConnectableAPI const &shadeNode,
                   TfTokenVector const & shaderSourceTypes)
{
    UsdShadeNodeDefAPI nodeDef(shadeNode.GetPrim());
    if (nodeDef) {
        // Extract the identifier of the node.
        // GetShaderNodeForSourceType will try to find/create an Sdr node for 
        // all three info cases: info:id, info:sourceAsset and info:sourceCode.
        TfToken id;
        if (!nodeDef.GetShaderId(&id)) {
            for (auto const& sourceType : shaderSourceTypes) {
                if (SdrShaderNodeConstPtr sdrNode = 
                        nodeDef.GetShaderNodeForSourceType(sourceType)) {
                    return sdrNode->GetIdentifier();
                }
            }
        }
        return id;
    }

    // Otherwise for connectable nodes that don't implement NodeDefAPI (such
    // UsdLux lights and light filters) the type name of the prim is used as
    // the node's identifier.
    return shadeNode.GetPrim().GetTypeName();
}

// Walk the shader graph and emit nodes in topological order to avoid
// forward-references.
// This current implementation of _WalkGraph flattens the shading network into
// a single graph with connectivity and values. It does not try to identify
// NodeGraphs that can be processed once and shared, or even look for a
// pre-baked implementation. Currently neither the material processing in Hydra
// nor any of the back-ends (like HdPrman) can make use of this anyway.
using _PathSet = std::unordered_set<SdfPath, SdfPath::Hash>;

static
void _WalkGraph(
    UsdShadeConnectableAPI const & shadeNode,
    HdMaterialNetwork* materialNetwork,
    _PathSet* visitedNodes,
    TfTokenVector const & shaderSourceTypes,
    UsdTimeCode time)
{
    // Store the path of the node
    HdMaterialNode node;
    node.path = shadeNode.GetPath();
    if (!TF_VERIFY(node.path != SdfPath::EmptyPath())) {
        return;
    }

    // If this node has already been found via another path, we do
    // not need to add it again.
    if (!visitedNodes->insert(shadeNode.GetPath()).second) {
        return;
    }

    // Visit the inputs of this node to ensure they are emitted first.
    const std::vector<UsdShadeInput> shadeNodeInputs = shadeNode.GetInputs();
    for (UsdShadeInput input: shadeNodeInputs) {

        TfToken inputName = input.GetBaseName();

        // Find the attribute this input is getting its value from, which might
        // be an output or an input, including possibly itself if not connected
        UsdShadeAttributeType attrType;
        UsdAttribute attr = input.GetValueProducingAttribute(&attrType);

        if (attrType == UsdShadeAttributeType::Output) {
            // If it is an output on a shading node we visit the node and also
            // create a relationship in the network
            _WalkGraph(UsdShadeConnectableAPI(
                attr.GetPrim()),
                materialNetwork,
                visitedNodes,
                shaderSourceTypes,
                time);

            HdMaterialRelationship relationship;
            relationship.outputId = node.path;
            relationship.outputName = inputName;
            relationship.inputId = attr.GetPrim().GetPath();
            relationship.inputName = UsdShadeOutput(attr).GetBaseName();
            materialNetwork->relationships.push_back(relationship);
        } else if (attrType == UsdShadeAttributeType::Input) {
            // If it is an input attribute we get the authored value.
            //
            // If its type is asset and contains <UDIM>,
            // we resolve the asset path with the udim pattern to a file
            // path with a udim pattern, e.g.,
            // /someDir/myImage.<UDIM>.exr to /filePath/myImage.<UDIM>.exr.
            const VtValue value =
                UsdImaging_ResolveMaterialParamValue(attr, time);
            if (!value.IsEmpty()) {
                node.parameters[inputName] = value;
            }
        }
    }

    // Extract the identifier of the node.
    // GetShaderNodeForSourceType will try to find/create an Sdr node for all
    // three info cases: info:id, info:sourceAsset and info:sourceCode.
    TfToken id = _GetNodeId(shadeNode, shaderSourceTypes);

    if (!id.IsEmpty()) {
        node.identifier = id;

        // GprimAdapter can filter-out primvars not used by a material to reduce
        // the number of primvars send to the render delegate. We extract the
        // primvar names from the material node to ensure these primvars are
        // not filtered-out by GprimAdapter.
        _ExtractPrimvarsFromNode(node, materialNetwork, shaderSourceTypes);
    }

    materialNetwork->nodes.push_back(node);
}

void
UsdImaging_BuildHdMaterialNetworkFromTerminal(
    UsdPrim const& usdTerminal,
    TfToken const& terminalIdentifier,
    TfTokenVector const& shaderSourceTypes,
    HdMaterialNetworkMap *materialNetworkMap,
    UsdTimeCode time)
{
    HdMaterialNetwork& network = materialNetworkMap->map[terminalIdentifier];
    std::vector<HdMaterialNode>& nodes = network.nodes;
    _PathSet visitedNodes;

    _WalkGraph(
        UsdShadeConnectableAPI(usdTerminal),
        &network,
        &visitedNodes, 
        shaderSourceTypes,
        time);

    if (!TF_VERIFY(!nodes.empty())) return;

    // _WalkGraph() inserts the terminal last in the nodes list.
    HdMaterialNode& terminalNode = nodes.back();
    
    // Store terminals on material so backend can easily access them.
    materialNetworkMap->terminals.push_back(terminalNode.path);

    // Validate that idenfitier (info:id) is known to Sdr.
    // Return empty network if it fails so backend can use fallback material.
    SdrRegistry &shaderReg = SdrRegistry::GetInstance();
    if (!shaderReg.GetNodeByIdentifier(terminalNode.identifier)) {
        TF_WARN("Invalid info:id %s node: %s", 
                terminalNode.identifier.GetText(),
                terminalNode.path.GetText());
        *materialNetworkMap = HdMaterialNetworkMap();
    }
};

static
bool _IsGraphTimeVarying(UsdShadeConnectableAPI const & shadeNode,
                         _PathSet* visitedNodes)
{
    // Store the path of the node
    if (!TF_VERIFY(shadeNode.GetPath() != SdfPath::EmptyPath())) {
        return false;
    }

    // If this node has already been found via another path, we do
    // not need to add it again.
    if (!visitedNodes->insert(shadeNode.GetPath()).second) {
        return false;
    }

    // Visit the inputs of this node to ensure they are emitted first.
    const std::vector<UsdShadeInput> shadeNodeInputs = shadeNode.GetInputs();
    for (UsdShadeInput input: shadeNodeInputs) {

        // Find the attribute this input is getting its value from, which might
        // be an output or an input, including possibly itself if not connected
        UsdShadeAttributeType attrType;
        UsdAttribute attr = input.GetValueProducingAttribute(&attrType);

        if (attrType == UsdShadeAttributeType::Output) {
            // If it is an output on a shading node we visit the node and also
            // create a relationship in the network
            if (_IsGraphTimeVarying(
                    UsdShadeConnectableAPI(attr.GetPrim()), visitedNodes)) {
                return true;
            }
        } else if (attrType == UsdShadeAttributeType::Input) {
            // If it is an input attribute we get the authored value.
            if (attr.ValueMightBeTimeVarying()) {
                return true;
            }
        }
    }

    return false;
}

bool
UsdImaging_IsHdMaterialNetworkTimeVarying(
    UsdPrim const& usdTerminal)
{
    _PathSet visitedNodes;
    return _IsGraphTimeVarying(
        UsdShadeConnectableAPI(usdTerminal), &visitedNodes);
};

PXR_NAMESPACE_CLOSE_SCOPE
