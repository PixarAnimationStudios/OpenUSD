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
#include <boost/python.hpp>

#include "pxr/pxr.h"
#include "pxr/usd/pcp/mapExpression.h"

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdPcpWrapMapExpression {

static std::string
_Str(const PcpMapExpression& e)
{
    return e.GetString();
}

} // anonymous namespace 

void
wrapMapExpression()
{
    typedef PcpMapExpression This;

    boost::python::class_<This>("MapExpression")
        .def("__str__", pxrUsdPcpWrapMapExpression::_Str)

        .def("Evaluate", &This::Evaluate,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("Identity", &This::Identity,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("Identity")
        .def("Constant", &This::Constant,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("Constant")
        .def("Inverse", &This::Inverse,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("Inverse")
        .def("AddRootIdentity", &This::AddRootIdentity,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("Compose", &This::Compose,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .def("MapSourceToTarget", &This::MapSourceToTarget,
            (boost::python::arg("path")))
        .def("MapTargetToSource", &This::MapTargetToSource,
            (boost::python::arg("path")))

        .add_property("timeOffset",
            boost::python::make_function(&This::GetTimeOffset,
                          boost::python::return_value_policy<boost::python::return_by_value>()) )
        .add_property("isIdentity", &This::IsIdentity)
        .add_property("isNull", &This::IsNull)
        ;
}
