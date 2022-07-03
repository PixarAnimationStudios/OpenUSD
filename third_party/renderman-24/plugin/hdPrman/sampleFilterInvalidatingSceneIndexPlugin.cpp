//
// Copyright 2022 Pixar
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

#include "hdPrman/sampleFilterInvalidatingSceneIndexPlugin.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/sampleFilterSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_SampleFilterInvalidatingSceneIndexPlugin"))
);

static const char * const _pluginDisplayName = "Prman";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_SampleFilterInvalidatingSceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 1000;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtEnd);
}

HdPrman_SampleFilterInvalidatingSceneIndexPlugin::
HdPrman_SampleFilterInvalidatingSceneIndexPlugin() = default;


namespace
{

VtArray<SdfPath>
_GetConnectedSampleFilters(const HdSceneIndexPrim &prim)
{
    HdContainerDataSourceHandle renderSettingsDs =
        HdContainerDataSource::Cast(prim.dataSource->Get(
            HdRenderSettingsSchemaTokens->renderSettings));
    if (!renderSettingsDs) {
        return VtArray<SdfPath>();
    }
    
    HdSampledDataSourceHandle valueDs =
        HdSampledDataSource::Cast(renderSettingsDs->Get(
            HdRenderSettingsSchemaTokens->sampleFilters));
    if (!valueDs) {
        return VtArray<SdfPath>();
    }

    VtValue pathArrayValue = valueDs->GetValue(0);
    if (pathArrayValue.IsHolding<VtArray<SdfPath>>()) {
        return pathArrayValue.UncheckedGet<VtArray<SdfPath>>();
    }
    return VtArray<SdfPath>();
}

TF_DECLARE_REF_PTRS(_HdPrmanSampleFilterInvalidatingSceneIndex);
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// \class _HdPrmanSampleFilterInvalidatingSceneIndex
///
/// The scene index feeding into HdDependencyForwardingSceneIndex and
/// constructed by the HdPrman_SampleFilterInvalidatingSceneIndexPlugin.
///
class _HdPrmanSampleFilterInvalidatingSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _HdPrmanSampleFilterInvalidatingSceneIndexRefPtr
        New(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _HdPrmanSampleFilterInvalidatingSceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetPrim(primPath);
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _HdPrmanSampleFilterInvalidatingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
      : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    {
    }

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }
        
        // RenderSettings are added when the connected SampleFilters change,
        // dirty these sampleFilters to make sure we get the correct visual
        HdSceneIndexObserver::DirtiedPrimEntries sampleFiltersToDirty;
        for(auto const& entry : entries) {
            if (entry.primType == HdPrimTypeTokens->renderSettings) {
                const HdSceneIndexPrim prim =
                    _GetInputSceneIndex()->GetPrim(entry.primPath);
                for (auto const& path : _GetConnectedSampleFilters(prim)) {
                    sampleFiltersToDirty.emplace_back(
                        path, HdSampleFilterSchema::GetDefaultLocator());
                }
            }
        }
        _SendPrimsAdded(entries);
        _SendPrimsDirtied(sampleFiltersToDirty);
    }

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override
    {
        if (!_IsObserved()) {
            return;
        }

        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override
    {
        HD_TRACE_FUNCTION();

        if (!_IsObserved()) {
            return;
        }

        // When the RenderSettings prim is dirtied make sure to dirty the
        // connected sampleFilters as well.
        HdSceneIndexObserver::DirtiedPrimEntries sampleFiltersToDirty;
        for(auto const &entry : entries) {
            if (entry.dirtyLocators.Intersects(
                    HdRenderSettingsSchema::GetDefaultLocator())) {
                const HdSceneIndexPrim prim =
                    _GetInputSceneIndex()->GetPrim(entry.primPath);
                for (auto const& path : _GetConnectedSampleFilters(prim)) {
                    sampleFiltersToDirty.emplace_back(
                        path, HdSampleFilterSchema::GetDefaultLocator());
                }
            }

        }
        _SendPrimsDirtied(entries);
        _SendPrimsDirtied(sampleFiltersToDirty);
    }
};

}

HdSceneIndexBaseRefPtr
HdPrman_SampleFilterInvalidatingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _HdPrmanSampleFilterInvalidatingSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
