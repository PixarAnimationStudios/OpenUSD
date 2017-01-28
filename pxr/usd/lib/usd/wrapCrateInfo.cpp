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
#include "pxr/usd/usd/crateInfo.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>

PXR_NAMESPACE_OPEN_SCOPE


using std::string;
using namespace boost::python;

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

PXR_NAMESPACE_CLOSE_SCOPE

