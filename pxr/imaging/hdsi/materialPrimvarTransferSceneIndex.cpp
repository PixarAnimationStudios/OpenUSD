//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hdsi/materialPrimvarTransferSceneIndex.h"

#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (materialPrimvarTransfer_materialBindingsToPrimvars)
    (materialPrimvarTransfer_materialPrimvarsToPrimvars)
    (materialPrimvarTransfer_materialBindingsToDependency)
);

namespace
{

void _AddIfNecessary(const TfToken &name, TfTokenVector * const names)
{
    if (std::find(names->begin(), names->end(), name) == names->end()) {
        names->push_back(name);
    }
}

SdfPath _GetMaterialPath(const HdMaterialBindingsSchema &materialBindings)
{
    HdPathDataSourceHandle const ds =
        materialBindings
            .GetMaterialBinding()
            .GetPath();
    if (!ds) {
        return {};
    }
    return ds->GetTypedValue(0.0f);
}

class _PrimDataSource final : public HdContainerDataSource
{
public:

    HD_DECLARE_DATASOURCE(_PrimDataSource);

    TfTokenVector GetNames() override
    {
        TfTokenVector result = _inputDs->GetNames();
        if (const HdMaterialBindingsSchema materialBindings =
                HdMaterialBindingsSchema::GetFromParent(_inputDs)) {
            _AddIfNecessary(HdDependenciesSchema::GetSchemaToken(), &result);
            if (!_GetMaterialPath(materialBindings).IsEmpty()) {
                _AddIfNecessary(HdPrimvarsSchema::GetSchemaToken(), &result);
            }
        }
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        HdDataSourceBaseHandle const ds = _inputDs->Get(name);

        if (name == HdPrimvarsSchema::GetSchemaToken()) {
            return
                HdOverlayContainerDataSource::OverlayedContainerDataSources(
                    // local primvars have stronger opinion
                    HdContainerDataSource::Cast(ds),
                    _GetPrimvarsFromMaterial());
        }
        if (name == HdDependenciesSchema::GetSchemaToken()) {
            return
                HdOverlayContainerDataSource::OverlayedContainerDataSources(
                    HdContainerDataSource::Cast(ds),
                    _GetDependencies());
        }
        return ds;
    }

private:
    _PrimDataSource(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputDs)
    : _inputScene(inputScene)
    , _inputDs(inputDs)
    {
    }

    HdContainerDataSourceHandle _GetPrimvarsFromMaterial() const {
        const SdfPath materialPath = _GetMaterialPath(
            HdMaterialBindingsSchema::GetFromParent(_inputDs));
        if (materialPath.IsEmpty()) {
            return nullptr;
        }
        const HdSceneIndexPrim materialPrim =
            _inputScene->GetPrim(materialPath);
        return
            HdPrimvarsSchema::GetFromParent(materialPrim.dataSource)
                .GetContainer();
    }

    HdContainerDataSourceHandle _GetDependencies() const {
        const HdMaterialBindingsSchema materialBindings =
            HdMaterialBindingsSchema::GetFromParent(_inputDs);
        if (!materialBindings) {
            // Note: while we support the case that the content of the
            // container data source at materialBindings change, we do
            // not support the case where a prim started with no
            // data source for materialBindings and adds that data source
            // later.
            return nullptr;
        }

        // We need to create three dependencies here:
        // 1) Our primvars potentially depend on the value of the
        //    material binding changing
        // 2) Our primvars depend on the primvars of the bound material
        //    prim.
        // 3) Dependency 2) itself depends on the value of the bound
        //    material prim!

        TfToken names[3];
        HdDataSourceBaseHandle dataSources[3];
        size_t count = 0;

        static HdLocatorDataSourceHandle const materialBindingsLocDs =
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdMaterialBindingsSchema::GetDefaultLocator());
        static HdLocatorDataSourceHandle const primvarsLocDs =
            HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                HdPrimvarsSchema::GetDefaultLocator());

        {
            static HdDataSourceBaseHandle const dependencyDs =
                HdDependencySchema::Builder()
                    // Prim depends on itself.
                    .SetDependedOnDataSourceLocator(materialBindingsLocDs)
                    .SetAffectedDataSourceLocator(primvarsLocDs)
                    .Build();
            names[count] =
                _tokens->materialPrimvarTransfer_materialBindingsToPrimvars;
            dataSources[count] =
                dependencyDs;
            count++;
        }

        if (HdPathDataSourceHandle const pathDs =
                materialBindings.GetMaterialBinding().GetPath()) {
            if (!pathDs->GetTypedValue(0.0f).IsEmpty()) {
                HdDataSourceBaseHandle const dependencyDs =
                    HdDependencySchema::Builder()
                        .SetDependedOnPrimPath(pathDs)
                        .SetDependedOnDataSourceLocator(primvarsLocDs)
                        .SetAffectedDataSourceLocator(primvarsLocDs)
                        .Build();
                names[count] =
                    _tokens->materialPrimvarTransfer_materialPrimvarsToPrimvars;
                dataSources[count] =
                    dependencyDs;
                count++;
            }
        }

        {
            static const HdLocatorDataSourceHandle dependencyLocDs =
                HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                    HdDependenciesSchema::GetDefaultLocator().Append(
                        _tokens->
                            materialPrimvarTransfer_materialPrimvarsToPrimvars
                        ));

            static HdDataSourceBaseHandle const dependencyDs =
                HdDependencySchema::Builder()
                    // Prim depends on itself.
                    .SetDependedOnDataSourceLocator(materialBindingsLocDs)
                    .SetAffectedDataSourceLocator(dependencyLocDs)
                    .Build();
            names[count] =
                _tokens->materialPrimvarTransfer_materialBindingsToDependency;
            dataSources[count] =
                dependencyDs;
            count++;
        }

        return HdRetainedContainerDataSource::New(
            count, names, dataSources);
    }

    HdSceneIndexBaseRefPtr const _inputScene;
    HdContainerDataSourceHandle const _inputDs;
};

} // anonymous namespace

// ----------------------------------------------------------------------------

HdsiMaterialPrimvarTransferSceneIndex::HdsiMaterialPrimvarTransferSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{}

HdsiMaterialPrimvarTransferSceneIndex::~HdsiMaterialPrimvarTransferSceneIndex()
    = default;

HdsiMaterialPrimvarTransferSceneIndexRefPtr
HdsiMaterialPrimvarTransferSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(new HdsiMaterialPrimvarTransferSceneIndex(
        inputSceneIndex));
}

HdSceneIndexPrim
HdsiMaterialPrimvarTransferSceneIndex::GetPrim(const SdfPath& primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    // won't have any bindings if we don't have a data source
    if (prim.dataSource) {
        prim.dataSource =
            _PrimDataSource::New(_GetInputSceneIndex(), prim.dataSource);
    }
    return prim;
}

SdfPathVector
HdsiMaterialPrimvarTransferSceneIndex::GetChildPrimPaths(
    const SdfPath& primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiMaterialPrimvarTransferSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    _SendPrimsAdded(entries);
}

void
HdsiMaterialPrimvarTransferSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);
}

void
HdsiMaterialPrimvarTransferSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
