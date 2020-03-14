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
#include "pxr/imaging/hdSt/textureResourceHandle.h"
#include "pxr/imaging/hdSt/textureResource.h"

#include "pxr/usd/sdf/path.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

HdStTextureResourceHandle::HdStTextureResourceHandle(
        HdStTextureResourceSharedPtr const & textureResource)
    : _textureResource(textureResource)
{
}

HdStTextureResourceHandle::~HdStTextureResourceHandle()
{
    // nothing
}

HdStTextureResourceSharedPtr const &
HdStTextureResourceHandle::GetTextureResource() const
{
    return _textureResource;
}

void
HdStTextureResourceHandle::SetTextureResource(
        HdStTextureResourceSharedPtr const & textureResource)
{
    _textureResource = textureResource;
}

/* static */
HDST_API
HdTextureResource::ID
HdStTextureResourceHandle::GetHandleKey(
            HdRenderIndex const * renderIndex,
            SdfPath const &textureHandleId)
{
    // Include the render index as part of the key used to register
    // texture handle scene path locations since the underlying resource
    // registry is shared by all HdStRenderDelegate instances in an
    // address space.
    size_t handleKey = textureHandleId.GetHash();
    boost::hash_combine(handleKey, renderIndex);
    return handleKey;
}

/* static */
bool
HdStTextureResourceHandle::IsIncompatibleTextureResource(
        HdStTextureResourceSharedPtr const & a,
        HdStTextureResourceSharedPtr const & b)
{
    // A texture resource is considered incompatible if it might
    // change the resource binding signature, e.g. UV vs Ptex
    // or fallback resource management, e.g. whether to bind a
    // fallback texture resource when a resource changes between
    // defined or undefined.

    // Okay if both are the same or both empty
    if (a == b) {
        return false;
    }
    // Not okay if not both defined with matching texture types
    return !(a && b && (a->GetTextureType() == b->GetTextureType()));
}


PXR_NAMESPACE_CLOSE_SCOPE
