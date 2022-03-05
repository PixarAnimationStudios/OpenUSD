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

#include "pxr/usd/usdUtils/sparseValueWriter.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/python.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/def.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

static UsdUtilsSparseAttrValueWriter *
__init__(const UsdAttribute &attr, 
         const boost::python::object &defaultValue)
{
    return new UsdUtilsSparseAttrValueWriter(attr, 
            UsdPythonToSdfType(defaultValue, attr.GetTypeName()));
}

static bool
_WrapSetTimeSample(
    UsdUtilsSparseAttrValueWriter &vc, 
    const boost::python::object &value, 
    const UsdTimeCode time)
{
    return vc.SetTimeSample(
            UsdPythonToSdfType(value, vc.GetAttr().GetTypeName()), 
            time);
}

static bool
_WrapSetAttribute(
    UsdUtilsSparseValueWriter &vc, 
    const UsdAttribute & attr, 
    const boost::python::object &value,
    const UsdTimeCode time) 
{
    return vc.SetAttribute(attr,
            UsdPythonToSdfType(value, attr.GetTypeName()), 
            time);
}

static std::vector<UsdUtilsSparseAttrValueWriter>
_WrapGetSparseAttrValueWriters(
    UsdUtilsSparseValueWriter &vc) 
{
    return vc.GetSparseAttrValueWriters();
}

void wrapSparseValueWriter()
{
    boost::python::class_<UsdUtilsSparseAttrValueWriter>("SparseAttrValueWriter", 
            boost::python::no_init)
        .def("__init__", boost::python::make_constructor(__init__, boost::python::default_call_policies(),
             (boost::python::arg("attr"), boost::python::arg("defaultValue")=boost::python::object())))

        .def("SetTimeSample", _WrapSetTimeSample, 
            (boost::python::arg("value"), boost::python::arg("time")))
    ;

    boost::python::class_<UsdUtilsSparseValueWriter>("SparseValueWriter", boost::python::init<>())
        .def("SetAttribute", _WrapSetAttribute, 
            (boost::python::arg("attr"), boost::python::arg("value"), boost::python::arg("time")=UsdTimeCode::Default()))

        .def("GetSparseAttrValueWriters", _WrapGetSparseAttrValueWriters)
    ;

    // Register to and from vector conversions.
    boost::python::to_python_converter<std::vector<UsdUtilsSparseAttrValueWriter>,
        TfPySequenceToPython<std::vector<UsdUtilsSparseAttrValueWriter> > >();
}
