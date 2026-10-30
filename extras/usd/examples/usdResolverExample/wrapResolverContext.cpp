//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "resolverContext.h"

#include "pxr/usd/ar/pyResolverContext.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>

#include <string>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

static
size_t
_Hash(const UsdResolverExampleResolverContext& ctx)
{
    return hash_value(ctx);
}

static
std::string
_Repr(const UsdResolverExampleResolverContext& ctx)
{
    return TF_PY_REPR_PREFIX + "ResolverContext" +
        (ctx.GetMappingFile().empty() ? "()" : 
            "('" + ctx.GetMappingFile() + "')");
}

void
wrapResolverContext(nb::module_ &m)
{
    using This = UsdResolverExampleResolverContext;

    nb::class_<UsdResolverExampleResolverContext>(m, "ResolverContext")
        .def(nb::init<>())
        .def(nb::init<const std::string&>(), nb::arg("mappingFile"))

        .def(nb::self == nb::self)
        .def("__ne__", [](const This& a, const This& b) { return !(a == b); },
             nb::is_operator())
        .def("__hash__", _Hash)
        .def("__repr__", _Repr)

        .def("GetMappingFile", &This::GetMappingFile,
            nb::rv_policy::copy)
        ;

    ArWrapResolverContextForPython<This>();
}
