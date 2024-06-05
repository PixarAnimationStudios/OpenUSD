//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/expiryNotifier.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

void (*Tf_ExpiryNotifier::_func)(void const *) = 0;
void (*Tf_ExpiryNotifier::_func2)(void const *) = 0;

void Tf_ExpiryNotifier::Invoke(void const *p)
{
    if (_func)
        _func(p);
}

void Tf_ExpiryNotifier::SetNotifier(void (*func)(void const *))
{
    if (func && _func)
        TF_FATAL_ERROR("cannot override already installed notification "
                       "function");
    _func = func;
}

void Tf_ExpiryNotifier::Invoke2(void const *p)
{
    if (_func2)
        _func2(p);
}

void Tf_ExpiryNotifier::SetNotifier2(void (*func)(void const *))
{
    if (func && _func2)
        TF_FATAL_ERROR("cannot override already installed notification(2) "
                       "function");
    _func2 = func;
}

PXR_NAMESPACE_CLOSE_SCOPE
