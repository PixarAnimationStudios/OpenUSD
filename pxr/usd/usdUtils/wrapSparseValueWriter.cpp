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
    ;
}
