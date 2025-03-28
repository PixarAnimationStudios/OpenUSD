//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdSt/dependencySceneIndexPlugin.h"

#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/lazyContainerDataSource.h"
#include "pxr/imaging/hd/mapContainerDataSource.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
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

    (storm_volumeFieldBindingToDependency)

    (storm_materialToMaterialBindings)
    (storm_materialBindingsToDependency)
);

static const char * const _pluginDisplayName = "GL";

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::Define<HdSt_DependencySceneIndexPlugin>();
}

TF_REGISTRY_FUNCTION(HdSceneIndexPlugin)
{
    // This scene index should be added *before*
    // HdSt_DependencyForwardingSceneIndexPlugin (which currently uses 1000).
    const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase
        = 100;

    HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
        _pluginDisplayName,
        _tokens->sceneIndexPluginName,
        nullptr,
        insertionPhase,
        HdSceneIndexPluginRegistry::InsertionOrderAtStart);
}

namespace
{

void _AddIfNecessary(const TfToken &name, TfTokenVector * const names)
{
    if (std::find(names->begin(), names->end(), name) == names->end()) {
        names->push_back(name);
    }
}

HdContainerDataSourceHandle
_ComputeMaterialBindingsDependency(
    HdContainerDataSourceHandle const &inputDs)
{
    const HdMaterialBindingsSchema materialBindings =
        HdMaterialBindingsSchema::GetFromParent(inputDs);
    
    TfToken names[2];
    HdDataSourceBaseHandle dataSources[2];
    size_t count = 0;

    static HdLocatorDataSourceHandle const materialLocDs =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdMaterialSchema::GetDefaultLocator());
    static HdLocatorDataSourceHandle const materialBindingsLocDs =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdMaterialBindingsSchema::GetDefaultLocator());

    // HdsiMaterialBindingResolvingSceneIndex already ran, so we can just
    // call GetMaterialBinding here (which uses the allPurpose binding).
    if (HdPathDataSourceHandle const pathDs =
            materialBindings.GetMaterialBinding().GetPath()) {
        if (!pathDs->GetTypedValue(0.0f).IsEmpty()) {
            HdDataSourceBaseHandle const dependencyDs =
                HdDependencySchema::Builder()
                     .SetDependedOnPrimPath(pathDs)
                     .SetDependedOnDataSourceLocator(materialLocDs)
                     .SetAffectedDataSourceLocator(materialBindingsLocDs)
                     .Build();
            names[count] = _tokens->storm_materialToMaterialBindings;
            dataSources[count] = dependencyDs;
            count++;
        }
    }

    {
        static const HdLocatorDataSourceHandle dependencyLocDs =
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdDependenciesSchema::GetDefaultLocator().Append(
                    _tokens->storm_materialToMaterialBindings));
        static HdDataSourceBaseHandle const dependencyDs =
            HdDependencySchema::Builder()
                // Prim depends on itself.
                .SetDependedOnDataSourceLocator(materialBindingsLocDs)
                .SetAffectedDataSourceLocator(dependencyLocDs)
                .Build();
        names[count] = _tokens->storm_materialBindingsToDependency;
        dataSources[count] = dependencyDs;
        count++;
    }
    
    return HdRetainedContainerDataSource::New(
        count, names, dataSources);
}
    
/// Given a prim path data source, returns a dependency of volumeFieldBinding
/// on volumeField of that given prim.
HdDataSourceBaseHandle
_ComputeVolumeFieldDependency(const HdDataSourceBaseHandle &pathDs)
{
    static HdLocatorDataSourceHandle dependedOnLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdVolumeFieldSchema::GetDefaultLocator());
    static HdLocatorDataSourceHandle affectedLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdVolumeFieldBindingSchema::GetDefaultLocator());

    return 
        HdDependencySchema::Builder()
            .SetDependedOnPrimPath(HdPathDataSource::Cast(pathDs))
            .SetDependedOnDataSourceLocator(dependedOnLocatorDataSource)
            .SetAffectedDataSourceLocator(affectedLocatorDataSource)
            .Build();
}

HdContainerDataSourceHandle
_ComputeVolumeFieldBindingDependencies(
    const HdContainerDataSourceHandle &primSource)
{
    return
        HdMapContainerDataSource::New(
            _ComputeVolumeFieldDependency,
            HdVolumeFieldBindingSchema::GetFromParent(primSource)
                .GetContainer());
}

/// Given a prim path, returns a dependency of __dependencies
/// on volumeFieldBinding of the given prim.

HdContainerDataSourceHandle
_ComputeVolumeFieldBindingDependencyDependencies()
{
    static HdLocatorDataSourceHandle dependedOnLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdVolumeFieldBindingSchema::GetDefaultLocator());
    static HdLocatorDataSourceHandle affectedLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdDependenciesSchema::GetDefaultLocator());

    return
        HdRetainedContainerDataSource::New(
            _tokens->storm_volumeFieldBindingToDependency,
            HdDependencySchema::Builder()
                .SetDependedOnDataSourceLocator(dependedOnLocatorDataSource)
                .SetAffectedDataSourceLocator(affectedLocatorDataSource)
                .Build());
}
    
class _PrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource)

    TfTokenVector GetNames() override
    {
        TfTokenVector result = _inputPrim.dataSource->GetNames();
        if ( _inputPrim.primType == HdPrimTypeTokens->volume
             || HdMaterialBindingsSchema::GetFromParent(
                 _inputPrim.dataSource)) {
            _AddIfNecessary(HdDependenciesSchema::GetSchemaToken(), &result);
        }
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdDependenciesSchema::GetSchemaToken()) {
            return _GetDependencies();
        }
        return _inputPrim.dataSource->Get(name);
    }

private:
    _PrimDataSource(
        const HdSceneIndexPrim &inputPrim)
    : _inputPrim(inputPrim)
    {
    }

    HdContainerDataSourceHandle _GetDependencies() const {
        HdContainerDataSourceHandle dataSources[10];
        size_t count = 0;

        if (HdContainerDataSourceHandle const ds =
                HdDependenciesSchema::GetFromParent(_inputPrim.dataSource)
                    .GetContainer()) {
            dataSources[count] =
                ds;
            count++;
        }

        if (_inputPrim.primType == HdPrimTypeTokens->mesh) {
            // TODO: We need to add dependencies on the material's
            // bound by geomSubset's since geomSubset's could bind a
            // material with a ptex.
            // Note that, along similar lines, adding or removing a geomSubset
            // can also mean we need to trigger the prim.
            // Unfortunately, the dependencies schema does not allow us to
            // say that we want to dirty a locator in response to child prims
            // being added or removed.
            dataSources[count] =
                _ComputeMaterialBindingsDependency(
                    _inputPrim.dataSource);
            count++;
        }
        
        if (_inputPrim.primType == HdPrimTypeTokens->volume) {
            dataSources[count] =
                _ComputeVolumeFieldBindingDependencies(
                    _inputPrim.dataSource);
            count++;
            dataSources[count] =
                _ComputeVolumeFieldBindingDependencyDependencies();
            count++;
        }

        switch(count) {
        case 0:  return nullptr;
        case 1:  return dataSources[0];
        default: return HdOverlayContainerDataSource::New(count, dataSources);
        }
    }
    
    const HdSceneIndexPrim _inputPrim;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TF_DECLARE_REF_PTRS(_SceneIndex);

/// \class _SceneIndex
///
/// The scene index that adds dependencies for volume prims.
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
        HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
        if (prim.dataSource) {
            prim.dataSource = _PrimDataSource::New(prim);
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
        SetDisplayName("HdSt: declare Storm dependencies");
    }

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override
    {
        _SendPrimsAdded(entries);
    }

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override
    {
        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override
    {
        _SendPrimsDirtied(entries);
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
