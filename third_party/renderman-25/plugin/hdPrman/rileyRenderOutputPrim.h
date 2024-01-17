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
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_RENDER_OUTPUT_PRIM_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_RENDER_OUTPUT_PRIM_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/sceneIndexObserverApi.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileyPrimBase.h"

PXR_NAMESPACE_OPEN_SCOPE

using HdPrman_RileyRenderOutputPrimHandle =
    std::shared_ptr<class HdPrman_RileyRenderOutputPrim>;

/// \class HdPrman_RileyRenderOutputPrim
///
/// Wraps a riley render output object, initializing or updating it
/// using the HdPrmanRileyRenderOutputSchema.
///
class HdPrman_RileyRenderOutputPrim : public HdPrman_RileyPrimBase
{
public:
    HdPrman_RileyRenderOutputPrim(
        HdContainerDataSourceHandle const &primSource,
        const HdsiPrimManagingSceneIndexObserver *observer,
        HdPrman_RenderParam * renderParam);

    ~HdPrman_RileyRenderOutputPrim() override;

    using RileyId = riley::RenderOutputId;
    using RileyIdList = riley::RenderOutputList;

    const RileyId &GetRileyId() const { return _rileyId; }

protected:
    void _Dirty(
        const HdSceneIndexObserver::DirtiedPrimEntry &entry,
        const HdsiPrimManagingSceneIndexObserver * observer) override;

private:
    const RileyId _rileyId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#endif

