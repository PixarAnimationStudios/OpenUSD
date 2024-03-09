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

#include "hdPrman/rileyPrimFactory.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileyRenderOutputPrim.h"
#include "hdPrman/rileyRenderTargetPrim.h"
#include "hdPrman/tokens.h"

#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

static
HdContainerDataSourceHandle
_GetPrimSource(
    const HdsiPrimManagingSceneIndexObserver * const observer,
    const SdfPath &path)
{
    return observer->GetSceneIndex()->GetPrim(path).dataSource;
}

HdPrman_RileyPrimFactory::HdPrman_RileyPrimFactory(
    HdPrman_RenderParam * const renderParam)
  : _renderParam(renderParam)
{
}

HdsiPrimManagingSceneIndexObserver::PrimBaseHandle
HdPrman_RileyPrimFactory::CreatePrim(
    const HdSceneIndexObserver::AddedPrimEntry &entry,
    const HdsiPrimManagingSceneIndexObserver * const observer)
{
    if (entry.primType == HdPrmanRileyPrimTypeTokens->renderOutput) {
        return std::make_shared<HdPrman_RileyRenderOutputPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    if (entry.primType == HdPrmanRileyPrimTypeTokens->renderTarget) {
        return std::make_shared<HdPrman_RileyRenderTargetPrim>(
            _GetPrimSource(observer, entry.primPath),
            observer,
            _renderParam);
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
