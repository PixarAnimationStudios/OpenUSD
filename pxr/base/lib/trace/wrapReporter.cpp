//
// Copyright 2018 Pixar
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

#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/reporterDataSourceCollector.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyEnum.h"

#include <boost/python/class.hpp>

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


static TraceReporterRefPtr
_Constructor1(const std::string &label)
{
    return TraceReporter::New(label, TraceReporterDataSourceCollector::New());
}

void wrapReporter()
{
    using This = TraceReporter;
    using ThisPtr = TraceReporterPtr;

    object reporter_class = 
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

        .add_property("aggregateTreeRoot", &This::GetAggregateTreeRoot)

        .def("UpdateAggregateTree", &This::UpdateAggregateTree)
        .def("UpdateEventTree", &This::UpdateEventTree)

        .def("ClearTree", &This::ClearTree)

        .add_property("groupByFunction",
            &This::GetGroupByFunction, &This::SetGroupByFunction)
        
        .add_property("foldRecursiveCalls",
            &This::GetFoldRecursiveCalls, &This::SetFoldRecursiveCalls)

        .add_static_property("globalReporter", &This::GetGlobalReporter)
        ;
};
