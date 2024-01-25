//
// Copyright 2023 Pixar
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
#include "hdPrman/portalLightResolvingSceneIndexPlugin.h"

#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/usd/sdf/assetPath.h"

#include <boost/functional/hash.hpp>

#include <algorithm>
#include <iterator>
#include <set>
#include <unordered_map>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (PortalLight)
    (PxrPortalLight)
    ((sceneIndexPluginName, "HdPrman_PortalLightResolvingSceneIndexPlugin"))

    // material network tokens
    (color)
    ((colorMap,                "texture:file"))
    ((domeColorMap,            "ri:light:domeColorMap"))
    (exposure)
    (intensity)
    ((intensityMult,           "ri:light:intensityMult"))
    ((portalName,              "ri:light:portalName"))
    ((portalToDome,            "ri:light:portalToDome"))
    ((tint,                    "ri:light:tint"))

    // render context / material network selector
    ((renderContext, "ri"))
);

// Material parameters for which we should overwrite unauthored values
// on a portal light with authored values from the portal's dome light.
TF_DEFINE_PRIVATE_TOKENS(
    _inheritedAttrTokens,

    (colorEnableTemperature)
    ((colorMapGamma,           "ri:light:colorMapGamma"))
    ((colorMapSaturation,      "ri:light:colorMapSaturation"))
    (colorTemperature)
    (diffuse)
    ((importanceMultiplier,    "ri:light:importanceMultiplier"))
    ((shadowColor,             "shadow:color"))
    ((shadowDistance,          "shadow:distance"))
    ((shadowEnable,            "shadow:enable"))
    ((shadowFalloff,           "shadow:falloff"))
    ((shadowFalloffGamma,      "shadow:falloffGamma"))
    (specular)
    ((thinShadow,              "ri:light:thinShadow"))
    ((traceLightPaths,         "ri:light:traceLightPaths"))
    ((visibleInRefractionPath, "ri:light:visibleInRefractionPath"))
);

static const char* const _pluginDisplayName = "Prman";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_PortalLightResolvingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // We need an insertion point that's *after* general material resolve.
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 115;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

namespace {

bool
_IsPortalLight(const HdSceneIndexPrim& prim, const SdfPath& primPath)
{
    auto matDataSource =
        HdMaterialSchema::GetFromParent(prim.dataSource)
            .GetMaterialNetwork(_tokens->renderContext)
#if HD_API_VERSION >= 63
            .GetContainer()
#endif
        ;
    HdDataSourceMaterialNetworkInterface matInterface(primPath, matDataSource,
                                                      prim.dataSource);

    const auto matTerminal =
        matInterface.GetTerminalConnection(HdMaterialTerminalTokens->light);
    const auto nodeName = matTerminal.second.upstreamNodeName;

    const TfToken nodeTypeName = matInterface.GetNodeType(nodeName);

    // We accept either the generic UsdLux "PortalLight" or the
    // RenderMan-specific "PxrPortalLight" here.  (The former can
    // occur when using Hydra render index emulation.  In that
    // setup, the scene index chain runs prior to applying the
    // renderContextNodeIdentifier to individual nodes.)
    return (nodeTypeName == _tokens->PxrPortalLight
        || nodeTypeName == _tokens->PortalLight);
}

// Helper function to extract a value from a light data source.
template <typename T>
T
_GetLightData(
    const HdContainerDataSourceHandle& primDataSource,
    const TfToken& name)
{
    if (auto lightSchema = HdLightSchema::GetFromParent(primDataSource)) {
        if (auto dataSource = HdTypedSampledDataSource<T>::Cast(
                lightSchema.GetContainer()->Get(name))) {
            return dataSource->GetTypedValue(0.0f);
        }
    }

    return {};
}

SdfPathVector
_GetPortalPaths(const HdContainerDataSourceHandle& primDataSource)
{
    return _GetLightData<SdfPathVector>(primDataSource, HdTokens->portals);
}

SdfPathVector
_GetLightFilterPaths(const HdContainerDataSourceHandle& primDataSource)
{
    return _GetLightData<SdfPathVector>(primDataSource, HdTokens->filters);
}

std::string
_GetPortalName(
    const std::string& domeColorMap,
    const GfMatrix4d& domeXform,
    const GfMatrix4d& portalXform)
{
    size_t hashValue = 0;
    boost::hash_combine(hashValue, domeColorMap);
    boost::hash_combine(hashValue, domeXform.ExtractRotation());
    boost::hash_combine(hashValue, portalXform.ExtractRotation());

    return std::to_string(hashValue);
}

HdContainerDataSourceHandle
_BuildDomeLightDataSource(
    const SdfPath& domePrimPath,
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    const auto domePrim = inputSceneIndex->GetPrim(domePrimPath);

    // The dome light has portals, or we wouldn't be calling this function.
    // Mute the dome light so that it doesn't show up in the render.

    // XXX -- Maybe we should also clear the filters in the dome's light data
    //        source. These filters will apply directly to the dome's portals
    //        rather than to the dome (which is muted anyway). However, it
    //        doesn't appear to be necessary to remove filters from the dome,
    //        and might require us to store and update dome filter paths in
    //        the scene index class (lest they be cleared prematurely here)
    //        so we won't bother for now.

    const HdContainerDataSourceHandle visibilityDataSource =
        HdVisibilitySchema::Builder()
            .SetVisibility(HdRetainedTypedSampledDataSource<bool>::New(false))
            .Build();

    return HdOverlayContainerDataSource::New(
        HdRetainedContainerDataSource::New(
            HdVisibilitySchemaTokens->visibility, visibilityDataSource),
        domePrim.dataSource);
}

HdContainerDataSourceHandle
_BuildPortalLightDataSource(
    const SdfPath& domePrimPath,
    const SdfPath& portalPrimPath,
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    const auto domePrim   = inputSceneIndex->GetPrim(domePrimPath);
    const auto portalPrim = inputSceneIndex->GetPrim(portalPrimPath);

    if (!domePrim.dataSource || !_IsPortalLight(portalPrim, portalPrimPath)) {
        // Without a dome prim there's nothing to do here.
        return portalPrim.dataSource;
    }

    // Get data sources for the associated dome light.
    // -------------------------------------------------------------------------
    const HdContainerDataSourceHandle domeMatDataSource = 
        HdMaterialSchema::GetFromParent(domePrim.dataSource)
            .GetMaterialNetwork(_tokens->renderContext)
#if HD_API_VERSION >= 63
            .GetContainer()
#endif
        ;
    HdDataSourceMaterialNetworkInterface domeMatInterface(domePrimPath,
                                                          domeMatDataSource,
                                                          domePrim.dataSource);

    const auto domeMatTerminal =
        domeMatInterface.GetTerminalConnection(HdMaterialTerminalTokens->light);

    HdXformSchema domeXformSchema =
        HdXformSchema::GetFromParent(domePrim.dataSource);

    // Get some relevant values from the dome light's data sources.
    // -------------------------------------------------------------------------
    const auto getDomeMatVal =
        [&domeMatInterface, &domeMatTerminal](const TfToken& paramName){
            return domeMatInterface.GetNodeParameterValue(
                domeMatTerminal.second.upstreamNodeName, paramName);
        };

    const VtValue domeColorMapVal  = getDomeMatVal(_tokens->colorMap);
    const VtValue domeColorVal     = getDomeMatVal(_tokens->color);
    const VtValue domeIntensityVal = getDomeMatVal(_tokens->intensity);
    const VtValue domeExposureVal  = getDomeMatVal(_tokens->exposure);

    const std::string domeColorMap =
        domeColorMapVal.IsHolding<SdfAssetPath>()
            ? domeColorMapVal.UncheckedGet<SdfAssetPath>().GetResolvedPath()
            : "";

    const auto domeColor     = domeColorVal.GetWithDefault(GfVec3f(1.0f));
    const auto domeIntensity = domeIntensityVal.GetWithDefault(1.0f);
    const auto domeExposure  = domeExposureVal.GetWithDefault(0.0f);

    GfMatrix4d domeXform;
    if (const auto origDomeXform = domeXformSchema.GetMatrix()) {
        // This matrix encodes a -90 deg rotation about the x-axis and a 90 deg
        // rotation about the y-axis, which correspond to the transform needed
        // for prman to convert right handed to left handed.
        static const GfMatrix4d domeXformAdjustment( 0.0, 0.0, -1.0, 0.0,
                                                    -1.0, 0.0,  0.0, 0.0,
                                                     0.0, 1.0,  0.0, 0.0,
                                                     0.0, 0.0,  0.0, 1.0);

        domeXform = domeXformAdjustment * origDomeXform->GetTypedValue(0.0f);
    }
    else {
        domeXform.SetIdentity();
    }

    // Get data sources for the portal light.
    // -------------------------------------------------------------------------
    const HdContainerDataSourceHandle portalMatDataSource = 
        HdMaterialSchema::GetFromParent(portalPrim.dataSource)
            .GetMaterialNetwork(_tokens->renderContext)
#if HD_API_VERSION >= 63
            .GetContainer()
#endif
        ;

    HdDataSourceMaterialNetworkInterface portalMatInterface(
        portalPrimPath, portalMatDataSource, portalPrim.dataSource);

    const auto portalMatTerminal =
        portalMatInterface.GetTerminalConnection(
            HdMaterialTerminalTokens->light);

    HdXformSchema portalXformSchema =
        HdXformSchema::GetFromParent(portalPrim.dataSource);

    // Get some relevant values from the portal light's data sources.
    // -------------------------------------------------------------------------
    const auto getPortalMatVal =
        [&portalMatInterface, &portalMatTerminal](const TfToken& paramName){
            return portalMatInterface.GetNodeParameterValue(
                portalMatTerminal.second.upstreamNodeName, paramName);
        };

    const VtValue portalTintVal    = getPortalMatVal(_tokens->tint);
    const VtValue portalIntMultVal = getPortalMatVal(_tokens->intensityMult);

    const auto portalTint    = portalTintVal.GetWithDefault(GfVec3f(1.0f));
    const auto portalIntMult = portalIntMultVal.GetWithDefault(1.0f);

    GfMatrix4d portalXform;
    if (const auto origPortalXform = portalXformSchema.GetMatrix()) {
        // This matrix encodes a 180 deg rotation about the y-axis and a -1
        // scale in y, which correspond to the transform needed for prman to
        // convert right handed to left handed.
        static const GfMatrix4d portalXformAdjustment(-1.0,  0.0,  0.0, 0.0,
                                                       0.0, -1.0,  0.0, 0.0,
                                                       0.0,  0.0, -1.0, 0.0,
                                                       0.0,  0.0,  0.0, 1.0);

        portalXform = portalXformAdjustment *
                      origPortalXform->GetTypedValue(0.0f);
    }
    else {
        portalXform.SetIdentity();
    }

    // Compute new values for the portal's material data source.
    // -------------------------------------------------------------------------
    const auto setPortalParamVal =
        [&portalMatInterface, &portalMatTerminal](
            const TfToken& paramName, const VtValue& value)
        {
            portalMatInterface.SetNodeParameterValue(
                portalMatTerminal.second.upstreamNodeName, paramName, value);
        };

    const auto computedPortalColor = GfCompMult(portalTint, domeColor);
    const auto computedPortalIntensity = portalIntMult * domeIntensity *
                                         powf(2.0f, domeExposure);
    const auto computedPortalToDome = portalXform * domeXform.GetInverse();
    const auto computedPortalName = _GetPortalName(domeColorMap, domeXform,
                                                   portalXform);

    setPortalParamVal(_tokens->domeColorMap, VtValue(domeColorMap));
    setPortalParamVal(_tokens->color,        VtValue(computedPortalColor));
    setPortalParamVal(_tokens->intensity,    VtValue(computedPortalIntensity));
    setPortalParamVal(_tokens->portalToDome, VtValue(computedPortalToDome));
    setPortalParamVal(_tokens->portalName,   VtValue(computedPortalName));

    // XXX -- We can probably delete the portal's tint and intensityMult params
    //        now, since they're not used by the RenderMan light shader.

    // Directly copy a bunch of other params from the dome to the portal.
    // XXX -- We'd like to do this only for *unauthored* portal params. However,
    //        there's no obvious way to tell which params are user-authored.
    for (const auto& attr: _inheritedAttrTokens->allTokens) {
        setPortalParamVal(attr, getDomeMatVal(attr));
    }

    // Compute new values for the portal's light data source.
    // -------------------------------------------------------------------------
    // All we're going to do is copy the light filter paths from the dome's
    // light.filters data source to the portal's light.filters data source.
    // This means that the filter prims will still just exist under the dome
    // and filter xforms will be relative to the dome, not the portal. That
    // xform behavior is expected; it matches what happens in Katana.
    SdfPathVector domeFilters = _GetLightFilterPaths(domePrim.dataSource);
    SdfPathVector allFilters  = _GetLightFilterPaths(portalPrim.dataSource);
    allFilters.insert(allFilters.end(),
                      std::make_move_iterator(domeFilters.begin()),
                      std::make_move_iterator(domeFilters.end()));
    const auto computedFiltersDataSource =
        HdRetainedTypedSampledDataSource<SdfPathVector>::New(allFilters);

    // XXX -- If the portal has an authored shadowLink value, we shouldn't
    //        overwrite it. (The shadowLink code should be updated when we have
    //        a good way to tell whether values are authored.)
    const auto computedShadowLinkDataSource =
        HdRetainedTypedSampledDataSource<TfToken>::New(
            _GetLightData<TfToken>(domePrim.dataSource, HdTokens->shadowLink));

    // Assemble the final data source for the portal light.
    // -------------------------------------------------------------------------
    std::vector<TfToken> names;
    std::vector<HdDataSourceBaseHandle> sources;

    names.push_back(HdMaterialSchemaTokens->material);
    sources.push_back(HdRetainedContainerDataSource::New(
        _tokens->renderContext, portalMatInterface.Finish()));

    names.push_back(HdLightSchemaTokens->light);
    sources.push_back(HdRetainedContainerDataSource::New(
        HdTokens->filters,    computedFiltersDataSource,
        HdTokens->shadowLink, computedShadowLinkDataSource));

    return HdOverlayContainerDataSource::New(
        HdRetainedContainerDataSource::New(
            names.size(), names.data(), sources.data()),
        portalPrim.dataSource);
}

//
// _PortalLightResolvingSceneIndex
//

TF_DECLARE_REF_PTRS(_PortalLightResolvingSceneIndex);

// Pixar-only, Prman-specific Hydra scene index to resolve portal lights.
class _PortalLightResolvingSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _PortalLightResolvingSceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

protected:
    _PortalLightResolvingSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);
    ~_PortalLightResolvingSceneIndex();

    void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;

private:
    SdfPathVector _AddMappingsForDome(const SdfPath& domePrimPath);
    SdfPathVector _RemoveMappingsForDome(const SdfPath& domePrimPath);

private:
    // Map dome light paths to flag indicating presence of associated portals.
    std::unordered_map<SdfPath, bool, SdfPath::Hash> _domesWithPortals;

    // Map portal path to dome path. A previous name for this map was
    // "_portalToDome", but that conflicts with a material param name.
    std::unordered_map<SdfPath, SdfPath, SdfPath::Hash> _portalsToDomes;
};

/* static */
_PortalLightResolvingSceneIndexRefPtr
_PortalLightResolvingSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(
        new _PortalLightResolvingSceneIndex(inputSceneIndex));
}

_PortalLightResolvingSceneIndex::_PortalLightResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
    // Do nothing
}

_PortalLightResolvingSceneIndex::
~_PortalLightResolvingSceneIndex() = default;

HdSceneIndexPrim 
_PortalLightResolvingSceneIndex::GetPrim(
    const SdfPath& primPath) const
{
    auto prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (prim.primType != HdPrimTypeTokens->light &&
        prim.primType != HdPrimTypeTokens->domeLight) {
        // No special behavior for prims that aren't portals or domes.
        return prim;
    }

    // Check for portal
    const auto portalIt = _portalsToDomes.find(primPath);
    if (portalIt != _portalsToDomes.end()) {
        const auto domePrimPath = portalIt->second;
        return {
            prim.primType,
            _BuildPortalLightDataSource(domePrimPath, primPath,
                                        _GetInputSceneIndex())
        };
    }

    // Check for dome
    const auto domeIt = _domesWithPortals.find(primPath);
    // If the dome has associated portals, wrap the data source.
    // Otherwise, pass it through as-is.
    if (domeIt != _domesWithPortals.end() && domeIt->second) {
        return {
            prim.primType,
            _BuildDomeLightDataSource(primPath, _GetInputSceneIndex())
        };
    }

    return prim;
}

SdfPathVector 
_PortalLightResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath& primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
_PortalLightResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{    
    if (!_IsObserved()) {
        return;
    }

    for (const auto& entry: entries) {
        if (entry.primType == HdPrimTypeTokens->domeLight) {
            _AddMappingsForDome(entry.primPath);
        }
    }

    _SendPrimsAdded(entries);
}

void 
_PortalLightResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    if (!_IsObserved()) {
        return;
    }

    for (const auto& entry: entries) {
        if (_domesWithPortals.count(entry.primPath)) {
            _RemoveMappingsForDome(entry.primPath);
        }
    }

    _SendPrimsRemoved(entries);
}

void
_PortalLightResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    if (!_IsObserved()) {
        return;
    }

    static const auto& lightLocator    = HdLightSchema::GetDefaultLocator();
    static const auto& materialLocator = HdMaterialSchema::GetDefaultLocator();
    static const auto& xformLocator    = HdXformSchema::GetDefaultLocator();

    HdSceneIndexObserver::DirtiedPrimEntries dirtied;
    SdfPathSet dirtiedPortals;
    for (const auto& entry: entries) {
        auto domeIt = _domesWithPortals.find(entry.primPath);
        if (domeIt != _domesWithPortals.end()) {
            // entry.primPath is a known dome
            if (entry.dirtyLocators.Contains(lightLocator)) {
                // The dome's portals may have changed.
                auto removedPortals =
                    _RemoveMappingsForDome(entry.primPath);
                _AddMappingsForDome(entry.primPath);

                dirtiedPortals.insert(
                    std::make_move_iterator(removedPortals.begin()),
                    std::make_move_iterator(removedPortals.end()));
            }
            if (entry.dirtyLocators.Contains(lightLocator) ||
                entry.dirtyLocators.Contains(materialLocator) ||
                entry.dirtyLocators.Contains(xformLocator)) {
                // Assume that the dome's portals should be considered dirty.
                for (const auto& [portalPath, domePath]: _portalsToDomes) {
                    if (domePath == entry.primPath) {
                        dirtiedPortals.insert(portalPath);
                    }
                }
            }
            dirtied.push_back(entry);
        }
        else if (_portalsToDomes.count(entry.primPath) &&
                 entry.dirtyLocators.Contains(xformLocator)) {
            // An xform change will affect portalToDome and portalName,
            // so we need to make sure the material data source gets dirtied.
            HdSceneIndexObserver::DirtiedPrimEntry newEntry(entry);
            newEntry.dirtyLocators.insert(materialLocator);
            dirtied.push_back(newEntry);
        }
        else {
            dirtied.push_back(entry);
        }
    }

    // Check for elements of "dirtiedPortals" that are already in "dirtied".
    for (auto& entry: dirtied) {
        if (dirtiedPortals.find(entry.primPath) != dirtiedPortals.end()) {
            // If the portal is already in the dirtied vector, we don't want to
            // add it again.
            dirtiedPortals.erase(entry.primPath);

            // We do, however, want to ensure that the material and light data
            // sources are considered dirty.
            entry.dirtyLocators.insert({materialLocator, lightLocator});
        }
    }

    for (const auto& portalPath: dirtiedPortals) {
        dirtied.emplace_back(
            portalPath,
            HdDataSourceLocatorSet{materialLocator, lightLocator});
    }

    _SendPrimsDirtied(dirtied);
}

SdfPathVector
_PortalLightResolvingSceneIndex::_AddMappingsForDome(
    const SdfPath& domePrimPath)
{
    const auto domePrim = _GetInputSceneIndex()->GetPrim(domePrimPath);

    if (domePrim.primType != HdPrimTypeTokens->domeLight) {
        // Caller should have already confirmed this is a dome.
        TF_CODING_ERROR("_AddMappingsForDome invoked for non-"
                        "domeLight path <%s>", domePrimPath.GetText());
        return SdfPathVector();
    }

    SdfPathVector portalPaths = _GetPortalPaths(domePrim.dataSource);

    _domesWithPortals[domePrimPath] = !portalPaths.empty();

    for (const auto& portalPath: portalPaths) {
        const auto it = _portalsToDomes.insert({portalPath,domePrimPath}).first;
        if (it->second != domePrimPath) {
            TF_WARN("Failed to register <%s> as a portal light for <%s>. "
                    "The portal is already in use with <%s> and cannot be "
                    "reused with another dome light.",
                    portalPath.GetText(), domePrimPath.GetText(),
                    it->second.GetText());
        }
    }
    return portalPaths;
}

SdfPathVector
_PortalLightResolvingSceneIndex::_RemoveMappingsForDome(
    const SdfPath& domePrimPath)
{
    SdfPathVector portalPaths;
    auto domeIt = _domesWithPortals.find(domePrimPath);
    if (domeIt != _domesWithPortals.end()) {
        const bool domeHasPortals = domeIt->second;
        if (domeHasPortals) {
            // We successfully found a dome prim to erase, so remove the
            // corresponding _portalsToDomes entries.
            for (auto it = _portalsToDomes.begin(); it != _portalsToDomes.end();) {
                if (it->second == domePrimPath) {
                    portalPaths.push_back(it->first);
                    it = _portalsToDomes.erase(it);
                }
                else {
                    it++;
                }
            }
        }
        _domesWithPortals.erase(domeIt);
    }
    return portalPaths;
}

} // anonymous namespace

//
// HdPrman_PortalLightResolvingSceneIndexPlugin
//

HdPrman_PortalLightResolvingSceneIndexPlugin::
HdPrman_PortalLightResolvingSceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdPrman_PortalLightResolvingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr& inputScene,
    const HdContainerDataSourceHandle& inputArgs)
{
    return _PortalLightResolvingSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
