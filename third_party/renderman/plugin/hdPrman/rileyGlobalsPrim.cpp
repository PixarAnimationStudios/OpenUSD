//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
////////////////////////////////////////////////////////////////////////

#include "hdPrman/rileyGlobalsPrim.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileyTypes.h"

#include "hdPrman/rileyGlobalsSchema.h"

#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_RileyGlobalsPrim::HdPrman_RileyGlobalsPrim(
    HdContainerDataSourceHandle const &primSource,
    const HdsiPrimManagingSceneIndexObserver * const observer,
    HdPrman_RenderParam * const renderParam)
  : HdPrman_RileyPrimBase(renderParam)
{
    HdPrmanRileyGlobalsSchema schema =
        HdPrmanRileyGlobalsSchema::GetFromParent(primSource);

    _SetOptions(schema);
}

void
HdPrman_RileyGlobalsPrim::_Dirty(
    const HdSceneIndexObserver::DirtiedPrimEntry &entry,
    const HdsiPrimManagingSceneIndexObserver * const observer)
{
    HdPrmanRileyGlobalsSchema schema =
        HdPrmanRileyGlobalsSchema::GetFromParent(
            observer->GetSceneIndex()->GetPrim(entry.primPath).dataSource);

    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyGlobalsSchema::GetOptionsLocator())) {
        _SetOptions(schema);
    }
}

void
HdPrman_RileyGlobalsPrim::_SetOptions(
    HdPrmanRileyGlobalsSchema globalsSchema)
{
    if (HdPrmanRileyParamListSchema schema = globalsSchema.GetOptions()) {
        HdPrman_RileyParamList options = HdPrman_RileyParamList(schema);
        _SetRileyOptions(options.rileyObject);
        
    }
}

HdPrman_RileyGlobalsPrim::~HdPrman_RileyGlobalsPrim() = default;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
