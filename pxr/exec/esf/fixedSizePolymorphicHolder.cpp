//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esf/fixedSizePolymorphicHolder.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

EsfFixedSizePolymorphicBase::~EsfFixedSizePolymorphicBase() = default;

void
EsfFixedSizePolymorphicBase::_CopyTo(std::byte *storage) const
{
    TF_UNUSED(storage);
    TF_VERIFY(false,
        "Must not call default implementation of "
        "EsfFixedSizePolymorphicBase::_CopyTo");
}

void
EsfFixedSizePolymorphicBase::_MoveTo(std::byte *storage)
{
    TF_UNUSED(storage);
    TF_VERIFY(false,
        "Must not call default implementation of "
        "EsfFixedSizePolymorphicBase::_MoveTo");
}

PXR_NAMESPACE_CLOSE_SCOPE