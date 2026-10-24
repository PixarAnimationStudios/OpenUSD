//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/attributeQuery.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/makePyConstructor.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <string>
#include <vector>

namespace nb = nanobind;
using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static vector<double>
_GetTimeSamples(const UsdAttributeQuery& query) 
{
    vector<double> result;
    query.GetTimeSamples(&result);
    return result;
}

static vector<double>
_GetTimeSamplesInInterval(const UsdAttributeQuery& query,
                          const GfInterval& interval) {
    vector<double> result;
    query.GetTimeSamplesInInterval(interval, &result);
    return result;
}

static vector<double>
_GetUnionedTimeSamples(const vector<UsdAttributeQuery> & attrQueries) 
{
    vector<double> result;
    UsdAttributeQuery::GetUnionedTimeSamples(attrQueries, &result);
    return result;
}

static vector<double>
_GetUnionedTimeSamplesInInterval(const vector<UsdAttributeQuery>& attrQueries,
                                 const GfInterval& interval) {
    vector<double> result;
    UsdAttributeQuery::GetUnionedTimeSamplesInInterval(attrQueries, interval,
                                                       &result);
    return result;
}

static nb::object
_GetBracketingTimeSamples(const UsdAttributeQuery& self, double desiredTime) 
{
    double lower = 0.0, upper = 0.0;
    bool hasTimeSamples = false;

    if (self.GetBracketingTimeSamples(
            desiredTime, &lower, &upper, &hasTimeSamples)) {
        return hasTimeSamples ? nb::make_tuple(lower, upper) : nb::make_tuple();
    }
    return nb::none();
}

static TfPyObjWrapper
_Get(const UsdAttributeQuery& self, UsdTimeCode time) 
{
    VtValue val;
    self.Get(&val, time);
    return UsdVtValueToPython(val);
}

static TfPyObjWrapper
_GetFallbackValue(const UsdAttributeQuery &self)
{
    VtValue val;
    self.GetFallbackValue(&val);
    return UsdVtValueToPython(val);
}

} // anonymous namespace 

void wrapUsdAttributeQuery(nb::module_ &m)
{
    nb::class_<UsdAttributeQuery>(m, "AttributeQuery")
        .def(nb::init<const UsdAttribute&>(),
             nb::arg("attribute"))
        
        .def(nb::init<const UsdPrim&, const TfToken&>(),
             nb::arg("prim"), nb::arg("attributeName"))

        .def(nb::init<const UsdAttribute&, const UsdResolveTarget&>(),
             nb::arg("attribute"), nb::arg("resolveTarget"))

        .def_static("CreateQueries", &UsdAttributeQuery::CreateQueries,
             nb::arg("prim"), nb::arg("attributeNames"))
        
        .def("IsValid", &UsdAttributeQuery::IsValid)
        .def("__bool__", [](const UsdAttributeQuery &self) { return bool(self); })

        .def("GetAttribute", &UsdAttributeQuery::GetAttribute,
             nb::rv_policy::copy)

        .def("GetTimeSamples", _GetTimeSamples)

        .def("GetTimeSamplesInInterval", _GetTimeSamplesInInterval,
             nb::arg("interval"))

        .def_static("GetUnionedTimeSamples", 
             _GetUnionedTimeSamples,
             nb::arg("attrQueries"))

        .def_static("GetUnionedTimeSamplesInInterval", 
             _GetUnionedTimeSamplesInInterval,
             nb::arg("attrQueries"), nb::arg("interval"))

        .def("GetNumTimeSamples", &UsdAttributeQuery::GetNumTimeSamples)
        .def("GetBracketingTimeSamples", _GetBracketingTimeSamples,
             nb::arg("desiredTime"))

        .def("HasValue", &UsdAttributeQuery::HasValue)
        .def("HasSpline", &UsdAttributeQuery::HasSpline)
        .def("HasAuthoredValueOpinion", 
                &UsdAttributeQuery::HasAuthoredValueOpinion)
        .def("HasAuthoredValue", &UsdAttributeQuery::HasAuthoredValue)
        .def("HasFallbackValue", &UsdAttributeQuery::HasFallbackValue)
        .def("GetFallbackValue", _GetFallbackValue)

        .def("ValueMightBeTimeVarying", 
             &UsdAttributeQuery::ValueMightBeTimeVarying)
        .def("ValueSourceMightBeTimeVarying", 
             &UsdAttributeQuery::ValueSourceMightBeTimeVarying)

        .def("Get", _Get, nb::arg("time")=UsdTimeCode::Default())
        .def("GetSpline", &UsdAttributeQuery::GetSpline,
             nb::rv_policy::copy)
         
        ;

    TfPyRegisterStlSequencesFromPython<UsdAttributeQuery>();
}
