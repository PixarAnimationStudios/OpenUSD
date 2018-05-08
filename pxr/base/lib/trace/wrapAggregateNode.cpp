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

#include "pxr/base/trace/aggregateNode.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/timing.h"

#include <boost/python/class.hpp>

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;


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

void wrapEventNode()
{
    using This = TraceAggregateNode;
    using ThisPtr = TraceAggregateNodePtr;

    class_<This, ThisPtr>("AggregateNode", no_init)
        .def(TfPyWeakPtr())
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
        
        ;

};
