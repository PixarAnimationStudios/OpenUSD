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
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/wrapUtils.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/tuple.hpp>

#include <string>
#include <vector>



PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static std::vector<double>
_GetTimeSamples(const UsdAttribute &self) {
    std::vector<double> result;
    self.GetTimeSamples(&result);
    return result;
}

static std::vector<double>
_GetTimeSamplesInInterval(const UsdAttribute &self,
                          const GfInterval& interval) {
    std::vector<double> result;
    self.GetTimeSamplesInInterval(interval, &result);
    return result;
}

static std::vector<double>
_GetUnionedTimeSamples(const std::vector<UsdAttribute> &attrs) {
    std::vector<double> result;
    UsdAttribute::GetUnionedTimeSamples(attrs, &result);
    return result;
}

static std::vector<double>
_GetUnionedTimeSamplesInInterval(const std::vector<UsdAttribute> &attrs,
                                 const GfInterval &interval) {
    std::vector<double> result;
    UsdAttribute::GetUnionedTimeSamplesInInterval(attrs, interval, &result);
    return result;
}

static boost::python::object
_GetBracketingTimeSamples(const UsdAttribute &self, double desiredTime) {
    double lower = 0.0, upper = 0.0;
    bool hasTimeSamples = false;

    if (self.GetBracketingTimeSamples(
            desiredTime, &lower, &upper, &hasTimeSamples)) {
        return hasTimeSamples ? boost::python::make_tuple(lower, upper) : boost::python::make_tuple();
    }
    return boost::python::object();
}

static TfPyObjWrapper
_Get(const UsdAttribute &self, UsdTimeCode time) {
    VtValue val;
    self.Get(&val, time);
    return UsdVtValueToPython(val);
}

static bool
_Set(const UsdAttribute &self, boost::python::object val, const UsdTimeCode &time) {
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static SdfPathVector
_GetConnections(const UsdAttribute &self)
{
    SdfPathVector result;
    self.GetConnections(&result);
    return result;
}

static std::string
__repr__(const UsdAttribute &self) {
    return self ? TfStringPrintf("%s.GetAttribute(%s)",
                                 TfPyRepr(self.GetPrim()).c_str(),
                                 TfPyRepr(self.GetName()).c_str())
        : "invalid " + self.GetDescription();
}

} // anonymous namespace 

void wrapUsdAttribute()
{
    boost::python::class_<UsdAttribute, boost::python::bases<UsdProperty> >("Attribute")
        .def(Usd_ObjectSubclass())
        .def("__repr__", __repr__)

        .def("GetVariability", &UsdAttribute::GetVariability)
        .def("SetVariability", &UsdAttribute::SetVariability,
             boost::python::arg("variability"))

        .def("GetTypeName", &UsdAttribute::GetTypeName)
        .def("SetTypeName", &UsdAttribute::SetTypeName, boost::python::arg("typeName"))

        .def("GetRoleName", &UsdAttribute::GetRoleName)

        .def("GetColorSpace", &UsdAttribute::GetColorSpace)
        .def("SetColorSpace", &UsdAttribute::SetColorSpace)
        .def("HasColorSpace", &UsdAttribute::HasColorSpace)
        .def("ClearColorSpace", &UsdAttribute::ClearColorSpace)

        .def("GetTimeSamples", _GetTimeSamples,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetTimeSamplesInInterval", _GetTimeSamplesInInterval,
             boost::python::arg("interval"),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetUnionedTimeSamples", 
             _GetUnionedTimeSamples,
             boost::python::arg("attrs"),
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetUnionedTimeSamples")

        .def("GetUnionedTimeSamplesInInterval", 
             _GetUnionedTimeSamplesInInterval,
             (boost::python::arg("attrs"), boost::python::arg("interval")),
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetUnionedTimeSamplesInInterval")

        .def("GetNumTimeSamples", &UsdAttribute::GetNumTimeSamples)

        .def("GetBracketingTimeSamples", _GetBracketingTimeSamples,
             boost::python::arg("desiredTime"))

        .def("HasValue", &UsdAttribute::HasValue)
        .def("HasAuthoredValueOpinion", &UsdAttribute::HasAuthoredValueOpinion)
        .def("HasAuthoredValue", &UsdAttribute::HasAuthoredValue)
        .def("HasFallbackValue", &UsdAttribute::HasFallbackValue)

        .def("ValueMightBeTimeVarying", &UsdAttribute::ValueMightBeTimeVarying)

        .def("Get", _Get, boost::python::arg("time")=UsdTimeCode::Default())
        .def("Set", _Set, (boost::python::arg("value"), boost::python::arg("time")=UsdTimeCode::Default()))

        .def("GetResolveInfo", &UsdAttribute::GetResolveInfo,
             boost::python::arg("time")=UsdTimeCode::Default())

        .def("Clear", &UsdAttribute::Clear)
        .def("ClearAtTime", &UsdAttribute::ClearAtTime, boost::python::arg("time"))
        .def("ClearDefault", &UsdAttribute::ClearDefault)

        .def("Block", &UsdAttribute::Block)

        .def("AddConnection", &UsdAttribute::AddConnection,
             (boost::python::arg("source"),
              boost::python::arg("position")=UsdListPositionBackOfPrependList))
        .def("RemoveConnection", &UsdAttribute::RemoveConnection, boost::python::arg("source"))
        .def("SetConnections", &UsdAttribute::SetConnections, boost::python::arg("sources"))
        .def("ClearConnections", &UsdAttribute::ClearConnections)
        .def("GetConnections", _GetConnections,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("HasAuthoredConnections", &UsdAttribute::HasAuthoredConnections)
        ;

    TfPyRegisterStlSequencesFromPython<UsdAttribute>();
    boost::python::to_python_converter<std::vector<UsdAttribute>,
                        TfPySequenceToPython<std::vector<UsdAttribute>>>();
}
