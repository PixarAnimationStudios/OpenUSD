//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "hdPrman/rileyEnergyFilterPrim.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
#if _PRMANAPI_VERSION_MAJOR_ >= 27

#include "hdPrman/rileyIds.h"
#include "hdPrman/rileyTypes.h"

#include "hdPrman/rileyEnergyFilterSchema.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_RileyEnergyFilterPrim::HdPrman_RileyEnergyFilterPrim(
    HdContainerDataSourceHandle const &primSource,
    const HdsiPrimManagingSceneIndexObserver * const observer,
    HdPrman_RenderParam * const renderParam)
  : HdPrman_RileyPrimBase(renderParam)
{
    HdPrmanRileyEnergyFilterSchema schema =
        HdPrmanRileyEnergyFilterSchema::GetFromParent(primSource);

    HdPrman_RileyShadingNetwork energyFilter =
        HdPrman_RileyShadingNetwork(
            schema.GetEnergyFilter());

    HdPrman_RileyParamList attributes =
        HdPrman_RileyParamList(
            schema.GetAttributes());

    _rileyId = _AcquireRiley()->CreateEnergyFilter(
        riley::UserId(),
        energyFilter.rileyObject,
        attributes.rileyObject);
}

void
HdPrman_RileyEnergyFilterPrim::_Dirty(
    const HdSceneIndexObserver::DirtiedPrimEntry &entry,
    const HdsiPrimManagingSceneIndexObserver * const observer)
{
    HdPrmanRileyEnergyFilterSchema schema =
        HdPrmanRileyEnergyFilterSchema::GetFromParent(
            observer->GetSceneIndex()->GetPrim(entry.primPath).dataSource);

    std::optional<HdPrman_RileyShadingNetwork> energyFilter;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyEnergyFilterSchema::GetEnergyFilterLocator())) {
        energyFilter =
            HdPrman_RileyShadingNetwork(
                schema.GetEnergyFilter());
    };

    std::optional<HdPrman_RileyParamList> attributes;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyEnergyFilterSchema::GetAttributesLocator())) {
        attributes =
            HdPrman_RileyParamList(
                schema.GetAttributes());
    };

    _AcquireRiley()->ModifyEnergyFilter(_rileyId,
        nullptr,
        HdPrman_GetRileyObjectPtr(energyFilter),
        HdPrman_GetRileyObjectPtr(attributes));
}

HdPrman_RileyEnergyFilterPrim::~HdPrman_RileyEnergyFilterPrim()
{
    _AcquireRiley()->DeleteEnergyFilter(_rileyId);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // _PRMANAPI_VERSION_MAJOR_ >= 27
#endif // HDPRMAN_USE_SCENE_INDEX_OBSERVER
