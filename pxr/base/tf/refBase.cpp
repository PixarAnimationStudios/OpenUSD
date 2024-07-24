//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

TfRefBase::UniqueChangedListener TfRefBase::_uniqueChangedListener;

TfRefBase::~TfRefBase()
{
}

void
TfRefBase::SetUniqueChangedListener(UniqueChangedListener listener)
{
    if (_uniqueChangedListener.lock ||
        _uniqueChangedListener.func ||
        _uniqueChangedListener.unlock) {
        TF_FATAL_ERROR("Setting an already set UniqueChangedListener");
    }
    _uniqueChangedListener = listener;
}

TfSimpleRefBase::~TfSimpleRefBase()
{
}

PXR_NAMESPACE_CLOSE_SCOPE
