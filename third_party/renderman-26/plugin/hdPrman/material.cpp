//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/material.h"
#include "hdPrman/debugCodes.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/utils.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/usd/sdf/types.h"
#if PXR_VERSION >= 2311
#include "pxr/base/tf/hash.h"
#endif
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hf/diagnostic.h"
#include "RiTypesHelper.h"

#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"
#if PXR_VERSION <= 2308
#include <boost/functional/hash.hpp>
#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HD_PRMAN_MATERIALID, true,
                      "Enable __materialid as hash of material network");
static bool _enableMaterialID =
    TfGetEnvSetting(HD_PRMAN_MATERIALID);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (PxrDisplace)
    (bxdf)
    (OSL)
    (omitFromRender)
    (material)
    (surface)
    ((materialid, "__materialid"))
    (light)
    (PrimvarPass)
    (PxrBakeTexture)
);

TF_DEFINE_ENV_SETTING(PRMAN_OSL_BEFORE_RIXPLUGINS, 1,
                      "Change priority of Rix plugins over osl");
TF_DEFINE_ENV_SETTING(HD_PRMAN_TEX_EXTS, "tex:dds",
                      "Colon separated list of all texture extensions"
                      "that do not require txmake processing."
                      "eg. tex:dds:tx");

TF_MAKE_STATIC_DATA(NdrTokenVec, _sourceTypesOslFirst) {
    *_sourceTypesOslFirst = {
        TfToken("OSL"),
        TfToken("RmanCpp"),
#ifdef PXR_MATERIALX_SUPPORT_ENABLED
        TfToken("mtlx")
#endif
    };}

TF_MAKE_STATIC_DATA(NdrTokenVec, _sourceTypesCppFirst) {
    *_sourceTypesCppFirst = {
        TfToken("RmanCpp"),
        TfToken("OSL"),
#ifdef PXR_MATERIALX_SUPPORT_ENABLED
        TfToken("mtlx")
#endif
    };}

struct _HashMaterial {
    size_t operator()(const HdMaterialNetwork2 &mat) const
    {
#if PXR_VERSION >= 2311
        size_t v = TfHash()(mat.primvars);
        for (auto const& node: mat.nodes) {
            v = TfHash::Combine(v, 
                node.first, node.second.nodeTypeId, node.second.parameters);
            for (auto const& input: node.second.inputConnections) {
                v = TfHash::Combine(v, input.first);
                for (auto const& conn: input.second) {
                    v = TfHash::Combine(
                        v, conn.upstreamNode, conn.upstreamOutputName);
                }
            }
        }
        for (auto const& term: mat.terminals) {
            v = TfHash::Combine(v, 
                term.first, 
                term.second.upstreamNode, term.second.upstreamOutputName);
        }
        return v;
#else
        size_t v=0;
        for (TfToken const& primvarName: mat.primvars) {
            boost::hash_combine(v, primvarName.Hash());
        }
        for (auto const& node: mat.nodes) {
            boost::hash_combine(v, node.first.GetHash());
            boost::hash_combine(v, node.second.nodeTypeId.Hash());
            for (auto const& param: node.second.parameters) {
                boost::hash_combine(v, param.first.Hash());
                boost::hash_combine(v, param.second.GetHash());
            }
            for (auto const& input: node.second.inputConnections) {
                boost::hash_combine(v, input.first.Hash());
                for (auto const& conn: input.second) {
                    boost::hash_combine(v, conn.upstreamNode.GetHash());
                    boost::hash_combine(v, conn.upstreamOutputName.Hash());
                }
            }
        }
        for (auto const& term: mat.terminals) {
            boost::hash_combine(v, term.first.Hash());
            boost::hash_combine(v, term.second.upstreamNode.GetHash());
            boost::hash_combine(v, term.second.upstreamOutputName.Hash());
        }
        return v;
#endif
    }
};

TF_MAKE_STATIC_DATA(NdrTokenVec, _texExts) {
    *_texExts = TfToTokenVector(TfStringSplit(
        TfGetEnvSetting(HD_PRMAN_TEX_EXTS), ":"));
    }


static TfTokenVector const&
_GetShaderSourceTypes()
{
    if(TfGetEnvSetting(PRMAN_OSL_BEFORE_RIXPLUGINS)) {
        return *_sourceTypesOslFirst;
    } else {
        return *_sourceTypesCppFirst;
    }
}

bool
HdPrmanMaterial::IsTexExt(const std::string& ext)
{
    for(auto e : *_texExts) {
        if(ext == e) {
            return true;
        }
    }
    return false;
}

TfTokenVector const&
HdPrmanMaterial::GetShaderSourceTypes()
{
    return _GetShaderSourceTypes();
}

HdMaterialNetwork2 const&
HdPrmanMaterial::GetMaterialNetwork() const
{
    // XXX We could make this API entry point do the sync as needed,
    // if we passed in the necessary context.  However, we should
    // remove this and the retained _materialNetwork entirely,
    // since it is solely used to allow UsdPreviewSurface materials
    // to supply a PrimvarPass shader that in turn sets a disp bound.
    // Now that scene indexes are handling UsdPreviewSurface
    // conversion and material primvar attribute transfer, we should
    // not need this whole affordance for that case.  In the
    // meantime, leave this here to guard against mis-usage.
    std::lock_guard<std::mutex> lock(_syncToRileyMutex);
    TF_VERIFY(_rileyIsInSync, "Must call SyncToRiley() first");

    return _materialNetwork;
}

HdPrmanMaterial::HdPrmanMaterial(SdfPath const& id)
    : HdMaterial(id)
    , _materialId(riley::MaterialId::InvalidId())
    , _displacementId(riley::DisplacementId::InvalidId())
    , _rileyIsInSync(false)
{
    /* NOTHING */
}

HdPrmanMaterial::~HdPrmanMaterial()
{
}

void
HdPrmanMaterial::Finalize(HdRenderParam *renderParam)
{
    HdPrman_RenderParam *param =
        static_cast<HdPrman_RenderParam*>(renderParam);
    riley::Riley *riley = param->AcquireRiley();

    std::lock_guard<std::mutex> lock(_syncToRileyMutex);
    _ResetMaterialWithLock(riley);
}

void
HdPrmanMaterial::_ResetMaterialWithLock(riley::Riley *riley)
{
    if(!riley) {
        return;
    }
    if (_materialId != riley::MaterialId::InvalidId()) {
        riley->DeleteMaterial(_materialId);
        _materialId = riley::MaterialId::InvalidId();
    }
    if (_displacementId != riley::DisplacementId::InvalidId()) {
        riley->DeleteDisplacement(_displacementId);
        _displacementId = riley::DisplacementId::InvalidId();
    }
}

static VtArray<GfVec3f>
_ConvertToVec3fArray(const VtArray<GfVec3d>& v)
{
    VtArray<GfVec3f> out;
    out.resize(v.size());
    for (size_t i=0; i<v.size(); ++i) {
        for (uint8_t e=0; e<3; ++e) {
            out[i][e] = v[i][e];
        }
    }
    return out;
}

static int
_ConvertOptionTokenToInt(
    const TfToken &option, const NdrOptionVec &options, bool *ok)
{
    for (const auto &tokenPair : options) {
        if (tokenPair.first == option) {
            *ok = true;
            return TfUnstringify<int>(tokenPair.second, ok);
        }
    }
    return 0;
}

using _PathSet = std::unordered_set<SdfPath, SdfPath::Hash>;

// See also TfGetenvBool().
static bool
_GetStringAsBool(std::string value, bool defaultValue)
{
    if (value.empty()) {
        return defaultValue;
    } else {
        for (char& c: value) {
            c = tolower(c);
        }
        return value == "true" ||
            value == "yes"  ||
            value == "on"   ||
            value == "1";
    }
}

static bool
_IsWriteAsset(const TfToken& nodeName, const RtUString& paramName)
{
    // At the moment the only shading node / parameter we want to avoid adding
    // "RtxHioImage" to is the bake texture filename
    static const RtUString us_filename("filename");
    if (nodeName == _tokens->PxrBakeTexture && paramName == us_filename)
        return true;
    return false;
}

// Recursively convert a HdMaterialNode2 and its upstream dependencies
// to Riley equivalents.  Avoids adding redundant nodes in the case
// of multi-path dependencies.
static bool
_ConvertNodes(
    HdMaterialNetwork2 const& network,
    SdfPath const& nodePath,
    std::vector<riley::ShadingNode> *result,
    _PathSet* visitedNodes,
    bool elideDefaults)
{
    // Check if we've processed this node before. If we have, we'll just return.
    // This is not an error, since we often have multiple connection paths
    // leading to the same upstream node.
    if (visitedNodes->count(nodePath) > 0) {
        return true;
    }
    visitedNodes->insert(nodePath);

    // Find HdMaterialNetwork2 node.
    auto iter = network.nodes.find(nodePath);
    if (iter == network.nodes.end()) {
        // This could be caused by a bad connection to a non-existent node.
        TF_WARN("Unknown material node '%s'", nodePath.GetText());
        return false;
    }
    HdMaterialNode2 const& node = iter->second;
    // Riley expects nodes to be provided in topological dependency order.
    // Pre-traverse upstream nodes.
    for (auto const& connEntry: node.inputConnections) {
        for (auto const& e: connEntry.second) {
            // This method will just return if we've visited this upstream node
            // before
            _ConvertNodes(network, e.upstreamNode, result, visitedNodes,
                          elideDefaults);
        }
    }

    // Ignore nodes of id "PrimvarPass". This node is a workaround for 
    // UsdPreviewSurface materials and is not a registered shader node.
    if (node.nodeTypeId == _tokens->PrimvarPass) {
        return true;
    }

    // Ignore nodes of id "PxrDisplace" that lack both parameters
    // and connections.  This can save render startup time by avoiding
    // creating unnecessary Riley displacement networks.
    if ((node.nodeTypeId == _tokens->PxrDisplace)
        && node.parameters.empty()
        && node.inputConnections.empty()) {
        return true;
    }

    // Find shader registry entry.
    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrEntry =
            sdrRegistry.GetShaderNodeByIdentifier(node.nodeTypeId,
                                                  _GetShaderSourceTypes());
    if (!sdrEntry) {
        TF_WARN("Unknown shader ID %s for node <%s>\n",
                node.nodeTypeId.GetText(), nodePath.GetText());
        return false;
    }
    // Create equivalent Riley shading node.
    riley::ShadingNode sn;
    if (sdrEntry->GetContext() == _tokens->bxdf ||
        sdrEntry->GetContext() == SdrNodeContext->Surface ||
        sdrEntry->GetContext() == SdrNodeContext->Volume) {
        sn.type = riley::ShadingNode::Type::k_Bxdf;
    }
    else if (sdrEntry->GetContext() == SdrNodeContext->Pattern ||
               sdrEntry->GetContext() == _tokens->OSL)
    {
        // In RMAN 24 all patterns are OSL shaders, that is, all patterns we have in Renderman
        // are going to be flagged as k_Pattern for Riley. In the case of displacement Riley
        // expects it to be flagged as k_Displacement and to be the last node of a network to
        // create a specific displacement. So, we need to check if the OSL node that we receive
        // is PxrDisplace to flag it as a displacement node instead of a general OSL node.
        // If we don't do that, Riley will check that there is no displacement node in the network
        // we are using and it will always return an invalid displacement handle to hdPrman.
        if (node.nodeTypeId == _tokens->PxrDisplace)
            sn.type = riley::ShadingNode::Type::k_Displacement;
        else
            sn.type = riley::ShadingNode::Type::k_Pattern;        
    }
    else if (sdrEntry->GetContext() == SdrNodeContext->Displacement)
    {
        // We need to keep this for backwards compatibility with C++ patterns in case we
        // use a version prior to RMAN 24.
        sn.type = riley::ShadingNode::Type::k_Displacement;
    } else if (sdrEntry->GetContext() == SdrNodeContext->Light) {
        sn.type = riley::ShadingNode::Type::k_Light;
    } else if (sdrEntry->GetContext() == SdrNodeContext->LightFilter) {
        sn.type = riley::ShadingNode::Type::k_LightFilter;
    } else {
        TF_WARN("Unknown shader entry type '%s' for shader '%s'",
                sdrEntry->GetContext().GetText(), sdrEntry->GetName().c_str());
        return false;
    }
    sn.handle = RtUString(nodePath.GetText());
    std::string shaderPath = sdrEntry->GetResolvedImplementationURI();
    if (shaderPath.empty()){
        TF_WARN("Shader '%s' did not provide a valid implementation path.",
                sdrEntry->GetName().c_str());
        return false;
    }
    if (sn.type == riley::ShadingNode::Type::k_Displacement ||
        sn.type == riley::ShadingNode::Type::k_Light || 
        sn.type == riley::ShadingNode::Type::k_LightFilter) {
        // Except for Displacement;
        // in that case let the renderer choose, since RIS
        // can only use a cpp Displacement shader and XPU
        // can only use osl.
        // Lights and light filters let the renderer choose by name too.
        shaderPath = sdrEntry->GetImplementationName();
    }

    sn.name = RtUString(shaderPath.c_str());
    // Convert params
    for (const auto& param: node.parameters) {
        const SdrShaderProperty* prop = sdrEntry->GetShaderInput(param.first);
        if (!prop) {
            TF_DEBUG(HDPRMAN_MATERIALS)
                .Msg("Unknown shader property '%s' for "
                     "shader '%s' at '%s'; ignoring.\n",
                     param.first.GetText(),
                     sdrEntry->GetName().c_str(),
                     nodePath.GetText());
            continue;
        }
        // Skip parameter values that match schema-defined defaults
        if (elideDefaults && param.second == prop->GetDefaultValue()) {
            continue;
        }
        // Filter by omitFromRender metadata to pre-empt warnings
        // from RenderMan.
        std::string omitFromRenderValStr;
        if (TfMapLookup(prop->GetMetadata(), _tokens->omitFromRender,
            &omitFromRenderValStr)) {
            if (_GetStringAsBool(omitFromRenderValStr, false)) {
                continue;
            }
        }
        TfToken propType = prop->GetType();
        if (propType.IsEmpty()) {
            // As a special case, silently ignore these on PxrDisplace.
            // Automatically promoting the same network for this
            // case causes a lot of errors.
            if (node.nodeTypeId == _tokens->PxrDisplace) {
                continue;
            }
            TF_DEBUG(HDPRMAN_MATERIALS)
                .Msg("Unknown shader entry field type for "
                     "field '%s' on shader '%s' at '%s'; ignoring.\n",
                     param.first.GetText(),
                     sdrEntry->GetName().c_str(),
                     nodePath.GetText());
            continue;
        }

        // Dispatch by propType and VtValue-held type.
        // Cast value types to match where feasible.
        bool ok = false;
        RtUString name(prop->GetImplementationName().c_str());
        if (propType == SdrPropertyTypes->Struct ||
            propType == SdrPropertyTypes->Vstruct) {
            // Ignore structs.  They are only used as ways to
            // pass data between shaders, not as a way to pass
            // in parameters.
            ok = true;
        } else if (param.second.IsHolding<GfVec2f>()) {
            GfVec2f v = param.second.UncheckedGet<GfVec2f>();
            if (propType == SdrPropertyTypes->Float) {
                sn.params.SetFloatArray(name, v.data(), 2);
                ok = true;
            } 
        } else if (param.second.IsHolding<GfVec3f>()) {
            GfVec3f v = param.second.UncheckedGet<GfVec3f>();
            if (propType == SdrPropertyTypes->Color) {
                sn.params.SetColor(name, RtColorRGB(v[0], v[1], v[2]));
                ok = true;
            } else if (propType == SdrPropertyTypes->Vector) {
                sn.params.SetVector(name, RtVector3(v[0], v[1], v[2]));
                ok = true;
            } else if (propType == SdrPropertyTypes->Point) {
                sn.params.SetPoint(name, RtPoint3(v[0], v[1], v[2]));
                ok = true;
            } else if (propType == SdrPropertyTypes->Normal) {
                sn.params.SetNormal(name, RtNormal3(v[0], v[1], v[2]));
                ok = true;
            } else if (propType == SdrPropertyTypes->Float) {
                sn.params.SetFloatArray(name, v.data(), 3);
                ok = true;
            }
        } else if (param.second.IsHolding<GfVec4f>()) {
            GfVec4f v = param.second.UncheckedGet<GfVec4f>();
            if (propType == SdrPropertyTypes->Float) {
                sn.params.SetFloatArray(name, v.data(), 4);
                ok = true;
            } 
        } else if (param.second.IsHolding<VtArray<GfVec3f>>()) {
            const VtArray<GfVec3f>& v =
                param.second.UncheckedGet<VtArray<GfVec3f>>();
            if (propType == SdrPropertyTypes->Color) {
                sn.params.SetColorArray(
                                      name,
                                      reinterpret_cast<const RtColorRGB*>(v.cdata()),
                                      v.size());
                ok = true;
            } else if (propType == SdrPropertyTypes->Vector) {
                sn.params.SetVectorArray(
                                       name,
                                       reinterpret_cast<const RtVector3*>(v.cdata()),
                                       v.size());
                ok = true;
            } else if (propType == SdrPropertyTypes->Point) {
                sn.params.SetPointArray(
                                      name,
                                      reinterpret_cast<const RtPoint3*>(v.cdata()),
                                      v.size());
                ok = true;
            } else if (propType == SdrPropertyTypes->Normal) {
                sn.params.SetNormalArray(
                                       name,
                                       reinterpret_cast<const RtNormal3*>(v.cdata()),
                                       v.size());
                ok = true;
            }
        } else if (param.second.IsHolding<GfVec3d>()) {
            const GfVec3d& v = param.second.UncheckedGet<GfVec3d>();
            if (propType == SdrPropertyTypes->Color) {
                sn.params.SetColor(name, RtColorRGB(v[0], v[1], v[2]));
                ok = true;
            } else if (propType == SdrPropertyTypes->Point) {
                sn.params.SetPoint(name, RtPoint3(v[0], v[1], v[2]));
                ok = true;
            }
        } else if (param.second.IsHolding<VtArray<GfVec3d>>()) {
            if (propType == SdrPropertyTypes->Color) {
                const VtArray<GfVec3d>& vd =
                    param.second.UncheckedGet<VtArray<GfVec3d>>();
                VtArray<GfVec3f> v = _ConvertToVec3fArray(vd);
                sn.params.SetColorArray(
                                      name,
                                      reinterpret_cast<const RtColorRGB*>(v.cdata()),
                                      v.size());
                ok = true;
            } else if (propType == SdrPropertyTypes->Point) {
                const VtArray<GfVec3d>& vd =
                    param.second.UncheckedGet<VtArray<GfVec3d>>();
                VtArray<GfVec3f> v = _ConvertToVec3fArray(vd);
                sn.params.SetPointArray(
                    name,
                    reinterpret_cast<const RtPoint3*>(v.cdata()),
                    v.size());
                ok = true;
            }
        } else if (param.second.IsHolding<float>()) {
            float v = param.second.UncheckedGet<float>();
            if (propType == SdrPropertyTypes->Int) {
                sn.params.SetInteger(name, int(v));
                ok = true;
            } else if (propType == SdrPropertyTypes->Float) {
                sn.params.SetFloat(name, v);
                ok = true;
            }
        } else if (param.second.IsHolding<VtArray<float>>()) {
            const VtArray<float>& v =
                param.second.UncheckedGet<VtArray<float>>();
            if (propType == SdrPropertyTypes->Float) {
                sn.params.SetFloatArray(name, v.cdata(), v.size());
                ok = true;
            }
        } else if (param.second.IsHolding<int>()) {
            int v = param.second.UncheckedGet<int>();
            if (propType == SdrPropertyTypes->Float) {
                sn.params.SetFloat(name, v);
                ok = true;
            } else if (propType == SdrPropertyTypes->Int) {
                sn.params.SetInteger(name, v);
                ok = true;
            }
        } else if (param.second.IsHolding<VtArray<int>>()) {
            if (propType == SdrPropertyTypes->Float) {
                const VtArray<float>& v =
                    param.second.UncheckedGet<VtArray<float>>();
                sn.params.SetFloatArray(name, v.cdata(), v.size());
                ok = true;
            } else if (propType == SdrPropertyTypes->Int) {
                const VtArray<int>& v =
                    param.second.UncheckedGet<VtArray<int>>();
                sn.params.SetIntegerArray(name, v.cdata(), v.size());
                ok = true;
            }
        } else if (param.second.IsHolding<TfToken>()) {
            TfToken v = param.second.UncheckedGet<TfToken>();
            // A token can represent and enum option for an Int property
            if (propType == SdrPropertyTypes->Int) {
                const int value = _ConvertOptionTokenToInt(
                    v, prop->GetOptions(), &ok);
                if (ok) {
                    sn.params.SetInteger(name, value);
                }
            } else {
                sn.params.SetString(name, RtUString(v.GetText()));
                ok = true;
            }
        } else if (param.second.IsHolding<std::string>()) {
            static const RtUString us_filename("filename");
            std::string v = param.second.UncheckedGet<std::string>();
            // A string can represent and enum option for an Int property
            if (propType == SdrPropertyTypes->Int) {
                const int value = _ConvertOptionTokenToInt(
                    TfToken(v), prop->GetOptions(), &ok);
                if (ok) {
                    sn.params.SetInteger(name, value);
                }
            } else if(name == us_filename) {
                SdfAssetPath path(v);
                bool isLight = (sn.type == riley::ShadingNode::Type::k_Light &&
                                param.first == HdLightTokens->textureFile);

                RtUString ustr = HdPrman_Utils::ResolveAssetToRtUString(
                    path,
                    !isLight, // only flip if NOT a light
                    _IsWriteAsset(node.nodeTypeId, name),
                    isLight ? _tokens->light.GetText() : 
                    _tokens->material.GetText());
                if(!ustr.Empty()) {
                    sn.params.SetString(name, ustr);
                    ok = true;
                } else {
                    sn.params.SetString(name, RtUString(v.c_str()));
                }
                ok = true;
            } else {
                sn.params.SetString(name, RtUString(v.c_str()));
                ok = true;
            }
        } else if (param.second.IsHolding<SdfAssetPath>()) {
            // This code processes nodes for both surface materials
            // and lights.  RenderMan does not flip light textures
            // as it does surface textures.
            bool isLight = (sn.type == riley::ShadingNode::Type::k_Light &&
                            param.first == HdLightTokens->textureFile);

            RtUString v = HdPrman_Utils::ResolveAssetToRtUString(
                param.second.UncheckedGet<SdfAssetPath>(),
                !isLight, // only flip if NOT a light
                _IsWriteAsset(node.nodeTypeId, name),
                isLight ? _tokens->light.GetText() : 
                          _tokens->material.GetText());

            sn.params.SetString(name, v);
            ok = true;
        } else if (param.second.IsHolding<bool>()) {
            // RixParamList (specifically, RixDataType) doesn't have
            // a bool entry; we convert to integer instead.
            int v = param.second.UncheckedGet<bool>();
            sn.params.SetInteger(name, v);
            ok = true;
        } else if (param.second.IsHolding<GfMatrix4d>()) {
            if (propType == SdrPropertyTypes->Matrix) {
                const RtMatrix4x4 v = HdPrman_Utils::GfMatrixToRtMatrix(
                    param.second.UncheckedGet<GfMatrix4d>());
                sn.params.SetMatrix(name, v);
                ok = true;
            }
        }
        if (!ok) {
            TF_DEBUG(HDPRMAN_MATERIALS)
                .Msg("Unknown shading parameter type '%s'; skipping "
                     "parameter '%s' on node '%s'; "
                     "expected type '%s'\n",
                     param.second.GetTypeName().c_str(),
                     param.first.GetText(),
                     nodePath.GetText(),
                     propType.GetText());
        }
    }
    // Convert connected inputs.
    for (auto const& connEntry: node.inputConnections) {
        // Find the shader properties, so that we can look up
        // the property implementation names.
        SdrShaderPropertyConstPtr downstreamProp =
            sdrEntry->GetShaderInput(connEntry.first);
        if (!downstreamProp) {
            TF_WARN("Unknown downstream property %s", connEntry.first.data());
            continue;
        }
        RtUString name(downstreamProp->GetImplementationName().c_str());
        TfToken const propType = downstreamProp->GetType();

        // Gather input (or inputs, for array-valued inputs) for shader 
        // property.
        std::vector<RtUString> inputRefs;

        for (auto const& e: connEntry.second) {
            // Find the output & input shader nodes of the connection.
            HdMaterialNode2 const* upstreamNode =
                TfMapLookupPtr(network.nodes, e.upstreamNode);
            if (!upstreamNode) {
                TF_WARN("Unknown upstream node %s", e.upstreamNode.GetText());
                continue;
            }
            // Ignore nodes of id "PrimvarPass". This node is a workaround for 
            // UsdPreviewSurface materials and is not a registered shader node.
            if (upstreamNode->nodeTypeId == _tokens->PrimvarPass) {
                continue;
            }

            SdrShaderNodeConstPtr upstreamSdrEntry =
                sdrRegistry.GetShaderNodeByIdentifier(
                    upstreamNode->nodeTypeId, _GetShaderSourceTypes());
            if (!upstreamSdrEntry) {
                TF_WARN("Unknown shader for upstream node %s",
                        e.upstreamNode.GetText());
                continue;
            }
            SdrShaderPropertyConstPtr upstreamProp =
                upstreamSdrEntry->GetShaderOutput(e.upstreamOutputName);
            // In the case of terminals there is no upstream output name
            // since the whole node is referenced as a whole
            if (!upstreamProp && propType != SdrPropertyTypes->Terminal) {
                TF_WARN("Unknown upstream property %s",
                        e.upstreamOutputName.data());
                continue;
            }
            // Prman syntax for parameter references is "handle:param".
            RtUString inputRef;
            if (!upstreamProp) {
                inputRef = RtUString(e.upstreamNode.GetString().c_str());
            } else {
                inputRef = RtUString(
                    (e.upstreamNode.GetString()+":"
                    + upstreamProp->GetImplementationName().c_str())
                    .c_str());
            }
            inputRefs.push_back(inputRef);
        }
        
        // Establish the Riley connection.
        size_t const numInputRefs = inputRefs.size();
        if (numInputRefs > 0) {
            if (propType == SdrPropertyTypes->Color) {
                if (numInputRefs == 1) {
                    sn.params.SetColorReference(name, inputRefs[0]);
                } else {
                    sn.params.SetColorReferenceArray(
                        name, inputRefs.data(), numInputRefs);
                }
            } else if (propType == SdrPropertyTypes->Vector) {
                if (numInputRefs == 1) {
                    sn.params.SetVectorReference(name, inputRefs[0]);
                } else {
                    sn.params.SetVectorReferenceArray(
                        name, inputRefs.data(), numInputRefs);
                }
            } else if (propType == SdrPropertyTypes->Point) {
                if (numInputRefs == 1) {
                    sn.params.SetPointReference(name, inputRefs[0]);
                } else {
                    sn.params.SetPointReferenceArray(
                        name, inputRefs.data(), numInputRefs);
                }
            } else if (propType == SdrPropertyTypes->Normal) {
                if (numInputRefs == 1) {
                    sn.params.SetNormalReference(name, inputRefs[0]);
                } else {
                    sn.params.SetNormalReferenceArray(
                        name, inputRefs.data(), numInputRefs);
                }
            } else if (propType == SdrPropertyTypes->Float) {
                if (numInputRefs == 1) {
                    sn.params.SetFloatReference(name, inputRefs[0]);
                } else {
                    sn.params.SetFloatReferenceArray(
                        name, inputRefs.data(), numInputRefs);
                }
            } else if (propType == SdrPropertyTypes->Int) {
                if (numInputRefs == 1) {
                    sn.params.SetIntegerReference(name, inputRefs[0]);
                } else {
                    sn.params.SetIntegerReferenceArray(
                        name, inputRefs.data(), numInputRefs);
                }
            } else if (propType == SdrPropertyTypes->String) {
                if (numInputRefs == 1) {
                    sn.params.SetStringReference(name, inputRefs[0]);
                } else {
                    sn.params.SetStringReferenceArray(
                        name, inputRefs.data(), numInputRefs);
                }
            } else if (propType == SdrPropertyTypes->Struct) {
                if (numInputRefs == 1) {
                    sn.params.SetStructReference(name, inputRefs[0]);
                } else {
                     TF_WARN("Unsupported type struct array for property '%s' "
                        "on shader '%s' at '%s'; ignoring.",
                        connEntry.first.data(),
                        sdrEntry->GetName().c_str(),
                        nodePath.GetText());
                }
            } else if (propType == SdrPropertyTypes->Terminal) {
                if (numInputRefs == 1) {
                    sn.params.SetBxdfReference(name, inputRefs[0]);
                } else {
                    sn.params.SetBxdfReferenceArray(
                        name, inputRefs.data(), numInputRefs);
                }
            } else if (propType == SdrPropertyTypes->Matrix) {
                if (numInputRefs == 1) {
                    sn.params.SetMatrixReference(name, inputRefs[0]);
                } else {
                    sn.params.SetMatrixReferenceArray(
                        name, inputRefs.data(), numInputRefs);
                }
            } else {
                TF_WARN("Unknown type '%s' for property '%s' "
                        "on shader '%s' at %s; ignoring.",
                        propType.GetText(),
                        connEntry.first.data(),
                        sdrEntry->GetName().c_str(),
                        nodePath.GetText());
            }
        }
    }

    result->emplace_back(std::move(sn));

    return true;
}
    
bool
HdPrman_ConvertHdMaterialNetwork2ToRmanNodes(
    HdMaterialNetwork2 const& network,
    SdfPath const& nodePath,
    std::vector<riley::ShadingNode> *result)
{
    // If XPU_INTERACTIVE_SHADER_EDITS is true, do not elide defaults.
    // This makes it faster to edit parameter values later.
    // Look this env var up here since it can be changed in-app.
    bool elideDefaults = !TfGetenvBool("XPU_INTERACTIVE_SHADER_EDITS", false);

    _PathSet visitedNodes;
    return _ConvertNodes(
        network, nodePath, result, &visitedNodes, elideDefaults);
}
    
// Debug helper
void
HdPrman_DumpNetwork(HdMaterialNetwork2 const& network, SdfPath const& id)
{
    printf("material network for %s:\n", id.GetText());
    for (auto const& nodeEntry: network.nodes) {
        printf("  --Node--\n");
        printf("    path: %s\n", nodeEntry.first.GetText());
        printf("    type: %s\n", nodeEntry.second.nodeTypeId.GetText());
        for (auto const& paramEntry: nodeEntry.second.parameters) {
            printf("    param: %s = %s\n",
                   paramEntry.first.GetText(),
                   TfStringify(paramEntry.second).c_str());
        }
        for (auto const& connEntry: nodeEntry.second.inputConnections) {
            for (auto const& e: connEntry.second) {
                printf("    connection: %s <-> %s @ %s\n",
                       connEntry.first.GetText(),
                       e.upstreamOutputName.GetText(),
                       e.upstreamNode.GetText());
        }
    }
}
    printf("  --Terminals--\n");
    for (auto const& terminalEntry: network.terminals) {
        printf("    %s (downstream) <-> %s @ %s (upstream)\n",
               terminalEntry.first.GetText(),
               terminalEntry.second.upstreamOutputName.GetText(),
               terminalEntry.second.upstreamNode.GetText());
    }
}
    
// Convert given HdMaterialNetwork2 to Riley material and displacement
// shader networks. If the Riley network exists, it will be modified;
// otherwise it will be created as needed.
static void
_ConvertHdMaterialNetwork2ToRman(
    HdSceneDelegate *sceneDelegate,
    riley::Riley *riley,
    SdfPath const& id,
    const HdMaterialNetwork2 &network,
    riley::MaterialId *materialId,
    riley::DisplacementId *displacementId)
{
    HD_TRACE_FUNCTION();
    std::vector<riley::ShadingNode> nodes;
    nodes.reserve(network.nodes.size());
    bool materialFound = false, displacementFound = false;

    for (auto const& terminal: network.terminals) {
        if (HdPrman_ConvertHdMaterialNetwork2ToRmanNodes(
                network, terminal.second.upstreamNode, &nodes)) {
            if (nodes.empty()) {
                // Already emitted a specific warning.
                continue;
            }
            // Compute a hash of the material network, and pass it as
            // __materialid on the terminal shader node.  RenderMan uses
            // this detect and re-use material netowrks, which is valuable
            // in production scenes where upstream scene instancing did
            // not already catch the reuse.
            if (_enableMaterialID) {
                static RtUString const materialid = RtUString("__materialid");
                const size_t networkHash = _HashMaterial()(network);
                nodes.back().params.SetString(
                    materialid,
                    RtUString(TfStringify(networkHash).c_str()));
            }
            if (terminal.first == HdMaterialTerminalTokens->surface ||
                terminal.first == HdMaterialTerminalTokens->volume) {
                // Create or modify Riley material.
                materialFound = true;
                TRACE_SCOPE("_ConvertHdMaterialNetwork2ToRman - Update Riley Material");
                if (*materialId == riley::MaterialId::InvalidId()) {
                    TRACE_SCOPE("riley::CreateMaterial");
                    *materialId = riley->CreateMaterial(
                        riley::UserId(stats::AddDataLocation(id.GetText()).GetValue()),
                        {static_cast<uint32_t>(nodes.size()), &nodes[0]},
                        RtParamList());
                } else {
                    TRACE_SCOPE("riley::ModifyMaterial");
                    riley::ShadingNetwork const material = {
                        static_cast<uint32_t>(nodes.size()), &nodes[0]};
                    riley->ModifyMaterial(*materialId, &material, nullptr);
                }
                if (*materialId == riley::MaterialId::InvalidId()) {
                    TF_WARN("Failed to create material %s\n",
                                     id.GetText());
                }
            } else if (terminal.first ==
                       HdMaterialTerminalTokens->displacement) {
                // Create or modify Riley displacement.
                TRACE_SCOPE("_ConvertHdMaterialNetwork2ToRman - Update Riley Displacement");
                displacementFound = true;
                if (*displacementId == riley::DisplacementId::InvalidId()) {
                    TRACE_SCOPE("riley::CreateDisplacement");
                    *displacementId = riley->CreateDisplacement(
                        riley::UserId(stats::AddDataLocation(id.GetText()).GetValue()),
                        {static_cast<uint32_t>(nodes.size()), &nodes[0]},
                        RtParamList());
                } else {
                    TRACE_SCOPE("riley::ModifyDisplacement");
                    riley::ShadingNetwork const displacement = {
                        static_cast<uint32_t>(nodes.size()), &nodes[0]};
                    riley->ModifyDisplacement(
                        *displacementId, &displacement, nullptr);
                }
                if (*displacementId == riley::DisplacementId::InvalidId()) {
                    TF_WARN("Failed to create displacement %s\n",
                                     id.GetText());
                }
            }
        } else {
            TF_WARN("Failed to convert nodes for %s\n", id.GetText());
        }
        nodes.clear();
    }
    // Free dis-used networks.
    if (!materialFound) {
        riley->DeleteMaterial(*materialId);
        *materialId = riley::MaterialId::InvalidId();
    }
    if (!displacementFound) {
        riley->DeleteDisplacement(*displacementId);
        *displacementId = riley::DisplacementId::InvalidId();
    }
}

/* virtual */
void
HdPrmanMaterial::Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits)
{  
    HD_TRACE_FUNCTION();

    HdPrman_RenderParam *param =
        static_cast<HdPrman_RenderParam*>(renderParam);

    if ((*dirtyBits & HdMaterial::DirtyResource) ||
        (*dirtyBits & HdMaterial::DirtyParams)) {

        std::lock_guard<std::mutex> lock(_syncToRileyMutex);
#if PXR_VERSION >= 2311
        if (_rileyIsInSync) {
#else
        // Houdini 20 (with 2308) crashes sometimes with deferred sync
        // so always sync here like we used to.
        if (true) {
#endif
            // Material was previously pushed to Riley, so sync
            // immediately, because we cannot assume there will be
            // a subsequent gprim update that would pull on this material
            _rileyIsInSync = false;
            _SyncToRileyWithLock(sceneDelegate, param->AcquireRiley());
        } else {
            // Otherwise, wait until a gprim pulls on this material
            // to sync it to Riley.  This avoids doing any further
            // work for unused materials, and moves remaining work
            // from single-threaded Hydra sprim sync
            // to multi-threaded Hydra rprim sync.
        }
    }
    *dirtyBits = HdChangeTracker::Clean;
}

void
HdPrmanMaterial::SyncToRiley(
    HdSceneDelegate *sceneDelegate,
    riley::Riley *riley)
{
    {
        TRACE_SCOPE("HdPrmanMaterial::SyncToRiley - wait for lock");
        _syncToRileyMutex.lock();
    }
    std::lock_guard<std::mutex> lock(_syncToRileyMutex, std::adopt_lock);
    if (!_rileyIsInSync) {
        _SyncToRileyWithLock(sceneDelegate, riley);
    }
}

void
HdPrmanMaterial::_SyncToRileyWithLock(
    HdSceneDelegate *sceneDelegate,
    riley::Riley *riley)
{
    SdfPath const& id = GetId();
    VtValue hdMatVal = sceneDelegate->GetMaterialResource(id);

    if (hdMatVal.IsHolding<HdMaterialNetworkMap>()) {
        TF_DESCRIBE_SCOPE("Processing material %s", id.GetName().c_str());
        // Convert HdMaterial to HdMaterialNetwork2 form.
        _materialNetwork = HdConvertToHdMaterialNetwork2(
                hdMatVal.UncheckedGet<HdMaterialNetworkMap>());
        if (TfDebug::IsEnabled(HDPRMAN_MATERIALS)) {
            HdPrman_DumpNetwork(_materialNetwork, id);
        }
        _ConvertHdMaterialNetwork2ToRman(sceneDelegate,
                                         riley, id, _materialNetwork,
                                         &_materialId, &_displacementId);
    } else {
        TF_CODING_ERROR("HdPrmanMaterial: Expected material resource "
            "for <%s> to contain material, but found %s instead.",
            id.GetText(), hdMatVal.GetTypeName().c_str());
        _ResetMaterialWithLock(riley);
    }

    _rileyIsInSync = true;
}

/* virtual */
HdDirtyBits
HdPrmanMaterial::GetInitialDirtyBitsMask() const
{
    return HdChangeTracker::AllDirty;
}

bool
HdPrmanMaterial::IsValid() const
{
    return _materialId != riley::MaterialId::InvalidId();
}

HdMaterialNetwork2
HdPrmanMaterial_GetFallbackSurfaceMaterialNetwork()
{
    // We expect this to be called once, at init time, but drop a trace
    // scope in just in case that changes.  Accordingly, we also don't
    // bother creating static tokens for the single-use cases below.
    HD_TRACE_FUNCTION();

    const std::map<SdfPath, HdMaterialNode2> nodes = {
        {
            // path
            SdfPath("/Primvar_displayColor"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("PxrPrimvar"),
                // parameters
                {
                    { TfToken("varname"),
                      VtValue(TfToken("displayColor")) },
                    { TfToken("defaultColor"),
                      VtValue(GfVec3f(0.5, 0.5, 0.5)) },
                    { TfToken("type"),
                      VtValue(TfToken("color")) },
                },
            },
        },
        {
            // path
            SdfPath("/Primvar_displayRoughness"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("PxrPrimvar"),
                // parameters
                {
                    { TfToken("varname"),
                      VtValue(TfToken("displayRoughness")) },
                    { TfToken("defaultFloat"),
                      VtValue(1.0f) },
                    { TfToken("type"),
                      VtValue(TfToken("float")) },
                },
            },
        },
        {
            // path
            SdfPath("/Primvar_displayOpacity"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("PxrPrimvar"),
                // parameters
                {
                    { TfToken("varname"),
                      VtValue(TfToken("displayOpacity")) },
                    { TfToken("defaultFloat"),
                      VtValue(1.0f) },
                    { TfToken("type"),
                      VtValue(TfToken("float")) },
                },
            },
        },
        {
            // path
            SdfPath("/Primvar_displayMetallic"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("PxrPrimvar"),
                // parameters
                {
                    { TfToken("varname"),
                      VtValue(TfToken("displayMetallic")) },
                    { TfToken("defaultFloat"),
                      VtValue(0.0f) },
                    { TfToken("type"),
                      VtValue(TfToken("float")) },
                },
            },
        },

        // UsdPreviewSurfaceParameters
        {
            // path
            SdfPath("/UsdPreviewSurfaceParameters"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("UsdPreviewSurfaceParameters"),
                // parameters
                {},
                // connections
                {
                    { TfToken("diffuseColor"),
                      { { SdfPath("/Primvar_displayColor"),
                            TfToken("resultRGB") } } },
                    { TfToken("roughness"),
                      { { SdfPath("/Primvar_displayRoughness"),
                          TfToken("resultF") } } },
                    { TfToken("metallic"),
                      { { SdfPath("/Primvar_displayMetallic"),
                          TfToken("resultF") } } },
                    { TfToken("opacity"),
                      { { SdfPath("/Primvar_displayOpacity"),
                          TfToken("resultF") } } },
                },
            },
        },
        // PxrSurface (connected to UsdPreviewSurfaceParameters)
        {
            // path
            SdfPath("/PxrSurface"),
            // node info
            HdMaterialNode2 {
                // nodeTypeId
                TfToken("PxrSurface"),
                // parameters
                {
                    { TfToken("specularModelType"),
                      VtValue(int(1)) },
                    { TfToken("diffuseDoubleSided"),
                      VtValue(int(1)) },
                    { TfToken("specularDoubleSided"),
                      VtValue(int(1)) },
                    { TfToken("specularFaceColor"),
                      VtValue(GfVec3f(0.04)) },
                    { TfToken("specularEdgeColor"),
                      VtValue(GfVec3f(1.0)) },
                },
                // connections
                {
                    { TfToken("diffuseColor"),
                      {{ SdfPath("/UsdPreviewSurfaceParameters"),
                         TfToken("diffuseColorOut") }} },
                    { TfToken("diffuseGain"),
                      {{ SdfPath("/UsdPreviewSurfaceParameters"),
                         TfToken("diffuseGainOut") }} },
                    { TfToken("specularFaceColor"),
                      {{ SdfPath("/UsdPreviewSurfaceParameters"),
                         TfToken("specularFaceColorOut") }} },
                    { TfToken("specularEdgeColor"),
                      {{ SdfPath("/UsdPreviewSurfaceParameters"),
                         TfToken("specularEdgeColorOut") }} },
                    { TfToken("specularRoughness"),
                      {{ SdfPath("/UsdPreviewSurfaceParameters"),
                         TfToken("specularRoughnessOut") }} },
                    { TfToken("presence"),
                      {{ SdfPath("/Primvar_displayOpacity"),
                         TfToken("resultF") }} },
                },
            },
        },
    };

    const std::map<TfToken, HdMaterialConnection2> terminals = {
        { TfToken("surface"),
          HdMaterialConnection2 {
            SdfPath("/PxrSurface"),
            TfToken("outputName") }
        },
    };

    const TfTokenVector primvars = {
        TfToken("displayColor"),
        TfToken("displayMetallic"),
        TfToken("displayOpacity"),
        TfToken("displayRoughness"),
    };

    return HdMaterialNetwork2{nodes, terminals, primvars};
}

PXR_NAMESPACE_CLOSE_SCOPE

