//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <boost/python/class.hpp>
#include <boost/python/str.hpp>

#include "pxr/pxr.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/usd/ar/asset.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static size_t
_Read(const ArAsset& self, boost::python::object& buffer, size_t count, size_t offset)
{
    // Extract the raw pointer from the Python buffer object, and confirm it supports the buffer interface:
    PyObject* pyObject = buffer.ptr();
    if (!PyObject_CheckBuffer(pyObject)) {
        TfPyThrowTypeError("Object does not support buffer interface");
    }

    // Confirm the provided Python buffer object can be written to:
    Py_buffer pyBuffer;
    if (PyObject_GetBuffer(pyObject, &pyBuffer, PyBUF_WRITABLE) < 0) {
        TfPyThrowTypeError("Unable to get writable buffer from object");
    }

    // Validate that the provided Python buffer has sufficient size to hold the requested data:
    if (size_t(pyBuffer.len) < count) {
        PyBuffer_Release(&pyBuffer);
        TfPyThrowValueError("Provided buffer is of insufficient size to hold the requested data size");
    }

    // Proceed to read the given number of elements from the ArAsset, starting at the given offset:
    size_t result = self.Read(pyBuffer.buf, count, offset);

    PyBuffer_Release(&pyBuffer);
    return result;
}

void
wrapAsset()
{
    // Bindings for "ArAsset":
    class_<ArAsset, std::shared_ptr<ArAsset>, boost::noncopyable>
        ("Asset", no_init)

        .def("GetSize", &ArAsset::GetSize)
        .def("Read", &_Read,
            (arg("buffer"), arg("count"), arg("offset")))
        ;
}
