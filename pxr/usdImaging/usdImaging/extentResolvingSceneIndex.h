//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_USD_IMAGING_USD_IMAGING_EXTENT_RESOLVING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_EXTENT_RESOLVING_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImagingExtentResolvingSceneIndex_Impl
{
using _InfoSharedPtr = std::shared_ptr<struct _Info>;
}

#define USDIMAGINGEXTENTRESOLVINGSCENEINDEX_TOKENS \
    (purposes)

TF_DECLARE_PUBLIC_TOKENS(UsdImagingExtentResolvingSceneIndexTokens,
                         USDIMAGING_API,
                         USDIMAGINGEXTENTRESOLVINGSCENEINDEX_TOKENS);

TF_DECLARE_REF_PTRS(UsdImagingExtentResolvingSceneIndex);

/// \class UsdImagingExtentResolvingSceneIndex
///
/// A scene index that uses UsdGeomModelAPI's extentsHint if
/// UsdGeomBoundable's extent has not been authored.
///
/// TODO: The UsdStageSceneIndex should consult the
/// UsdGeomComputeExtentFunction and this scene index should use it.
///
class UsdImagingExtentResolvingSceneIndex
        : public HdSingleInputFilteringSceneIndexBase
{
public:
    
    // Datasource purposes at inputArgs is supposed to be a vector data source
    // of token data sources. These tokens are hydra purposes (in particular,
    // use HdTokens->geometry rather than the corresponding
    // UsdGeomTokens->default_).
    USDIMAGING_API
    static UsdImagingExtentResolvingSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);

    USDIMAGING_API
    ~UsdImagingExtentResolvingSceneIndex() override;
    
    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    UsdImagingExtentResolvingSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);

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
    UsdImagingExtentResolvingSceneIndex_Impl::_InfoSharedPtr _info;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_EXTENT_RESOLVING_SCENE_INDEX_H
