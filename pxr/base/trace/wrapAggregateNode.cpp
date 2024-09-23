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

#include "pxr/external/boost/python/class.hpp"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


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
        TraceAggregateNode::Id(), 
        key, ArchSecondsToTicks(timeMS / 1e3), count, exclusiveCount);
}

void wrapAggregateNode()
{
    using This = TraceAggregateNode;
    using ThisPtr = TraceAggregateNodePtr;

    class_<This, ThisPtr>("AggregateNode", no_init)
        .def(TfPyWeakPtr())
        .def("__init__", 
            TfMakePyConstructor(_New),
            (arg("key") = TfToken("root"),
             arg("timeMS") = 0,
             arg("count") = 1,
             arg("exclusiveCount") = 1))
        .add_property("key", &This::GetKey )
        .add_property("id", 
            make_function(&This::GetId, 
                          return_value_policy<return_by_value>()))
        .add_property("count", GetCount)
        .add_property("exclusiveCount", &This::GetExclusiveCount)
        .add_property("inclusiveTime", GetInclusiveTime)
        .add_property("exclusiveTime", GetExclusiveTime)
        .add_property("children", 
            make_function(&This::GetChildren,
                          return_value_policy<TfPySequenceToList>()) )
        .add_property("expanded", &This::IsExpanded, &This::SetExpanded)
        .def("Append", _Append)
        ;

};
