//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_IMAGING_INSTANCE_PROXY_PATH_TRANSLATION_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_INSTANCE_PROXY_PATH_TRANSLATION_SCENE_INDEX_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/dataSourceHash.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdMergingSceneIndex);
TF_DECLARE_WEAK_AND_REF_PTRS(UsdImaging_InstanceProxyPathTranslationSceneIndex);

namespace UsdImaging_InstanceProxyPathTranslationSceneIndexImpl
{
    // Forward declaration of data shared between the scene index below and the
    // wrapping prim container data source so that the latter doesn't need a
    // handle to the scene index.
    struct Data;
    using DataSharedPtr = std::shared_ptr<Data>;
}

/// \class UsdImaging_InstanceProxyPathTranslationSceneIndex
///
/// A scene index that translates SdfPath-valued data sources pointing
/// under instances to point to the corresponding prototype paths.
/// This scene index is stateless and relies on querying the input
/// scene index to perform the translation.
///
class UsdImaging_InstanceProxyPathTranslationSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    USDIMAGING_API
    static UsdImaging_InstanceProxyPathTranslationSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex,
        TfTokenVector const& proxyPathDataSourceNames);

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

private:
    UsdImaging_InstanceProxyPathTranslationSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        TfTokenVector const& proxyPathDataSourceNames);

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    UsdImaging_InstanceProxyPathTranslationSceneIndexImpl::DataSharedPtr
        _data;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
