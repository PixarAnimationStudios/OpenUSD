//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/crateInfo.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>

using std::string;
using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdCrateInfo()
{
    scope thisClass = class_<UsdCrateInfo>("CrateInfo")
        .def("Open", &UsdCrateInfo::Open, arg("fileName"))
        .staticmethod("Open")
        .def("GetSummaryStats", &UsdCrateInfo::GetSummaryStats)
        .def("GetSections", &UsdCrateInfo::GetSections,
             return_value_policy<TfPySequenceToList>())
        .def("GetFileVersion", &UsdCrateInfo::GetFileVersion)
        .def("GetSoftwareVersion", &UsdCrateInfo::GetSoftwareVersion)
        .def(!self)
        ;

    class_<UsdCrateInfo::Section>("Section")
        .def(init<string, int64_t, int64_t>(
                 (arg("name"), arg("start"), arg("size"))))
        .def_readwrite("name", &UsdCrateInfo::Section::name)
        .def_readwrite("start", &UsdCrateInfo::Section::start)
        .def_readwrite("size", &UsdCrateInfo::Section::size)
        ;

    using SummaryStats = UsdCrateInfo::SummaryStats;
    class_<SummaryStats>("SummaryStats")
        .def_readwrite("numSpecs", &SummaryStats::numSpecs)
        .def_readwrite("numUniquePaths", &SummaryStats::numUniquePaths)
        .def_readwrite("numUniqueTokens", &SummaryStats::numUniqueTokens)
        .def_readwrite("numUniqueStrings", &SummaryStats::numUniqueStrings)
        .def_readwrite("numUniqueFields", &SummaryStats::numUniqueFields)
        .def_readwrite("numUniqueFieldSets", &SummaryStats::numUniqueFieldSets)
        ;
}
