//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/trace/reporter.h"

#include "pxr/base/trace/aggregateTree.h"
#include "pxr/base/trace/reporterDataSourceCollector.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static void
_Report(
    const TraceReporterPtr &self,
    int iterationCount)
{
    self->Report(std::cout, iterationCount);
}

static void
_ReportToFile(
    const TraceReporterPtr &self,
    const std::string &fileName,
    int iterationCount,
    bool append)
{
    std::ofstream os(fileName.c_str(),
                     append ? std::ios_base::app : std::ios_base::out);
    self->Report(os, iterationCount);
}

static void
_ReportTimes(TraceReporterPtr self)
{
    self->ReportTimes(std::cout);
}

static void
_ReportChromeTracing(
    const TraceReporterPtr &self)
{
    self->ReportChromeTracing(std::cout);
}

static void
_ReportChromeTracingToFile(
    const TraceReporterPtr &self,
    const std::string &fileName)
{
    std::ofstream os(fileName.c_str());
    self->ReportChromeTracing(os);
}

static std::vector<TraceReporter::ParsedTree> 
_LoadReport(
    const std::string &fileName)
{
    std::ifstream fileStream(fileName.c_str());
    if (!fileStream.is_open()) {
        TF_RUNTIME_ERROR("Failed to open file at %s", fileName.c_str());
        return {};
    }

    return TraceReporter::LoadReport(fileStream);
}

static TraceReporterRefPtr
_Constructor1(const std::string &label)
{
    return TraceReporter::New(label, TraceReporterDataSourceCollector::New());
}

void wrapReporter(nanobind::module_& m)
{
    namespace nb = nanobind;
    using This = TraceReporter;

    nb::class_<This> reporter_class(
        m, "Reporter", nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbRefAndWeakPtr(reporter_class);
    reporter_class
        .def(TfNbMakePyConstructor(_Constructor1))

        .def("GetLabel", &This::GetLabel)

        .def("Report", &::_Report,
             nb::arg("iterationCount") = 1)

        .def("Report", &::_ReportToFile,
             nb::arg("fileName"),
             nb::arg("iterationCount") = 1,
             nb::arg("append") = false)

        .def("ReportTimes", &::_ReportTimes)

        .def("ReportChromeTracing", &::_ReportChromeTracing)
        .def("ReportChromeTracingToFile", &::_ReportChromeTracingToFile)

        .def_static("LoadReport", &::_LoadReport,
                    nb::arg("fileName"))

        .def_prop_ro("aggregateTreeRoot", &This::GetAggregateTreeRoot)

        .def("UpdateTraceTrees", &This::UpdateTraceTrees)

        .def("ClearTree", &This::ClearTree)

        .def_prop_rw("groupByFunction",
            &This::GetGroupByFunction, &This::SetGroupByFunction)
        
        .def_prop_rw("foldRecursiveCalls",
            &This::GetFoldRecursiveCalls, &This::SetFoldRecursiveCalls)

        .def_prop_rw("shouldAdjustForOverheadAndNoise",
            &This::ShouldAdjustForOverheadAndNoise,
            &This::SetShouldAdjustForOverheadAndNoise)

        .def_prop_ro_static("globalReporter", [](nb::handle) {
            return This::GetGlobalReporter();
        })
        ;

    nb::class_<This::ParsedTree> parsedTreeCls(reporter_class, "ParsedTree");
    parsedTreeCls
        .def_prop_ro("tree", [](const This::ParsedTree &self) {
            return self.tree;
        })
        .def_ro("iterationCount", &This::ParsedTree::iterationCount)
        ;
};
