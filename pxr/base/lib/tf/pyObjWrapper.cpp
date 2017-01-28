//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyObjWrapper.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/type.h"

#include <boost/python/object.hpp>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TfPyObjWrapper>();
}

PXR_NAMESPACE_CLOSE_SCOPE

namespace {

// A custom deleter for shared_ptr<boost::python::object> that takes the
// python lock before deleting the python object.  This is necessary since it's
// invalid to decrement the python refcount without holding the lock.
struct _DeleteObjectWithLock {
    void operator()(boost::python::object *obj) const {
        PXR_NS::TfPyLock lock;
        delete obj;
    }
};
}

PXR_NAMESPACE_OPEN_SCOPE

TfPyObjWrapper::TfPyObjWrapper()
{
    TfPyLock lock;
    TfPyObjWrapper none((boost::python::object())); // <- extra parens for "most
                                                    // vexing parse".
    *this = none;
}

TfPyObjWrapper::TfPyObjWrapper(boost::python::object obj)
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
    return not (*this == other);
}

PXR_NAMESPACE_CLOSE_SCOPE
