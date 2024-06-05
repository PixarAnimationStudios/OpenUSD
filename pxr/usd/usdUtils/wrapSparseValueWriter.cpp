//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/usd/usdUtils/sparseValueWriter.h"
#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/python.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static UsdUtilsSparseAttrValueWriter *
__init__(const UsdAttribute &attr, 
         const object &defaultValue)
{
    return new UsdUtilsSparseAttrValueWriter(attr, 
            UsdPythonToSdfType(defaultValue, attr.GetTypeName()));
}

static bool
_WrapSetTimeSample(
    UsdUtilsSparseAttrValueWriter &vc, 
    const object &value, 
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
    const object &value,
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
    class_<UsdUtilsSparseAttrValueWriter>("SparseAttrValueWriter", 
            no_init)
        .def("__init__", make_constructor(__init__, default_call_policies(),
             (arg("attr"), arg("defaultValue")=object())))

        .def("SetTimeSample", _WrapSetTimeSample, 
            (arg("value"), arg("time")))
    ;

    class_<UsdUtilsSparseValueWriter>("SparseValueWriter", init<>())
        .def("SetAttribute", _WrapSetAttribute, 
            (arg("attr"), arg("value"), arg("time")=UsdTimeCode::Default()))

        .def("GetSparseAttrValueWriters", _WrapGetSparseAttrValueWriters)
    ;

    // Register to and from vector conversions.
    boost::python::to_python_converter<std::vector<UsdUtilsSparseAttrValueWriter>,
        TfPySequenceToPython<std::vector<UsdUtilsSparseAttrValueWriter> > >();
}
