//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapRefPtrTracker.cpp

#include "pxr/pxr.h"

#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/refPtrTracker.h"
#include "pxr/base/tf/pySingleton.h"

#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE


namespace {

static
std::string
_ReportAllWatchedCounts(TfRefPtrTracker& tracker)
{
    std::ostringstream s;
    tracker.ReportAllWatchedCounts(s);
    return s.str();
}

static
std::string
_ReportAllTraces(TfRefPtrTracker& tracker)
{
    std::ostringstream s;
    tracker.ReportAllTraces(s);
    return s.str();
}

static
std::string
_ReportTracesForWatched(TfRefPtrTracker& tracker, uintptr_t ptr)
{
    std::ostringstream s;
    tracker.ReportTracesForWatched(s, (TfRefBase*)ptr);
    return s.str();
}

} // anonymous namespace 

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/stl/string.h>

void
wrapRefPtrTracker(nanobind::module_& m)
{
     typedef TfRefPtrTracker This;
     
     nanobind::class_<This>(m, "RefPtrTracker", nanobind::is_weak_referenceable())
        .def(TfNbSingleton())

        .def("GetAllWatchedCountsReport", _ReportAllWatchedCounts)
        .def("GetAllTracesReport", _ReportAllTraces)
        .def("GetTracesReportForWatched", _ReportTracesForWatched)
        ;
}
