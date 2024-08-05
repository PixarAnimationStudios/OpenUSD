//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/wrapUtils.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python/class.hpp>
#include <boost/python/tuple.hpp>

#include <string>
#include <vector>

using std::string;
using std::vector;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static vector<double>
_GetTimeSamples(const UsdAttribute &self) {
    vector<double> result;
    self.GetTimeSamples(&result);
    return result;
}

static vector<double>
_GetTimeSamplesInInterval(const UsdAttribute &self,
                          const GfInterval& interval) {
    vector<double> result;
    self.GetTimeSamplesInInterval(interval, &result);
    return result;
}

static vector<double>
_GetUnionedTimeSamples(const vector<UsdAttribute> &attrs) {
    vector<double> result;
    UsdAttribute::GetUnionedTimeSamples(attrs, &result);
    return result;
}

static vector<double>
_GetUnionedTimeSamplesInInterval(const vector<UsdAttribute> &attrs,
                                 const GfInterval &interval) {
    vector<double> result;
    UsdAttribute::GetUnionedTimeSamplesInInterval(attrs, interval, &result);
    return result;
}

static object
_GetBracketingTimeSamples(const UsdAttribute &self, double desiredTime) {
    double lower = 0.0, upper = 0.0;
    bool hasTimeSamples = false;

    if (self.GetBracketingTimeSamples(
            desiredTime, &lower, &upper, &hasTimeSamples)) {
        return hasTimeSamples ? make_tuple(lower, upper) : make_tuple();
    }
    return object();
}

static TfPyObjWrapper
_Get(const UsdAttribute &self, UsdTimeCode time) {
    VtValue val;
    self.Get(&val, time);
    return UsdVtValueToPython(val);
}

static bool
_Set(const UsdAttribute &self, object val, const UsdTimeCode &time) {
    return self.Set(UsdPythonToSdfType(val, self.GetTypeName()), time);
}

static SdfPathVector
_GetConnections(const UsdAttribute &self)
{
    SdfPathVector result;
    self.GetConnections(&result);
    return result;
}

static string
__repr__(const UsdAttribute &self) {
    return self ? TfStringPrintf("%s.GetAttribute(%s)",
                                 TfPyRepr(self.GetPrim()).c_str(),
                                 TfPyRepr(self.GetName()).c_str())
        : "invalid " + self.GetDescription();
}

} // anonymous namespace 

void wrapUsdAttribute()
{
    class_<UsdAttribute, bases<UsdProperty> >("Attribute")
        .def(Usd_ObjectSubclass())
        .def("__repr__", __repr__)

        .def("GetVariability", &UsdAttribute::GetVariability)
        .def("SetVariability", &UsdAttribute::SetVariability,
             arg("variability"))

        .def("GetTypeName", &UsdAttribute::GetTypeName)
        .def("SetTypeName", &UsdAttribute::SetTypeName, arg("typeName"))

        .def("GetRoleName", &UsdAttribute::GetRoleName)

        .def("GetColorSpace", &UsdAttribute::GetColorSpace)
        .def("SetColorSpace", &UsdAttribute::SetColorSpace)
        .def("HasColorSpace", &UsdAttribute::HasColorSpace)
        .def("ClearColorSpace", &UsdAttribute::ClearColorSpace)

        .def("GetTimeSamples", _GetTimeSamples,
             return_value_policy<TfPySequenceToList>())

        .def("GetTimeSamplesInInterval", _GetTimeSamplesInInterval,
             arg("interval"),
             return_value_policy<TfPySequenceToList>())

        .def("GetUnionedTimeSamples", 
             _GetUnionedTimeSamples,
             arg("attrs"),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetUnionedTimeSamples")

        .def("GetUnionedTimeSamplesInInterval", 
             _GetUnionedTimeSamplesInInterval,
             (arg("attrs"), arg("interval")),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetUnionedTimeSamplesInInterval")

        .def("GetNumTimeSamples", &UsdAttribute::GetNumTimeSamples)

        .def("GetBracketingTimeSamples", _GetBracketingTimeSamples,
             arg("desiredTime"))

        .def("HasValue", &UsdAttribute::HasValue)
        .def("HasAuthoredValueOpinion", &UsdAttribute::HasAuthoredValueOpinion)
        .def("HasAuthoredValue", &UsdAttribute::HasAuthoredValue)
        .def("HasFallbackValue", &UsdAttribute::HasFallbackValue)

        .def("ValueMightBeTimeVarying", &UsdAttribute::ValueMightBeTimeVarying)

        .def("Get", _Get, arg("time")=UsdTimeCode::Default())
        .def("Set", _Set, (arg("value"), arg("time")=UsdTimeCode::Default()))

        .def("HasSpline", &UsdAttribute::HasSpline)
        .def("GetSpline", &UsdAttribute::GetSpline)
        .def("SetSpline", &UsdAttribute::SetSpline)

        .def("GetResolveInfo", 
             (UsdResolveInfo (UsdAttribute::*)(UsdTimeCode) const) 
                 &UsdAttribute::GetResolveInfo,
             arg("time"))
        .def("GetResolveInfo", 
             (UsdResolveInfo (UsdAttribute::*)() const) 
                 &UsdAttribute::GetResolveInfo)

        .def("Clear", &UsdAttribute::Clear)
        .def("ClearAtTime", &UsdAttribute::ClearAtTime, arg("time"))
        .def("ClearDefault", &UsdAttribute::ClearDefault)

        .def("Block", &UsdAttribute::Block)

        .def("AddConnection", &UsdAttribute::AddConnection,
             (arg("source"),
              arg("position")=UsdListPositionBackOfPrependList))
        .def("RemoveConnection", &UsdAttribute::RemoveConnection, arg("source"))
        .def("SetConnections", &UsdAttribute::SetConnections, arg("sources"))
        .def("ClearConnections", &UsdAttribute::ClearConnections)
        .def("GetConnections", _GetConnections,
             return_value_policy<TfPySequenceToList>())
        .def("HasAuthoredConnections", &UsdAttribute::HasAuthoredConnections)
        ;

    TfPyRegisterStlSequencesFromPython<UsdAttribute>();
    to_python_converter<std::vector<UsdAttribute>,
                        TfPySequenceToPython<std::vector<UsdAttribute>>>();
}
