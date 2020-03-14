//
// Copyright 2016 Pixar
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
#ifndef PXR_IMAGING_HD_RPRIM_SHARED_DATA_H
#define PXR_IMAGING_HD_RPRIM_SHARED_DATA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/bbox3d.h"

PXR_NAMESPACE_OPEN_SCOPE


// HdRprimSharedData is an assortment of data being shared across HdReprs,
// owned by HdRprim. HdDrawItem holds a const pointer to HdRprimSharedData.
//
// HdRprim
//   |
//   +--HdRepr(s)
//   |    |
//   |    +--HdDrawItem(s)-----.
//   |                         |
//   +--HdRprimSharedData  <---'
//

struct HdRprimSharedData {
    HdRprimSharedData(int barContainerSize)
        : barContainer(barContainerSize)
        , bounds()
        , instancerLevels(-1)
        , visible(true)
        , rprimID()
        , materialTag(HdMaterialTagTokens->defaultMaterialTag)
    { }

    HdRprimSharedData(int barContainerSize,
                      bool visible)
        : barContainer(barContainerSize)
        , bounds()
        , instancerLevels(-1)
        , visible(visible)
        , rprimID()
        , materialTag(HdMaterialTagTokens->defaultMaterialTag)
    { }

    // BufferArrayRange array
    HdBufferArrayRangeContainer barContainer;

    // Used for CPU frustum culling.
    GfBBox3d bounds;

    // The number of levels of instancing applied to this rprim.
    int instancerLevels;

    // Used for authored/delegate visibility.
    bool visible;

    // The owning Rprim's identifier.
    SdfPath rprimID;

    // Used to organize drawItems into collections based on material properties.
    TfToken materialTag;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_RPRIM_SHARED_DATA_H
