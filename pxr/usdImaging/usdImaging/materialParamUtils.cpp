//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/materialParamUtils.h"

#include "pxr/base/tf/pathUtils.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/nodeDefAPI.h"
#include "pxr/usd/usdShade/udimUtils.h"
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdLux/lightFilter.h"

PXR_NAMESPACE_OPEN_SCOPE

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

    // Not a UDIM, resolve symlinks and exit.
    if (!UsdShadeUdimUtils::IsUdimIdentifier(assetPath.GetAssetPath())) {
        return _ResolveAssetSymlinks(assetPath);
    }

    const std::string resolvedPath = 
        UsdShadeUdimUtils::ResolveUdimPath(
            assetPath.GetAssetPath(), _FindLayerHandle(attr, time));
    // If the path doesn't resolve, return the input path
    if (resolvedPath.empty()) {
        return assetPath;
    }
    return SdfAssetPath(assetPath.GetAssetPath(), resolvedPath);
}

// Get the value from the usd attribute at given time. If it is an
// SdfAssetPath containing a UDIM pattern, e.g., //SHOW/myImage.<UDIM>.exr,
// the resolved path of the SdfAssetPath will be updated to a file path
// with a UDIM pattern, e.g., /filePath/myImage.<UDIM>.exr.
// There might be support for different patterns, e.g., myImage._MAPID_.exr,
// but this function always normalizes it to myImage.<UDIM>.exr.
//
// The function assumes that the correct ArResolverContext is bound.
//
static VtValue
_ResolveMaterialParamValue(
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
                   TfTokenVector const & shaderSourceTypes,
                   TfTokenVector const& renderContexts)
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

    // If the node is a light filter that doesn't have a NodeDefAPI, then we 
    // try to get the light shader ID from the light filter for the given 
    // render contexts.
    UsdLuxLightFilter lightFilter(shadeNode);
    if (lightFilter) {
        TfToken id = lightFilter.GetShaderId(renderContexts);
        if (!id.IsEmpty()) {
            return id;
        }
    } else {
        // Otherwise, if the node is a light that doesn't have a NodeDefAPI, 
        // then we try to get the light shader ID from the light for the given 
        // render contexts.
        UsdLuxLightAPI light(shadeNode);
        if (light) {
            TfToken id = light.GetShaderId(renderContexts);
            if (!id.IsEmpty()) {
                return id;
            }
        }
    }

    // Otherwise for connectable nodes that don't implement NodeDefAPI and we
    // fail to get a light shader ID for, the type name of the prim is used as
    // the node's identifier. This will currently always be the case for prims
    // like light filters.
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
    TfTokenVector const & renderContexts,
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

        // Find the attributes this input is getting its value from, which might
        // be an output or an input, including possibly itself if not connected
        
        const UsdShadeAttributeVector attrs = 
            input.GetValueProducingAttributes(/*shaderOutputsOnly*/ false);

        for (const UsdAttribute& attr : attrs) {

            UsdShadeAttributeType attrType = 
                UsdShadeUtils::GetType(attr.GetName());

            if (attrType == UsdShadeAttributeType::Output) {
                // If it is an output on a shading node we visit the node and also
                // create a relationship in the network
                _WalkGraph(UsdShadeConnectableAPI(
                    attr.GetPrim()),
                    materialNetwork,
                    visitedNodes,
                    shaderSourceTypes,
                    renderContexts,
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
                const VtValue value = _ResolveMaterialParamValue(attr, time);
                if (!value.IsEmpty()) {
                    node.parameters[inputName] = value;
                }
                // If the attribute has a colorspace add an additional parameter
                // of the form 'colorSpace:inputName'
                if (attr.HasColorSpace()) {
                    TfToken colorSpaceInputName(SdfPath::JoinIdentifier(
                        SdfFieldKeys->ColorSpace, inputName));
                    node.parameters[colorSpaceInputName] =
                        VtValue(attr.GetColorSpace());
                }
            }
        }
    }

    // Extract the identifier of the node.
    // GetShaderNodeForSourceType will try to find/create an Sdr node for all
    // three info cases: info:id, info:sourceAsset and info:sourceCode.
    TfToken id = _GetNodeId(shadeNode, shaderSourceTypes, renderContexts);

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
UsdImagingBuildHdMaterialNetworkFromTerminal(
    UsdPrim const& usdTerminal,
    TfToken const& terminalIdentifier,
    TfTokenVector const& shaderSourceTypes,
    TfTokenVector const& renderContexts,
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
        renderContexts,
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
        
        const UsdShadeAttributeVector attrs = 
            input.GetValueProducingAttributes(/*shaderOutputsOnly*/ false);

        for (const UsdAttribute& attr : attrs) {
            UsdShadeAttributeType attrType = 
                UsdShadeUtils::GetType(attr.GetName());
            if (attrType == UsdShadeAttributeType::Output) {
                // If it is an output on a shading node we visit the node and 
                // also create a relationship in the network
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
    }

    return false;
}

bool
UsdImagingIsHdMaterialNetworkTimeVarying(
    UsdPrim const& usdTerminal)
{
    _PathSet visitedNodes;
    return _IsGraphTimeVarying(
        UsdShadeConnectableAPI(usdTerminal), &visitedNodes);
};

PXR_NAMESPACE_CLOSE_SCOPE
