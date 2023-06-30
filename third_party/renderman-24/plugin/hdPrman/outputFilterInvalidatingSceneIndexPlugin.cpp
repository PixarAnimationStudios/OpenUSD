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

#include "hdPrman/outputFilterInvalidatingSceneIndexPlugin.h"
#include "hdPrman/debugCodes.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/sampleFilterSchema.h"
#include "pxr/imaging/hd/displayFilterSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_OutputFilterInvalidatingSceneIndexPlugin"))
    ((outputsRiSampleFilters, "outputs:ri:sampleFilters"))
    ((outputsRiDisplayFilters, "outputs:ri:displayFilters"))
);

static const char * const _pluginDisplayName = "Prman";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_OutputFilterInvalidatingSceneIndexPlugin>();
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

HdPrman_OutputFilterInvalidatingSceneIndexPlugin::
HdPrman_OutputFilterInvalidatingSceneIndexPlugin() = default;


namespace
{

VtArray<SdfPath>
_GetConnectedOutputFilters(const HdSceneIndexPrim &prim)
{
    const HdContainerDataSourceHandle renderSettingsDs =
        HdContainerDataSource::Cast(prim.dataSource->Get(
            HdRenderSettingsSchemaTokens->renderSettings));
    if (!renderSettingsDs) {
        return VtArray<SdfPath>();
    }
    HdRenderSettingsSchema rsSchema = HdRenderSettingsSchema(renderSettingsDs);
    if (!rsSchema.IsDefined()) {
        return VtArray<SdfPath>();
    }
    HdContainerDataSourceHandle namespacedSettingsDS = 
        rsSchema.GetNamespacedSettings();
    if (!namespacedSettingsDS) {
        return VtArray<SdfPath>();
    }

    const TfToken filterTokens[] = {
        _tokens->outputsRiSampleFilters,
        _tokens->outputsRiDisplayFilters
    };

    VtArray<SdfPath> filters;
    for (const auto& filterToken : filterTokens) {
        const HdSampledDataSourceHandle valueDs =
            HdSampledDataSource::Cast(namespacedSettingsDS->Get(filterToken));
        if (!valueDs) {
            continue;
        }
        const VtValue pathsValue = valueDs->GetValue(0);
        const SdfPathVector paths = pathsValue.GetWithDefault<SdfPathVector>();
        for (const auto& path : paths) {
            filters.push_back(path);
        }
    }
    
    return filters;
}

TF_DECLARE_REF_PTRS(_HdPrmanOutputFilterInvalidatingSceneIndex);

////////////////////////////////////////////////////////////////////////////////
/// \class _HdPrmanOutputFilterInvalidatingSceneIndex
///
/// The scene index feeding into HdDependencyForwardingSceneIndex and
/// constructed by the HdPrman_OutputFilterInvalidatingSceneIndexPlugin.
///

class _HdPrmanOutputFilterInvalidatingSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _HdPrmanOutputFilterInvalidatingSceneIndexRefPtr
        New(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _HdPrmanOutputFilterInvalidatingSceneIndex(inputSceneIndex));
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
    _HdPrmanOutputFilterInvalidatingSceneIndex(
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
        
        // RenderSettings are added when the connected filters change,
        // dirty these filters to make sure we get the correct visual
        HdSceneIndexObserver::DirtiedPrimEntries filtersToDirty;
        for(auto const& entry : entries) {
            if (entry.primType == HdPrimTypeTokens->renderSettings) {
                const HdSceneIndexPrim prim =
                    _GetInputSceneIndex()->GetPrim(entry.primPath);
                for (auto const& path : _GetConnectedOutputFilters(prim)) {
                    const TfToken filterType = 
                        _GetInputSceneIndex()->GetPrim(path).primType;
                    if (filterType == HdPrimTypeTokens->sampleFilter) {
                        filtersToDirty.emplace_back(path, 
                            HdSampleFilterSchema::GetDefaultLocator());
                    } else if (filterType == HdPrimTypeTokens->displayFilter) {
                        filtersToDirty.emplace_back(path,
                            HdDisplayFilterSchema::GetDefaultLocator());
                    }                    
                }
            }
        }
        _SendPrimsAdded(entries);
        _SendPrimsDirtied(filtersToDirty);
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
        // connected filters as well.
        HdSceneIndexObserver::DirtiedPrimEntries filtersToDirty;
        for(auto const &entry : entries) {
            if (entry.dirtyLocators.Intersects(
                    HdRenderSettingsSchema::GetDefaultLocator())) {
                const HdSceneIndexPrim prim =
                    _GetInputSceneIndex()->GetPrim(entry.primPath);
                for (auto const& path : _GetConnectedOutputFilters(prim)) {
                    const HdSceneIndexPrim prim =
                        _GetInputSceneIndex()->GetPrim(path);
                    if (prim.primType == HdPrimTypeTokens->sampleFilter) {
                        filtersToDirty.emplace_back(path,
                            HdSampleFilterSchema::GetDefaultLocator());
                    } else if (prim.primType == HdPrimTypeTokens->displayFilter) {
                        filtersToDirty.emplace_back(path,
                            HdDisplayFilterSchema::GetDefaultLocator());
                    }
                }
            }

        }
        _SendPrimsDirtied(entries);
        _SendPrimsDirtied(filtersToDirty);
    }
};

}

HdSceneIndexBaseRefPtr
HdPrman_OutputFilterInvalidatingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _HdPrmanOutputFilterInvalidatingSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
