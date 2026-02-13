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

class _PrimvarsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimvarsDataSource);

    TfTokenVector GetNames() override
    {
        HdContainerDataSourceHandle mpds = _GetPrimvarsFromMaterial();
        // Null checks.
        if (!_primvarsDs && !mpds) {
            return TfTokenVector();
        } else if (!mpds) {
            return _primvarsDs->GetNames();
        } else if (!_primvarsDs) {
            return mpds->GetNames();
        }

        TfTokenVector primvarsNames = _primvarsDs->GetNames();
        TfTokenVector mpdsNames = mpds->GetNames();

        // If one or the other is empty.
        if (mpdsNames.empty()) {
            return primvarsNames;
        } else if (primvarsNames.empty()) {
            return mpdsNames;
        }

        // Otherwise, union them.
        TfDenseHashSet<TfToken, TfToken::HashFunctor> names;
        names.insert(primvarsNames.begin(), primvarsNames.end());
        names.insert(mpdsNames.begin(), mpdsNames.end());
        return TfTokenVector(names.begin(), names.end());
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        // Use the compose function if provided.
        // If not provided, we use the code path below, which avoids
        // calling _GetPrimvarsFromMaterial() until necessary.
        if (_composeFn) {
            return _composeFn(_primvarsDs, _GetPrimvarsFromMaterial(), name);
        }
        if (HdDataSourceBaseHandle primvar = 
                _primvarsDs ? _primvarsDs->Get(name) : nullptr) {
            return primvar;
        }
        if (HdContainerDataSourceHandle mpds = _GetPrimvarsFromMaterial()) {
            return mpds->Get(name);
        }
        return nullptr;
    }

private:
    _PrimvarsDataSource(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &primDs,
        const HdContainerDataSourceHandle &primvarsDs,
        const HdsiMaterialPrimvarTransferSceneIndex::ComposeFn &composeFn)
    : _inputScene(inputScene)
    , _primDs(primDs)
    , _primvarsDs(primvarsDs)
    , _composeFn(composeFn)
    {
    }

    HdContainerDataSourceHandle _GetPrimvarsFromMaterial() {
        HdContainerDataSourceHandle mpds =
            HdContainerDataSource::AtomicLoad(_materialPrimvarsDs);
        if (mpds) {
            return mpds;
        }

        const SdfPath materialPath = _GetMaterialPath(
            HdMaterialBindingsSchema::GetFromParent(_primDs));
        if (materialPath.IsEmpty()) {
            return nullptr;
        }
        const HdSceneIndexPrim materialPrim =
            _inputScene->GetPrim(materialPath);
        mpds = HdPrimvarsSchema::GetFromParent(materialPrim.dataSource)
                .GetContainer();

        HdContainerDataSource::AtomicStore(_materialPrimvarsDs, mpds);
        return mpds;
    }

    HdSceneIndexBaseRefPtr const _inputScene;
    HdContainerDataSourceHandle const _primDs;
    HdContainerDataSourceHandle const _primvarsDs;
    HdContainerDataSourceAtomicHandle _materialPrimvarsDs;
    HdsiMaterialPrimvarTransferSceneIndex::ComposeFn _composeFn;
};

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
            // Note that the "primvars" container we pass in might be null...
            return _PrimvarsDataSource::New(_inputScene, _inputDs,
                HdContainerDataSource::Cast(ds), _composeFn);
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
        const HdContainerDataSourceHandle &inputDs,
        const HdsiMaterialPrimvarTransferSceneIndex::ComposeFn &composeFn)
    : _inputScene(inputScene)
    , _inputDs(inputDs)
    , _composeFn(composeFn)
    {
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
    HdsiMaterialPrimvarTransferSceneIndex::ComposeFn _composeFn;
};

} // anonymous namespace

// ----------------------------------------------------------------------------



HdsiMaterialPrimvarTransferSceneIndex::HdsiMaterialPrimvarTransferSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const ComposeFn& composeFn)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
, _composeFn(composeFn)
{}

HdsiMaterialPrimvarTransferSceneIndex::~HdsiMaterialPrimvarTransferSceneIndex()
    = default;

HdDataSourceBaseHandle
HdsiMaterialPrimvarTransferSceneIndex::DefaultComposeFn(
    HdContainerDataSourceHandle const& geometryPrimvarDs,
    HdContainerDataSourceHandle const& materialPrimvarDs,
    const TfToken& name)
{
    if (geometryPrimvarDs) {
        if (HdDataSourceBaseHandle v = geometryPrimvarDs->Get(name)) {
            return v;
        }
    }
    if (materialPrimvarDs) {
        if (HdDataSourceBaseHandle v = materialPrimvarDs->Get(name)) {
            return v;
        }
    }
    return nullptr;
}

HdsiMaterialPrimvarTransferSceneIndexRefPtr
HdsiMaterialPrimvarTransferSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex,
    const ComposeFn& composeFn)
{
    return TfCreateRefPtr(new HdsiMaterialPrimvarTransferSceneIndex(
        inputSceneIndex, composeFn));
}

HdSceneIndexPrim
HdsiMaterialPrimvarTransferSceneIndex::GetPrim(const SdfPath& primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    // won't have any bindings if we don't have a data source
    if (prim) {
        prim.dataSource =
            _PrimDataSource::New(_GetInputSceneIndex(), prim.dataSource,
                                 _composeFn);
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
