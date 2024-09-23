//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyObjWrapper.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/type.h"

#include "pxr/external/boost/python/object.hpp"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TfPyObjWrapper>();
}

namespace {

// A custom deleter for shared_ptr<pxr_boost::python::object> that takes the
// python lock before deleting the python object.  This is necessary since it's
// invalid to decrement the python refcount without holding the lock.
struct _DeleteObjectWithLock {
    void operator()(pxr_boost::python::object *obj) const {
        PXR_NS::TfPyLock lock;
        delete obj;
    }
};
}

TfPyObjWrapper::TfPyObjWrapper()
{
    TfPyLock lock;
    TfPyObjWrapper none((pxr_boost::python::object())); // <- extra parens for "most
                                                    // vexing parse".
    *this = none;
}

TfPyObjWrapper::TfPyObjWrapper(pxr_boost::python::object obj)
    : _objectPtr(new object(obj), _DeleteObjectWithLock())
{
}

PyObject *
TfPyObjWrapper::ptr() const
{
    return _objectPtr->ptr();
}

bool
TfPyObjWrapper::operator==(TfPyObjWrapper const &other) const
{
    // If they point to the exact same object instance, we know they're equal.
    if (_objectPtr == other._objectPtr)
        return true;

    // Otherwise lock and let python determine equality.
    TfPyLock lock;
    return Get() == other.Get();
}

bool
TfPyObjWrapper::operator!=(TfPyObjWrapper const &other) const
{
    return !(*this == other);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_PYTHON_SUPPORT_ENABLED
