//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/trace/aggregateNode.h"

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/timing.h"


#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE



static double
GetInclusiveTime(TraceAggregateNodePtr &self) {
    return ArchTicksToSeconds( 
            uint64_t(self->GetInclusiveTime() * 1e3) );
}

static double
GetExclusiveTime(TraceAggregateNodePtr &self) {
    return ArchTicksToSeconds( 
            uint64_t(self->GetExclusiveTime(false /* recursive */) * 1e3) );
}

static int
GetCount(TraceAggregateNodePtr &self) {
    return self->GetCount(false /* recursive */);
}

static void
_Append(
    TraceAggregateNodePtr &self, 
    const TraceAggregateNodePtr &other)
{
    self->Append(other);
}

static TraceAggregateNodeRefPtr
_New(const TfToken &key,
    const double timeMS,
    const int count,
    const int exclusiveCount)
{
    return TraceAggregateNode::New(
        key, ArchSecondsToTicks(timeMS / 1e3), count, exclusiveCount);
}

void wrapAggregateNode(nanobind::module_& m)
{
    namespace nb = nanobind;
    using This = TraceAggregateNode;

    nb::class_<This> cls(m, "AggregateNode",
                         nb::is_weak_referenceable(), nb::dynamic_attr());
    TfNbWeakPtr(cls);
    cls.def(TfNbMakePyConstructor(_New),
            nb::arg("key") = TfToken("root"),
            nb::arg("timeMS") = 0,
            nb::arg("count") = 1,
            nb::arg("exclusiveCount") = 1)
        .def_prop_ro("key", &This::GetKey)
        .def_prop_ro("count", GetCount)
        .def_prop_ro("exclusiveCount", &This::GetExclusiveCount)
        .def_prop_ro("inclusiveTime", GetInclusiveTime)
        .def_prop_ro("exclusiveTime", GetExclusiveTime)
        .def_prop_ro("children", &This::GetChildren)
        .def_prop_rw("expanded", &This::IsExpanded, &This::SetExpanded)
        .def("Append", _Append)
        ;
};
