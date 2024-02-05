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
//

#include "hdPrman/rileyCoordinateSystemPrim.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileyPrimUtil.h"
#include "hdPrman/rileyCoordinateSystemSchema.h"
#include "hdPrman/utils.h"

#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_RileyCoordinateSystemPrim::HdPrman_RileyCoordinateSystemPrim(
    HdContainerDataSourceHandle const &primSource,
    const HdsiPrimManagingSceneIndexObserver * const observer,
    HdPrman_RenderParam * const renderParam)
  : HdPrman_RileyPrimBase(renderParam)
{
    HdPrmanRileyCoordinateSystemSchema schema =
        HdPrmanRileyCoordinateSystemSchema::GetFromParent(primSource);

    HdPrman_RileyTransform transform(
        schema.GetXform(), _GetShutterInterval());

    _rileyId =
        _AcquireRiley()->CreateCoordinateSystem(
            riley::UserId(),
            transform.rileyObject,
            HdPrman_Utils::ParamsFromDataSource(
                schema.GetAttributes()));
}

void
HdPrman_RileyCoordinateSystemPrim::_Dirty(
    const HdSceneIndexObserver::DirtiedPrimEntry &entry,
    const HdsiPrimManagingSceneIndexObserver * const observer)
{
    HdContainerDataSourceHandle const primSource =
        observer->GetSceneIndex()->GetPrim(entry.primPath).dataSource;

    HdPrmanRileyCoordinateSystemSchema schema =
        HdPrmanRileyCoordinateSystemSchema::GetFromParent(primSource);

    std::optional<HdPrman_RileyTransform> transform;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyCoordinateSystemSchema::GetXformLocator())) {
        transform = HdPrman_RileyTransform(schema.GetXform(), _GetShutterInterval());
    }

    std::optional<RtParamList> attributes;
    if (entry.dirtyLocators.Intersects(
            HdPrmanRileyCoordinateSystemSchema::GetAttributesLocator())) {
        attributes =
            HdPrman_Utils::ParamsFromDataSource(schema.GetAttributes());
    }

    _AcquireRiley()->ModifyCoordinateSystem(
        _rileyId,
        HdPrman_GetPtrRileyObject(transform),
        HdPrman_GetPtr(attributes));

}

HdPrman_RileyCoordinateSystemPrim::~HdPrman_RileyCoordinateSystemPrim()
{
    _AcquireRiley()->DeleteCoordinateSystem(_rileyId);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
