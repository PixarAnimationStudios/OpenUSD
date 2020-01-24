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
#ifndef PXR_IMAGING_HD_ST_TEXTURE_RESOURCE_HANDLE_H
#define PXR_IMAGING_HD_ST_TEXTURE_RESOURCE_HANDLE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/textureResource.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class SdfPath;

typedef boost::shared_ptr<class HdStTextureResourceHandle>
                HdStTextureResourceHandleSharedPtr;
typedef boost::shared_ptr<class HdStTextureResource>
                HdStTextureResourceSharedPtr;

/// HdStTextureResourceHandle is an indirect handle to a GL texture resource
///
/// This class provides a way for an HdStTextureResource to be registered
/// for a specific scene path. This allows clients (e.g. shader code) to
/// access the most recently registered texture resource without having
/// to directly observe changes to upstream material, texture, or drawtarget
/// texture resource changes.
///
/// The underlying HdStTextureResource instances are typically created
/// outside of HdSt. They are obtained from the scene delegate and are
/// registered with the resource registry via an HdTextureResource::ID
/// which is also provided by the scene delegate.
///
/// Clients which use HdStTexture resources (e.g. shader code) get the
/// underlying GL texture object and GL sampler object IDs from the
/// texture resource.
///
/// So:
///  HdStShaderCode has an array of texture descriptors holding
///    HdStTextureResourceHandles. HdStShaderCode binds GL texture
///    and sampler objects for the current set of underlying
///    HdStTextureResource instances.
///
///  HdStTexture Bprim and HdStDrawTarget Sprim and HdStMaterial Sprim
///    register HdStTextureResource instances with their scene delegate
///    provided HdTextureResource::ID and also register their current
///    HdStTextureResource instances with scene path locations using
///    HdStTextureResourceHandles.
///
///  HdStMaterial Sprim can assign HdStTextureResourceHandles to
///    HdStShaderCode to satisfy input texture scene path connections
///    and allow HdStShaderCode to resolve GL texture and sampler objects
///    during resource bind/unbind.
///
/// We then need to propagate dirtiness from HdTexture to HdMaterial
/// only when an incompatible change is made to the texture resource
/// binding, avoiding an expensive invalidation, e.g. for animated
/// texture sources.
///
/// This class provides static helper methods to compute registry keys and
/// to identify incompatible texture resource changes.
///
class HdStTextureResourceHandle {
public:
    HDST_API
    HdStTextureResourceHandle(
        HdStTextureResourceSharedPtr const & textureResource = nullptr);

    HDST_API
    virtual ~HdStTextureResourceHandle();

    HDST_API
    HdStTextureResourceSharedPtr const & GetTextureResource() const;

    HDST_API
    void SetTextureResource(
                HdStTextureResourceSharedPtr const & textureResource);

    HDST_API
    static HdTextureResource::ID GetHandleKey(
                HdRenderIndex const * renderIndex,
                SdfPath const &textureHandleId);

    HDST_API
    static bool IsIncompatibleTextureResource(
                        HdStTextureResourceSharedPtr const & a,
                        HdStTextureResourceSharedPtr const & b);

private:
    HdStTextureResourceSharedPtr _textureResource;

    HdStTextureResourceHandle(const HdStTextureResourceHandle &)
        = delete;
    HdStTextureResourceHandle &operator =(const HdStTextureResourceHandle &)
        = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_TEXTURE_RESOURCE_HANDLE_H
