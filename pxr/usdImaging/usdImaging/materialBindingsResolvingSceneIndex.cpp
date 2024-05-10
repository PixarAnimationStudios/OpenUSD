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

#include "pxr/usdImaging/usdImaging/materialBindingsResolvingSceneIndex.h"

#include "pxr/usdImaging/usdImaging/collectionMaterialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/collectionMaterialBindingsSchema.h"
#include "pxr/usdImaging/usdImaging/directMaterialBindingSchema.h"
#include "pxr/usdImaging/usdImaging/directMaterialBindingsSchema.h"

#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

bool
_HasDirectOrCollectionMaterialBindings(
    const HdContainerDataSourceHandle &c)
{
    return  UsdImagingDirectMaterialBindingsSchema::GetFromParent(c) ||
            UsdImagingCollectionMaterialBindingsSchema::GetFromParent(c);
}


// Container that computes the resolved material binding from the flattened 
// direct material bindings.
//
// XXX The flattened direct binding is returned as the resolved binding.
//     This needs to be updated to factor collection bindings.
// 
class _HdMaterialBindingsDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_HdMaterialBindingsDataSource);

    _HdMaterialBindingsDataSource(
        const HdContainerDataSourceHandle &primContainer,
        const HdSceneIndexBaseRefPtr &si,
        const SdfPath &primPath)
    : _primContainer(primContainer)
    , _si(si)
    , _primPath(primPath)
    {}

    TfTokenVector
    GetNames() override
    {
        // For now, simply return the purposes available on the flattened
        // direct material bindings.
        // XXX This should be reworked to factor collection bindings.
        return _GetAvailableDirectBindingPurposes();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        const TfToken &purpose = name;

        UsdImagingDirectMaterialBindingsSchema dirBindingsSchema =
            UsdImagingDirectMaterialBindingsSchema::GetFromParent(
                _primContainer);
        
        UsdImagingDirectMaterialBindingSchema dirBindingSchema =
            dirBindingsSchema.GetDirectMaterialBinding(purpose);
        
        if (HdPathDataSourceHandle pathDs =
                dirBindingSchema.GetMaterialPath()) {
            // XXX This should be reworked to factor collection bindings.
            const SdfPath resolvedMaterialPath = pathDs->GetTypedValue(0.0);

            return _BuildHdMaterialBindingDataSource(resolvedMaterialPath);
        }

        return nullptr;
    }

private:

    TfTokenVector
    _GetAvailableDirectBindingPurposes() const
    {
        UsdImagingDirectMaterialBindingsSchema dirBindingsSchema =
            UsdImagingDirectMaterialBindingsSchema::GetFromParent(
                _primContainer);
        
        return dirBindingsSchema.GetPurposes();
    }

    HdDataSourceBaseHandle
    _BuildHdMaterialBindingDataSource(
        const SdfPath &resolvedMaterialPath)
    {
        return
            HdMaterialBindingSchema::Builder()
            .SetPath(HdRetainedTypedSampledDataSource<SdfPath>::New(
                resolvedMaterialPath))
            .Build();
    }

private:
    HdContainerDataSourceHandle _primContainer;
    const HdSceneIndexBaseRefPtr _si; // currently unused, but will be used for
                                      // collection membership queries.
    const SdfPath _primPath;
};

class _MeshDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MeshDataSource);

    _MeshDataSource(
        const HdContainerDataSourceHandle &meshContainer,
        const HdSceneIndexBaseRefPtr &si,
        const SdfPath &primPath)
    : _meshContainer(meshContainer)
    , _si(si)
    , _primPath(primPath)
    {}

    TfTokenVector
    GetNames() override
    {
        return _meshContainer->GetNames();
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _meshContainer->Get(name);

        return result;
    }

private:
    HdContainerDataSourceHandle _meshContainer;
    const HdSceneIndexBaseRefPtr _si;
    const SdfPath _primPath;
};

// Prim container override that provides the resolved hydra material bindings
// if direct or collection USD material bindings are present.
// 
class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(
        const HdContainerDataSourceHandle &primContainer,
        const HdSceneIndexBaseRefPtr &si,
        const SdfPath &primPath)
    : _primContainer(primContainer)
    , _si(si)
    , _primPath(primPath)
    {}

    TfTokenVector
    GetNames() override
    {
        TfTokenVector names = _primContainer->GetNames();
        names.push_back(HdMaterialBindingsSchema::GetSchemaToken());
        return names;
    }

    HdDataSourceBaseHandle
    Get(const TfToken &name) override
    {
        HdDataSourceBaseHandle result = _primContainer->Get(name);

        // Material bindings on the prim.
        if (name == HdMaterialBindingsSchema::GetSchemaToken()) {

            // Check if we have direct or collection material bindings to
            // avoid returning an empty non-null container.
            if (_HasDirectOrCollectionMaterialBindings(_primContainer)) {
                // We don't expect to have hydra material bindings on the
                // prim container. Use an overlay just in case such that the
                // existing opinion wins.
                return
                    HdOverlayContainerDataSource::New(
                        HdContainerDataSource::Cast(result),
                        _HdMaterialBindingsDataSource::New(
                            _primContainer, _si, _primPath));
            }
        }

        return result;
    }

private:
    HdContainerDataSourceHandle _primContainer;
    const HdSceneIndexBaseRefPtr _si;
    const SdfPath _primPath;
};

}

// -----------------------------------------------------------------------------
// UsdImagingMaterialBindingsResolvingSceneIndex
// -----------------------------------------------------------------------------

UsdImagingMaterialBindingsResolvingSceneIndexRefPtr
UsdImagingMaterialBindingsResolvingSceneIndex::New(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
{
    return TfCreateRefPtr(
        new UsdImagingMaterialBindingsResolvingSceneIndex(
            inputSceneIndex, inputArgs));
}

UsdImagingMaterialBindingsResolvingSceneIndex::
UsdImagingMaterialBindingsResolvingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdContainerDataSourceHandle &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

UsdImagingMaterialBindingsResolvingSceneIndex::
~UsdImagingMaterialBindingsResolvingSceneIndex() = default;

HdSceneIndexPrim
UsdImagingMaterialBindingsResolvingSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    // Wrap the prim container to provide the resolved hydra bindings via
    // the "materialBindings" locator.
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (prim.dataSource) {
        prim.dataSource =
            _PrimDataSource::New(
                prim.dataSource, _GetInputSceneIndex(), primPath);
    }

    return prim;
}

SdfPathVector
UsdImagingMaterialBindingsResolvingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    // This scene index does not mutate the topology.
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
UsdImagingMaterialBindingsResolvingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // For now, just forward the notices. We could suppress notices
    // for USD material bindings schemata locators since scene indices
    // downstream shouldn't be interested in these notices.
    //
    // Additional processing may be required here to support collection material
    // bindings (e.g., discover collections targeted by bindings).
    //
    _SendPrimsAdded(entries);
}

void
UsdImagingMaterialBindingsResolvingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // Comments above in _PrimsAdded are relevant here.
    _SendPrimsRemoved(entries);
}

void
UsdImagingMaterialBindingsResolvingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();
    
    static const HdDataSourceLocatorSet usdMaterialBindingLocators =
    {
        UsdImagingDirectMaterialBindingsSchema::GetDefaultLocator(),
        UsdImagingCollectionMaterialBindingsSchema::GetDefaultLocator(),
    };

    // Check if the notice entries can be forwarded as-is.
    bool hasDirtyUsdMaterialBindings = false;
    for (auto const &entry : entries) {
        if (entry.dirtyLocators.Intersects(usdMaterialBindingLocators)) {
            hasDirtyUsdMaterialBindings = true;
            break;
        }
    }

    if (!hasDirtyUsdMaterialBindings) {
        _SendPrimsDirtied(entries);
        return;
    }

    // Transform dirty notices for USD material bindings into ones for
    // Hydra material bindings. This effectively suppresses the former notices,
    // which is fine because downstream consumers should work off the
    // Hydra material binding notices.
    //
    HdSceneIndexObserver::DirtiedPrimEntries newEntries;
    for (auto const &entry : entries) {
        if (entry.dirtyLocators.Intersects(usdMaterialBindingLocators)) {

            HdDataSourceLocatorSet newLocators(entry.dirtyLocators);
            newLocators = newLocators.ReplacePrefix(
                UsdImagingDirectMaterialBindingsSchema::GetDefaultLocator(),
                HdMaterialBindingsSchema::GetDefaultLocator());
            newLocators = newLocators.ReplacePrefix(
                UsdImagingCollectionMaterialBindingsSchema::GetDefaultLocator(),
                HdMaterialBindingsSchema::GetDefaultLocator());
            newEntries.push_back({entry.primPath, newLocators});
        } else {
            newEntries.push_back(entry);
        }
    }

    _SendPrimsDirtied(newEntries);
}


PXR_NAMESPACE_CLOSE_SCOPE
