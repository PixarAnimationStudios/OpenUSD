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
#include "pxr/imaging/hdSt/materialParam.h"
#include "pxr/imaging/hdSt/materialXFilter.h"
#include "pxr/imaging/hdSt/materialXShaderGen.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdMtlx/hdMtlx.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hgi/capabilities.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/diagnostic.h"

#include <MaterialXGenShader/Util.h>
#include <MaterialXGenShader/DefaultColorManagementSystem.h>
#include <MaterialXRender/Util.h>
#include <MaterialXRender/LightHandler.h> 

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (mtlx)

    // Default Texture Coordinate Token
    (st)
    (texcoord)
    (geomprop)
    (index)
    ((defaultInput, "default"))

    // Opacity Parameters
    (UsdPreviewSurface)
    (opacity)
    (opacityThreshold)

    (standard_surface)
    (transmission)

    (open_pbr_surface)
    (transmission_weight)
    (geometry_opacity)

    (gltf_pbr)
    (alpha_mode)
    (alpha_cutoff)
    (alpha)

    (convert)
    (ND_convert_color4_surfaceshader)
    (ND_convert_vector4_surfaceshader)

    // Fallback Dome Light Tokens
    (domeLightFallback)
    (ND_image_color3)
    (file)

    // Colorspace Tokens
    (sourceColorSpace)

    // Anonymization constants
    (NG_Anonymized)

    // Primvar detection constants
    (geompropvalue)
);

TF_DEFINE_PRIVATE_TOKENS(
    _topologicalTokens,
    // This represents living knowledge of the internals of the MaterialX shader generator
    // for both GLSL and Metal. Such knowledge should reside inside the generator class
    // provided by MaterialX.

    // Dot filename is always topological due to code that prevents creating extra OpenGL samplers
    // this is the only shader node id required. All other tests are done on the shader family.
    (ND_dot_filename)
    // Topo affecting nodes due to object/model/world space parameter
    (position)
    (normal)
    (tangent)
    (bitangent)
    // Topo affecting nodes due to channel index.
    (texcoord)
    (geomcolor)
    // Geompropvalue primvar name is topo-affecting.
    (geompropvalue)
    // Swizzles are inlined into the codegen and affect topology.
    (swizzle)
    // Some conversion nodes are implemented by codegen.
    (convert)
    // Constants: they get inlined in the source.
    (constant)
);

TF_DEFINE_PRIVATE_TOKENS(
    _textureParamTokens,
    (filtertype)
    (uaddressmode)
    (vaddressmode)
);

////////////////////////////////////////////////////////////////////////////////
// Shader Gen Functions

// Generate the Glsl Pixel Shader based on the given mxContext and mxElement
// Based on MaterialXViewer Material::generateShader()
static mx::ShaderPtr
_GenMaterialXShader(
    mx::GenContext & mxContext,
    mx::ElementPtr const& mxElem)
{
    bool hasTransparency = mxContext.getOptions().hwTransparency;

    mx::GenContext materialContext = mxContext;
    materialContext.getOptions().hwTransparency = hasTransparency;
    materialContext.getOptions().hwShadowMap = 
        materialContext.getOptions().hwShadowMap && !hasTransparency;

    // MaterialX v1.38.5 added Transmission Refraction method as the default
    // method, this maintains the previous Transmission Opacity behavior.
    materialContext.getOptions().hwTransmissionRenderMethod =
        mx::HwTransmissionRenderMethod::TRANSMISSION_OPACITY;
    
    // Use the domeLightPrefilter texture instead of sampling the Environment Map
    materialContext.getOptions().hwSpecularEnvironmentMethod =
        mx::HwSpecularEnvironmentMethod::SPECULAR_ENVIRONMENT_PREFILTER;

    return mx::createShader("Shader", materialContext, mxElem);
}

// Results in lightData.type = 1 for point lights in the Mx Shader
static const std::string mxDirectLightString = 
R"(
<?xml version="1.0"?>
<materialx version="1.38">
  <point_light name="pt_light" type="lightshader">
  </point_light>
  <directional_light name="dir_light" type="lightshader">
  </directional_light>
</materialx>
)";

static mx::GenContext
_CreateHdStMaterialXContext(
    HdSt_MxShaderGenInfo const& mxHdInfo,
    TfToken const& apiName)
{
    if (apiName == HgiTokens->Metal) {
        return HdStMaterialXShaderGenMsl::create(mxHdInfo);
    }
    if (apiName == HgiTokens->OpenGL) {
        return HdStMaterialXShaderGenGlsl::create(mxHdInfo);
    }
    else {
        TF_CODING_ERROR(
            "MaterialX Shader Generator doesn't support %s API.",
            apiName.GetText());
        return mx::ShaderGeneratorPtr();
    }
}

using HdMtlxNodePathMap = std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>;

static bool
_IsTopologicalShader(TfToken const& nodeId)
{
    static const TfToken::HashSet topologicalTokenSet(
        _topologicalTokens->allTokens.begin(),
        _topologicalTokens->allTokens.end());

    if (nodeId == _topologicalTokens->ND_dot_filename) {
        return true;
    }

    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr sdrNode = 
        sdrRegistry.GetShaderNodeByIdentifierAndType(nodeId, _tokens->mtlx);

    return sdrNode && topologicalTokenSet.count(sdrNode->GetFamily()) > 0;
}

size_t _BuildEquivalentMaterialNetwork(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNetwork2* topoNetwork,
    HdMtlxNodePathMap* nodePathMap)
{
    // The goal here is to strip all local names in the network paths in order to produce MaterialX
    // data that do not have uniform parameter names that vary based on USD node names.

    // We also want to strip all non-topological parameters in order to get a shader that has
    // default values for all parameters and can be re-used.

    size_t nodeCounter = 0;
    nodePathMap->clear();

    // Paths will go /NG_Anonymized/N0, /NG_Anonymized/N1, /NG_Anonymized/N2...
    SdfPath ngBase(_tokens->NG_Anonymized);

    // We will traverse the network in a depth-first traversal starting at the
    // terminals. This will allow a stable traversal that will not be affected
    // by the ordering of the SdfPaths and make sure we assign the same index to
    // all nodes regardless of the way they are sorted in the network node map.
    std::vector<const SdfPath*> pathsToTraverse;
    for (const auto& terminal : hdNetwork.terminals) {
        const auto& connection = terminal.second;
        pathsToTraverse.push_back(&(connection.upstreamNode));
    }
    while (!pathsToTraverse.empty()) {
        const SdfPath* path = pathsToTraverse.back();
        pathsToTraverse.pop_back();
        if (!(*nodePathMap).count(*path)) {
            const HdMaterialNode2& node = hdNetwork.nodes.find(*path)->second;
            // We only need to create the anonymized name at this time:
            (*nodePathMap)[*path] = ngBase.AppendChild(TfToken("N" + std::to_string(nodeCounter++)));
            for (const auto& input : node.inputConnections) {
                for (const auto& connection : input.second) {
                    pathsToTraverse.push_back(&(connection.upstreamNode));
                }
            }
        }
    }

    // Copy the incoming network using only the anonymized names:
    topoNetwork->primvars = hdNetwork.primvars;
    for (const auto& terminal : hdNetwork.terminals) {
        topoNetwork->terminals.emplace(
            terminal.first,
            HdMaterialConnection2 { (*nodePathMap)[terminal.second.upstreamNode],
                                    terminal.second.upstreamOutputName });
    }
    for (const auto& nodePair : hdNetwork.nodes) {
        const HdMaterialNode2& inNode = nodePair.second;
        HdMaterialNode2        outNode;
        outNode.nodeTypeId = inNode.nodeTypeId;
        if (_IsTopologicalShader(inNode.nodeTypeId)) {
            // Topological nodes have parameters that affect topology. We can not strip
            // them.
            outNode.parameters = inNode.parameters;
        } else {
            // Parameters that are color managed are also topological as they
            // result in different nodes being added in the MaterialX graph
            for (const auto& param: inNode.parameters) {
                const auto colorManagedFile = 
                    SdfPath::StripPrefixNamespace(param.first.GetString(),
                                                  SdfFieldKeys->ColorSpace);
                if (colorManagedFile.second) {
                    outNode.parameters.insert(param);
                    // Need an empty asset as well:
                    outNode.parameters.emplace(TfToken(colorManagedFile.first),
                                               VtValue{SdfAssetPath{}});
                }
            }
        }

        for (const auto& cnxPair : inNode.inputConnections) {
            std::vector<HdMaterialConnection2> outCnx;
            for (const auto& c : cnxPair.second) {
                outCnx.emplace_back(HdMaterialConnection2 { (*nodePathMap)[c.upstreamNode],
                                                            c.upstreamOutputName });
            }
            outNode.inputConnections.emplace(cnxPair.first, std::move(outCnx));
        }
        topoNetwork->nodes.emplace((*nodePathMap)[nodePair.first], std::move(outNode));
    }

    // Build the topo hash from the topo network:
    Tf_HashState topoHash;
    for (const auto& terminal : topoNetwork->terminals) {
        TfHashAppend(topoHash, terminal.first);
        TfHashAppend(topoHash, terminal.second.upstreamNode.GetName());
    }
    for (const auto& node : topoNetwork->nodes) {
        TfHashAppend(topoHash, node.first.GetName());
        TfHashAppend(topoHash, node.second.nodeTypeId);
        for (const auto& param : node.second.parameters) {
            TfHashAppend(topoHash, param.first);
            TfHashAppend(topoHash, param.second.GetHash());
        }
        for (const auto& connection : node.second.inputConnections) {
            TfHashAppend(topoHash, connection.first);
            for (const auto& source : connection.second) {
                TfHashAppend(topoHash, source.upstreamNode.GetName());
                TfHashAppend(topoHash, source.upstreamOutputName);
            }
        }
    }

    return topoHash.GetCode();
}

// Use the given mxDocument to generate the corresponding glsl shader
// Based on MaterialXViewer Viewer::loadDocument()
mx::ShaderPtr
HdSt_GenMaterialXShader(
    mx::DocumentPtr const& mxDoc,
    mx::DocumentPtr const& stdLibraries,
    mx::FileSearchPath const& searchPaths,
    HdSt_MxShaderGenInfo const& mxHdInfo,
    TfToken const& apiName)
{
    TRACE_FUNCTION_SCOPE("Create GlslShader from MtlxDocument")
    // Initialize the Context for shaderGen. 
    mx::GenContext mxContext = _CreateHdStMaterialXContext(mxHdInfo, apiName);

    mxContext.getOptions().hwTransparency
        = mxHdInfo.materialTag != HdStMaterialTagTokens->defaultMaterialTag;

    // Starting from MaterialX 1.38.4 at PR 877, we must remove the "libraries" part:
    mx::FileSearchPath libSearchPaths;
    for (const mx::FilePath &path : searchPaths) {
        if (path.getBaseName() == "libraries") {
            libSearchPaths.append(path.getParentPath());
        }
        else {
            libSearchPaths.append(path);
        }
    }
    mxContext.registerSourceCodeSearchPath(libSearchPaths);

    // Initialize the color management system
    mx::DefaultColorManagementSystemPtr cms =
        mx::DefaultColorManagementSystem::create(
            mxContext.getShaderGenerator().getTarget());
    cms->loadLibrary(stdLibraries);
    mxContext.getShaderGenerator().setColorManagementSystem(cms);

    // Set the colorspace
    // XXX: This is the equivalent of the default source colorSpace, which does
    // not yet have a schema and is therefore not yet accessable here 
    mxDoc->setColorSpace("lin_rec709");

    // Add the Direct Light mtlx file to the mxDoc 
    mx::DocumentPtr lightDoc = mx::createDocument();
    mx::readFromXmlString(lightDoc, mxDirectLightString);
    mxDoc->importLibrary(lightDoc);

    // Make sure the Light data properties are added to the mxLightData struct
    mx::LightHandler lightHandler;
    std::vector<mx::NodePtr> lights;
    lightHandler.findLights(mxDoc, lights);
    lightHandler.registerLights(mxDoc, lights, mxContext);

    // Find renderable elements in the Mtlx Document.
    std::vector<mx::TypedElementPtr> renderableElements;
    mx::findRenderableElements(mxDoc, renderableElements);

    // Should have exactly one renderable element (material).
    if (renderableElements.size() != 1) {
        TF_CODING_ERROR("Generated MaterialX Document does not "
                        "have 1 material");
        return nullptr;
    }

    // Extract out the Surface Shader Node for the Material Node 
    mx::TypedElementPtr renderableElem = renderableElements.at(0);
    mx::NodePtr node = renderableElem->asA<mx::Node>();
    if (node && node->getType() == mx::MATERIAL_TYPE_STRING) {
        // Use auto so can compile against MaterialX 1.38.0 or 1.38.1
        auto mxShaderNodes = mx::getShaderNodes(node, mx::SURFACE_SHADER_TYPE_STRING);
        if (!mxShaderNodes.empty()) {
            renderableElem = *mxShaderNodes.begin();
        }
    }
    // Generate the PixelShader for the renderable element (surfaceshader).
    const mx::ElementPtr & mxElem = mxDoc->getDescendant(
                                            renderableElem->getNamePath());
    mx::TypedElementPtr typedElem = mxElem ? mxElem->asA<mx::TypedElement>()
                                         : nullptr;
    if (typedElem) {
        return _GenMaterialXShader(mxContext, typedElem);
    }
    TF_CODING_ERROR("Unable to generate a shader from the MaterialX Document");
    return nullptr;
}


////////////////////////////////////////////////////////////////////////////////
// Helper Functions to convert MX texture node parameters to Hd parameters

// Get the Hydra VtValue for the given MaterialX input value
static VtValue
_GetHdFilterValue(std::string const& mxInputValue)
{
    if (mxInputValue == "closest") {
        return VtValue(HdStTextureTokens->nearestMipmapNearest);
    }
    // linear/cubic
    return VtValue(HdStTextureTokens->linearMipmapLinear);
}

// Get the Hydra VtValue for the given MaterialX input value
static VtValue
_GetHdSamplerValue(std::string const& mxInputValue)
{
    if (mxInputValue == "constant") {
        return VtValue(HdStTextureTokens->black);
    }
    if (mxInputValue == "clamp") {
        return VtValue(HdStTextureTokens->clamp);
    }
    if (mxInputValue == "mirror") {
        return VtValue(HdStTextureTokens->mirror);
    }
    // periodic
    return VtValue(HdStTextureTokens->repeat);
}

// Translate the MaterialX texture node input into the Hydra equivalents
static void
_GetHdTextureParameters(
    std::string const& mxInputName,
    std::string const& mxInputValue,
    std::map<TfToken, VtValue>* hdTextureParams)
{
    // MaterialX has two texture2d node types <image> and <tiledimage>

    // Properties common to both <image> and <tiledimage> texture nodes:
    if (mxInputName == _textureParamTokens->filtertype) {
        (*hdTextureParams)[HdStTextureTokens->minFilter] = 
            _GetHdFilterValue(mxInputValue);
        (*hdTextureParams)[HdStTextureTokens->magFilter] = 
            VtValue(HdStTextureTokens->linear);
    }

    // Properties specific to <image> nodes:
    else if (mxInputName == _textureParamTokens->uaddressmode) {
        (*hdTextureParams)[HdStTextureTokens->wrapS] = 
            _GetHdSamplerValue(mxInputValue);
    }
    else if (mxInputName == _textureParamTokens->vaddressmode) {
        (*hdTextureParams)[HdStTextureTokens->wrapT] = 
            _GetHdSamplerValue(mxInputValue);
    }
}

static void
_AddDefaultMtlxTextureValues(std::map<TfToken, VtValue>* hdTextureParams)
{
    // MaterialX uses repeat/periodic for the default wrap values, without
    // this the texture will use the Hydra default useMetadata. 
    // Note that these will get overwritten by any authored values
    (*hdTextureParams)[HdStTextureTokens->wrapS] = 
        VtValue(HdStTextureTokens->repeat);
    (*hdTextureParams)[HdStTextureTokens->wrapT] = 
        VtValue(HdStTextureTokens->repeat);

    // Set the default colorSpace to be 'raw'. This allows MaterialX to handle
    // colorspace transforms.
    (*hdTextureParams) [_tokens->sourceColorSpace] = VtValue(HdStTokens->raw);
}

static void
_AddDefaultMtlxTextureValues(
    mx::NodeDefPtr const& nodeDef,
    std::map<TfToken, VtValue>* hdTextureParams)
{
    _AddDefaultMtlxTextureValues(hdTextureParams);

    if (nodeDef->getCategory() == mx::ShaderNode::IMAGE) {
        for (auto const& inputName: _textureParamTokens->allTokens) {
            auto mxInput = nodeDef->getActiveInput(inputName.GetString());
            if (mxInput && mxInput->hasValueString()) {
                _GetHdTextureParameters(inputName,
                                        mxInput->getValueString(),
                                        hdTextureParams);
            }
        }
    }

    // Everything boils down to an <image> node. We might have to dig it out of
    // the nodegraph. Unsure about triplanar that has 3 image nodes. Does Storm
    // require per-image texture params? How does one specify that using a single
    // token?
    const mx::InterfaceElementPtr& impl = nodeDef->getImplementation();
    if (!(impl && impl->isA<mx::NodeGraph>())) {
        return;
    }

    // We should go recursive in case we have an image nested more than one level
    // deep via custom NodeDefs, but, for the moment, we dig only one level down
    // since this is sufficient for the default set of MaterialX texture nodes.
    const auto imageNodes = impl->asA<mx::NodeGraph>()->getNodes(mx::ShaderNode::IMAGE);
    if (imageNodes.empty()) {
        return;
    }

    for (auto const& inputName: _textureParamTokens->allTokens) {
        auto mxInput = imageNodes.front()->getInput(inputName.GetString());
        if (!mxInput) {
            continue;
        }
        if (mxInput->hasInterfaceName()) {
            mxInput = nodeDef->getActiveInput(mxInput->getInterfaceName());
        }
        if (mxInput->hasValueString()) {
            _GetHdTextureParameters(inputName.GetString(),
                                    mxInput->getValueString(),
                                    hdTextureParams);
        }
    }
}

// Find the HdNode and its corresponding NodePath in the given HdNetwork 
// based on the given HdConnection
static bool 
_FindConnectedNode(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialConnection2 const& hdConnection,
    HdMaterialNode2* hdNode,
    SdfPath* hdNodePath)
{
    // Get the path to the connected node
    const SdfPath & connectionPath = hdConnection.upstreamNode;

    // If this path is not in the network raise a warning
    auto hdNodeIt = hdNetwork.nodes.find(connectionPath);
    if (hdNodeIt == hdNetwork.nodes.end()) {
        TF_WARN("Unknown material node '%s'", connectionPath.GetText());
        return false;
    }

    // Otherwise return the HdNode and corresponding NodePath
    *hdNode = hdNodeIt->second;
    *hdNodePath = connectionPath;
    return true;
}

// Return the specified or default Texture coordinate name as a TfToken, 
// and initialize the primvar type or default name for MaterialX ShaderGen.
static TfToken
_GetTextureCoordinateName(
    mx::DocumentPtr const& mxDoc,
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& hdTextureNode,
    SdfPath const& hdTextureNodePath,
    mx::StringMap* mxHdPrimvarMap,
    std::string* defaultTexcoordName)
{
    // Get the Texture Coordinate name through the connected node
    bool textureCoordSet = false;
    std::string textureCoordName;
    for (auto const& inputConnections : hdTextureNode.inputConnections) {
        // Texture Coordinates are connected through the 'texcoord' input
        if ( inputConnections.first != _tokens->texcoord) {
            continue;
        }

        for (auto const& currConnection : inputConnections.second) {
            // Get the connected Texture Coordinate node
            SdfPath hdCoordNodePath;
            HdMaterialNode2 hdCoordNode;
            const bool found = _FindConnectedNode(hdNetwork, currConnection,
                                            &hdCoordNode, &hdCoordNodePath);
            if (!found) {
                continue;
            }
            
            // Get the texture coordinate name from the 'geomprop' parameter
            auto coordNameIt = hdCoordNode.parameters.find(_tokens->geomprop);
            if (coordNameIt != hdCoordNode.parameters.end()) {

                textureCoordName = HdMtlxConvertToString(coordNameIt->second);

                // Save texture coordinate primvar name for the glslfx header;
                // figure out the mx typename
                const mx::NodeDefPtr mxNodeDef = mxDoc->getNodeDef(
                        hdCoordNode.nodeTypeId.GetString());
                if (mxNodeDef) {
                    (*mxHdPrimvarMap)[textureCoordName] = mxNodeDef->getType();
                    textureCoordSet = true;
                    break;
                }
            }
        }
    }
    
    // If we did not have a connected node, and the 'st' parameter is not set
    // get the default texture cordinate name from the textureNodes sdr metadata 
    if ( !textureCoordSet && hdTextureNode.parameters.find(_tokens->st) == 
                             hdTextureNode.parameters.end()) {
        // Get the sdr node for the mxTexture node
        SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
        const SdrShaderNodeConstPtr sdrTextureNode = 
            sdrRegistry.GetShaderNodeByIdentifierAndType(
                hdTextureNode.nodeTypeId, _tokens->mtlx);

        if (sdrTextureNode) {
            // Get the texture coordinate name from the sdrTextureNode metadata
            auto metadata = sdrTextureNode->GetMetadata();
            textureCoordName = metadata[SdrNodeMetadata->Primvars];

            // Save the default texture coordinate name for the glslfx header,
            // but only for simple nodes requiring only texture coordinates.
            // For example, the <triplanarprojection> reports "st|Nworld|Pworld"
            // and gets rejected.
            if (textureCoordName.find('|') == std::string::npos) {
                *defaultTexcoordName = textureCoordName;
            }
        }
    }
    return TfToken(textureCoordName);
}

static void
_AddFallbackTextureMaps(
    HdMaterialNode2 const& hdTerminalNode,
    SdfPath const& hdTerminalNodePath,
    mx::StringMap* mxHdTextureMap)
{
    const SdfPath domeTexturePath =
        hdTerminalNodePath.ReplaceName(_tokens->domeLightFallback);

    // Add the Dome Texture name to the TextureMap for MaterialXShaderGen
    (*mxHdTextureMap)[domeTexturePath.GetName()] = domeTexturePath.GetName();

    // Check the terminal node for any file inputs requiring special
    // handling due to node remapping:
    const mx::NodeDefPtr mxMaterialNodeDef =
        HdMtlxStdLibraries()->getNodeDef(hdTerminalNode.nodeTypeId.GetString());
    if (mxMaterialNodeDef) {
        for (auto const& mxInput : mxMaterialNodeDef->getActiveInputs()) {
            if (mxInput->getType() == "filename") {
                (*mxHdTextureMap)[mxInput->getName()] = mxInput->getName();
            }
        }
    }
}

static void
_AddFallbackDomeLightTextureNode(
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& hdTerminalNodePath)
{
    // Create and add a Fallback Dome Light Texture Node to the hdNetwork
    HdMaterialNode2 hdDomeTextureNode;
    hdDomeTextureNode.nodeTypeId = _tokens->ND_image_color3;
    hdDomeTextureNode.parameters[_tokens->file] =
        VtValue(SdfAssetPath(
            HdStPackageFallbackDomeLightTexture(), 
            HdStPackageFallbackDomeLightTexture()));
    const SdfPath domeTexturePath = 
        hdTerminalNodePath.ReplaceName(_tokens->domeLightFallback);
    hdNetwork->nodes.insert({domeTexturePath, hdDomeTextureNode});

    // Connect the new Texture Node to the Terminal Node
    HdMaterialConnection2 domeTextureConn;
    domeTextureConn.upstreamNode = domeTexturePath;
    domeTextureConn.upstreamOutputName = domeTexturePath.GetNameToken();
    hdNetwork->nodes[hdTerminalNodePath].
        inputConnections[domeTextureConn.upstreamOutputName] = {domeTextureConn};
}

static TfToken
_GetHdNodeTypeId(mx::NodePtr const& mxNode)
{
    std::string nodeDefName = mxNode->getNodeDefString();
    if (nodeDefName.empty()) {
        if (mx::NodeDefPtr const& imageNodeDef = mxNode->getNodeDef()) {
            nodeDefName = imageNodeDef->getName();
        }
    }
    return TfToken(nodeDefName);
}

// Add the Hydra texture node parameters to the texture nodes and connect the 
// texture nodes to the terminal node
static void 
_UpdateTextureNodes(
    mx::DocumentPtr const &mxDoc,
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& hdTerminalNode,
    SdfPath const& hdTerminalNodePath,
    std::set<SdfPath> const& hdTextureNodes,
    HdMtlxTexturePrimvarData::TextureMap const& hdMtlxTextureInfo,
    mx::StringMap* mxHdTextureMap,
    mx::StringMap* mxHdPrimvarMap,
    std::string* defaultTexcoordName)
{
    for (SdfPath const& texturePath : hdTextureNodes) {
        auto mtlxTextureInfo = hdMtlxTextureInfo.find(texturePath.GetName());
        if (mtlxTextureInfo != hdMtlxTextureInfo.end()) {
            for (auto const& fileInputName: mtlxTextureInfo->second) {
                // Make and add a new connection to the terminal node
                const auto newConnName = texturePath.GetName() + "_" + fileInputName;
                mxHdTextureMap->emplace(newConnName, newConnName);
            }
        }
    }
}

// Connect the primvar nodes to the terminal node
static void 
_UpdatePrimvarNodes(
    mx::DocumentPtr const& mxDoc,
    HdMaterialNetwork2 const& hdNetwork,
    SdfPath const& hdTerminalNodePath,
    std::set<SdfPath> const& hdPrimvarNodes,
    mx::StringMap* mxHdPrimvarMap,
    mx::StringMap* mxHdPrimvarDefaultValueMap)
{
    for (auto const& primvarPath : hdPrimvarNodes) {
        auto const& hdPrimvarNode = hdNetwork.nodes.at(primvarPath);

        // Save primvar name for the glslfx header
        auto primvarNameIt = hdPrimvarNode.parameters.find(_tokens->geomprop);
        if (primvarNameIt != hdPrimvarNode.parameters.end()) {
            std::string const& primvarName =
                HdMtlxConvertToString(primvarNameIt->second);

            // Figure out the mx typename
            mx::NodeDefPtr mxNodeDef = mxDoc->getNodeDef(
                    hdPrimvarNode.nodeTypeId.GetString());
            if (mxNodeDef) {
                (*mxHdPrimvarMap)[primvarName] = mxNodeDef->getType();
            }

            // Get the Default value if authored
            std::string defaultPrimvarValue;
            const auto defaultPrimvarValueIt =
                hdPrimvarNode.parameters.find(_tokens->defaultInput);
            if (hdPrimvarNode.parameters.end() != defaultPrimvarValueIt) {
                defaultPrimvarValue = 
                    HdMtlxConvertToString(defaultPrimvarValueIt->second);
            }
            (*mxHdPrimvarDefaultValueMap)[primvarName] = defaultPrimvarValue;
        }

        // Texcoord nodes will have an index parameter set
        primvarNameIt = hdPrimvarNode.parameters.find(_tokens->index);
        if (primvarNameIt != hdPrimvarNode.parameters.end()) {
            // Get the sdr node for the texcoord node
            SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
            const SdrShaderNodeConstPtr sdrTexCoordNode = 
                sdrRegistry.GetShaderNodeByIdentifierAndType(
                    hdPrimvarNode.nodeTypeId, _tokens->mtlx);

            // Get the default texture coordinate name from the sdr metadata
            std::string texCoordName;
            if (sdrTexCoordNode) {
                auto metadata = sdrTexCoordNode->GetMetadata();
                texCoordName = metadata[SdrNodeMetadata->Primvars];
            }

            // Figure out the mx typename
            mx::NodeDefPtr mxNodeDef = mxDoc->getNodeDef(
                    hdPrimvarNode.nodeTypeId.GetString());
            if (mxNodeDef) {
                (*mxHdPrimvarMap)[texCoordName] = mxNodeDef->getType();
            }
        }
    }
}

template <typename T>
static bool
_ParameterDiffersFrom(
    HdMaterialNode2 const& terminal,
    TfToken const& paramName,
    T const& paramValue)
{
    // A connected value is always considered to differ:
    if (terminal.inputConnections.find(paramName) != terminal.inputConnections.end()) {
        return true;
    }
    // Check the value itself:
    const auto paramIt = terminal.parameters.find(paramName);
    if (paramIt != terminal.parameters.end() && paramIt->second != paramValue)
    {
        return true;
    }
    // Assume default value is equal to paramValue.
    return false;
}

static std::string const&
_GetUsdPreviewSurfaceMaterialTag(HdMaterialNode2 const& terminal)
{
    // See https://openusd.org/release/spec_usdpreviewsurface.html
    // and implementation in MaterialX libraries/bxdf/usd_preview_surface.mtlx

    // Non-zero opacityThreshold (or connected) triggers masked mode:
    if (_ParameterDiffersFrom(terminal, _tokens->opacityThreshold, 0.0f))
    {
        return HdStMaterialTagTokens->masked.GetString();
    }

    // Opacity less than 1.0 (or connected) triggers transparent mode:
    if (_ParameterDiffersFrom(terminal, _tokens->opacity, 1.0f))
    {
        return HdStMaterialTagTokens->translucent.GetString();
    }

    return HdStMaterialTagTokens->defaultMaterialTag.GetString();
}

static std::string const&
_GetStandardSurfaceMaterialTag(HdMaterialNode2 const& terminal)
{
    // See https://autodesk.github.io/standard-surface/
    // and implementation in MaterialX libraries/bxdf/standard_surface.mtlx
    if (_ParameterDiffersFrom(terminal, _tokens->transmission, 0.0f) ||
        _ParameterDiffersFrom(terminal, _tokens->opacity, GfVec3f(1.0f, 1.0f, 1.0f)))
    {
        return HdStMaterialTagTokens->translucent.GetString();
    }

    return HdStMaterialTagTokens->defaultMaterialTag.GetString();
}

static std::string const&
_GetOpenPBRSurfaceMaterialTag(HdMaterialNode2 const& terminal)
{
    // See https://academysoftwarefoundation.github.io/OpenPBR/
    // and the provided implementation
    if (_ParameterDiffersFrom(terminal, _tokens->transmission_weight, 0.0f) ||
        _ParameterDiffersFrom(terminal, _tokens->geometry_opacity, GfVec3f(1.0f, 1.0f, 1.0f)))
    {
        return HdStMaterialTagTokens->translucent.GetString();
    }

    return HdStMaterialTagTokens->defaultMaterialTag.GetString();
}

static std::string const&
_GetGlTFSurfaceMaterialTag(HdMaterialNode2 const& terminal)
{
    // See https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#alpha-coverage
    // And implementation in MaterialX /libraries/bxdf/gltf_pbr.mtlx

    int alphaMode = 0; // Opaque
    if (terminal.inputConnections.find(_tokens->alpha_mode) != terminal.inputConnections.end()) {
        // A connected alpha_mode is non-standard, but will be considered to overall imply blend.
        alphaMode = 2; // Blend
    } else {
        const auto alphaModeIt = terminal.parameters.find(_tokens->alpha_mode);
        if (alphaModeIt != terminal.parameters.end())
        {
            if (alphaModeIt->second.IsHolding<int>()) {
                const auto value = alphaModeIt->second.UncheckedGet<int>();
                if (value >= 0 && value <= 2) {
                    alphaMode = value;
                }
            }
        }
    }

    TfToken materialToken = HdStMaterialTagTokens->defaultMaterialTag;
    if (alphaMode == 1) // Mask
    {
        if (_ParameterDiffersFrom(terminal, _tokens->alpha_cutoff, 1.0f) && 
            _ParameterDiffersFrom(terminal, _tokens->alpha, 1.0f)) {
            materialToken = HdStMaterialTagTokens->masked;
        }
    }
    else if (alphaMode == 2) // Blend
    {
        if (_ParameterDiffersFrom(terminal, _tokens->alpha, 1.0f)) {
            materialToken = HdStMaterialTagTokens->translucent;
        }
    }

    if (_ParameterDiffersFrom(terminal, _tokens->transmission, 0.0f))
    {
        return HdStMaterialTagTokens->translucent.GetString();
    }

    return materialToken.GetString();
}

static const mx::TypeDesc*
_MxGetTypeDescription(std::string const& typeName)
{
    // Add whatever is necessary for current codebase:
    static const auto _typeLibrary = std::map<std::string, const mx::TypeDesc*>{
        {"float", mx::Type::FLOAT},
        {"color3", mx::Type::COLOR3},
        {"color4", mx::Type::COLOR4},
        {"vector2", mx::Type::VECTOR2},
        {"vector3", mx::Type::VECTOR3},
        {"vector4", mx::Type::VECTOR4},
        {"surfaceshader", mx::Type::SURFACESHADER}
    };

    const auto typeDescIt = _typeLibrary.find(typeName);
    if (typeDescIt != _typeLibrary.end()) {
        return typeDescIt->second;
    }
    return nullptr;
}

static mx::NodePtr
_MxAddStrippedSurfaceNode(
    mx::DocumentPtr mxDocument,
    std::string const& nodeName,
    HdMaterialNode2 const& hdNode,
    HdMaterialNetwork2 const& hdNetwork)
{
    mx::NodeDefPtr mxNodeDef =
        HdMtlxStdLibraries()->getNodeDef(hdNode.nodeTypeId.GetString());
    auto mxNode = mxDocument->addNodeInstance(mxNodeDef, nodeName);

    for (auto const& connIt: hdNode.inputConnections) {
        const auto inputDef = mxNodeDef->getActiveInput(connIt.first.GetString());
        if (!inputDef) {
            continue;
        }
        auto const* typeDesc = _MxGetTypeDescription(inputDef->getType());
        if (!typeDesc) {
            continue;
        }
        if (typeDesc == mx::Type::SURFACESHADER) {
            auto const& hdConnectedPath = connIt.second.front().upstreamNode;
            auto const& hdConnectedNode = hdNetwork.nodes.at(hdConnectedPath);
            auto mxConnectedNode =
                _MxAddStrippedSurfaceNode(mxDocument, hdConnectedPath.GetName(),
                                          hdConnectedNode, hdNetwork);
            auto mxInput = mxNode->addInput(inputDef->getName(), inputDef->getType());
            mxInput->setConnectedNode(mxConnectedNode);
        } else if (typeDesc->getBaseType() == mx::TypeDesc::BASETYPE_FLOAT &&
                   typeDesc->getSemantic() != mx::TypeDesc::SEMANTIC_MATRIX)
        {
            // No need to connect. Just set every component to 0.5
            auto mxInput = mxNode->addInput(inputDef->getName(), inputDef->getType());
            std::string value = "0.5";
            for (size_t i = 1; i < typeDesc->getSize(); ++i) {
                value = value + ", 0.5";
            }
            mxInput->setValueString(value);
        }
    }
    for (auto const& paramIt: hdNode.parameters) {
        const auto inputDef = mxNodeDef->getActiveInput(paramIt.first.GetString());
        if (!inputDef) {
            continue;
        }
        auto const* typeDesc = _MxGetTypeDescription(inputDef->getType());
        if (!typeDesc) {
            continue;
        }
        if (typeDesc->getBaseType() == mx::TypeDesc::BASETYPE_FLOAT &&
            typeDesc->getSemantic() != mx::TypeDesc::SEMANTIC_MATRIX)
        {
            // Convert the value to MaterialX:
            auto mxInput = mxNode->addInput(inputDef->getName(), inputDef->getType());
            mxInput->setValueString(HdMtlxConvertToString(paramIt.second));
        }
    }
    return mxNode;
}

static bool
_MxIsTransparentShader(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& terminal)
{
    // Generating just enough MaterialX to get an answer, but not the
    // full shader graph.
    auto mxDocument = mx::createDocument();
    mxDocument->importLibrary(HdMtlxStdLibraries());

    auto terminalNode = _MxAddStrippedSurfaceNode(mxDocument, "MxTerminalNode", terminal, hdNetwork);

    if (mx::isTransparentSurface(terminalNode)) {
        return true;
    }

    return false;
}

static std::string const&
_GetMaterialTag(HdMaterialNetwork2 const& hdNetwork, HdMaterialNode2 const& terminal)
{
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr mtlxSdrNode =
        sdrRegistry.GetShaderNodeByIdentifierAndType(terminal.nodeTypeId, _tokens->mtlx);
    
    // Cover the most frequent and fully specified terminal nodes:
    if (mtlxSdrNode->GetFamily() == _tokens->UsdPreviewSurface) {
        return _GetUsdPreviewSurfaceMaterialTag(terminal);
    }

    if (mtlxSdrNode->GetFamily() == _tokens->standard_surface) {
        return _GetStandardSurfaceMaterialTag(terminal);
    }

    if (mtlxSdrNode->GetFamily() == _tokens->open_pbr_surface) {
        return _GetOpenPBRSurfaceMaterialTag(terminal);
    }

    if (mtlxSdrNode->GetFamily() == _tokens->gltf_pbr) {
        return _GetGlTFSurfaceMaterialTag(terminal);
    }

    // At this point, we start having to require MaterialX information:
    const mx::DocumentPtr& stdLibraries = HdMtlxStdLibraries();
    mx::NodeDefPtr mxNodeDef =
        stdLibraries->getNodeDef(mtlxSdrNode->GetIdentifier().GetString());

    const auto activeOutputs = mxNodeDef->getActiveOutputs();
    if (activeOutputs.size() != 1 || 
        activeOutputs.back()->getType() != mx::SURFACE_SHADER_TYPE_STRING) {
        // Outputting anything that is not surfaceshader will be
        // considered opaque, unless color4 or vector4. Not fully
        // per USD specs, but supported by MaterialX.
        auto const* typeDesc = _MxGetTypeDescription(activeOutputs.back()->getType());
        if (typeDesc == mx::Type::COLOR4 || typeDesc == mx::Type::VECTOR4) {
            return HdStMaterialTagTokens->translucent.GetString();
        }
        return HdStMaterialTagTokens->defaultMaterialTag.GetString();
    }

    if (mtlxSdrNode->GetFamily() == _tokens->convert) {
        if (terminal.nodeTypeId == _tokens->ND_convert_color4_surfaceshader ||
            terminal.nodeTypeId == _tokens->ND_convert_vector4_surfaceshader)
        {
            return HdStMaterialTagTokens->translucent.GetString();
        }
        return HdStMaterialTagTokens->defaultMaterialTag.GetString();
    }

    // Out of easy answers. Delegate to MaterialX.
    if (_MxIsTransparentShader(hdNetwork, terminal)) {
        return HdStMaterialTagTokens->translucent.GetString();
    }
    return HdStMaterialTagTokens->defaultMaterialTag.GetString();
}

// Returns true is the node requires primvar support for texcoord
static bool
_NodeUsesTexcoordPrimvar(const SdrShaderNodeConstPtr mtlxSdrNode)
{
    if (mtlxSdrNode->GetFamily() == _tokens->texcoord) {
        return true;
    }

    const mx::DocumentPtr& stdLibraries = HdMtlxStdLibraries();
    mx::NodeDefPtr mxNodeDef =
        stdLibraries->getNodeDef(mtlxSdrNode->GetIdentifier().GetString());
    mx::InterfaceElementPtr impl = mxNodeDef->getImplementation();
    if (impl && impl->isA<mx::NodeGraph>()) {
        mx::NodeGraphPtr nodegraph = impl->asA<mx::NodeGraph>();
        if (!nodegraph->getNodes("texcoord").empty()) {
            return true;
        }
    }
    return false;
}

// Returns the MaterialX default texcoord name as registered when loading the library
static std::string const&
_GetDefaultTexcoordPrimvarName()
{
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr mtlxSdrNode =
        sdrRegistry.GetShaderNodeByIdentifierAndType(_tokens->ND_image_color3, _tokens->mtlx);
    auto const& metadata = mtlxSdrNode->GetMetadata();
    const auto primvarIt = metadata.find(SdrNodeMetadata->Primvars);
    return primvarIt != metadata.end() ? primvarIt->second : _tokens->st.GetString();
}

// Browse the nodes to find primvar connections to add to the terminal node
static void
_AddMaterialXHydraPrimvarParams(
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& terminalNodePath)
{
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    for (auto& node: hdNetwork->nodes)
    {
        const SdrShaderNodeConstPtr mtlxSdrNode =
            sdrRegistry.GetShaderNodeByIdentifierAndType(node.second.nodeTypeId,
                                                         _tokens->mtlx);
        if (mtlxSdrNode->GetFamily() == _tokens->geompropvalue ||
            _NodeUsesTexcoordPrimvar(mtlxSdrNode))
        {
            // Connect the primvar node to the terminal node for HdStMaterialNetwork
            // Create a unique name for the new connection.
            std::string newConnName = node.first.GetName() + "_primvarconn";
            HdMaterialConnection2 primvarConn;
            primvarConn.upstreamNode = node.first;
            primvarConn.upstreamOutputName = TfToken(newConnName);
            hdNetwork->nodes[terminalNodePath]
                .inputConnections[primvarConn.upstreamOutputName] = {primvarConn};
        }
    }
}

// Add the default Hydra texture sampler params to a discovered texture node and
// the required Hydra texture connection on the terminal node
static void _AddMaterialXHydraTextureParams(
    TfToken mtlxParamName,
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& terminalNodePath,
    SdfPath const& textureNodePath)
{
    auto& hdTextureNode = hdNetwork->nodes[textureNodePath];
    hdTextureNode.parameters[_tokens->st] = TfToken(_GetDefaultTexcoordPrimvarName());

    // Gather the Hydra Texture Parameters
    std::map<TfToken, VtValue> hdParameters;
    const auto textureNodeDef =
        HdMtlxStdLibraries()->getNodeDef(hdTextureNode.nodeTypeId.GetString());
    _AddDefaultMtlxTextureValues(textureNodeDef, &hdParameters);

    // Override values with Hydra parameters:
    for (auto const& param : hdTextureNode.parameters) {
        // Get the Hydra equivalents for the Mx Texture node parameters
        std::string const& mxInputName = param.first.GetString();
        std::string const mxInputValue = HdMtlxConvertToString(param.second);
        _GetHdTextureParameters(mxInputName, mxInputValue, &hdParameters);
    }

    // Add the Hydra Texture Parameters to the Texture Node
    for (auto const& param : hdParameters) {
        hdTextureNode.parameters[param.first] = param.second;
    }

    // Add connections on the terminal for Hydra texture inputs
    HdMaterialConnection2 textureConn;
    textureConn.upstreamOutputName = mtlxParamName;
    textureConn.upstreamNode = textureNodePath;
    hdNetwork->nodes[terminalNodePath].
        inputConnections[mtlxParamName] = {textureConn};
}

static void
_ReplaceFilenameInput(
    std::string const& mtlxParamName,
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& hdTerminalNodePath)
{
    auto const& hdTerminalNode = hdNetwork->nodes.at(hdTerminalNodePath);
    const mx::NodeDefPtr mxNodeDef =
        HdMtlxStdLibraries()->getNodeDef(hdTerminalNode.nodeTypeId.GetString());
    if (!mxNodeDef) {
        return;
    }

    const auto mxInput = mxNodeDef->getActiveInput(mtlxParamName);
    if (!mxInput) {
        return;
    }

    const mx::InterfaceElementPtr impl = mxNodeDef->getImplementation();
    if (!impl || !impl->isA<mx::NodeGraph>()) {
        return;
    }

    // Find out which node in the nodegraph interfaces with mtlxParamName.
    mx::NodePtr mxTextureNode;
    std::string mxTextureFileInput;
    for (auto const& node: impl->asA<mx::NodeGraph>()->getNodes()) {
        for (auto const& input: node->getInputs()) {
            if (input->getType() != "filename") {
                continue;
            }
            mxTextureFileInput = input->getName();
            if (input->getInterfaceName() == mtlxParamName) {
                mxTextureNode = node;
                break;
            }
            // We need to handle correctly the situation where there are
            // "dot" nodes in the NodeGraph.
            auto dotNode = input->getConnectedNode();
            while (dotNode && dotNode->getCategory() == "dot") {
                auto dotInput = dotNode->getInput("in");
                if (dotInput && dotInput->getInterfaceName() == mtlxParamName) {
                    mxTextureNode = node;
                    break;
                }
                dotNode = dotNode->getConnectedNode("in");
            }
        }
        if (mxTextureNode) {
            break;
        }
    }

    if (!mxTextureNode) {
        return;
    }

    auto mxTextureNodeDef = mxTextureNode->getNodeDef();
    if (!mxTextureNodeDef) {
        return;
    }

    // Gather texture parameters on the image node.
    std::map<TfToken, VtValue> terminalTextureParams;
    _AddDefaultMtlxTextureValues(mxTextureNodeDef, &terminalTextureParams);
    for (auto const& inputName: _textureParamTokens->allTokens) {
        auto mxInput = mxTextureNode->getInput(inputName.GetString());
        if (mxInput && mxInput->hasValueString()) {
            _GetHdTextureParameters(inputName,
                                    mxInput->getValueString(),
                                    &terminalTextureParams);
        }
    }
    // Gather the Hydra Texture Parameters on the terminal node.
    for (auto const& param : hdTerminalNode.parameters) {
        // Get the Hydra equivalents for the Mx Texture node parameters
        std::string const& mxInputName = param.first.GetString();
        std::string const mxInputValue = HdMtlxConvertToString(param.second);
        _GetHdTextureParameters(mxInputName,
                                mxInputValue,
                                &terminalTextureParams);
    }

    // Get the texture node from the Implementation Nodegraph and gather
    // nodeTypeId and parameter information.
    auto terminalTextureTypeId = TfToken(mxTextureNodeDef->getName());

    // Get the filename parameter value from the terminal node
    const TfToken filenameToken(mtlxParamName);
    auto filenameParamIt = hdTerminalNode.parameters.find(filenameToken);
    if (filenameParamIt == hdTerminalNode.parameters.end()) {
        return;
    }

    // Create a new Texture Node
    HdMaterialNode2 terminalTextureNode;
    terminalTextureNode.nodeTypeId = terminalTextureTypeId;
    terminalTextureNode.parameters[TfToken(mxTextureFileInput)] =
        filenameParamIt->second;
    terminalTextureNode.parameters[_tokens->st] = _tokens->st;
    for (auto const& param : terminalTextureParams) {
        terminalTextureNode.parameters[param.first] = param.second;
    }

    // Add the Texture Node to the hdNetwork
    const SdfPath terminalTexturePath =
        hdTerminalNodePath.AppendChild(filenameToken);
    hdNetwork->nodes.insert({terminalTexturePath, terminalTextureNode});

    // Make a new connection to the terminal node
    HdMaterialConnection2 terminalTextureConn;
    terminalTextureConn.upstreamNode = terminalTexturePath;
    terminalTextureConn.upstreamOutputName = terminalTexturePath.GetNameToken();

    // Replace the filename parameter with the TerminalTextureConnection
    hdNetwork->nodes[hdTerminalNodePath].parameters.erase(filenameParamIt);
    hdNetwork->nodes[hdTerminalNodePath].
        inputConnections[terminalTextureConn.upstreamOutputName] =
            {terminalTextureConn};

}

// Gather the Material Params from the glslfx ShaderPtr
void
_AddMaterialXParams(
    mx::ShaderPtr const& glslfxShader,
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& terminalNodePath,
    HdMtlxNodePathMap const& nodePathMap,
    HdSt_MaterialParamVector* materialParams)
{
    TRACE_FUNCTION_SCOPE("Collect Mtlx params from glslfx shader.")
    if (!glslfxShader) {
        return;
    }

    _AddMaterialXHydraPrimvarParams(hdNetwork, terminalNodePath);

    // Build reverse mapping from MaterialX to Hydra:
    std::map<std::string, VtValue> mxValuesFromHd;
    for (auto const& node: hdNetwork->nodes) {
        // Terminal parameters are unprefixed.
        std::string nodePart;
        if (node.first != terminalNodePath) {
            const auto itRemapped = nodePathMap.find(node.first);
            if (itRemapped != nodePathMap.end()) {
                nodePart = itRemapped->second.GetName() + "_";
            }
        }
        for (auto const& param: node.second.parameters) {
            if (param.second.IsHolding<std::string>() ||
                param.second.IsHolding<TfToken>()) {
                continue;
            }
            mxValuesFromHd.emplace(nodePart + param.first.GetString(), param.second);
        }
    }

    // Also build a mapping from the node name to the original SdfPath to allow
    // finding back discovered texture nodes
    std::map<std::string, SdfPath> mxNodeToHdPath;
    for (auto const& remapPair: nodePathMap) {
        if (remapPair.first != terminalNodePath) {
            mxNodeToHdPath.emplace(remapPair.second.GetName(), remapPair.first);
        }
    }

    const mx::ShaderStage& pxlStage = glslfxShader->getStage(mx::Stage::PIXEL);
    const auto& paramsBlock = pxlStage.getUniformBlock(mx::HW::PUBLIC_UNIFORMS);
    for (size_t i = 0; i < paramsBlock.size(); ++i) {

        // MaterialX parameter Information
        const auto* variable = paramsBlock[i];
        const auto varValue = variable->getValue();
        std::istringstream valueStream(varValue
            ? varValue->getValueString() : std::string());

        // Create a corresponding HdSt_MaterialParam
        HdSt_MaterialParam param;
        param.paramType = HdSt_MaterialParam::ParamTypeFallback;
        param.name = TfToken(variable->getVariable());

        // Get the parameter value from the terminal node
        const auto varType = variable->getType();
        const auto paramIt = mxValuesFromHd.find(variable->getVariable());
        if (paramIt != mxValuesFromHd.end()) {
            if (varType->getBaseType() == mx::TypeDesc::BASETYPE_BOOLEAN ||
                varType->getBaseType() == mx::TypeDesc::BASETYPE_FLOAT ||
                varType->getBaseType() == mx::TypeDesc::BASETYPE_INTEGER) {
                param.fallbackValue = paramIt->second;
            }
        }
        else {
            std::string separator;
            if (varType->getBaseType() == mx::TypeDesc::BASETYPE_BOOLEAN) {
                const bool val = valueStream.str() == "true";
                param.fallbackValue = VtValue(val);
            }
            else if (varType->getBaseType() == mx::TypeDesc::BASETYPE_FLOAT) {
                if (varType->getSize() == 1) {
                    float val;
                    valueStream >> val;
                    param.fallbackValue = VtValue(val);
                }
                else if (varType->getSize() == 2) {
                    GfVec2f val;
                    valueStream >> val[0] >> separator >> val[1];
                    param.fallbackValue = VtValue(val);
                }
                else if (varType->getSize() == 3) {
                    GfVec3f val;
                    valueStream >> val[0] >> separator >> val[1] >> separator 
                                >> val[2];
                    param.fallbackValue = VtValue(val);
                }
                else if (varType->getSize() == 4) {
                    GfVec4f val;
                    valueStream >> val[0] >> separator >> val[1] >> separator
                                >> val[2] >> separator >> val[3];
                    param.fallbackValue = VtValue(val);
                }
            }
            else if (varType->getBaseType() == mx::TypeDesc::BASETYPE_INTEGER) {
                if (varType->getSize() == 1) {
                    int val;
                    valueStream >> val;
                    param.fallbackValue = VtValue(val);
                }
                else if (varType->getSize() == 2) {
                    GfVec2i val;
                    valueStream >> val[0] >> separator >> val[1];
                    param.fallbackValue = VtValue(val);
                }
                else if (varType->getSize() == 3) {
                    GfVec3i val;
                    valueStream >> val[0] >> separator >> val[1] >> separator 
                                >> val[2];
                    param.fallbackValue = VtValue(val);
                }
                else if (varType->getSize() == 4) {
                    GfVec4i val;
                    valueStream >> val[0] >> separator >> val[1] >> separator
                        >> val[2] >> separator >> val[3];
                    param.fallbackValue = VtValue(val);
                }
            }
        }

        if (!param.fallbackValue.IsEmpty()) {
            materialParams->push_back(std::move(param));
        }

        if (varType->getSemantic() == mx::TypeDesc::SEMANTIC_FILENAME) {
            // Found a texture input. Manage its associated Hydra texture node

            // Find back the node path from the param name:
            auto nodeName = std::string{variable->getVariable()};
            auto underscorePos = nodeName.find('_');
            if (underscorePos != std::string_view::npos) {
                nodeName = nodeName.substr(0, underscorePos);
            }
            const auto originalPath = mxNodeToHdPath.find(nodeName);
            if (originalPath != mxNodeToHdPath.end()) {
                _AddMaterialXHydraTextureParams(param.name,
                                                hdNetwork,
                                                terminalNodePath,
                                                originalPath->second);
            } else {
                // Storm does not expect textures to be direct inputs on materials,
                // replace with a connection to an image node
                _ReplaceFilenameInput(variable->getVariable(),
                                      hdNetwork,
                                      terminalNodePath);
            }
        }
    }
}

static mx::ShaderPtr
_GenerateMaterialXShader(
    HdMaterialNetwork2 const& hdNetwork,
    SdfPath const& materialPath,
    HdMaterialNode2 const& terminalNode,
    SdfPath const& terminalNodePath,
    TfToken const& materialTagToken,
    TfToken const& apiName,
    bool const bindlessTexturesEnabled)
{
    // Get Standard Libraries and SearchPaths (for mxDoc and mxShaderGen)
    const mx::DocumentPtr& stdLibraries = HdMtlxStdLibraries();
    const mx::FileSearchPath& searchPaths = HdMtlxSearchPaths();

    // Create the MaterialX Document from the HdMaterialNetwork
    HdSt_MxShaderGenInfo mxHdInfo;
    HdMtlxTexturePrimvarData hdMtlxData;
    mx::DocumentPtr mtlxDoc = HdMtlxCreateMtlxDocumentFromHdNetwork(
                                    hdNetwork,
                                    terminalNode,   // MaterialX HdNode
                                    terminalNodePath,
                                    materialPath,
                                    stdLibraries,
                                    &hdMtlxData);

    // Add Hydra parameters for each of the Texture nodes
    _UpdateTextureNodes(mtlxDoc, hdNetwork, terminalNode, terminalNodePath,
                        hdMtlxData.hdTextureNodes, hdMtlxData.mxHdTextureMap,
                        &mxHdInfo.textureMap, &mxHdInfo.primvarMap,
                        &mxHdInfo.defaultTexcoordName);

    _UpdatePrimvarNodes(mtlxDoc, hdNetwork, terminalNodePath,
                        hdMtlxData.hdPrimvarNodes, &mxHdInfo.primvarMap,
                        &mxHdInfo.primvarDefaultValueMap);

    mxHdInfo.materialTag = materialTagToken.GetString();
    mxHdInfo.bindlessTexturesEnabled = bindlessTexturesEnabled;

    // Add domelight and other textures to mxHdInfo so the proper entry points
    // get generated
    _AddFallbackTextureMaps(terminalNode, terminalNodePath, &mxHdInfo.textureMap);

    // Generate the glslfx source code from the mtlxDoc
    return HdSt_GenMaterialXShader(
        mtlxDoc, stdLibraries, searchPaths, mxHdInfo, apiName);
}

void
HdSt_ApplyMaterialXFilter(
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& materialPath,
    HdMaterialNode2 const& terminalNode,
    SdfPath const& terminalNodePath,
    HdSt_MaterialParamVector* materialParams,
    HdStResourceRegistry *resourceRegistry)
{
    // Check if the Terminal is a MaterialX Node
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr mtlxSdrNode =
        sdrRegistry.GetShaderNodeByIdentifierAndType(terminalNode.nodeTypeId,
                                                     _tokens->mtlx);

    if (mtlxSdrNode) {
        TRACE_FUNCTION_SCOPE("ApplyMaterialXFilter: Found Mtlx Node.")

        // Anonymize the network to make sure shader code does not depend
        // on node names:
        HdMtlxNodePathMap nodePathMap;
        HdMaterialNetwork2 topoNetwork;
        auto topoHash = _BuildEquivalentMaterialNetwork(*hdNetwork, &topoNetwork, &nodePathMap);
        SdfPath anonymTerminalNodePath = nodePathMap[terminalNodePath];

        mx::ShaderPtr glslfxShader;
        const TfToken materialTagToken(_GetMaterialTag(*hdNetwork, terminalNode));
        const bool bindlessTexturesEnabled =
            resourceRegistry->GetHgi()->GetCapabilities()->IsSet(
                HgiDeviceCapabilitiesBitsBindlessTextures);
        const TfToken apiName = resourceRegistry->GetHgi()->GetAPIName();

        // Utilize the Resource Registry to cache the generated MaterialX glslfx Shader
        Tf_HashState shaderHash;
        TfHashAppend(shaderHash, topoHash);
        TfHashAppend(shaderHash, materialTagToken);
        HdInstance<mx::ShaderPtr> glslfxInstance =
            resourceRegistry->RegisterMaterialXShader(shaderHash.GetCode());

        if (glslfxInstance.IsFirstInstance()) {
            try {
                glslfxShader = _GenerateMaterialXShader(
                    topoNetwork, materialPath, terminalNode, anonymTerminalNodePath,
                    materialTagToken, apiName, bindlessTexturesEnabled);
            } catch (mx::Exception& exception) {
                TF_CODING_ERROR("Unable to create the Glslfx Shader.\n"
                    "MxException: %s", exception.what());
            }

            // Store the mx::ShaderPtr
            glslfxInstance.SetValue(glslfxShader);

        } else {
            // Get the mx::ShaderPtr from the resource registry
            glslfxShader = glslfxInstance.GetValue();

        }

        // Add a Fallback DomeLight texture node to the network
        _AddFallbackDomeLightTextureNode(hdNetwork, terminalNodePath);

        // Add material parameters from the original network
        _AddMaterialXParams(glslfxShader, hdNetwork, terminalNodePath, nodePathMap,
                            materialParams);

        // Create a new terminal node with the glslfxShader
        if (glslfxShader) {
            const std::string glslfxSourceCode =
                glslfxShader->getSourceCode(mx::Stage::PIXEL);
            SdrShaderNodeConstPtr sdrNode =
                sdrRegistry.GetShaderNodeFromSourceCode(glslfxSourceCode,
                                                        HioGlslfxTokens->glslfx,
                                                        NdrTokenMap()); // metadata
            HdMaterialNode2 newTerminalNode;
            newTerminalNode.nodeTypeId = sdrNode->GetIdentifier();
            newTerminalNode.inputConnections = terminalNode.inputConnections;
            newTerminalNode.parameters = terminalNode.parameters;

            // Replace the original terminalNode with this newTerminalNode
            hdNetwork->nodes[terminalNodePath] = newTerminalNode;
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
