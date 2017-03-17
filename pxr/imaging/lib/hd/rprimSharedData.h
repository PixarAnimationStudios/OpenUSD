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
#ifndef HD_RPRIM_SHARED_DATA_H
#define HD_RPRIM_SHARED_DATA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferArrayRange.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/bbox3d.h"

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdShaderCode> HdShaderCodeSharedPtr;



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
        , surfaceShader()
        , bounds()
        , hasInstancer(false)
        , visible(true)
        , rprimID()
    { }

    HdRprimSharedData(int barContainerSize,
                      bool hasInstancer,
                      bool visible)
        : barContainer(barContainerSize)
        , surfaceShader()
        , bounds()
        , hasInstancer(hasInstancer)
        , visible(visible)
        , rprimID()
    { }

    // BufferArrayRange array
    HdBufferArrayRangeContainer barContainer;

    // The ID of the surface shader to which the Rprim is bound.
    HdShaderCodeSharedPtr surfaceShader;

    // Used for CPU frustum culling.
    GfBBox3d bounds;

    // True if the rprim is an instance prototype.
    bool hasInstancer;

    // Used for authored/delegate visibility.
    bool visible;

    // The owning Rprim's identifier.
    SdfPath rprimID;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_RPRIM_SHARED_DATA_H
