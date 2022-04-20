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

#include "pxr/imaging/hdSt/dependencySceneIndexPlugin.h"

#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/volumeFieldBindingSchema.h"
#include "pxr/imaging/hd/volumeFieldSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_DependencySceneIndexPlugin"))
);

static const char * const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdSt_DependencySceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

namespace
{

/// Computes the dependencies for the volumeFieldBindings of a volume
/// prim. They need to be returned when the data source locator __dependencies
/// is querried.
HdRetainedContainerDataSourceHandle
_ComputeVolumeFieldBindingDependencies(HdContainerDataSourceHandle &primSource)
{
    HD_TRACE_FUNCTION();

    HdVolumeFieldBindingSchema schema =
        HdVolumeFieldBindingSchema::GetFromParent(primSource);

    const TfTokenVector names = schema.GetVolumeFieldBindingNames(); 
    std::vector<HdDataSourceBaseHandle> dependencies;
    dependencies.reserve(names.size());

    for (const TfToken &name : names) {
        HdDependencySchema::Builder builder;
        builder.SetDependedOnPrimPath(
            schema.GetVolumeFieldBinding(name));
        builder.SetDependedOnDataSourceLocator(
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdVolumeFieldSchema::GetDefaultLocator()));
        builder.SetAffectedDataSourceLocator(
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdVolumeFieldBindingSchema::GetDefaultLocator()));
        dependencies.push_back(builder.Build());
    }
    
    return HdRetainedContainerDataSource::New(
        names.size(), names.data(), dependencies.data());
}

/// Data source adding __dependencies given the data source of a
/// volume.
class _VolumePrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_VolumePrimDataSource);

    _VolumePrimDataSource(
        const HdContainerDataSourceHandle &primSource)
      : _primSource(primSource)
    {
    }

    bool Has(const TfToken &name) override
    {
        if (!_primSource) {
            return false;
        }

        if (name == HdDependenciesSchemaTokens->__dependencies) {
            return true;
        }

        return _primSource->Has(name);
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector result;

        if (!_primSource) {
            return result;
        }

        result = _primSource->GetNames();
        result.push_back(HdDependenciesSchemaTokens->__dependencies);

        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (!_primSource) {
            return nullptr;
        }

        HdDataSourceBaseHandle src = _primSource->Get(name);

        if (name == HdDependenciesSchemaTokens->__dependencies) {
            HdContainerDataSourceHandle sources[] = {
                _ComputeVolumeFieldBindingDependencies(_primSource),
                HdContainerDataSource::Cast(src) };

            return HdOverlayContainerDataSource::New(
                TfArraySize(sources),
                sources);
        }
        return std::move(src);
    }

private:
    HdContainerDataSourceHandle _primSource;
};

HD_DECLARE_DATASOURCE_HANDLES(_VolumePrimDataSource);

TF_DECLARE_REF_PTRS(_SceneIndex);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/// \class _SceneIndex
///
/// The scene index feeding into HdDependencyForwardingSceneIndex constructed by
/// by the HdSt_DependencySceneIndexPlugin.
///
class _SceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    static _SceneIndexRefPtr New(
        const HdSceneIndexBaseRefPtr &inputSceneIndex)
    {
        return TfCreateRefPtr(new _SceneIndex(inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override
    {
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (prim.primType == HdPrimTypeTokens->volume) {
            return
                { prim.primType, _VolumePrimDataSource::New(prim.dataSource) };
        }
        return prim;
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override
    {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }

protected:
    _SceneIndex(
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

        _SendPrimsAdded(entries);
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

        // If the volumeFieldBinding locator is dirtied, we also need to
        // dirty the __dependencies locator.

        std::vector<size_t> indices;
        for (size_t i = 0; i < entries.size(); i++) {
            const HdDataSourceLocatorSet &locators = entries[i].dirtyLocators;
            if (locators.Intersects(
                    HdVolumeFieldBindingSchema::GetDefaultLocator())) {
                indices.push_back(i);
            }
        }
        
        if (indices.empty()) {
            _SendPrimsDirtied(entries);
            return;
        }

        HdSceneIndexObserver::DirtiedPrimEntries newEntries(entries);
        for (const size_t i : indices) {
            newEntries[i].dirtyLocators.insert(
                HdDependenciesSchema::GetDefaultLocator());
        }

        _SendPrimsDirtied(newEntries);
    }
};

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Implementation of HdSt_DependencySceneIndexPlugin

HdSt_DependencySceneIndexPlugin::HdSt_DependencySceneIndexPlugin() = default;

HdSceneIndexBaseRefPtr
HdSt_DependencySceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene,
    const HdContainerDataSourceHandle &inputArgs)
{
    return _SceneIndex::New(inputScene);
}

PXR_NAMESPACE_CLOSE_SCOPE
