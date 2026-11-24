//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/stacked.h"

#include <tbb/enumerable_thread_specific.h>

PXR_NAMESPACE_OPEN_SCOPE

struct Tf_StackedPerThreadSlot::_Impl {
    tbb::enumerable_thread_specific<void *> slots { nullptr };
};

Tf_StackedPerThreadSlot::Tf_StackedPerThreadSlot()
    : _impl(new _Impl)
{
}

Tf_StackedPerThreadSlot::~Tf_StackedPerThreadSlot()
{
    delete _impl;
}

void *&
Tf_StackedPerThreadSlot::Local()
{
    return _impl->slots.local();
}

PXR_NAMESPACE_CLOSE_SCOPE
