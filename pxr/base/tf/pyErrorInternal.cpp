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
#include "pxr/base/tf/staticData.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(TF_PYTHON_EXCEPTION);
}

// The held PyObject* is intentionally leaked to avoid Python refcount ops
// during shutdown, which is unsafe if Python has been finalized.
static TfStaticData<PyObject *> _ExceptionClass;

nanobind::handle Tf_PyGetErrorExceptionClass()
{
    return nanobind::handle(*_ExceptionClass);
}

void Tf_PySetErrorExceptionClass(nanobind::object const &cls)
{
    Py_XINCREF(cls.ptr());
    *_ExceptionClass = cls.ptr();
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
