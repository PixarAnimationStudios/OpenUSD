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

#include "hdPrman/renderTerminalOutputInvalidatingSceneIndexPlugin.h"
#include "hdPrman/debugCodes.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/integratorSchema.h"
#include "pxr/imaging/hd/sampleFilterSchema.h"
#include "pxr/imaging/hd/displayFilterSchema.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin"))
    ((outputsRiIntegrator, "outputs:ri:integrator"))
    ((outputsRiSampleFilters, "outputs:ri:sampleFilters"))
    ((outputsRiDisplayFilters, "outputs:ri:displayFilters"))
);

static const char * const _pluginDisplayName = "Prman";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<
        HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin>();
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

HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin::
HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin() = default;


namespace
{

VtArray<SdfPath>
_GetConnectedOutputs(const HdSceneIndexPrim &prim)
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

    const TfToken outputTokens[] = {
        _tokens->outputsRiIntegrator,
        _tokens->outputsRiSampleFilters,
        _tokens->outputsRiDisplayFilters
    };

    VtArray<SdfPath> connectedOutputs;
    for (const auto& outputToken : outputTokens) {
        const HdSampledDataSourceHandle valueDs =
            HdSampledDataSource::Cast(namespacedSettingsDS->Get(outputToken));
        if (!valueDs) {
            continue;
        }
        const VtValue pathsValue = valueDs->GetValue(0);
        const SdfPathVector paths = pathsValue.GetWithDefault<SdfPathVector>();
        for (const auto& path : paths) {
            connectedOutputs.push_back(path);
        }
    }
    
    return connectedOutputs;
}

TF_DECLARE_REF_PTRS(_HdPrmanRenderTerminalOutputFnvalidatingSceneIndex);

////////////////////////////////////////////////////////////////////////////////
/// \class _HdPrmanRenderTerminalOutputFnvalidatingSceneIndex
///
/// The scene index feeding into HdDependencyForwardingSceneIndex and
/// constructed by the HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin.
///

class _HdPrmanRenderTerminalOutputFnvalidatingSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _HdPrmanRenderTerminalOutputFnvalidatingSceneIndexRefPtr
        New(const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new _HdPrmanRenderTerminalOutputFnvalidatingSceneIndex(
                inputSceneIndex));
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
    _HdPrmanRenderTerminalOutputFnvalidatingSceneIndex(
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
        
        // RenderSettings are added when the connected terminals change,
        // dirty these terminal outputs to make sure we get the correct visual
        HdSceneIndexObserver::DirtiedPrimEntries outputsToDirty;
        for(auto const& entry : entries) {
            if (entry.primType == HdPrimTypeTokens->renderSettings) {
                const HdSceneIndexPrim prim =
                    _GetInputSceneIndex()->GetPrim(entry.primPath);
                for (auto const& path : _GetConnectedOutputs(prim)) {
                    const TfToken outputType = 
                        _GetInputSceneIndex()->GetPrim(path).primType;
                    if (outputType == HdPrimTypeTokens->integrator) {
                        outputsToDirty.emplace_back(path, 
                            HdIntegratorSchema::GetDefaultLocator());
                    } else if (outputType == HdPrimTypeTokens->sampleFilter) {
                        outputsToDirty.emplace_back(path, 
                            HdSampleFilterSchema::GetDefaultLocator());
                    } else if (outputType == HdPrimTypeTokens->displayFilter) {
                        outputsToDirty.emplace_back(path,
                            HdDisplayFilterSchema::GetDefaultLocator());

                    }
                }
            }
        }
        _SendPrimsAdded(entries);
        _SendPrimsDirtied(outputsToDirty);
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

        // When the Namespaced Settings on a RenderSettings prim is dirtied 
        // make sure to dirty the connected render terminal outputs as well.
        HdSceneIndexObserver::DirtiedPrimEntries outputsToDirty;
        for(auto const &entry : entries) {
            if (entry.dirtyLocators.Intersects(
                    HdRenderSettingsSchema::GetNamespacedSettingsLocator())) {
                const HdSceneIndexPrim prim =
                    _GetInputSceneIndex()->GetPrim(entry.primPath);
                for (auto const& path : _GetConnectedOutputs(prim)) {
                    const HdSceneIndexPrim prim =
                        _GetInputSceneIndex()->GetPrim(path);
                    if (prim.primType == HdPrimTypeTokens->integrator) {
                        outputsToDirty.emplace_back(path, 
                            HdIntegratorSchema::GetDefaultLocator());
                    } else if (prim.primType == HdPrimTypeTokens->sampleFilter) {
                        outputsToDirty.emplace_back(path,
                            HdSampleFilterSchema::GetDefaultLocator());
                    } else if (prim.primType == HdPrimTypeTokens->displayFilter) {
                        outputsToDirty.emplace_back(path,
                            HdDisplayFilterSchema::GetDefaultLocator());
                    }
                }
            }

        }
        _SendPrimsDirtied(entries);
        _SendPrimsDirtied(outputsToDirty);
    }
};

}

HdSceneIndexBaseRefPtr
HdPrman_RenderTerminalOutputInvalidatingSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _HdPrmanRenderTerminalOutputFnvalidatingSceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
