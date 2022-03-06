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
#include "pxr/usd/usd/attributeQuery.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/makePyConstructor.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/tuple.hpp>

#include <string>
#include <vector>



PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdWrapAttributeQuery {

static std::vector<double>
_GetTimeSamples(const UsdAttributeQuery& query) 
{
    std::vector<double> result;
    query.GetTimeSamples(&result);
    return result;
}

static std::vector<double>
_GetTimeSamplesInInterval(const UsdAttributeQuery& query,
                          const GfInterval& interval) {
    std::vector<double> result;
    query.GetTimeSamplesInInterval(interval, &result);
    return result;
}

static std::vector<double>
_GetUnionedTimeSamples(const std::vector<UsdAttributeQuery> & attrQueries) 
{
    std::vector<double> result;
    UsdAttributeQuery::GetUnionedTimeSamples(attrQueries, &result);
    return result;
}

static std::vector<double>
_GetUnionedTimeSamplesInInterval(const std::vector<UsdAttributeQuery>& attrQueries,
                                 const GfInterval& interval) {
    std::vector<double> result;
    UsdAttributeQuery::GetUnionedTimeSamplesInInterval(attrQueries, interval,
                                                       &result);
    return result;
}

static boost::python::object
_GetBracketingTimeSamples(const UsdAttributeQuery& self, double desiredTime) 
{
    double lower = 0.0, upper = 0.0;
    bool hasTimeSamples = false;

    if (self.GetBracketingTimeSamples(
            desiredTime, &lower, &upper, &hasTimeSamples)) {
        return hasTimeSamples ? boost::python::make_tuple(lower, upper) : boost::python::make_tuple();
    }
    return boost::python::object();
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
    boost::python::class_<UsdAttributeQuery, boost::noncopyable>
        ("AttributeQuery", boost::python::no_init)
        .def(boost::python::init<const UsdAttribute&>(
                (boost::python::arg("attribute"))))
        
        .def(boost::python::init<const UsdPrim&, const TfToken&>(
                (boost::python::arg("prim"), boost::python::arg("attributeName"))))

        .def("CreateQueries", &UsdAttributeQuery::CreateQueries,
             (boost::python::arg("prim"), boost::python::arg("attributeNames")),
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("CreateQueries")
        
        .def("IsValid", &UsdAttributeQuery::IsValid)
        .def(!boost::python::self)

        .def("GetAttribute", &UsdAttributeQuery::GetAttribute,
             boost::python::return_value_policy<boost::python::return_by_value>())

        .def("GetTimeSamples", pxrUsdUsdWrapAttributeQuery::_GetTimeSamples,
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetTimeSamplesInInterval", pxrUsdUsdWrapAttributeQuery::_GetTimeSamplesInInterval,
             boost::python::arg("interval"),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetUnionedTimeSamples", 
             pxrUsdUsdWrapAttributeQuery::_GetUnionedTimeSamples,
             boost::python::arg("attrQueries"),
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetUnionedTimeSamples")

        .def("GetUnionedTimeSamplesInInterval", 
             pxrUsdUsdWrapAttributeQuery::_GetUnionedTimeSamplesInInterval,
             (boost::python::arg("attrQueries"), boost::python::arg("interval")),
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetUnionedTimeSamplesInInterval")

        .def("GetNumTimeSamples", &UsdAttributeQuery::GetNumTimeSamples)
        .def("GetBracketingTimeSamples", pxrUsdUsdWrapAttributeQuery::_GetBracketingTimeSamples,
             boost::python::arg("desiredTime"))

        .def("HasValue", &UsdAttributeQuery::HasValue)
        .def("HasAuthoredValueOpinion", 
                &UsdAttributeQuery::HasAuthoredValueOpinion)
        .def("HasAuthoredValue", &UsdAttributeQuery::HasAuthoredValue)
        .def("HasFallbackValue", &UsdAttributeQuery::HasFallbackValue)

        .def("ValueMightBeTimeVarying", 
             &UsdAttributeQuery::ValueMightBeTimeVarying)

        .def("Get", pxrUsdUsdWrapAttributeQuery::_Get, boost::python::arg("time")=UsdTimeCode::Default())
         
        ;

    TfPyRegisterStlSequencesFromPython<UsdAttributeQuery>();
}
