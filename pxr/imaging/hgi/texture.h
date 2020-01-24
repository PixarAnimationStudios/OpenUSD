//
// Copyright 2019 Pixar
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
#ifndef PXR_IMAGING_HGI_TEXTURE_H
#define PXR_IMAGING_HGI_TEXTURE_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"


PXR_NAMESPACE_OPEN_SCOPE

struct HgiTextureDesc;


///
/// \class HgiTexture 
///
/// Represents a graphics platform independent GPU texture resource.
///
/// Base class for Hgi textures.
/// To the client (HdSt) texture resources are referred to via
/// opaque, stateless handles (HgTextureHandle).
///
class HgiTexture {
public:
    HGI_API
    HgiTexture(HgiTextureDesc const& desc);

    HGI_API
    virtual ~HgiTexture();

private:
    HgiTexture() = delete;
    HgiTexture & operator=(const HgiTexture&) = delete;
    HgiTexture(const HgiTexture&) = delete;
};

typedef HgiTexture* HgiTextureHandle;



/// \struct HgiTextureDesc
///
/// Describes the properties needed to create a GPU texture.
///
/// <ul>
/// <li>usage:
///   Describes how the texture is intended to be used.</li>
/// <li>format:
///   The format of the texture.
/// <li>dimensions:
///   The resolution of the texture (width,height,layers).</li>
/// <li>sampleCount:
///   samples per texel (multi-sampling)</li>
/// </ul>
///
struct HgiTextureDesc {
    HgiTextureDesc()
    : usage(HgiTextureUsageBitsColorTarget)
    , format(HgiFormatInvalid)
    , dimensions(0)
    , sampleCount(HgiSampleCount1)
    {}

    HgiTextureUsage usage;
    HgiFormat format;
    GfVec3i dimensions;
    HgiSampleCount sampleCount;
};

HGI_API
bool operator==(
    const HgiTextureDesc& lhs,
    const HgiTextureDesc& rhs);

HGI_API
inline bool operator!=(
    const HgiTextureDesc& lhs,
    const HgiTextureDesc& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
