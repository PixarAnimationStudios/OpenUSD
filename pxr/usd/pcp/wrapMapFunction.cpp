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

#include "pxr/pxr.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include <boost/python.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static std::string
_Repr(const PcpMapFunction &f)
{
    if (f.IsIdentity()) {
        return "Pcp.MapFunction.Identity()";
    }
    std::string s = "Pcp.MapFunction(";
    if (!f.IsNull()) {
        const boost::python::dict sourceToTargetMap = 
            TfPyCopyMapToDictionary(f.GetSourceToTargetMap());

        s += TfPyObjectRepr(sourceToTargetMap);
        if (f.GetTimeOffset() != SdfLayerOffset()) {
            s += ", ";
            s += TfPyRepr(f.GetTimeOffset());
        }
    }
    s += ")";
    return s;
}

static std::string
_Str(const PcpMapFunction& f)
{
    return f.GetString();
}

static PcpMapFunction*
_Create(const boost::python::dict & sourceToTargetMap, 
        SdfLayerOffset offset)
{
    PcpMapFunction::PathMap pathMap;
    boost::python::list keys = sourceToTargetMap.keys();
    for (int i=0; i < boost::python::len(keys); ++i) {
        // Just blindly try to extract SdfPaths.
        // If the dict is not holding the right types,
        // we'll raise a boost exception, which boost
        // will turn into a suitable TypeError.
        SdfPath source =
            boost::python::extract<SdfPath>(keys[i]);
        SdfPath target =
            boost::python::extract<SdfPath>(sourceToTargetMap[keys[i]]);
        pathMap[source] = target;
    }

    PcpMapFunction mapFunction = PcpMapFunction::Create(pathMap, offset);

    // Return a newly allocated instance.  boost::python will free this
    // object when the holding python object expires.
    return new PcpMapFunction(mapFunction);
}

} // anonymous namespace 

void wrapMapFunction()
{    
    typedef PcpMapFunction This;

    TfPyContainerConversions::from_python_sequence<
        std::vector< PcpMapFunction >,
        TfPyContainerConversions::variable_capacity_policy >();

    boost::python::class_<This>("MapFunction")
        .def(boost::python::init<const This &>())
        .def("__init__",
             boost::python::make_constructor(
                &_Create,
                boost::python::default_call_policies(),
                (boost::python::arg("sourceToTargetMap"),
                 boost::python::arg("timeOffset") = SdfLayerOffset())))

        .def("__repr__", _Repr)
        .def("__str__", _Str)

        .def("Identity", &This::Identity,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("Identity")
        .def("IdentityPathMap", &This::IdentityPathMap,
             boost::python::return_value_policy<TfPyMapToDictionary>())
        .staticmethod("IdentityPathMap")
        .add_property("isIdentity", &This::IsIdentity)
        .add_property("isNull", &This::IsNull)

        .def("MapSourceToTarget", &This::MapSourceToTarget,
            (boost::python::arg("path")))
        .def("MapTargetToSource", &This::MapTargetToSource,
            (boost::python::arg("path")))
        .def("Compose", &This::Compose)
        .def("ComposeOffset", &This::ComposeOffset, boost::python::arg("offset"))
        .def("GetInverse", &This::GetInverse)

        .add_property("sourceToTargetMap",
            boost::python::make_function( &This::GetSourceToTargetMap,
                           boost::python::return_value_policy<TfPyMapToDictionary>()) )
        .add_property("timeOffset",
            boost::python::make_function( &This::GetTimeOffset,
                           boost::python::return_value_policy<boost::python::return_by_value>()) )

        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)
        ;
}
