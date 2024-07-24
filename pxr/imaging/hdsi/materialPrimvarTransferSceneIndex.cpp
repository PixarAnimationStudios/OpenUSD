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

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (materialPrimvarTransfer_dep)
    (materialPrimvarTransfer_primvarsToBinding)
    (materialPrimvarTransfer_primvarsToMaterial)
);

namespace
{

class _PrimDataSource final : public HdContainerDataSource
{
public:

    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputDs)
    : _inputScene(inputScene)
    , _inputDs(inputDs)
    {
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector result;
        if (_inputDs) {
            result = _inputDs->GetNames();
            if (HdMaterialBindingsSchema::GetFromParent(_inputDs)) {
                for (const TfToken &name : {
                        HdPrimvarsSchema::GetSchemaToken(),
                        HdDependenciesSchema::GetSchemaToken() }) {
                    if (std::find(result.begin(), result.end(), name)
                            == result.end()) {
                        result.push_back(name);
                    }
                }
            }
        }
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        HdDataSourceBaseHandle inputResult;
        if (_inputDs) {
            inputResult = _inputDs->Get(name);
        }

        if (name == HdDependenciesSchema::GetSchemaToken()) {
            if (HdPathDataSourceHandle pathDs = _GetMaterialBindingPath()) {

                // We need to create three dependencies here:
                // 1) Our primvars potentially depend on the value of the
                //    material binding changing
                // 2) Our primvars depend on the primvars of the bound material
                //    prim.
                // 3) Dependency (2) itself depends on the value of the bound
                //    material prim!

                const static HdLocatorDataSourceHandle primvarsLocDs =
                    HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                            HdPrimvarsSchema::GetDefaultLocator());

                const static HdLocatorDataSourceHandle materialBindingsLocDs =
                    HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                            HdMaterialBindingsSchema::GetDefaultLocator());

                static const HdLocatorDataSourceHandle primvarsToMaterialLocDs =
                    HdRetainedTypedSampledDataSource<HdDataSourceLocator>::New(
                        HdDependenciesSchema::GetDefaultLocator()
                            .Append(_tokens->
                                materialPrimvarTransfer_primvarsToMaterial));

                HdContainerDataSourceHandle depsDs =
                    HdRetainedContainerDataSource::New(
                        // primvars -> bound material primvars
                        _tokens->materialPrimvarTransfer_primvarsToMaterial,
                        HdDependencySchema::Builder()
                            .SetDependedOnPrimPath(pathDs)
                            .SetDependedOnDataSourceLocator(primvarsLocDs)
                            .SetAffectedDataSourceLocator(primvarsLocDs)
                            .Build(),

                        // above dependency -> material binding
                        _tokens->materialPrimvarTransfer_dep,
                        HdDependencySchema::Builder()
                            .SetDependedOnPrimPath(nullptr) // self
                            .SetDependedOnDataSourceLocator(
                                materialBindingsLocDs)
                            .SetAffectedDataSourceLocator(primvarsLocDs)
                            .Build(),

                        // primvars -> material binding
                        _tokens->materialPrimvarTransfer_primvarsToBinding,
                        HdDependencySchema::Builder()
                            .SetDependedOnPrimPath(nullptr) // self
                            .SetDependedOnDataSourceLocator(
                                materialBindingsLocDs)
                            .SetAffectedDataSourceLocator(
                                primvarsToMaterialLocDs)
                            .Build());

                if (HdContainerDataSourceHandle inputResultContainer =
                        HdContainerDataSource::Cast(inputResult)) {
                    return HdOverlayContainerDataSource::New(
                        depsDs, inputResultContainer);
                } else {
                    return depsDs;
                }
            }
        } else if (name == HdPrimvarsSchema::GetSchemaToken()) {

            if (HdPathDataSourceHandle pathDs = _GetMaterialBindingPath()) {
                const SdfPath materialPath = pathDs->GetTypedValue(0.0f);
                HdSceneIndexPrim prim = _inputScene->GetPrim(materialPath);

                if (HdPrimvarsSchema pvSchema =
                        HdPrimvarsSchema::GetFromParent(prim.dataSource))
                {
                    if (HdContainerDataSourceHandle inputResultContainer =
                            HdContainerDataSource::Cast(inputResult)) {
                        // local primvars have stronger opinion
                        return HdOverlayContainerDataSource::New(
                            inputResultContainer, pvSchema.GetContainer());
                    } else {
                        return pvSchema.GetContainer();
                    }
                }
            }
        }
        return inputResult;
    }

private:

    HdPathDataSourceHandle _GetMaterialBindingPath()
    {
        HdMaterialBindingsSchema materialBindings =
            HdMaterialBindingsSchema::GetFromParent(_inputDs);
        HdMaterialBindingSchema materialBinding =
            materialBindings.GetMaterialBinding();
        return materialBinding.GetPath();
    }

    HdSceneIndexBaseRefPtr _inputScene;
    HdContainerDataSourceHandle _inputDs;
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
    if (HdSceneIndexBaseRefPtr input = _GetInputSceneIndex()) {
        HdSceneIndexPrim prim = input->GetPrim(primPath);

        // won't have any bindings if we don't have a data source
        if (prim.dataSource) {
            prim.dataSource = _PrimDataSource::New(input, prim.dataSource);
        }
        return prim;
    }
    return { TfToken(), nullptr };
}

SdfPathVector
HdsiMaterialPrimvarTransferSceneIndex::GetChildPrimPaths(
    const SdfPath& primPath) const
{
    if (HdSceneIndexBaseRefPtr input = _GetInputSceneIndex()) {
        return input->GetChildPrimPaths(primPath);
    }
    return {};
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
