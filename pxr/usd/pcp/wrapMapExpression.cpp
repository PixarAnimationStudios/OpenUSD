//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <boost/python.hpp>

#include "pxr/pxr.h"
#include "pxr/usd/pcp/mapExpression.h"

#include <string>

using namespace boost::python;
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
wrapMapExpression()
{
    typedef PcpMapExpression This;

    class_<This>("MapExpression")
        .def("__str__", _Str)

        .def("Evaluate", &This::Evaluate,
             return_value_policy<return_by_value>())
        .def("Identity", &This::Identity,
             return_value_policy<return_by_value>())
        .staticmethod("Identity")
        .def("Constant", &This::Constant,
             return_value_policy<return_by_value>())
        .staticmethod("Constant")
        .def("Inverse", &This::Inverse,
             return_value_policy<return_by_value>())
        .staticmethod("Inverse")
        .def("AddRootIdentity", &This::AddRootIdentity,
             return_value_policy<return_by_value>())
        .def("Compose", &This::Compose,
             return_value_policy<return_by_value>())
        .def("MapSourceToTarget", &This::MapSourceToTarget,
            (arg("path")))
        .def("MapTargetToSource", &This::MapTargetToSource,
            (arg("path")))

        .add_property("timeOffset",
            make_function(&This::GetTimeOffset,
                          return_value_policy<return_by_value>()) )
        .add_property("isIdentity", &This::IsIdentity)
        .add_property("isNull", &This::IsNull)
        ;
}
