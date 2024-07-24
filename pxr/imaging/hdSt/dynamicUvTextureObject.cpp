//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/dynamicUvTextureObject.h"

#include "pxr/imaging/hdSt/dynamicUvTextureImplementation.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/textureHandleRegistry.h"
#include "pxr/imaging/hdSt/subtextureIdentifier.h"

#include "pxr/imaging/hgi/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStDynamicUvTextureObject::HdStDynamicUvTextureObject(
    const HdStTextureIdentifier &textureId,
    HdSt_TextureObjectRegistry * const textureObjectRegistry)
  : HdStUvTextureObject(textureId, textureObjectRegistry)
{
}

HdStDynamicUvTextureObject::~HdStDynamicUvTextureObject()
{
    _DestroyTexture();
}

HdStDynamicUvTextureImplementation *
HdStDynamicUvTextureObject::_GetImpl() const
{
    const HdStDynamicUvSubtextureIdentifier * const subId =
        dynamic_cast<const HdStDynamicUvSubtextureIdentifier *>(
            GetTextureIdentifier().GetSubtextureIdentifier());
    if (!TF_VERIFY(subId)) {
        return nullptr;
    }

    return subId->GetTextureImplementation();
}

bool
HdStDynamicUvTextureObject::IsValid() const
{
    if (HdStDynamicUvTextureImplementation * const impl = _GetImpl()) {
        return impl->IsValid(this);
    }
    return true;
}

void
HdStDynamicUvTextureObject::_Load()
{
    if (HdStDynamicUvTextureImplementation * const impl = _GetImpl()) {
        impl->Load(this);
    }
}

void
HdStDynamicUvTextureObject::_Commit()
{
    if (HdStDynamicUvTextureImplementation * const impl = _GetImpl()) {
        impl->Commit(this);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
