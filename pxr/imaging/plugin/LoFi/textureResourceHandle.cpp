//
// Copyright 2020 benmalartre
//
// Unlicensed
// 
#include "pxr/imaging/plugin/LoFi/textureResourceHandle.h"
#include "pxr/imaging/plugin/LoFi/textureResource.h"

#include "pxr/usd/sdf/path.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

LoFiTextureResourceHandle::LoFiTextureResourceHandle(
        LoFiTextureResourceSharedPtr const & textureResource)
    : _textureResource(textureResource)
{
}

LoFiTextureResourceHandle::~LoFiTextureResourceHandle()
{
    // nothing
}

LoFiTextureResourceSharedPtr const &
LoFiTextureResourceHandle::GetTextureResource() const
{
    return _textureResource;
}

void
LoFiTextureResourceHandle::SetTextureResource(
        LoFiTextureResourceSharedPtr const & textureResource)
{
    _textureResource = textureResource;
}

/* static */
HdTextureResource::ID
LoFiTextureResourceHandle::GetHandleKey(
            HdRenderIndex const * renderIndex,
            SdfPath const &textureHandleId)
{
    // Include the render index as part of the key used to register
    // texture handle scene path locations since the underlying resource
    // registry is shared by all LoFiRenderDelegate instances in an
    // address space.
    size_t handleKey = textureHandleId.GetHash();
    boost::hash_combine(handleKey, renderIndex);
    return handleKey;
}

/* static */
bool
LoFiTextureResourceHandle::IsIncompatibleTextureResource(
        LoFiTextureResourceSharedPtr const & a,
        LoFiTextureResourceSharedPtr const & b)
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
