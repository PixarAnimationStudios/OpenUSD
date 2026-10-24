//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/ar/pyResolverContext.h"
#include "pxr/usd/ar/resolverContext.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>

PXR_NAMESPACE_OPEN_SCOPE

class Ar_ResolverContextPythonAccess
{
public:
    static nanobind::list GetAsList(const ArResolverContext& ctx)
    {
        nanobind::list l;
        for (const auto& data : ctx._contexts) {
            l.append(nanobind::borrow<nanobind::object>(
                data->GetPythonObj().ptr()));
        }
        return l;
    }

    static std::string GetRepr(const ArResolverContext& ctx)
    {
        std::vector<std::string> objReprs;
        for (const auto& data : ctx._contexts) {
            objReprs.push_back(
                TfPyObjectRepr(nanobind::handle(data->GetPythonObj().ptr())));
        }
        return TF_PY_REPR_PREFIX +
            TfStringPrintf("ResolverContext(%s)", 
                TfStringJoin(objReprs, ", ").c_str());
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

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
wrapResolverContext(nanobind::module_& m)
{
    namespace nb = nanobind;

    nb::class_<ArResolverContext>(m, "ResolverContext")
        .def(nb::init<>())
        .def("__init__", [](ArResolverContext* self, nb::handle obj) {
            ArResolverContext ctx;
            if (!Ar_ConvertFromPythonToResolverContext(obj.ptr(), &ctx)) {
                throw nb::type_error("Invalid object for Ar.ResolverContext");
            }
            new (self) ArResolverContext(std::move(ctx));
        }, nb::arg().none())

        .def("IsEmpty", &ArResolverContext::IsEmpty)
        .def("Get", &Ar_ResolverContextPythonAccess::GetAsList)
        .def("GetDebugString", &ArResolverContext::GetDebugString)

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def(nb::self < nb::self)

        .def("__hash__", &_Hash)
        .def("__repr__", &Ar_ResolverContextPythonAccess::GetRepr)
        ;

    // Helper function for unit tests to exercise implicit conversion of
    // context objects into an ArResolverContext.
    m.def("_TestImplicitConversion", &_TestImplicitConversion,
          nb::arg().none());
}
