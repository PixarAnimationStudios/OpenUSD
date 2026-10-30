//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ar/defaultResolverContext.h"
#include "pxr/usd/ar/pyResolverContext.h"
#include "pxr/base/tf/pyUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

static std::string
_Repr(const ArDefaultResolverContext& ctx)
{
    std::string repr = TF_PY_REPR_PREFIX;
    repr += "DefaultResolverContext(";
    if (!ctx.GetSearchPath().empty()) {
        repr += TfPyRepr(ctx.GetSearchPath());
    }
    repr += ")";
    return repr;
}

static size_t
_Hash(const ArDefaultResolverContext& ctx)
{
    return hash_value(ctx);
}

#include "pxr/base/tf/pyNanobindBridge.h"

void
wrapDefaultResolverContext(nanobind::module_& m)
{
    namespace nb = nanobind;
    using This = ArDefaultResolverContext;

    nb::class_<This>(m, "DefaultResolverContext")
        .def(nb::init<>())
        .def(nb::init<const std::vector<std::string>&>(),
             nb::arg("searchPaths"))

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("GetSearchPath", &This::GetSearchPath)

        .def("__str__", &This::GetAsString)
        .def("__repr__", &_Repr)
        .def("__hash__", &_Hash)
        ;

    ArWrapResolverContextForPython<This>();
}
