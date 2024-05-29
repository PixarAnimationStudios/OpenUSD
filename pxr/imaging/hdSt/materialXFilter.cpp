//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    (filename)

    // Opacity Parameters - USD Preview Surface
    (UsdPreviewSurface)
    (opacity)
    (opacityThreshold)
    // Opacity Parameters - Standard Surface
    (standard_surface)
    (transmission)
    // Opacity Parameters - Open PBR
    (open_pbr_surface)
    (transmission_weight)
    (geometry_opacity)
    // Opacity Parameters - GlTF 
    (gltf_pbr)
    (alpha_mode)
    (alpha_cutoff)
    (alpha)
    // Opacity Parameters - Other Surface shaders
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
    // This represents living knowledge of the internals of the MaterialX 
    // shader generator for both GLSL and Metal. Such knowledge should reside
    // inside the generator class provided by MaterialX.

    // Dot filename is always topological due to code that prevents creating 
    // extra OpenGL samplers this is the only shader node id required. All 
    // other tests are done on the shader family.
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
    _mxTextureParamTokens,    // mxTextureTokens <-> HdStTextureTokens
    (filtertype)
    (uaddressmode)
    (vaddressmode)
);

// To store the mapping between the node paths in the HdMaterialNetwork to  
// the corresponding anonymized node paths - <hdNodePath, annonNodePath> 
using HdAnnonNodePathMap = std::unordered_map<SdfPath, SdfPath, SdfPath::Hash>;

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
    if (apiName == HgiTokens->Vulkan) {
        return HdStMaterialXShaderGenVkGlsl::create(mxHdInfo);
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
        auto mxShaderNodes = 
            mx::getShaderNodes(node, mx::SURFACE_SHADER_TYPE_STRING);
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

// Translate the MaterialX texture node input into the Hydra equivalents and
// store in the given hdTextureParams map
static void
_GetMxInputAsHdTextureParam(
    std::string const& mxInputName,
    std::string const& mxInputValue,
    std::map<TfToken, VtValue>* hdTextureParams)
{
    // MaterialX stdlib has two texture2d node types <image> and <tiledimage>

    // Properties common to both <image> and <tiledimage> texture nodes:
    if (mxInputName == _mxTextureParamTokens->filtertype) {
        (*hdTextureParams)[HdStTextureTokens->minFilter] = 
            _GetHdFilterValue(mxInputValue);
        (*hdTextureParams)[HdStTextureTokens->magFilter] = 
            VtValue(HdStTextureTokens->linear);
    }

    // Properties specific to <image> nodes:
    else if (mxInputName == _mxTextureParamTokens->uaddressmode) {
        (*hdTextureParams)[HdStTextureTokens->wrapS] = 
            _GetHdSamplerValue(mxInputValue);
    }
    else if (mxInputName == _mxTextureParamTokens->vaddressmode) {
        (*hdTextureParams)[HdStTextureTokens->wrapT] = 
            _GetHdSamplerValue(mxInputValue);
    }
}

static void
_AddDefaultMtlxTextureValues(
    mx::NodeDefPtr const& nodeDef,
    std::map<TfToken, VtValue>* hdTextureParams)
{    
    // Add the stdlib texture node default values 
    {
        // MaterialX uses repeat/periodic for the default wrap values, without
        // this the texture will use the Hydra default useMetadata. 
        // Note that these will get overwritten by any authored values
        (*hdTextureParams)[HdStTextureTokens->wrapS] = 
            VtValue(HdStTextureTokens->repeat);
        (*hdTextureParams)[HdStTextureTokens->wrapT] = 
            VtValue(HdStTextureTokens->repeat);

        // Set the default colorSpace to be 'raw'. This allows MaterialX to 
        // handle colorspace transforms.
        (*hdTextureParams) [_tokens->sourceColorSpace] =
            VtValue(HdStTokens->raw);
    }

    // Add custom texture node default values
    {
        // All custom Texture nodes boil down to an <image> node. Go into the 
        // implementation nodegraph to get the default texture values in 
        // case they differ from the above stdlib defaults

        // XXX Unsure about triplanar that has 3 image nodes. Does Storm
        // require per-image texture params? How does one specify that using a 
        // single token?

        // XXX We should recursively search for the <image> node in case it is 
        // nested more than one level deep via custom NodeDefs. For now, we 
        // only dig one level down since this is sufficient for the default 
        // set of MaterialX texture nodes.

        // Get the underlying image node from the implementation nodegraph
        const mx::InterfaceElementPtr& impl = nodeDef->getImplementation();
        if (!(impl && impl->isA<mx::NodeGraph>())) {
            return;
        }
        const auto imageNodes = 
            impl->asA<mx::NodeGraph>()->getNodes(mx::ShaderNode::IMAGE);
        if (imageNodes.empty()) {
            return;
        }

        // Get the default values for the underlying image node 
        for (TfToken const& inputName: _mxTextureParamTokens->allTokens) {
            mx::InputPtr mxInput = imageNodes.front()->getInput(inputName);
            if (!mxInput) {
                continue;
            }
            if (mxInput->hasInterfaceName()) {
                mxInput = nodeDef->getActiveInput(mxInput->getInterfaceName());
            }
            if (mxInput->hasValueString()) {
                _GetMxInputAsHdTextureParam(
                    inputName, mxInput->getValueString(), hdTextureParams);
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

// Store texture node connections, default dome light texture path and any
// filename inputs from the terminal nodeto the mxHdTextureMap
static void 
_UpdateMxHdTextureMap(
    std::set<SdfPath> const& hdTextureNodes,
    HdMtlxTexturePrimvarData::TextureMap const& hdMtlxTextureInfo,
    HdMaterialNode2 const& hdTerminalNode,
    SdfPath const& hdTerminalNodePath,
    mx::StringMap* mxHdTextureMap)
{
    // Store the added connection to the terminal node for MaterialXShaderGen
    for (SdfPath const& texturePath : hdTextureNodes) {
        auto mtlxTextureInfo = hdMtlxTextureInfo.find(texturePath.GetName());
        if (mtlxTextureInfo != hdMtlxTextureInfo.end()) {
            for (std::string const& fileInputName: mtlxTextureInfo->second) {
                // Note these connections were made in _UpdateTextureNode()
                const std::string newConnName =
                    texturePath.GetName() + "_" + fileInputName;
                mxHdTextureMap->emplace(newConnName, newConnName);
            }
        }
    }

    // Add the Dome Texture name to the TextureMap for MaterialXShaderGen
    const SdfPath domeTexturePath =
        hdTerminalNodePath.ReplaceName(_tokens->domeLightFallback);
    (*mxHdTextureMap)[domeTexturePath.GetName()] = domeTexturePath.GetName();

    // Check the terminal node for any filename inputs requiring special
    // handling due to node remapping:
    const mx::NodeDefPtr mxMaterialNodeDef =
        HdMtlxStdLibraries()->getNodeDef(hdTerminalNode.nodeTypeId.GetString());
    if (mxMaterialNodeDef) {
        for (auto const& mxInput : mxMaterialNodeDef->getActiveInputs()) {
            if (mxInput->getType() == _tokens->filename) {
                (*mxHdTextureMap)[mxInput->getName()] = mxInput->getName();
            }
        }
    }
}

// Connect the primvar nodes to the terminal node
static void 
_UpdatePrimvarNodes(
    mx::DocumentPtr const& mxDoc,
    HdMaterialNetwork2 const& hdNetwork,
    std::set<SdfPath> const& hdPrimvarNodes,
    mx::StringMap* mxHdPrimvarMap,
    mx::StringMap* mxHdPrimvarDefaultValueMap)
{
    for (auto const& primvarPath : hdPrimvarNodes) {
        const HdMaterialNode2& hdPrimvarNode = hdNetwork.nodes.at(primvarPath);

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

////////////////////////////////////////////////////////////////////////////////
// Helper functions to get the Material Tag 

template <typename T>
static bool
_ParamDiffersFrom(
    HdMaterialNode2 const& terminal,
    TfToken const& paramName,
    T const& paramValue)
{
    // A connected value is always considered to differ
    if (terminal.inputConnections.find(paramName) != 
        terminal.inputConnections.end()) {
        return true;
    }
    // Check the value itself
    const auto paramIt = terminal.parameters.find(paramName);
    if (paramIt != terminal.parameters.end() && paramIt->second != paramValue) {
        return true;
    }
    // Assume a default value is equal to the paramValue.
    return false;
}

static std::string const&
_GetUsdPreviewSurfaceMaterialTag(HdMaterialNode2 const& terminal)
{
    // See https://openusd.org/release/spec_usdpreviewsurface.html
    // and implementation in MaterialX libraries/bxdf/usd_preview_surface.mtlx

    // Non-zero opacityThreshold (or connected) triggers masked mode:
    if (_ParamDiffersFrom(terminal, _tokens->opacityThreshold, 0.0f)) {
        return HdStMaterialTagTokens->masked.GetString();
    }

    // Opacity less than 1.0 (or connected) triggers transparent mode:
    if (_ParamDiffersFrom(terminal, _tokens->opacity, 1.0f)) {
        return HdStMaterialTagTokens->translucent.GetString();
    }

    return HdStMaterialTagTokens->defaultMaterialTag.GetString();
}

static std::string const&
_GetStandardSurfaceMaterialTag(HdMaterialNode2 const& terminal)
{
    // See https://autodesk.github.io/standard-surface/
    // and implementation in MaterialX libraries/bxdf/standard_surface.mtlx
    if (_ParamDiffersFrom(terminal, _tokens->transmission, 0.0f) ||
        _ParamDiffersFrom(terminal, _tokens->opacity, GfVec3f(1.0f))) {
        return HdStMaterialTagTokens->translucent.GetString();
    }

    return HdStMaterialTagTokens->defaultMaterialTag.GetString();
}

static std::string const&
_GetOpenPBRSurfaceMaterialTag(HdMaterialNode2 const& terminal)
{
    // See https://academysoftwarefoundation.github.io/OpenPBR/
    // and the provided implementation
    if (_ParamDiffersFrom(terminal, _tokens->transmission_weight, 0.0f) ||
        _ParamDiffersFrom(terminal, _tokens->geometry_opacity, GfVec3f(1.0f))) {
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
    if (terminal.inputConnections.find(_tokens->alpha_mode) != 
        terminal.inputConnections.end()) {
        // A connected alpha_mode is non-standard, but is considered to 
        // imply blend.
        alphaMode = 2; // Blend
    } else {
        const auto alphaModeIt = terminal.parameters.find(_tokens->alpha_mode);
        if (alphaModeIt != terminal.parameters.end()) {
            if (alphaModeIt->second.IsHolding<int>()) {
                const auto value = alphaModeIt->second.UncheckedGet<int>();
                if (value >= 0 && value <= 2) {
                    alphaMode = value;
                }
            }
        }
    }

    TfToken materialToken = HdStMaterialTagTokens->defaultMaterialTag;
    if (alphaMode == 1) { // Mask
        if (_ParamDiffersFrom(terminal, _tokens->alpha_cutoff, 1.0f) && 
            _ParamDiffersFrom(terminal, _tokens->alpha, 1.0f)) {
            materialToken = HdStMaterialTagTokens->masked;
        }
    }
    else if (alphaMode == 2) { // Blend
        if (_ParamDiffersFrom(terminal, _tokens->alpha, 1.0f)) {
            materialToken = HdStMaterialTagTokens->translucent;
        }
    }

    if (_ParamDiffersFrom(terminal, _tokens->transmission, 0.0f)) {
        return HdStMaterialTagTokens->translucent.GetString();
    }

    return materialToken.GetString();
}

static const mx::TypeDesc*
_GetMxTypeDescription(std::string const& typeName)
{
    // Add whatever is necessary for current codebase:
    static const auto _typeLibrary = 
        std::map<std::string, const mx::TypeDesc*>{
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

// This function adds a stripped down version of the surfaceshader node to the
// given MaterialX document. Parameters are added as inputs and any connections
// are replaced with dummy values (0.5). 
static mx::NodePtr
_AddStrippedSurfaceNode(
    mx::DocumentPtr mxDocument,
    std::string const& nodeName,
    HdMaterialNode2 const& hdNode,
    HdMaterialNetwork2 const& hdNetwork)
{
    // Add the hdNode to the mxDocument
    mx::NodeDefPtr mxNodeDef =
        HdMtlxStdLibraries()->getNodeDef(hdNode.nodeTypeId.GetString());
    mx::NodePtr mxNode = mxDocument->addNodeInstance(mxNodeDef, nodeName);

    // Add inputs to the hdNode for each connection
    for (auto const& connIt: hdNode.inputConnections) {
        const mx::InputPtr mxInputDef =
            mxNodeDef->getActiveInput(connIt.first.GetString());
        if (!mxInputDef) {
            continue;
        }
        auto const* mxTypeDesc = _GetMxTypeDescription(mxInputDef->getType());
        if (!mxTypeDesc) {
            continue;
        }
        // If hdNode is connected to the surfaceshader node, recursively call 
        // this function to make sure that surfaceshader node is added to 
        // the mxDocument
        if (mxTypeDesc == mx::Type::SURFACESHADER) {
            auto const& hdConnectedPath = connIt.second.front().upstreamNode;
            auto const& hdConnectedNode = hdNetwork.nodes.at(hdConnectedPath);
            mx::NodePtr mxConnectedNode =
                _AddStrippedSurfaceNode(mxDocument, hdConnectedPath.GetName(),
                                        hdConnectedNode, hdNetwork);
            mx::InputPtr mxInput =
                mxNode->addInput(mxInput->getName(), mxInput->getType());
            mxInput->setConnectedNode(mxConnectedNode);
        }
        // Add the connection as an input with each component set to 0.5
        else if (mxTypeDesc->getBaseType() == mx::TypeDesc::BASETYPE_FLOAT &&
                 mxTypeDesc->getSemantic() != mx::TypeDesc::SEMANTIC_MATRIX) {
            std::string valueStr = "0.5";
            for (size_t i = 1; i < mxTypeDesc->getSize(); ++i) {
                valueStr += ", 0.5";
            }
            mx::InputPtr mxInput =
                mxNode->addInput(mxInputDef->getName(), mxInputDef->getType());
            mxInput->setValueString(valueStr);
        }
    }

    // Add inputs to the hdNode for each parameter
    for (auto const& paramIt: hdNode.parameters) {
        const mx::InputPtr mxInputDef =
            mxNodeDef->getActiveInput(paramIt.first.GetString());
        if (!mxInputDef) {
            continue;
        }
        auto const* mxTypeDesc = _GetMxTypeDescription(mxInputDef->getType());
        if (!mxTypeDesc) {
            continue;
        }

        if (mxTypeDesc->getBaseType() == mx::TypeDesc::BASETYPE_FLOAT &&
            mxTypeDesc->getSemantic() != mx::TypeDesc::SEMANTIC_MATRIX) {
            // Add the parameter as an input to the mxNode in the mx Document
            mx::InputPtr mxInput =
                mxNode->addInput(mxInputDef->getName(), mxInputDef->getType());
            mxInput->setValueString(HdMtlxConvertToString(paramIt.second));
        }
    }
    return mxNode;
}

// Use MaterialX to determine if the given terminal is a transparent surface
static bool
_IsTransparentShader(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& terminal)
{
    // Create a materialX document with a simplified version of the hdNetwork
    // containing a stripped down version of the surfaceshader node without the
    // full shader graph so we can use the MaterialX utility below to determine
    // if the network contains a transparent surface

    mx::DocumentPtr mxDocument = mx::createDocument();
    mxDocument->importLibrary(HdMtlxStdLibraries());

    mx::NodePtr terminalNode = _AddStrippedSurfaceNode(
        mxDocument, "MxTerminalNode", terminal, hdNetwork);

    return mx::isTransparentSurface(terminalNode);
}

static std::string const&
_GetMaterialTag(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNode2 const& terminal)
{
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr mtlxSdrNode =
        sdrRegistry.GetShaderNodeByIdentifierAndType(
            terminal.nodeTypeId, _tokens->mtlx);
    
    // Cover the most frequent and fully specified terminal nodes
    const TfToken & mtlxNodeFamily = mtlxSdrNode->GetFamily();
    if (mtlxNodeFamily == _tokens->UsdPreviewSurface) {
        return _GetUsdPreviewSurfaceMaterialTag(terminal);
    }
    if (mtlxNodeFamily == _tokens->standard_surface) {
        return _GetStandardSurfaceMaterialTag(terminal);
    }
    if (mtlxNodeFamily == _tokens->open_pbr_surface) {
        return _GetOpenPBRSurfaceMaterialTag(terminal);
    }
    if (mtlxNodeFamily == _tokens->gltf_pbr) {
        return _GetGlTFSurfaceMaterialTag(terminal);
    }

    // For terminal nodes not fully specified we require more MaterialX info
    const mx::DocumentPtr& stdLibraries = HdMtlxStdLibraries();
    mx::NodeDefPtr mxNodeDef =
        stdLibraries->getNodeDef(mtlxSdrNode->GetIdentifier().GetString());

    const auto activeOutputs = mxNodeDef->getActiveOutputs();
    if (activeOutputs.size() != 1 || 
        activeOutputs.back()->getType() != mx::SURFACE_SHADER_TYPE_STRING) {
        // Outputting anything that is not a surfaceshader will be
        // considered opaque, unless outputting a color4 or vector4.
        // XXX This is not fully per USD specs, but is supported by MaterialX.
        auto const* typeDesc = 
            _GetMxTypeDescription(activeOutputs.back()->getType());
        if (typeDesc == mx::Type::COLOR4 || typeDesc == mx::Type::VECTOR4) {
            return HdStMaterialTagTokens->translucent.GetString();
        }
        return HdStMaterialTagTokens->defaultMaterialTag.GetString();
    }

    if (mtlxNodeFamily == _tokens->convert) {
        if (terminal.nodeTypeId == _tokens->ND_convert_color4_surfaceshader ||
            terminal.nodeTypeId == _tokens->ND_convert_vector4_surfaceshader) {
            return HdStMaterialTagTokens->translucent.GetString();
        }
        return HdStMaterialTagTokens->defaultMaterialTag.GetString();
    }

    // Out of easy answers, delegate to MaterialX
    if (_IsTransparentShader(hdNetwork, terminal)) {
        return HdStMaterialTagTokens->translucent.GetString();
    }
    return HdStMaterialTagTokens->defaultMaterialTag.GetString();
}

////////////////////////////////////////////////////////////////////////////////
// Helper functions for to make sure the hdNetwork is organized in a way that 
// Storm can find all the textures and primvars. 

// Returns true is the given mtlxSdrNode requires primvar support for texcoords
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
        if (!nodegraph->getNodes(_tokens->texcoord).empty()) {
            return true;
        }
    }
    return false;
}

// Browse the nodes to find primvar connections to add to the terminal node
static void
_ConnectPrimvarNodesToTerminalNode(
    SdfPath const& terminalNodePath,
    HdMaterialNetwork2* hdNetwork)
{
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();

    for (auto& hdNodePair: hdNetwork->nodes) {
        const SdrShaderNodeConstPtr mtlxSdrNode =
            sdrRegistry.GetShaderNodeByIdentifierAndType(
                hdNodePair.second.nodeTypeId,_tokens->mtlx);

        if (mtlxSdrNode->GetFamily() != _tokens->geompropvalue ||
            !_NodeUsesTexcoordPrimvar(mtlxSdrNode)) {
            return;
        }

        // Connect the primvar node to the terminal node for HdStMaterialNetwork
        // And create a unique name for the new connection.
        const std::string newConnName =
            hdNodePair.first.GetName() + "_primvarconn";
        HdMaterialConnection2 primvarConn;
        primvarConn.upstreamNode = hdNodePair.first;
        primvarConn.upstreamOutputName = TfToken(newConnName);

        hdNetwork->nodes[terminalNodePath]
            .inputConnections[primvarConn.upstreamOutputName] = {primvarConn};
    }
}

// Returns the default texture cordinate name from the textureNodes sdr metadata
// if no name was specified return 'st'
static TfToken
_GetDefaultTexcoordName()
{
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    const SdrShaderNodeConstPtr mtlxSdrNode =
        sdrRegistry.GetShaderNodeByIdentifierAndType(
            _tokens->ND_image_color3, _tokens->mtlx);
    auto const& metadata = mtlxSdrNode->GetMetadata();
    const auto primvarIt = metadata.find(SdrNodeMetadata->Primvars);
    return primvarIt != metadata.end() 
            ? TfToken(primvarIt->second) 
            : _tokens->st;
}

// Add the Hydra texture node parameters to the texture node and connect the 
// texture node to the terminal node
static void
_UpdateTextureNode(
    TfToken mtlxParamName,
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& terminalNodePath,
    SdfPath const& textureNodePath)
{
    HdMaterialNode2& hdTextureNode = hdNetwork->nodes[textureNodePath];

    // Set the default texture coordinate name as the 'st' parameter.
    hdTextureNode.parameters[_tokens->st] = TfToken(_GetDefaultTexcoordName());

    // Gather the default Texture Parameters
    std::map<TfToken, VtValue> hdParameters;
    _AddDefaultMtlxTextureValues(
        HdMtlxStdLibraries()->getNodeDef(hdTextureNode.nodeTypeId.GetString()), 
        &hdParameters);

    // Gather the authored Texture Parameters
    for (auto const& param : hdTextureNode.parameters) {
        // Get the Hydra equivalents for the Mx Texture node parameters
        std::string const& mxInputName = param.first.GetString();
        std::string const mxInputValue = HdMtlxConvertToString(param.second);
        _GetMxInputAsHdTextureParam(mxInputName, mxInputValue, &hdParameters);
    }

    // Add the Hydra Texture Parameters to the Texture Node
    for (auto const& param : hdParameters) {
        hdTextureNode.parameters[param.first] = param.second;
    }

    // Make and add a new connection to the terminal node
    HdMaterialConnection2 textureConn;
    textureConn.upstreamOutputName = mtlxParamName;
    textureConn.upstreamNode = textureNodePath;
    hdNetwork->nodes[terminalNodePath].
        inputConnections[mtlxParamName] = {textureConn};
}

static void
_ReplaceFilenameInput(
    HdMaterialNetwork2* hdNetwork,
    SdfPath const& hdTerminalNodePath,
    std::string const& mxFilenameInputName)
{
    const auto& hdTerminalNode = hdNetwork->nodes.at(hdTerminalNodePath);
    const mx::NodeDefPtr mxNodeDef =
        HdMtlxStdLibraries()->getNodeDef(hdTerminalNode.nodeTypeId.GetString());
    if (!mxNodeDef) {
        return;
    }
    const mx::InputPtr mxInput = mxNodeDef->getActiveInput(mxFilenameInputName);
    if (!mxInput) {
        return;
    }
    const mx::InterfaceElementPtr impl = mxNodeDef->getImplementation();
    if (!impl || !impl->isA<mx::NodeGraph>()) {
        return;
    }

    // Find the mxTextureNode in the nodegraph that interfaces with the
    // filename input
    mx::NodePtr mxTextureNode;
    std::string mxTextureNodefilenameInputName;
    for (mx::NodePtr const& node: impl->asA<mx::NodeGraph>()->getNodes()) {
        for (mx::InputPtr const& input: node->getInputs()) {
            if (input->getType() != _tokens->filename) {
                continue;
            }
            // Get the Texture node and input name for the filename input
            mxTextureNodefilenameInputName = input->getName();
            if (input->getInterfaceName() == mxFilenameInputName) {
                mxTextureNode = node;
                break;
            }
            // We need to handle correctly the situation where there are
            // "dot" nodes in the NodeGraph.
            mx::NodePtr dotNode = input->getConnectedNode();
            while (dotNode && dotNode->getCategory() == "dot") {
                mx::InputPtr dotInput = dotNode->getInput("in");
                if (dotInput && 
                    dotInput->getInterfaceName() == mxFilenameInputName) {
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
    const mx::NodeDefPtr mxTextureNodeDef = mxTextureNode->getNodeDef();
    if (!mxTextureNodeDef) {
        return;
    }

    // Gather texture parameters on the found mxTextureNode
    std::map<TfToken, VtValue> terminalTextureParams;
    _AddDefaultMtlxTextureValues(mxTextureNodeDef, &terminalTextureParams);
    for (TfToken const& mxInputName: _mxTextureParamTokens->allTokens) {
        const mx::InputPtr mxInput = mxTextureNode->getInput(mxInputName);
        // Get the Hydra equivalents for the Mx Texture node parameters
        if (mxInput && mxInput->hasValueString()) {
            _GetMxInputAsHdTextureParam(
                mxInputName, mxInput->getValueString(), &terminalTextureParams);
        }
    }
    // Gather the Hydra Texture Parameters on the terminal node.
    for (auto const& param : hdTerminalNode.parameters) {
        // Get the Hydra equivalents for the Mx Texture node parameters
        std::string const& mxInputName = param.first.GetString();
        std::string const mxInputValue = HdMtlxConvertToString(param.second);
        _GetMxInputAsHdTextureParam(
            mxInputName, mxInputValue, &terminalTextureParams);
    }

    // Get the texture node from the Implementation Nodegraph and gather the
    // nodeTypeId and parameter information.

    // Get the filename parameter value from the terminal node
    const TfToken filenameToken(mxFilenameInputName);
    auto filenameParamIt = hdTerminalNode.parameters.find(filenameToken);
    if (filenameParamIt == hdTerminalNode.parameters.end()) {
        return;
    }

    // Create a new Texture Node
    HdMaterialNode2 terminalTextureNode;
    terminalTextureNode.nodeTypeId = TfToken(mxTextureNodeDef->getName());
    terminalTextureNode.parameters[TfToken(mxTextureNodefilenameInputName)] =
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
    HdAnnonNodePathMap const& hdToAnnonNodePathMap,
    HdSt_MaterialParamVector* materialParams)
{
    TRACE_FUNCTION_SCOPE("Collect Mtlx params from glslfx shader.")
    if (!glslfxShader) {
        return;
    }

    // Storm can only find primvar nodes when they are connected to the terminal
    _ConnectPrimvarNodesToTerminalNode(terminalNodePath, hdNetwork);

    // Store all the parameter values, mapped by the anonymized names used 
    // for MaterialXShaderGen
    // <annonNodeName_paramName, hdParamVtValue>
    std::map<std::string, VtValue> mxParamNameToValue;
    for (auto const& node: hdNetwork->nodes) {
        // Terminal Node parameters are not prefixed.
        std::string annonNodeNamePrefix;
        if (node.first != terminalNodePath) {
            const auto annonNodePathIt = hdToAnnonNodePathMap.find(node.first);
            if (annonNodePathIt != hdToAnnonNodePathMap.end()) {
                annonNodeNamePrefix = annonNodePathIt->second.GetName() + "_";
            }
        }
        for (auto const& param: node.second.parameters) {
            if (param.second.IsHolding<std::string>() ||
                param.second.IsHolding<TfToken>()) {
                continue;
            }
            mxParamNameToValue.emplace(
                annonNodeNamePrefix + param.first.GetString(), param.second);
        }
    }

    // Build a mapping from the anonymized node name to the original Hydra
    // SdfPath. This is to help find texture nodes associated with filename 
    // inputs found in the uniform block below.
    std::map<std::string, SdfPath> annonToHdNodePathMap;
    for (auto const& pathPair: hdToAnnonNodePathMap) {
        if (pathPair.first != terminalNodePath) {
            annonToHdNodePathMap.emplace(
                pathPair.second.GetName(), pathPair.first);
        }
    }

    const mx::ShaderStage& pxlStage = glslfxShader->getStage(mx::Stage::PIXEL);
    const auto& paramsBlock = pxlStage.getUniformBlock(mx::HW::PUBLIC_UNIFORMS);
    for (size_t i = 0; i < paramsBlock.size(); ++i) {

        // MaterialX parameter Information
        const auto* variable = paramsBlock[i];
        const auto varType = variable->getType();

        // Create a corresponding HdSt_MaterialParam
        HdSt_MaterialParam param;
        param.paramType = HdSt_MaterialParam::ParamTypeFallback;
        param.name = TfToken(variable->getVariable());

        // Get the parameter value from the map created above
        const auto paramValueIt =
            mxParamNameToValue.find(variable->getVariable());
        if (paramValueIt != mxParamNameToValue.end()) {
            if (varType->getBaseType() == mx::TypeDesc::BASETYPE_BOOLEAN ||
                varType->getBaseType() == mx::TypeDesc::BASETYPE_FLOAT ||
                varType->getBaseType() == mx::TypeDesc::BASETYPE_INTEGER) {
                param.fallbackValue = paramValueIt->second;
            }
        }
        // If it was not found in the mapping use the value from the MaterialX
        // variable to get the value. 
        else {
            std::string separator;
            const auto varValue = variable->getValue();
            std::istringstream valueStream(varValue
                ? varValue->getValueString() : std::string());
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

        // For filename inputs, manage the associated texture node
        if (varType->getSemantic() == mx::TypeDesc::SEMANTIC_FILENAME) {
            // Get the anonymized MaterialX node name from the param name
            // annonNodeName_paramName -> annonNodeName
            std::string mxNodeName = variable->getVariable();
            const auto underscorePos = mxNodeName.find('_');
            if (underscorePos != std::string_view::npos) {
                mxNodeName = mxNodeName.substr(0, underscorePos);
            }

            // Get the original hdNodeName from the MaterialX node name
            const auto hdNodePathIt = annonToHdNodePathMap.find(mxNodeName);
            if (hdNodePathIt != annonToHdNodePathMap.end()) {
                _UpdateTextureNode(
                    param.name, hdNetwork, 
                    terminalNodePath, hdNodePathIt->second);
            } else {
                // Storm does not expect textures/filename to be direct inputs 
                // on materials, replace this filename input with a connection
                // to an image node
                _ReplaceFilenameInput(
                    hdNetwork, terminalNodePath, variable->getVariable());
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
    const mx::DocumentPtr mtlxDoc =
        HdMtlxCreateMtlxDocumentFromHdNetwork(
            hdNetwork, terminalNode, terminalNodePath, materialPath,
            stdLibraries, &hdMtlxData);

    // Add domelight and other textures to mxHdInfo so the proper entry points
    // get generated in MaterialXShaderGen
    _UpdateMxHdTextureMap(
        hdMtlxData.hdTextureNodes, hdMtlxData.mxHdTextureMap,
        terminalNode, terminalNodePath, &mxHdInfo.textureMap);

    _UpdatePrimvarNodes(
        mtlxDoc, hdNetwork, hdMtlxData.hdPrimvarNodes, 
        &mxHdInfo.primvarMap, &mxHdInfo.primvarDefaultValueMap);

    mxHdInfo.materialTag = materialTagToken.GetString();
    mxHdInfo.bindlessTexturesEnabled = bindlessTexturesEnabled;

    // Generate the glslfx source code from the mtlxDoc
    return HdSt_GenMaterialXShader(
        mtlxDoc, stdLibraries, searchPaths, mxHdInfo, apiName);
}

////////////////////////////////////////////////////////////////////////////////
// Helper functions to create an anonymized HdMaterialNetwork2
//

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

// Build the topoNetwork, equivalent to the given hdNetwork but anonymized and 
// stripped of non-topological parameters to better re-use the generated shader.  
size_t _BuildEquivalentMaterialNetwork(
    HdMaterialNetwork2 const& hdNetwork,
    HdMaterialNetwork2* topoNetwork,
    HdAnnonNodePathMap* annonNodePathMap)
{
    // The goal here is to strip all local names in the network paths in order 
    // to produce MaterialX data that does not have uniform parameter names 
    // that vary based on USD node names.
    // We also want to strip all non-topological parameters in order to get a 
    // shader that has default values for all parameters and can be re-used.
    annonNodePathMap->clear();

    // Annonymized paths will be of the form:
    //   /NG_Anonymized/N0, /NG_Anonymized/N1, /NG_Anonymized/N2...
    SdfPath annonBaseName(_tokens->NG_Anonymized);

    // Create anonymized names for each of the nodes in the material network. 
    // To do this we process the network in a depth-first traversal starting
    // at the terminals. This provides a stable traversal and consistant 
    // anonymized renaming that will not be affected by the ordering of the 
    // SdfPaths in the hdNetwork. 
    size_t nodeCounter = 0;
    std::vector<const SdfPath*> pathsToTraverse;
    for (const auto& terminal : hdNetwork.terminals) {
        const auto& connection = terminal.second;
        pathsToTraverse.push_back(&(connection.upstreamNode));
    }
    while (!pathsToTraverse.empty()) {
        const SdfPath *path = pathsToTraverse.back();
        pathsToTraverse.pop_back();

        if (!annonNodePathMap->count(*path)) {
            const HdMaterialNode2& node = hdNetwork.nodes.find(*path)->second;
            (*annonNodePathMap)[*path] = annonBaseName.AppendChild(
                TfToken("N" + std::to_string(nodeCounter++)));

            for (const auto& input : node.inputConnections) {
                for (const auto& connection : input.second) {
                    pathsToTraverse.push_back(&(connection.upstreamNode));
                }
            }
        }
    }

    // Copy the incoming hdNetwork to the topoNetwork using only the 
    // anonymized names
    topoNetwork->primvars = hdNetwork.primvars;
    for (const auto& terminal : hdNetwork.terminals) {
        topoNetwork->terminals.emplace(
            terminal.first,
            HdMaterialConnection2 { 
                (*annonNodePathMap)[terminal.second.upstreamNode],
                terminal.second.upstreamOutputName });
    }
    for (const auto& nodePair : hdNetwork.nodes) {
        const HdMaterialNode2& inNode = nodePair.second;

        HdMaterialNode2 outNode;
        outNode.nodeTypeId = inNode.nodeTypeId;
        if (_IsTopologicalShader(inNode.nodeTypeId)) {
            // Topological nodes have parameters that affect topology. 
            // We can not strip them.
            outNode.parameters = inNode.parameters;
        } else {
            // Parameters that are color managed are also topological as they
            // result in different nodes being added in the MaterialX graph
            for (const auto& param: inNode.parameters) {
                const auto colorManagedInput = 
                    SdfPath::StripPrefixNamespace(param.first.GetString(),
                                                  SdfFieldKeys->ColorSpace);
                // If this parameter is to indicate a colorspace on a color 
                // managed input, find and add that corresponding input
                if (colorManagedInput.second) {
                    outNode.parameters.insert(param);

                    // Get the parameter value for the input
                    const TfToken colorInputParam(colorManagedInput.first);
                    const auto colorInputIt =
                        inNode.parameters.find(colorInputParam);
                    if (colorInputIt != inNode.parameters.end()) {
                        VtValue colorInputValue = colorInputIt->second;

                        // Use an empty asset for color managed files
                        if (colorInputValue.IsHolding<SdfAssetPath>()) {
                            colorInputValue = VtValue(SdfAssetPath());
                        }
                        outNode.parameters.emplace(
                            colorInputParam, 
                            colorInputValue); 
                    }
                }
            }
        }

        for (const auto& connPair : inNode.inputConnections) {
            std::vector<HdMaterialConnection2> outConn;
            for (const auto& inConn : connPair.second) {
                outConn.emplace_back(
                    HdMaterialConnection2 { 
                        (*annonNodePathMap)[inConn.upstreamNode], 
                        inConn.upstreamOutputName });
            }
            outNode.inputConnections.emplace(connPair.first, std::move(outConn));
        }
        topoNetwork->nodes.emplace(
            (*annonNodePathMap)[nodePair.first], std::move(outNode));
    }

    // Build the topo hash from the topo network
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


mx::ShaderPtr
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
    if (!mtlxSdrNode) {
        return nullptr;
    }

    TRACE_FUNCTION_SCOPE("ApplyMaterialXFilter: Found Mtlx Node.")

    // Anonymize the network to make sure shader code does not depend
    // on node names
    HdAnnonNodePathMap annonNodePathMap;
    HdMaterialNetwork2 annonNetwork;
    auto topoHash = _BuildEquivalentMaterialNetwork(
        *hdNetwork, &annonNetwork, &annonNodePathMap);
    SdfPath anonTerminalNodePath = annonNodePathMap[terminalNodePath];

    mx::ShaderPtr glslfxShader;
    const TfToken materialTag(_GetMaterialTag(*hdNetwork, terminalNode));
    const bool bindlessTexturesEnabled = 
        resourceRegistry->GetHgi()->GetCapabilities()->IsSet(
            HgiDeviceCapabilitiesBitsBindlessTextures);
    const TfToken apiName = resourceRegistry->GetHgi()->GetAPIName();

    // Use the Resource Registry to cache the generated MaterialX 
    // glslfx Shader
    Tf_HashState shaderHash;
    TfHashAppend(shaderHash, topoHash);
    TfHashAppend(shaderHash, materialTag);
    HdInstance<mx::ShaderPtr> glslfxInstance =
        resourceRegistry->RegisterMaterialXShader(shaderHash.GetCode());

    if (glslfxInstance.IsFirstInstance()) {
        try {
            glslfxShader = _GenerateMaterialXShader(
                annonNetwork, materialPath, terminalNode, 
                anonTerminalNodePath, materialTag, apiName, 
                bindlessTexturesEnabled);
        } catch (mx::Exception& exception) {
            TF_CODING_ERROR("Unable to create the Glslfx Shader.\n"
                "MxException: %s", exception.what());
        }

        // Store the mx::ShaderPtr
        glslfxInstance.SetValue(glslfxShader);
    }
    else {
        // Get the mx::ShaderPtr from the resource registry
        glslfxShader = glslfxInstance.GetValue();
    }

    // Add a Fallback DomeLight texture node to the network
    _AddFallbackDomeLightTextureNode(hdNetwork, terminalNodePath);

    // Add material parameters from the original network
    _AddMaterialXParams(
        glslfxShader, hdNetwork, terminalNodePath,
        annonNodePathMap, materialParams);

    // Create a new terminal node with the glslfxShader
    if (glslfxShader) {
        const std::string glslfxSourceCode =
            glslfxShader->getSourceCode(mx::Stage::PIXEL);
        SdrShaderNodeConstPtr sdrNode =
            sdrRegistry.GetShaderNodeFromSourceCode(
                glslfxSourceCode,
                HioGlslfxTokens->glslfx,
                NdrTokenMap()); // metadata
        HdMaterialNode2 newTerminalNode;
        newTerminalNode.nodeTypeId = sdrNode->GetIdentifier();
        newTerminalNode.inputConnections = terminalNode.inputConnections;
        newTerminalNode.parameters = terminalNode.parameters;

        // Replace the original terminalNode with this newTerminalNode
        hdNetwork->nodes[terminalNodePath] = newTerminalNode;
    }

    return glslfxShader;
}

PXR_NAMESPACE_CLOSE_SCOPE
