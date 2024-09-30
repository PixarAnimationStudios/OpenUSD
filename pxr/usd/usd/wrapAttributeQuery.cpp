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
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/makePyConstructor.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/tuple.hpp"

#include <string>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

static object
_GetBracketingTimeSamples(const UsdAttributeQuery& self, double desiredTime) 
{
    double lower = 0.0, upper = 0.0;
    bool hasTimeSamples = false;

    if (self.GetBracketingTimeSamples(
            desiredTime, &lower, &upper, &hasTimeSamples)) {
        return hasTimeSamples ? make_tuple(lower, upper) : make_tuple();
    }
    return object();
}

static TfPyObjWrapper
_Get(const UsdAttributeQuery& self, UsdTimeCode time) 
{
    VtValue val;
    self.Get(&val, time);
    return UsdVtValueToPython(val);
}

} // anonymous namespace 

void wrapUsdAttributeQuery()
{
    class_<UsdAttributeQuery, noncopyable>
        ("AttributeQuery", no_init)
        .def(init<const UsdAttribute&>(
                (arg("attribute"))))
        
        .def(init<const UsdPrim&, const TfToken&>(
                (arg("prim"), arg("attributeName"))))

        .def(init<const UsdAttribute&, const UsdResolveTarget&>(
                (arg("attribute"), arg("resolveTarget"))))

        .def("CreateQueries", &UsdAttributeQuery::CreateQueries,
             (arg("prim"), arg("attributeNames")),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("CreateQueries")
        
        .def("IsValid", &UsdAttributeQuery::IsValid)
        .def(!self)

        .def("GetAttribute", &UsdAttributeQuery::GetAttribute,
             return_value_policy<return_by_value>())

        .def("GetTimeSamples", _GetTimeSamples,
             return_value_policy<TfPySequenceToList>())

        .def("GetTimeSamplesInInterval", _GetTimeSamplesInInterval,
             arg("interval"),
             return_value_policy<TfPySequenceToList>())

        .def("GetUnionedTimeSamples", 
             _GetUnionedTimeSamples,
             arg("attrQueries"),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetUnionedTimeSamples")

        .def("GetUnionedTimeSamplesInInterval", 
             _GetUnionedTimeSamplesInInterval,
             (arg("attrQueries"), arg("interval")),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetUnionedTimeSamplesInInterval")

        .def("GetNumTimeSamples", &UsdAttributeQuery::GetNumTimeSamples)
        .def("GetBracketingTimeSamples", _GetBracketingTimeSamples,
             arg("desiredTime"))

        .def("HasValue", &UsdAttributeQuery::HasValue)
        .def("HasAuthoredValueOpinion", 
                &UsdAttributeQuery::HasAuthoredValueOpinion)
        .def("HasAuthoredValue", &UsdAttributeQuery::HasAuthoredValue)
        .def("HasFallbackValue", &UsdAttributeQuery::HasFallbackValue)

        .def("ValueMightBeTimeVarying", 
             &UsdAttributeQuery::ValueMightBeTimeVarying)

        .def("Get", _Get, arg("time")=UsdTimeCode::Default())
         
        ;

    TfPyRegisterStlSequencesFromPython<UsdAttributeQuery>();
}
