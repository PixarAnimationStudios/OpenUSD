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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/material.h"
#include "pxr/imaging/hdSt/materialBufferSourceAndTextureHelper.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"
#include "pxr/imaging/hdSt/surfaceShader.h"
#include "pxr/imaging/hdSt/textureResource.h"
#include "pxr/imaging/hdSt/textureResourceHandle.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/textureHandle.h"
#include "pxr/imaging/glf/textureRegistry.h"
#include "pxr/imaging/glf/uvTextureStorage.h"
#include "pxr/imaging/hio/glslfx.h"

#include "pxr/base/tf/staticTokens.h"

#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"

#include <boost/pointer_cast.hpp>

// XXX In progress of moving HdStMaterial to use 
// HdSceneDelegate::GetMaterialResource to obtain full material networks
// which will let us receive more general networks in Storm in the future.
#include "pxr/base/tf/getenv.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (limitSurfaceEvaluation)
    (opacity)
);

typedef std::unique_ptr<HioGlslfx> HioGlslfxUniquePtr;

HioGlslfx *HdStMaterial::_fallbackSurfaceShader = nullptr;

// XXX In progress of deprecating hydra material adapter
static bool _IsEnabledStormMaterialNetworks() {
    static std::string _stormMatNet = 
        TfGetenv("STORM_ENABLE_MATERIAL_NETWORKS");

    return !_stormMatNet.empty() && std::stoi(_stormMatNet) > 0;
}

/// \struct HdStMaterialConnection
///
/// Describes a single connection to an upsream node and output port 
///
/// XXX Replacement for HdRelationship. Unify with HdPrman: MatfiltConnection.
struct HdStMaterialConnection {
    SdfPath upstreamNode;
    TfToken upstreamOutputName;
};

/// \struct HdStMaterialNode
///
/// Describes an instance of a node within a network
/// A node contains a (shader) type identifier, parameter values, and 
/// connections to upstream nodes. A single input (mapped by TfToken) may have
/// multiple upstream connections to describe connected array elements.
///
/// XXX Replacement for HdMaterialNode. Unify with HdPrman: MatfiltNode.
struct HdStMaterialNode {
    TfToken nodeTypeId;
    std::map<TfToken, VtValue> parameters;
    std::map<TfToken, std::vector<HdStMaterialConnection>> inputConnections;
};

/// \struct HdStMaterialNetwork
/// 
/// Container of nodes and top-level terminal connections. This is the mutable
/// representation of a shading network sent to filtering functions by a
/// MatfiltFilterChain.
///
/// XXX Replacement for HdMaterialNetwork. Unify with HdPrman: MatfiltNetwork.
struct HdStMaterialNetwork {
    std::map<SdfPath, HdStMaterialNode> nodes;
    std::map<TfToken, HdStMaterialConnection> terminals;
    TfTokenVector primvars;
};

// XXX We wish to transition to a new material network description and unify
// with HdPrman's MatfiltNetwork. For now we internally convert the deprecated
// HdMaterialNetwork over to the new description so we do not have to redo all
// the Storm code when we swap the classes in Hd to the new description.
//
// Equivilant of: HdPrman's MatfiltConvertFromHdMaterialNetworkMapTerminal.
// With some modifications since HdMaterialNetworkMap now has 'terminals'.
bool
_ConvertLegacyHdMaterialNetwork(
    HdMaterialNetworkMap const& hdNetworkMap,
    TfToken const & terminalName,
    HdStMaterialNetwork *result)
{
    auto iter = hdNetworkMap.map.find(terminalName);
    if (iter == hdNetworkMap.map.end()) {
        return false;
    }
    const HdMaterialNetwork & hdNetwork = iter->second;

    // Transfer over individual nodes
    for (const HdMaterialNode & node : hdNetwork.nodes) {
        HdStMaterialNode & newNode = result->nodes[node.path];
        newNode.nodeTypeId = node.identifier;
        newNode.parameters = node.parameters;

        // Check if this node is a terminal
        auto const& termIt = std::find(
            hdNetworkMap.terminals.begin(), 
            hdNetworkMap.terminals.end(), 
            node.path);

        if (termIt != hdNetworkMap.terminals.end()) {
            result->terminals[terminalName].upstreamNode = node.path;
        }
    }

    // Transfer relationships to inputConnections on receiving/downstream nodes.
    for (HdMaterialRelationship const& rel : hdNetwork.relationships) {
        // outputId (in hdMaterial terms) is the input of the receiving node
        auto iter = result->nodes.find(rel.outputId);
        // skip connection if the destination node doesn't exist
        if (iter == result->nodes.end()) {
            continue;
        }
        iter->second.inputConnections[rel.outputName]
            .emplace_back( HdStMaterialConnection{rel.inputId, rel.inputName});
    }

    // Transfer primvars:
    result->primvars = hdNetwork.primvars;

    return true;
}

static TfToken
_GetMaterialTag(
    VtDictionary const& metadata,
    HdStMaterialNode const& terminal)
{
    // Strongest materialTag opinion is a hardcoded tag in glslfx meta data.
    // This can be used for additive, translucent or volume materials.
    // See HdMaterialTagTokens.
    VtValue vtMetaTag = TfMapLookupByValue(
        metadata,
        HdShaderTokens->materialTag,
        VtValue());

    if (vtMetaTag.IsHolding<std::string>()) {
        return TfToken(vtMetaTag.UncheckedGet<std::string>());
    }

    bool isTranslucent = false;

    // Next strongest opinion is a connection to 'terminal.opacity'
    auto const& opacityConIt = terminal.inputConnections.find(_tokens->opacity);
    isTranslucent = (opacityConIt != terminal.inputConnections.end());

    // Weakest opinion is an authored terminal.opacity value.
    if (!isTranslucent) {
        for (auto const& paramIt : terminal.parameters) {
            if (paramIt.first != _tokens->opacity) continue;

            VtValue const& vtOpacity = paramIt.second;
            isTranslucent = vtOpacity.Get<float>() < 1.0f;
            break;
        }
    }

    if (isTranslucent) {
        // Default to our cheapest blending: unsorted additive
        return HdStMaterialTagTokens->additive;
    }

    // An empty materialTag on the HdRprimCollection level means: 'ignore all
    // materialTags and add everything to the collection'. Instead we return a
    // default token because we want materialTags to drive HdSt collections.
    return HdStMaterialTagTokens->defaultMaterialTag;
}

static void
_GetGlslfxForTerminal(
    HioGlslfxUniquePtr& glslfxOut,
    TfToken const& nodeTypeId)
{
    // 1. info::id was set in usda (token info:id = "UsdPreviewSurface").
    //
    // We have an info:id so we can use Sdr to get to the source code path
    // for glslfx. GetShaderNodeByIdentifierAndType() will insert a SdrNode
    // and we can use GetSourceURI to query the source code path.
    auto &shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifierAndType(
        nodeTypeId, HioGlslfxTokens->glslfx);

    if (sdrNode) {
        std::string const& glslfxPath = sdrNode->GetSourceURI();
        glslfxOut.reset(new HioGlslfx(glslfxPath));
        return;
    }

    // 2. info::sourceAsset (asset info:glslfx:sourceAsset = @custm.glslfx@)
    // We did not have info::id so we expect the terminal type id token to
    // have been resolved into the path or source code for the glslfx.
    // E.g. UsdImagingMaterialAdapter handles this for us.
    if (TF_VERIFY(!nodeTypeId.IsEmpty())) {
        // Most likely: the identifier is a path to a glslfx file
        glslfxOut.reset(new HioGlslfx(nodeTypeId));
        if (!glslfxOut->IsValid()) {
            // Less likely: the identifier is a glslfx code snippet
            std::istringstream sourceCodeStream(nodeTypeId);
            glslfxOut.reset(new HioGlslfx(sourceCodeStream));
        }
    }
}

static HdStMaterialNode const*
_GetTerminalNode(
    SdfPath const& id,
    HdStMaterialNetwork const& network)
{
    if (network.terminals.size() != 1) {
        if (network.terminals.size() > 1) {
            TF_WARN("Unsupported number of terminals [%d] in material [%s]", 
                    (int)network.terminals.size(), id.GetText());
        }
        return nullptr;
    }

    auto const& terminalConnIt = network.terminals.begin();
    HdStMaterialConnection const& connection = terminalConnIt->second;
    SdfPath const& terminalPath = connection.upstreamNode;
    auto const& terminalIt = network.nodes.find(terminalPath);
    return &terminalIt->second;
}

static SdfPath const&
_GetFirstConnectionToParameter(HdStMaterialNode const& node)
{
    if (!node.inputConnections.empty()) {
        HdStMaterialConnection const& connection = 
            node.inputConnections.begin()->second.front();
        return connection.upstreamNode;
    }

    static SdfPath _emptyPath;
    return _emptyPath;
}

// XXX This function is missing a lot of details that HydraMaterialAdapeter had.
// For example it needs to determine primvars connected to texture nodes, volume
// fields etc. Currently it can only handle a single terminal node in network.
// This instead needs to walk the network to gather the final result of each
// parameter on the terminal.
static HdMaterialParamVector 
_GatherMaterialParams(
    HdStMaterialNode const& node,
    HioGlslfxUniquePtr const& glslfx) 
{
    HdMaterialParamVector params;

    auto &shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrNode = shaderReg.GetShaderNodeByIdentifierAndType(
        node.nodeTypeId, HioGlslfxTokens->glslfx);

    // XXX We won't have a valid sdrNode for shaders using custom glslfx.
    // This is because we do not have a Sdr glslfx parser (yet).

    if (sdrNode) {
        NdrTokenVec const& inputNames = sdrNode->GetInputNames();
        for (TfToken const& inputName : inputNames) {
            VtValue fallbackValue;

            // Find the value of the input. This 'fallback value' will be the 
            // value of the material param if nothing is connected.

            auto const& it = node.parameters.find(inputName);
            if (it != node.parameters.end()) {
                fallbackValue = it->second;
            } else {
                SdrShaderPropertyConstPtr const& sdrInput = 
                    sdrNode->GetShaderInput(inputName);

                if (sdrInput) {
                    fallbackValue = sdrInput->GetDefaultValue();
                } else {
                    TF_WARN("%s not found in Sdr", inputName.GetText());
                }
            }

            // Check if we have something connected to the input.
            // This is a 'stronger opinion' than the fallback value.
            SdfPath const& inputConn = 
                _GetFirstConnectionToParameter(node);

            // Add a material parameter for this material input.
            HdMaterialParam matParam(
                    HdMaterialParam::ParamTypeFallback,/*paramType*/
                    inputName,/*name*/
                    fallbackValue,/*fallbackValue*/
                    inputConn,/*_connection*/
                    TfTokenVector(), /*_samplerCoords*/
                    HdTextureType::Uv /*_textureType*/);
            params.emplace_back(std::move(matParam));
        }
    } else if (glslfx) {

        // XXX Since we do not have a Sdr glslfx parser we directly consult
        // glslfx for the parameters of the shader (custom glslfx case).
        for (HioGlslfxConfig::Parameter const& input: glslfx->GetParameters()) {

            // Extract the fallback value for this glslfx param-input.
            // This value is used if nothing is connected to the param.

            VtValue fallbackValue = input.defaultValue;
            TfToken inputName = TfToken(input.name);

            // Check if we have something connected to the input.
            // This is a 'stronger opinion' than the fallback value.
            SdfPath const& inputConn = 
                _GetFirstConnectionToParameter(node);

            // Add a material parameter for this material input.
            HdMaterialParam matParam(
                    HdMaterialParam::ParamTypeFallback,/*paramType*/
                    inputName, /*name*/
                    fallbackValue,/*fallbackValue*/
                    inputConn,/*_connection*/
                    TfTokenVector(), /*_samplerCoords*/
                    HdTextureType::Uv /*_textureType*/);
            params.emplace_back(std::move(matParam));
        }
    } else {
        TF_WARN("Unknown material configuration");
    }

    return params;
}

HdStMaterial::HdStMaterial(SdfPath const &id)
 : HdMaterial(id)
 , _surfaceShader(new HdStSurfaceShader)
 , _isInitialized(false)
 , _hasPtex(false)
 , _hasLimitSurfaceEvaluation(false)
 , _hasDisplacement(false)
 , _materialTag(HdStMaterialTagTokens->defaultMaterialTag)
{
    TF_DEBUG(HDST_MATERIAL_ADDED).Msg("HdStMaterial Created: %s\n",
                                      id.GetText());
}

HdStMaterial::~HdStMaterial()
{
    TF_DEBUG(HDST_MATERIAL_REMOVED).Msg("HdStMaterial Removed: %s\n",
                                        GetId().GetText());
}

/* virtual */
void
HdStMaterial::Sync(HdSceneDelegate *sceneDelegate,
                 HdRenderParam   *renderParam,
                 HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    HdResourceRegistrySharedPtr const &resourceRegistry = 
        sceneDelegate->GetRenderIndex().GetResourceRegistry();
    HdDirtyBits bits = *dirtyBits;

    bool markBatchesDirty = bits & DirtySurfaceShader;
    bool needsRprimMaterialStateUpdate = false;

    std::string fragmentSource;
    std::string geometrySource;
    VtDictionary materialMetadata;
    TfToken materialTag;
    HdMaterialParamVector params;

    if ((bits & DirtyResource) && _IsEnabledStormMaterialNetworks()) {
        //
        // Consume material network
        //
        HdMaterialNetworkMap const& hdNetworkMap = 
            _GetMaterialResource(sceneDelegate);

        HdStMaterialNetwork surfaceNetwork;
        HdStMaterialNetwork displacementNetwork;

        // The fragment source comes from the 'surface' network or the
        // 'volume' network.
        _ConvertLegacyHdMaterialNetwork(
            hdNetworkMap,
            HdMaterialTerminalTokens->surface,
            &surfaceNetwork);

        bool isVolume = surfaceNetwork.terminals.empty();
        if (!isVolume) {
            _ConvertLegacyHdMaterialNetwork(
                hdNetworkMap,
                HdMaterialTerminalTokens->volume,
                &surfaceNetwork);
        }

        // Geometry source can be provided via a 'displacement' network.
        _ConvertLegacyHdMaterialNetwork(
            hdNetworkMap,
            HdMaterialTerminalTokens->displacement,
            &displacementNetwork);

        if (HdStMaterialNode const* surfTerminal = 
                _GetTerminalNode(GetId(), surfaceNetwork)) 
        {
            // Extract the glslfx and metadata for surface/volume.
            HioGlslfxUniquePtr surfaceGfx;
            _GetGlslfxForTerminal(surfaceGfx, surfTerminal->nodeTypeId);
            if (surfaceGfx && surfaceGfx->IsValid()) {
                fragmentSource = isVolume ? surfaceGfx->GetVolumeSource() : 
                    surfaceGfx->GetSurfaceSource();
                materialMetadata = surfaceGfx->GetMetadata();
                materialTag = _GetMaterialTag(materialMetadata, *surfTerminal);
                params = _GatherMaterialParams(*surfTerminal, surfaceGfx);
            }
        }

        if (HdStMaterialNode const* dispTerminal = 
                _GetTerminalNode(GetId(), displacementNetwork)) 
        {
            // Extract the glslfx for displacement.
            HioGlslfxUniquePtr displacementGfx;
            _GetGlslfxForTerminal(displacementGfx, dispTerminal->nodeTypeId);
            if (displacementGfx && displacementGfx->IsValid()) {
                geometrySource = displacementGfx->GetDisplacementSource();
            }
        }
    } else {
        //
        // XXX Consume deprecated material
        //
        if (bits & DirtySurfaceShader) {
            fragmentSource = GetSurfaceShaderSource(sceneDelegate);
            geometrySource = GetDisplacementShaderSource(sceneDelegate);
            materialMetadata = GetMaterialMetadata(sceneDelegate);
            materialTag = _GetMaterialTagDeprecated(materialMetadata);
        }
        if (bits & DirtyParams) {
            params = GetMaterialParams(sceneDelegate);
        }
    }

    // If a prim changes material tag (e.g. goes from opaque to translucent) we
    // need to rebatch to sort translucent prims into a different collection.
    markBatchesDirty |= _materialTag != materialTag;

    //
    // Propagate shader changes
    //
    bool shaderIsDirty= ((bits & DirtyResource) || (bits & DirtySurfaceShader));

    if (shaderIsDirty) {
        if (fragmentSource.empty() && geometrySource.empty()) {
            _InitFallbackShader();
            _surfaceShader->SetFragmentSource(
                                   _fallbackSurfaceShader->GetFragmentSource());
            _surfaceShader->SetGeometrySource(
                                   _fallbackSurfaceShader->GetGeometrySource());

            materialMetadata = _fallbackSurfaceShader->GetMetadata();
        } else {

            if (!markBatchesDirty) {
                // XXX instead of doing two (large) string compares to determine
                // if we need to re-batch, we could instead compare old and new
                // network toplogy (without comparing node.parameters!)
                std::string const& oldFragmentSource = 
                    _surfaceShader->GetSource(HdShaderTokens->fragmentShader);
                std::string const& oldGeometrySource = 
                    _surfaceShader->GetSource(HdShaderTokens->geometryShader);
                
                markBatchesDirty |= (oldFragmentSource!=fragmentSource) || 
                                    (oldGeometrySource!=geometrySource);
            }

            _surfaceShader->SetFragmentSource(fragmentSource);
            _surfaceShader->SetGeometrySource(geometrySource);
        }

        bool hasDisplacement = !(geometrySource.empty());

        if (_hasDisplacement != hasDisplacement) {
            _hasDisplacement = hasDisplacement;
            needsRprimMaterialStateUpdate = true;
        }

        bool hasLimitSurfaceEvaluation =
            _GetHasLimitSurfaceEvaluation(materialMetadata);

        if (_hasLimitSurfaceEvaluation != hasLimitSurfaceEvaluation) {
            _hasLimitSurfaceEvaluation = hasLimitSurfaceEvaluation;
            needsRprimMaterialStateUpdate = true;
        }

        if (_materialTag != materialTag) {
            _materialTag = materialTag;
            _surfaceShader->SetMaterialTag(_materialTag);
            needsRprimMaterialStateUpdate = true;
        }
    }

    // Mark batches dirty to force batch validation/rebuild.
    if (markBatchesDirty) {
        sceneDelegate->GetRenderIndex().GetChangeTracker().
            MarkBatchesDirty();
    }

    //
    // Update material parameters
    //
    bool paramsAreDirty = (bits & DirtyResource || bits & DirtyParams);
    if (paramsAreDirty) {
        _surfaceShader->SetParams(params);

        // Release any fallback texture resources
        _fallbackTextureResourceHandles.clear();

        HdSt_MaterialBufferSourceAndTextureHelper sourcesAndTextures;

        bool hasPtex = false;
        for (HdMaterialParam const & param: params) {
            if (param.IsPrimvar()) {
                sourcesAndTextures.ProcessPrimvarMaterialParam(
                    param);
            } else if (param.IsFallback()) {
                // XXX Deprecate the use of sceneDelegate here.
                // We can use Sdr or glslfx to get the fallback value once we
                // switch over to only consume material networks.
                sourcesAndTextures.ProcessFallbackMaterialParam(
                    param, sceneDelegate, GetId());
            } else if (param.IsTexture()) {
                sourcesAndTextures.ProcessTextureMaterialParam(
                    param, 
                    _GetTextureResourceHandle(sceneDelegate, param),
                    &hasPtex);
            }
        }

        _surfaceShader->SetTextureDescriptors(
            sourcesAndTextures.textures);
        _surfaceShader->SetBufferSources(
            sourcesAndTextures.sources, resourceRegistry);

        if (_hasPtex != hasPtex) {
            _hasPtex = hasPtex;
            needsRprimMaterialStateUpdate = true;
        }
    }

    if (needsRprimMaterialStateUpdate && _isInitialized) {
        // XXX Forcing rprims to have a dirty material id to re-evaluate
        // their material state as we don't know which rprims are bound to
        // this one. We can skip this invalidation the first time this
        // material is Sync'd since any affected Rprim should already be
        // marked with a dirty material id.
        HdChangeTracker& changeTracker =
                         sceneDelegate->GetRenderIndex().GetChangeTracker();
        changeTracker.MarkAllRprimsDirty(HdChangeTracker::DirtyMaterialId);
    }

    _isInitialized = true;
    *dirtyBits = Clean;
}

HdStTextureResourceHandleSharedPtr
HdStMaterial::_GetTextureResourceHandle(
        HdSceneDelegate *sceneDelegate,
        HdMaterialParam const &param)
{
    HdStResourceRegistrySharedPtr const& resourceRegistry =
        boost::static_pointer_cast<HdStResourceRegistry>(
            sceneDelegate->GetRenderIndex().GetResourceRegistry());

    HdStTextureResourceSharedPtr texResource;
    HdStTextureResourceHandleSharedPtr handle;

    SdfPath const &connection = param.connection;
    if (!connection.IsEmpty()) {
        HdTextureResource::ID texID =
            GetTextureResourceID(sceneDelegate, connection);

        if (texID != HdTextureResource::ID(-1)) {
            // Use render index to convert local texture id into global
            // texture key
            HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
            HdResourceRegistry::TextureKey texKey =
                                               renderIndex.GetTextureKey(texID);

            HdInstance<HdResourceRegistry::TextureKey,
                        HdTextureResourceSharedPtr> texInstance;

            bool textureResourceFound = false;
            std::unique_lock<std::mutex> regLock =
                resourceRegistry->FindTextureResource
                                  (texKey, &texInstance, &textureResourceFound);

            // A bad asset can cause the texture resource to not
            // be found. Hence, issue a warning and continue onto the
            // next param.
            if (!textureResourceFound) {
                TF_WARN("No texture resource found with path %s",
                    param.connection.GetText());
            } else {
                texResource =
                    boost::dynamic_pointer_cast<HdStTextureResource>
                    (texInstance.GetValue());
            }
        }

        HdResourceRegistry::TextureKey handleKey =
            HdStTextureResourceHandle::GetHandleKey(
                &sceneDelegate->GetRenderIndex(), connection);

        HdInstance<HdResourceRegistry::TextureKey,
                    HdStTextureResourceHandleSharedPtr> handleInstance;

        bool handleFound = false;
        std::unique_lock<std::mutex> regLock =
            resourceRegistry->FindTextureResourceHandle
                              (handleKey, &handleInstance, &handleFound);

        // A bad asset can cause the texture resource to not
        // be found. Hence, issue a warning and continue onto the
        // next param.
        if (!handleFound) {
            TF_WARN("No texture resource handle found with path %s",
                param.connection.GetText());
        } else {
            handle = handleInstance.GetValue();
            handle->SetTextureResource(texResource);
        }
    }

    // There are many reasons why texResource could be null here:
    // - A missing or invalid connection path,
    // - A deliberate (-1) or accidental invalid texture id
    // - Scene delegate failed to return a texture resource (due to asset error)
    //
    // In all these cases fallback to a simple texture with the provided
    // fallback value
    //
    // XXX todo handle fallback Ptex textures
    if (!(handle && handle->GetTextureResource())) {
        // Fallback texture are only supported for UV textures.
        if (param.textureType != HdTextureType::Uv) {
            return {};
        }
        GlfUVTextureStorageRefPtr texPtr =
            GlfUVTextureStorage::New(1,1, param.fallbackValue);
        GlfTextureHandleRefPtr texture =
            GlfTextureRegistry::GetInstance().GetTextureHandle(texPtr);
        HdStTextureResourceSharedPtr texResource(
            new HdStSimpleTextureResource(texture,
                                          HdTextureType::Uv,
                                          HdWrapClamp,
                                          HdWrapClamp,
                                          HdWrapClamp,
                                          HdMinFilterNearest,
                                          HdMagFilterNearest,
                                          0));
        handle.reset(new HdStTextureResourceHandle(texResource));
        _fallbackTextureResourceHandles.push_back(handle);
    }

    return handle;
}

bool
HdStMaterial::_GetHasLimitSurfaceEvaluation(VtDictionary const & metadata) const
{
    VtValue value = TfMapLookupByValue(metadata,
                                       _tokens->limitSurfaceEvaluation,
                                       VtValue());
    return value.IsHolding<bool>() && value.Get<bool>();
}

// XXX Deprecated. This is used for old material descriptions where
// HydraMaterialAdapter calculates the materialTag and we extract it here from
// the metadata. The new '_GetMaterialTag' function is at top of file.
// Once we exclusively use HdMaterialNetwork for storm we can remove this.
TfToken
HdStMaterial::_GetMaterialTagDeprecated(VtDictionary const & metadata) const
{
    VtValue value = TfMapLookupByValue(metadata,
                                       HdShaderTokens->materialTag,
                                       VtValue());

    // A string when the materialTag is hardcoded in the glslfx.
    // A token if the materialTag is auto-determined in MaterialAdapter.
    if (value.IsHolding<TfToken>()) {
        return value.UncheckedGet<TfToken>();
    } else if (value.IsHolding<std::string>()) {
        return TfToken(value.UncheckedGet<std::string>());
    }

    // An empty materialTag on the HdRprimCollection level means: 'ignore all
    // materialTags and add everything to the collection'. Instead we return a
    // default token because we do want materialTags to drive HdSt collections.
    return HdStMaterialTagTokens->defaultMaterialTag;
}

// virtual
HdDirtyBits
HdStMaterial::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}


//virtual
void
HdStMaterial::Reload()
{
    _surfaceShader->Reload();
}

HdStShaderCodeSharedPtr
HdStMaterial::GetShaderCode() const
{
    return boost::static_pointer_cast<HdStShaderCode>(_surfaceShader);
}

void
HdStMaterial::SetSurfaceShader(HdStSurfaceShaderSharedPtr &shaderCode)
{
    _surfaceShader = shaderCode;
}

void
HdStMaterial::_InitFallbackShader()
{
    if (_fallbackSurfaceShader != nullptr) {
        return;
    }

    const TfToken &filePath = HdStPackageFallbackSurfaceShader();

    _fallbackSurfaceShader = new HioGlslfx(filePath);

    // Check fallback shader loaded, if not continue with the invalid shader
    // this would mean the shader compilation fails and the prim would not
    // be drawn.
    TF_VERIFY(_fallbackSurfaceShader->IsValid(),
              "Failed to load fallback surface shader!");
}

HdMaterialNetworkMap const&
HdStMaterial::_GetMaterialResource(HdSceneDelegate* sceneDelegate) const
{
    VtValue vtMat = sceneDelegate->GetMaterialResource(GetId());
    if (vtMat.IsHolding<HdMaterialNetworkMap>()) {
        return vtMat.UncheckedGet<HdMaterialNetworkMap>();
    } else {
        TF_CODING_ERROR("Not a valid material network map");
        static const HdMaterialNetworkMap emptyNetworkMap;
        return emptyNetworkMap;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
