//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <string>

using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static string
_Str(const PcpMapExpression& e)
{
    return e.GetString();
}

} // anonymous namespace 

void
wrapMapExpression(nanobind::module_ &m)
{
    namespace nb = nanobind;
    typedef PcpMapExpression This;

    nb::class_<This>(m, "MapExpression")
        .def(nb::init<>())
        .def("__str__", _Str)

        .def("Evaluate", &This::Evaluate,
             nb::rv_policy::copy)
        .def_static("Identity", &This::Identity,
                    nb::rv_policy::copy)
        .def_static("Constant", &This::Constant,
                    nb::rv_policy::copy)
        .def_static("ImpliedClass", &This::ImpliedClass)
        .def("Inverse", &This::Inverse,
             nb::rv_policy::copy)
        .def("AddRootIdentity", &This::AddRootIdentity,
             nb::rv_policy::copy)
        .def("Compose", &This::Compose,
             nb::rv_policy::copy)
        .def("MapSourceToTarget", &This::MapSourceToTarget,
             nb::arg("path"))
        .def("MapTargetToSource", &This::MapTargetToSource,
             nb::arg("path"))

        .def_prop_ro("timeOffset", &This::GetTimeOffset,
                     nb::rv_policy::copy)
        .def_prop_ro("isIdentity", &This::IsIdentity)
        .def_prop_ro("isNull", &This::IsNull)
        ;

}
