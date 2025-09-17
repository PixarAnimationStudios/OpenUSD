//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/weakBase.h"

PXR_NAMESPACE_OPEN_SCOPE

Tf_Remnant::~Tf_Remnant()
{
    if (ARCH_UNLIKELY(_notify)) {
        Tf_ExpiryNotifier::Invoke(this);
    }
}

void const*
TfWeakBase::GetUniqueIdentifier() const
{
    return _Register()->_GetUniqueIdentifier();
}

void
TfWeakBase::EnableNotification2() const
{
    _Register()->_notify2 = true;
}

void const *
Tf_Remnant::_GetUniqueIdentifier() const
{
    return this;
}

void
Tf_Remnant::EnableNotification() const
{
    _notify = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
