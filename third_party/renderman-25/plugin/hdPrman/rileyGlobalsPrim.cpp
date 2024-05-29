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
        _AcquireRiley()->SetOptions(
            options.rileyObject);
    }
}

HdPrman_RileyGlobalsPrim::~HdPrman_RileyGlobalsPrim() = default;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
