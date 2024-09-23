//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyErrorInternal.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

#include "pxr/external/boost/python/handle.hpp"
#include "pxr/external/boost/python/object.hpp"

PXR_NAMESPACE_OPEN_SCOPE

using namespace pxr_boost::python;

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(TF_PYTHON_EXCEPTION);
}

// Should probably use a better mechanism.

static handle<> _ExceptionClass;

handle<> Tf_PyGetErrorExceptionClass()
{
    return _ExceptionClass;
}

void Tf_PySetErrorExceptionClass(object const &cls)
{
    _ExceptionClass = handle<>(borrowed(cls.ptr()));
}

TfPyExceptionStateScope::TfPyExceptionStateScope() :
    _state(TfPyExceptionState::Fetch())
{
}

TfPyExceptionStateScope::~TfPyExceptionStateScope()
{
    _state.Restore();
}

PXR_NAMESPACE_CLOSE_SCOPE
