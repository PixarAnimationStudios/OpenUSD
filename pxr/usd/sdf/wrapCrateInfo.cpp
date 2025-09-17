//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/crateInfo.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python.hpp"

using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void wrapCrateInfo()
{
    scope thisClass = class_<SdfCrateInfo>("CrateInfo")
        .def("Open", &SdfCrateInfo::Open, arg("fileName"))
        .staticmethod("Open")
        .def("GetSummaryStats", &SdfCrateInfo::GetSummaryStats)
        .def("GetSections", &SdfCrateInfo::GetSections,
             return_value_policy<TfPySequenceToList>())
        .def("GetFileVersion", &SdfCrateInfo::GetFileVersion)
        .def("GetSoftwareVersion", &SdfCrateInfo::GetSoftwareVersion)
        .def(!self)
        ;

    class_<SdfCrateInfo::Section>("Section")
        .def(init<string, int64_t, int64_t>(
                 (arg("name"), arg("start"), arg("size"))))
        .def_readwrite("name", &SdfCrateInfo::Section::name)
        .def_readwrite("start", &SdfCrateInfo::Section::start)
        .def_readwrite("size", &SdfCrateInfo::Section::size)
        ;

    using SummaryStats = SdfCrateInfo::SummaryStats;
    class_<SummaryStats>("SummaryStats")
        .def_readwrite("numSpecs", &SummaryStats::numSpecs)
        .def_readwrite("numUniquePaths", &SummaryStats::numUniquePaths)
        .def_readwrite("numUniqueTokens", &SummaryStats::numUniqueTokens)
        .def_readwrite("numUniqueStrings", &SummaryStats::numUniqueStrings)
        .def_readwrite("numUniqueFields", &SummaryStats::numUniqueFields)
        .def_readwrite("numUniqueFieldSets", &SummaryStats::numUniqueFieldSets)
        ;
}
