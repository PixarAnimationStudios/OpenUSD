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

#include <boost/python/class.hpp>
#include <boost/python/scope.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;

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

void wrapReporter()
{
    using This = TraceReporter;
    using ThisPtr = TraceReporterPtr;

    scope reporter_class = 
        class_<This, ThisPtr, boost::noncopyable>("Reporter", no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(_Constructor1))

        .def("GetLabel", &This::GetLabel,
             return_value_policy<return_by_value>())

        .def("Report", &::_Report,
             (arg("iterationCount")=1))

        .def("Report", &::_ReportToFile,
             (arg("iterationCount")=1,
              arg("append")=false))

        .def("ReportTimes", &::_ReportTimes)

        .def("ReportChromeTracing", &::_ReportChromeTracing)
        .def("ReportChromeTracingToFile", &::_ReportChromeTracingToFile)

        .def("LoadReport", &::_LoadReport, 
            (arg("fileName")),
            return_value_policy<TfPySequenceToList>())
        .staticmethod("LoadReport")

        .add_property("aggregateTreeRoot", &This::GetAggregateTreeRoot)

        .def("UpdateTraceTrees", &This::UpdateTraceTrees)

        .def("ClearTree", &This::ClearTree)

        .add_property("groupByFunction",
            &This::GetGroupByFunction, &This::SetGroupByFunction)
        
        .add_property("foldRecursiveCalls",
            &This::GetFoldRecursiveCalls, &This::SetFoldRecursiveCalls)

        .add_property("shouldAdjustForOverheadAndNoise",
            &This::ShouldAdjustForOverheadAndNoise,
            &This::SetShouldAdjustForOverheadAndNoise)

        .add_static_property("globalReporter", &This::GetGlobalReporter)
        ;

    class_<This::ParsedTree>("ParsedTree", no_init)
        .add_property("tree", 
            make_function(+[](const This::ParsedTree &self) {
                return self.tree;
            },
            return_value_policy<TfPyRefPtrFactory<>>()))
        .def_readonly("iterationCount", &This::ParsedTree::iterationCount)
        ;
};
