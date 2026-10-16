//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/ar/pyResolverContext.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/usd/ar/asset.h"

#include <nanobind/nanobind.h>
#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

nb::object
_GetBuffer(const Ar_PyAsset& self)
{
    if (!self._asset) {
        TfPyThrowRuntimeError("Unable to access invalid asset");
    }

    std::shared_ptr<const char> buffer;
    size_t bufferSize = 0;
    {
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        buffer = self._asset->GetBuffer();
        bufferSize = self._asset->GetSize();
    }

    if (!buffer) {
        return nb::none();
    }

    return nb::steal<nb::object>(
        PyBytes_FromStringAndSize(buffer.get(), bufferSize));
}

size_t
_GetSize(const Ar_PyAsset& self)
{
    if (!self._asset) {
        TfPyThrowRuntimeError("Unable to access invalid asset");
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();
    return self._asset->GetSize();
}

nb::object
_Read(Ar_PyAsset& self, size_t count, size_t offset)
{
    const size_t assetSize = _GetSize(self);
    if (offset >= assetSize) {
        TfPyThrowValueError("Invalid read offset");
    }

    // Prevent allocating a buffer larger than the amount of data that
    // we can read from the asset
    count = std::min(count, assetSize - offset);

    PyObject *pyBytes = PyBytes_FromStringAndSize(NULL, count);
    char *buffer = PyBytes_AsString(pyBytes);
    size_t bytesRead = 0;

    {
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        bytesRead = self._asset->Read(buffer, count, offset);
    }

    if (bytesRead != count) {
        // If we did not read all the bytes we are after, truncate the allocated
        // buffer to avoid returning extra data.
        if (_PyBytes_Resize(&pyBytes, bytesRead) == -1) {
            TfPyThrowRuntimeError("Failed to read Asset data");
        }
    }

    return nb::steal<nb::object>(pyBytes);
}

} // anonymous namespace

void wrapAsset(nanobind::module_& m)
{
    typedef Ar_PyAsset This;

    nb::class_<This>(m, "Ar_PyAsset")
        .def("GetBuffer", &_GetBuffer)
        .def("GetSize", &_GetSize)
        .def("Read", &_Read, nb::arg("count"), nb::arg("offset"))

        .def("__bool__", [](const This& self) {
            return self._asset != nullptr;
        })
        .def("__enter__", [](nb::object self) {
            if (!nb::cast<This&>(self)._asset) {
                TfPyThrowRuntimeError("Unable to access invalid asset");
            }
            return self;
        })
        .def("__exit__", [](This& self, nb::handle, nb::handle, nb::handle) {
            self._asset.reset();
        }, nb::arg().none(), nb::arg().none(), nb::arg().none())
        ;

    Tf_PyRegisterCustomToPythonConverter(
        typeid(std::shared_ptr<ArAsset>),
        +[](void const *p) -> PyObject * {
            auto const &src = *static_cast<std::shared_ptr<ArAsset> const *>(p);
            if (!src) {
                Py_RETURN_NONE;
            }
            return nb::detail::type_caster_base<Ar_PyAsset>::from_cpp(
                       Ar_PyAsset(src), nb::rv_policy::move, nullptr)
                .ptr();
        });
}