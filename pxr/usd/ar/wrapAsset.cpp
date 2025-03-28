//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/usd/ar/asset.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_arg.hpp"
#include "pxr/external/boost/python/to_python_converter.hpp"
#include "pxr/external/boost/python/converter/from_python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

// Data structure for exposing ArAsset objects exposed via ArResolver APIs to
// Python.
class Ar_PyAsset
{
public:
    // Create a Python representation of the given \c ArAsset resource.
    // \param asset \c ArAsset for which to create a Python representation.
    Ar_PyAsset(std::shared_ptr<ArAsset> asset)
        : _asset(asset)
        {}

    // Return a buffer with the contents of the asset, or None if the data could
    // not be read.
    object GetBuffer() const
    {
        if (!_asset) {
            TfPyThrowRuntimeError("Unable to access invalid asset");
        }

        std::shared_ptr<const char> buffer;
        size_t bufferSize = 0;
        {
            TF_PY_ALLOW_THREADS_IN_SCOPE();
            buffer = _asset->GetBuffer();
            bufferSize = _asset->GetSize();
        }

        if (!buffer) {
            return {};
        }

        // Create a Python bytes object from the buffer:
        return object(handle<>(
                PyBytes_FromStringAndSize(buffer.get(), bufferSize)));
    }

    // Reads the specifed amount of data from the underlying ArAsset at the
    // given offset
    object Read(size_t count, size_t offset)
    {
        size_t assetSize = GetSize();
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
            bytesRead = _asset->Read(buffer, count, offset);
        }

        if (bytesRead != count) {
            // If we did not read all the bytes we are after, truncate the allocated
            // buffer to avoid returning extra data.
            if (_PyBytes_Resize(&pyBytes, bytesRead) == -1) {
                TfPyThrowRuntimeError("Failed to read Asset data");
            }
        }

        return object(handle<>(pyBytes));
    }

    // Returns the size of the underlying ArAsset
    size_t GetSize() const {
        if (!_asset) {
            TfPyThrowRuntimeError("Unable to access invalid asset");
        }

        TF_PY_ALLOW_THREADS_IN_SCOPE();
        return _asset->GetSize();
    }

    // Checks validity of this object.
    bool __bool__() const
    {
        return _asset != nullptr;
    }

    // Enter the Python Context Manager for the representation of the ArAsset.
    void __enter__() const
    {
        if (!_asset) {
            TfPyThrowRuntimeError("Unable to access invalid asset");
        }
    }

    // Exit the Python Context Manager for the representation of the ArAsset.
    void __exit__(const object&, const object&, const object&)
    {
        _asset.reset();
    }

    static void RegisterConversions() {
        // to-python
        to_python_converter<std::shared_ptr<ArAsset>, Ar_PyAsset>();
        //from-python
        converter::registry::push_back(&_convertible, &_construct,
            pxr_boost::python::type_id<std::shared_ptr<ArAsset>>());
    }

    // to-python conversion of std::shared_ptr<ArAsset>.
    static PyObject *convert(const std::shared_ptr<ArAsset> &asset) {
        return incref(object(Ar_PyAsset{asset}).ptr());
    }

private:
    static void* _convertible(PyObject *obj_ptr) {
        extract<Ar_PyAsset> extractor(obj_ptr);
        return extractor.check() ? obj_ptr : nullptr;
    }

    static void _construct(PyObject *obj_ptr,
                           converter::rvalue_from_python_stage1_data *data) 
    {
        void *storage = ((converter::rvalue_from_python_storage<
                          std::shared_ptr<ArAsset>>*)data)->storage.bytes;
        Ar_PyAsset pyAsset = extract<Ar_PyAsset>(obj_ptr);
        new (storage) std::shared_ptr<ArAsset>(std::move(pyAsset._asset));
        data->convertible = storage;
    }

    std::shared_ptr<ArAsset> _asset;
};

void wrapAsset()
{
    typedef Ar_PyAsset This;

    class_<This>
        ("Ar_PyAsset", no_init)
        .def("GetBuffer", &This::GetBuffer)
        .def("GetSize", &This::GetSize)
        .def("Read", &This::Read, (arg("count"), arg("offset")))

        .def("__bool__", &This::__bool__)
        .def("__enter__", &This::__enter__, return_self<>())
        .def("__exit__", &This::__exit__)
        ;

    Ar_PyAsset::RegisterConversions();
}