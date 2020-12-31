//
// Copyright 2020 benmalartre
//
// Unlicensed
//

#include "pxr/imaging/plugin/LoFi/dynamicUvTextureObject.h"

#include "pxr/imaging/plugin/LoFi/dynamicUvTextureImplementation.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/imaging/plugin/LoFi/textureHandleRegistry.h"
#include "pxr/imaging/plugin/LoFi/subtextureIdentifier.h"

#include "pxr/imaging/hgi/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

LoFiDynamicUvTextureObject::LoFiDynamicUvTextureObject(
    const LoFiTextureIdentifier &textureId,
    LoFiTextureObjectRegistry * const textureObjectRegistry)
  : LoFiUvTextureObject(textureId, textureObjectRegistry)
{
}

LoFiDynamicUvTextureObject::~LoFiDynamicUvTextureObject()
{
    _DestroyTexture();
}

LoFiDynamicUvTextureImplementation *
LoFiDynamicUvTextureObject::_GetImpl() const
{
    const LoFiDynamicUvSubtextureIdentifier * const subId =
        dynamic_cast<const LoFiDynamicUvSubtextureIdentifier *>(
            GetTextureIdentifier().GetSubtextureIdentifier());
    if (!TF_VERIFY(subId)) {
        return nullptr;
    }

    return subId->GetTextureImplementation();
}

bool
LoFiDynamicUvTextureObject::IsValid() const
{
    if (LoFiDynamicUvTextureImplementation * const impl = _GetImpl()) {
        return impl->IsValid(this);
    }
    return true;
}

void
LoFiDynamicUvTextureObject::_Load()
{
    if (LoFiDynamicUvTextureImplementation * const impl = _GetImpl()) {
        impl->Load(this);
    }
}

void
LoFiDynamicUvTextureObject::_Commit()
{
    if (LoFiDynamicUvTextureImplementation * const impl = _GetImpl()) {
        impl->Commit(this);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
