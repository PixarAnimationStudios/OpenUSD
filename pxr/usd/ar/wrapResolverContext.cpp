//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/list.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/tuple.hpp"

#include "pxr/pxr.h"
#include "pxr/usd/ar/resolverContext.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

// Helper struct to allow implicit conversions from None or a
// sequence of context objects to an Ar.ResolverContext object.
struct Ar_ResolverContextFromPython
{
    Ar_ResolverContextFromPython() 
    {
        converter::registry::push_back(
            &_convertible, &_construct,
            pxr_boost::python::type_id<ArResolverContext>());
    }

    static void* _convertible(PyObject* obj_ptr) 
    {
        object obj(handle<>(borrowed(obj_ptr)));
        if (obj.is_none() ||
            extract<std::vector<ArResolverContext>>(obj).check()) {
            return obj_ptr;
        }
        return 0;
    }
    
    static void _construct(
        PyObject* obj_ptr, converter::rvalue_from_python_stage1_data *data) 
    {
        void *storage = ((converter::rvalue_from_python_storage<ArResolverContext>*)data)
            ->storage.bytes;

        ArResolverContext context;

        object obj(handle<>(borrowed(obj_ptr)));
        if (!obj.is_none()) {
            context = ArResolverContext(
                extract<std::vector<ArResolverContext>>(obj)());
        }

        new (storage) ArResolverContext(context);
        data->convertible = storage;
    }
};

} // anonymous namespace 

PXR_NAMESPACE_OPEN_SCOPE

class Ar_ResolverContextPythonAccess
{
public:
    static pxr_boost::python::list GetAsList(const ArResolverContext& ctx)
    {
        pxr_boost::python::list l;
        for (const auto& data : ctx._contexts) {
            l.append(data->GetPythonObj().Get());
        }
        return l;
    }

    static std::string GetRepr(const ArResolverContext& ctx)
    {
        std::vector<std::string> objReprs;
        for (const auto& data : ctx._contexts) {
            objReprs.push_back(TfPyObjectRepr(data->GetPythonObj().Get()));
        }
        return TF_PY_REPR_PREFIX +
            TfStringPrintf("ResolverContext(%s)", 
                TfStringJoin(objReprs, ", ").c_str());
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

static ArResolverContext*
_Create(const pxr_boost::python::object& obj)
{
    extract<ArResolverContext> convertToContext(obj);
    return new ArResolverContext(convertToContext());
}

static size_t
_Hash(const ArResolverContext& ctx)
{
    return hash_value(ctx);
}

static ArResolverContext
_TestImplicitConversion(const ArResolverContext& ctx)
{
    return ctx;
}

void
wrapResolverContext()
{
    Ar_ResolverContextFromPython();

    class_<ArResolverContext>
        ("ResolverContext", no_init)
        .def(init<>())
        .def("__init__", make_constructor(&_Create))

        .def("IsEmpty", &ArResolverContext::IsEmpty)
        .def("Get", &Ar_ResolverContextPythonAccess::GetAsList)
        .def("GetDebugString", &ArResolverContext::GetDebugString)

        .def(self == self)
        .def(self != self)
        .def(self < self)

        .def("__hash__", &_Hash)
        .def("__repr__", &Ar_ResolverContextPythonAccess::GetRepr)
        ;

    TfPyContainerConversions::from_python_sequence<
        std::vector<ArResolverContext>,
        TfPyContainerConversions::variable_capacity_policy >();

    // Helper function for unit tests to exercise implicit conversion of
    // context objects into an ArResolverContext.
    def("_TestImplicitConversion", &_TestImplicitConversion);
}
