//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/energyFilter.h"

#if PXR_VERSION >= 2308 && _PRMANAPI_VERSION_MAJOR_ >= 27

#include "hdPrman/debugCodes.h"
#include "hdPrman/renderDelegate.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/utils.h"

#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/imaging/hd/energyFilterSchema.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

#if PXR_VERSION < 2505
using SdrTokenVec = NdrTokenVec;
#endif

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((riEnergyFilterEnabled,  "ri:energyFilter:enabled"))
    ((riEnergyFilterLpe,      "ri:energyFilter:lpe"))
    ((riEnergyFilterOrder,    "ri:energyFilter:order"))
);

TF_MAKE_STATIC_DATA(SdrTokenVec, _sourceTypes) {
    *_sourceTypes = { TfToken("RmanCpp") }; }

HdPrman_EnergyFilter::HdPrman_EnergyFilter(
    SdfPath const& id)
    : HdSprim(id)
{
}

void
HdPrman_EnergyFilter::Finalize(HdRenderParam *renderParam)
{
}

void
HdPrman_EnergyFilter::_CreateRmanEnergyFilter(
    HdSceneDelegate *sceneDelegate,
    HdPrman_RenderParam *renderParam,
    SdfPath const& filterPrimPath,
    HdMaterialNode2 const& energyFilterNode)
{
    // Build the Riley shading node for the energy filter type
    riley::ShadingNode rileyNode;
    rileyNode.type = riley::ShadingNode::Type::k_Pattern;
    rileyNode.handle = RtUString(filterPrimPath.GetText());

    SdrRegistry &sdrRegistry = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrEntry = sdrRegistry.GetShaderNodeByIdentifier(
        energyFilterNode.nodeTypeId, *_sourceTypes);
    if (!sdrEntry) {
        TF_WARN("Unknown shader ID '%s' for energy filter <%s>\n",
                energyFilterNode.nodeTypeId.GetText(),
                filterPrimPath.GetText());
        return;
    }
    std::string shaderPath = sdrEntry->GetImplementationName();
    if (shaderPath.empty()) {
        TF_WARN("Energy filter shader '%s' did not provide a valid "
                "implementation path.", sdrEntry->GetName().c_str());
        return;
    }
    rileyNode.name = RtUString(shaderPath.c_str());

    // Shader node params: mult, clamp, saturation, hue (inputs:ri:*)
    TF_DEBUG(HDPRMAN_ENERGY_FILTER).Msg(
        "[HdPrman_EnergyFilter] id=%s paramCount=%zu nodeType=%s\n",
        filterPrimPath.GetText(), energyFilterNode.parameters.size(),
        energyFilterNode.nodeTypeId.GetText());
    for (const auto &param : energyFilterNode.parameters) {
        TF_DEBUG(HDPRMAN_ENERGY_FILTER).Msg(
            "[HdPrman_EnergyFilter] param=%s prop=%s\n",
            param.first.GetText(),
            sdrEntry->GetShaderInput(param.first) ? "found" : "NOT FOUND");
        const SdrShaderProperty* prop = sdrEntry->GetShaderInput(param.first);
        if (!prop) {
            TF_WARN("Unknown shader property '%s' for energy filter '%s' "
                    "at <%s>, ignoring.\n",
                    param.first.GetText(),
                    energyFilterNode.nodeTypeId.GetText(),
                    filterPrimPath.GetText());
            continue;
        }
        HdPrman_Utils::SetParamFromVtValue(
            RtUString(prop->GetImplementationName().c_str()),
            param.second, prop->GetType(), &rileyNode.params);
    }

    // Properties: enabled, lpe, order - passed as RtParamList to Riley.
    // Also copy mult/clamp/saturation/hue here as float arrays/floats, since
    // energyfilterimpl.cpp reads them from this RtParamList via GetFloatArray /
    // GetFloat, and SetParamFromVtValue stores color3f as SetColor (not
    // SetFloatArray), causing GetFloatArray to fail.
    RtParamList properties;

    for (const auto &param : energyFilterNode.parameters) {
        const SdrShaderProperty* prop = sdrEntry->GetShaderInput(param.first);
        if (!prop) { continue; }
        const std::string impl = prop->GetImplementationName();
        if (impl == "mult") {
            if (param.second.IsHolding<GfVec3f>()) {
                const GfVec3f &v = param.second.UncheckedGet<GfVec3f>();
                float arr[3] = { v[0], v[1], v[2] };
                properties.SetFloatArray(RtUString("mult"), arr, 3);
            }
        } else if (impl == "clamp" || impl == "saturation" || impl == "hue") {
            if (param.second.IsHolding<float>()) {
                properties.SetFloat(RtUString(impl.c_str()),
                                    param.second.UncheckedGet<float>());
            }
        }
    }

    // enabled defaults to 1 (on); only override if explicitly authored.
    properties.SetInteger(RtUString("enabled"), 1);
    const VtValue enabledVal =
        sceneDelegate->Get(filterPrimPath, _tokens->riEnergyFilterEnabled);
    if (enabledVal.IsHolding<bool>()) {
        properties.SetInteger(RtUString("enabled"),
                              enabledVal.UncheckedGet<bool>() ? 1 : 0);
    }

    // lpe: only set if authored - the schema default is intentionally broad
    // and should not be used as a filter expression.
    const VtValue lpeVal =
        sceneDelegate->Get(filterPrimPath, _tokens->riEnergyFilterLpe);
    if (lpeVal.IsHolding<std::string>()) {
        const std::string &lpe = lpeVal.UncheckedGet<std::string>();
        if (!lpe.empty()) {
            properties.SetString(RtUString("lpe"), RtUString(lpe.c_str()));
        }
    }

    // order defaults to -1 (encounter order); only override if explicitly authored.
    properties.SetInteger(RtUString("order"), -1);
    const VtValue orderVal =
        sceneDelegate->Get(filterPrimPath, _tokens->riEnergyFilterOrder);
    if (orderVal.IsHolding<int>()) {
        properties.SetInteger(RtUString("order"),
                              orderVal.UncheckedGet<int>());
    }

    RtUString lpeStr;
    properties.GetString(RtUString("lpe"), lpeStr);
    TF_DEBUG(HDPRMAN_ENERGY_FILTER).Msg(
        "[HdPrman_EnergyFilter] AddEnergyFilter id=%s lpe=%s\n",
        filterPrimPath.GetText(), lpeStr.CStr());
    renderParam->AddEnergyFilter(
        sceneDelegate, filterPrimPath, rileyNode, properties);
}

void
HdPrman_EnergyFilter::Sync(
    HdSceneDelegate *sceneDelegate,
    HdRenderParam *renderParam,
    HdDirtyBits *dirtyBits)
{
    const SdfPath &id = GetId();
    HdPrman_RenderParam *param =
        static_cast<HdPrman_RenderParam*>(renderParam);

    TF_DEBUG(HDPRMAN_ENERGY_FILTER).Msg(
        "[HdPrman_EnergyFilter::Sync] id=%s dirtyBits=0x%x\n",
        id.GetText(), (unsigned)*dirtyBits);

    if (*dirtyBits & HdChangeTracker::DirtyParams) {
        const SdfPathVector& filters = param->GetEnergyFilterPaths();
        const bool inPaths = std::find(filters.begin(), filters.end(), id)
            != filters.end();
        TF_DEBUG(HDPRMAN_ENERGY_FILTER).Msg(
            "[HdPrman_EnergyFilter::Sync] id=%s inPaths=%d pathCount=%zu\n",
            id.GetText(), (int)inPaths, (size_t)filters.size());
        if (inPaths) {
            const VtValue energyFilterResourceValue =
                sceneDelegate->Get(id, HdEnergyFilterSchemaTokens->resource);
            TF_DEBUG(HDPRMAN_ENERGY_FILTER).Msg(
                "[HdPrman_EnergyFilter::Sync] id=%s resource holding=%d\n",
                id.GetText(),
                (int)energyFilterResourceValue.IsHolding<HdMaterialNode2>());

            if (energyFilterResourceValue.IsHolding<HdMaterialNode2>()) {
                HdMaterialNode2 energyFilterNode =
                    energyFilterResourceValue.UncheckedGet<HdMaterialNode2>();
                _CreateRmanEnergyFilter(
                    sceneDelegate, param, id, energyFilterNode);
            }
        }
    }
    else if (*dirtyBits & HdChangeTracker::DirtyVisibility) {
        param->CreateEnergyFilterNetwork(sceneDelegate);
    }

    *dirtyBits = HdChangeTracker::Clean;
}

HdDirtyBits HdPrman_EnergyFilter::GetInitialDirtyBitsMask() const
{
    int mask =
        HdChangeTracker::Clean |
        HdChangeTracker::DirtyParams |
        HdChangeTracker::DirtyVisibility;
    return (HdDirtyBits)mask;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2308 && _PRMANAPI_VERSION_MAJOR_ >= 27
