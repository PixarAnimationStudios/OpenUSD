//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/crateInfo.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"

using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

void wrapCrateInfo(nanobind::module_ &m)
{
    namespace nb = nanobind;

    nb::class_<SdfCrateInfo> thisClass(m, "CrateInfo");
    thisClass
        .def(nb::init<>())
        .def_static("Open", &SdfCrateInfo::Open, nb::arg("fileName"))
        .def("GetSummaryStats", &SdfCrateInfo::GetSummaryStats)
        .def("GetSections", &SdfCrateInfo::GetSections)
        .def("GetFileVersion", &SdfCrateInfo::GetFileVersion)
        .def("GetSoftwareVersion", &SdfCrateInfo::GetSoftwareVersion)
        .def("__bool__", [](const SdfCrateInfo &self) { return !(!self); })
        ;

    nb::class_<SdfCrateInfo::Section>(thisClass, "Section")
        .def(nb::init<>())
        .def(nb::init<string, int64_t, int64_t>(),
             nb::arg("name"), nb::arg("start"), nb::arg("size"))
        .def_rw("name", &SdfCrateInfo::Section::name)
        .def_rw("start", &SdfCrateInfo::Section::start)
        .def_rw("size", &SdfCrateInfo::Section::size)
        ;

    using SummaryStats = SdfCrateInfo::SummaryStats;
    nb::class_<SummaryStats>(thisClass, "SummaryStats")
        .def(nb::init<>())
        .def_rw("numSpecs", &SummaryStats::numSpecs)
        .def_rw("numUniquePaths", &SummaryStats::numUniquePaths)
        .def_rw("numUniqueTokens", &SummaryStats::numUniqueTokens)
        .def_rw("numUniqueStrings", &SummaryStats::numUniqueStrings)
        .def_rw("numUniqueFields", &SummaryStats::numUniqueFields)
        .def_rw("numUniqueFieldSets", &SummaryStats::numUniqueFieldSets)
        ;
}
