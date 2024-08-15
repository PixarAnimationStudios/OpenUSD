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
#include "pxr/imaging/hd/primvarsSchema.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/volumeFieldBindingSchema.h"
#include "pxr/imaging/hd/volumeFieldSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((sceneIndexPluginName, "HdSt_DependencySceneIndexPlugin"))
    (primvarsToMaterial)
    (primvarsToMaterialDependencyToMaterialBindings)
    (primvarsToMaterialBindings)
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

/// Given a prim path data source, returns a dependency of volumeFieldBinding
/// on volumeField of that given prim.
HdDataSourceBaseHandle
_ComputeVolumeFieldDependency(const HdDataSourceBaseHandle &src)
{
    HdDependencySchema::Builder builder;

    builder.SetDependedOnPrimPath(HdPathDataSource::Cast(src));

    static HdLocatorDataSourceHandle dependedOnLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdVolumeFieldSchema::GetDefaultLocator());
    builder.SetDependedOnDataSourceLocator(dependedOnLocatorDataSource);

    static HdLocatorDataSourceHandle affectedLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdVolumeFieldBindingSchema::GetDefaultLocator());
    builder.SetAffectedDataSourceLocator(affectedLocatorDataSource);
    return builder.Build();
}

/// Given a prim path, returns a dependency of __dependencies
/// on volumeFieldBinding of the given prim.

HdContainerDataSourceHandle
_ComputeVolumeFieldBindingDependency(const SdfPath &primPath)
{
    HdDependencySchema::Builder builder;

    builder.SetDependedOnPrimPath(
        HdRetainedTypedSampledDataSource<SdfPath>::New(
            primPath));

    static HdLocatorDataSourceHandle dependedOnLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdVolumeFieldBindingSchema::GetDefaultLocator());
    builder.SetDependedOnDataSourceLocator(dependedOnLocatorDataSource);

    static HdLocatorDataSourceHandle affectedLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdDependenciesSchema::GetDefaultLocator());
    builder.SetAffectedDataSourceLocator(affectedLocatorDataSource);

    return
        HdRetainedContainerDataSource::New(
            HdVolumeFieldBindingSchemaTokens->volumeFieldBinding,
            builder.Build());
}

HdContainerDataSourceHandle
_ComputeVolumeFieldBindingDependencies(
    const SdfPath &primPath,
    const HdContainerDataSourceHandle &primSource)
{
    return HdOverlayContainerDataSource::New(
        HdMapContainerDataSource::New(
            _ComputeVolumeFieldDependency,
            HdContainerDataSource::Cast(
                HdContainerDataSource::Get(
                    primSource,
                    HdVolumeFieldBindingSchema::GetDefaultLocator()))),
        _ComputeVolumeFieldBindingDependency(primPath));
}

/// Given a material prim path data source, returns a dependency of primvars
/// on material of that given prim.
HdContainerDataSourceHandle
_ComputePrimvarsToMaterialDependency(const SdfPath &materialPrimPath)
{
    HdDependencySchema::Builder builder;

    builder.SetDependedOnPrimPath(
        HdRetainedTypedSampledDataSource<SdfPath>::New(
            materialPrimPath));

    static HdLocatorDataSourceHandle dependedOnLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdMaterialSchema::GetDefaultLocator());
    builder.SetDependedOnDataSourceLocator(dependedOnLocatorDataSource);

    static HdLocatorDataSourceHandle affectedLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdPrimvarsSchema::GetDefaultLocator());
    builder.SetAffectedDataSourceLocator(affectedLocatorDataSource);

    return
        HdRetainedContainerDataSource::New(
            _tokens->primvarsToMaterial,
            builder.Build());
}

/// Returns a dependency of the above (primvars -> material) dependency 
/// on material bindings.
HdContainerDataSourceHandle
_ComputeDependencyToMaterialBindingsDependency()
{
    HdDependencySchema::Builder builder;

    static HdLocatorDataSourceHandle dependedOnLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdMaterialBindingsSchema::GetDefaultLocator());
    builder.SetDependedOnDataSourceLocator(dependedOnLocatorDataSource);

    static HdLocatorDataSourceHandle affectedLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdDependenciesSchema::GetDefaultLocator().Append(
                _tokens->primvarsToMaterial));
    builder.SetAffectedDataSourceLocator(affectedLocatorDataSource);

    return
        HdRetainedContainerDataSource::New(
            _tokens->primvarsToMaterialDependencyToMaterialBindings,
            builder.Build());
}

/// Returns a dependency of primvars on material bindings.
HdContainerDataSourceHandle
_ComputePrimvarsToMaterialBindingsDependency()
{
    HdDependencySchema::Builder builder;

    static HdLocatorDataSourceHandle dependedOnLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdMaterialBindingsSchema::GetDefaultLocator());
    builder.SetDependedOnDataSourceLocator(dependedOnLocatorDataSource);

    static HdLocatorDataSourceHandle affectedLocatorDataSource =
        HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
            HdPrimvarsSchema::GetDefaultLocator());
    builder.SetAffectedDataSourceLocator(affectedLocatorDataSource);

    return
        HdRetainedContainerDataSource::New(
            _tokens->primvarsToMaterialBindings,
            builder.Build());
}

HdContainerDataSourceHandle
_ComputePrimvarsToMaterialDependencies(
    const SdfPath &materialPrimPath,
    const HdContainerDataSourceHandle &primSource)
{
    return HdOverlayContainerDataSource::New(
        _ComputePrimvarsToMaterialDependency(materialPrimPath),
        _ComputeDependencyToMaterialBindingsDependency(),
        _ComputePrimvarsToMaterialBindingsDependency());
}

TF_DECLARE_REF_PTRS(_SceneIndex);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

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
        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

        HdContainerDataSourceEditor editedDs =
            HdContainerDataSourceEditor(prim.dataSource);

        if (prim.primType == HdPrimTypeTokens->volume) {
            editedDs.Overlay(
                HdDependenciesSchema::GetDefaultLocator(),
                HdLazyContainerDataSource::New(
                    std::bind(_ComputeVolumeFieldBindingDependencies,
                              primPath, prim.dataSource)));
        }
    
        // If prim has material binding, overlay dependencies from material to 
        // prim's primvars.
        if (HdPathDataSourceHandle materialPrimPathDs = _GetMaterialBindingPath(
                prim.dataSource)) {
            const SdfPath materialPrimPath =
                materialPrimPathDs->GetTypedValue(0.f);
            
            editedDs.Overlay(
                HdDependenciesSchema::GetDefaultLocator(),
                HdLazyContainerDataSource::New(
                    std::bind(_ComputePrimvarsToMaterialDependencies,
                              materialPrimPath, prim.dataSource)));
        }

        return  { prim.primType, editedDs.Finish() };
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

        _SendPrimsDirtied(entries);
    }

    HdPathDataSourceHandle
    _GetMaterialBindingPath(const HdContainerDataSourceHandle &ds) const
    {
        HdMaterialBindingsSchema materialBindings =
            HdMaterialBindingsSchema::GetFromParent(ds);
        HdMaterialBindingSchema materialBinding =
            materialBindings.GetMaterialBinding();
        return materialBinding.GetPath();
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
