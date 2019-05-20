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

#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/event.h"
#include "pxr/base/trace/collectionNotice.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static std::shared_ptr<TraceEventList>
CreateInnerOnlyTestEvents() {
    const TraceEvent::TimeStamp ms = 1;
    std::shared_ptr<TraceEventList> events = std::make_shared<TraceEventList>();

    events->EmplaceBack(
        TraceEvent::Timespan,
        events->CacheKey("Test Timespan 1"),
        2*ms,
        6*ms,
        TraceCategory::Default
    );

    events->EmplaceBack(
        TraceEvent::Marker,
        events->CacheKey("Test Marker"),
        3*ms,
        TraceCategory::Default
    );

    events->EmplaceBack(
        TraceEvent::Timespan,
        events->CacheKey("Test Timespan 2"),
        7*ms,
        9*ms,
        TraceCategory::Default
    );

    return events;
}

static std::shared_ptr<TraceEventList>
CreateUseOuterTestEvents() {
    const TraceEvent::TimeStamp ms = 1;
    std::shared_ptr<TraceEventList> events(new TraceEventList);

    int numEvents = 20;
    for (int i = 0; i < numEvents; i++) {
        events->EmplaceBack(
            TraceEvent::Timespan,
            events->CacheKey("Timespan " + std::to_string(i)),
            i*ms,
            (i+1)*ms,
            TraceCategory::Default
        );
    }
    return events;
}

static void
_TestForwardIteration(
    const std::shared_ptr<TraceEventList>& eventList)
{

    std::cout << "    Forward" << std::endl;
    for(TraceEventList::const_iterator iter = eventList->begin(); 
        iter != eventList->end(); ++iter){
        const TraceEvent& e = *iter;
        std::cout << "        Found event" << std::endl;
        std::cout << "            Begin: " << e.GetStartTimeStamp() << std::endl;
        std::cout << "            End: " << e.GetEndTimeStamp() << std::endl;
    }   
}


static void
_TestReverseIteration(
    const std::shared_ptr<TraceEventList>& eventList)
{
    std::cout << "    Reverse" << std::endl;

    for(TraceEventList::const_reverse_iterator iter = eventList->rbegin(); 
        iter != eventList->rend(); ++iter){
        const TraceEvent& e = *iter;
        std::cout << "        Found event" << std::endl;
        std::cout << "            Begin: " << e.GetStartTimeStamp() << std::endl;
        std::cout << "            End: " << e.GetEndTimeStamp() << std::endl;
    }   
}

int
main(int argc, char *argv[]) 
{
    std::cout << "Empty list:" << std::endl;
    std::shared_ptr<TraceEventList> emptyEvents = 
        std::make_shared<TraceEventList>();
    TF_AXIOM(emptyEvents->begin() == emptyEvents->end());
    TF_AXIOM(emptyEvents->rbegin() == emptyEvents->rend());
    _TestForwardIteration(emptyEvents);
    _TestReverseIteration(emptyEvents);

    std::cout << "Inner Only list:" << std::endl;
    std::shared_ptr<TraceEventList> innerOnlyEventList = 
        CreateInnerOnlyTestEvents();
    _TestForwardIteration(innerOnlyEventList);
    _TestReverseIteration(innerOnlyEventList);

    std::cout << "Use Outer list:" << std::endl;
    std::shared_ptr<TraceEventList> useOuterEventList = 
        CreateUseOuterTestEvents();
    _TestForwardIteration(useOuterEventList);
    _TestReverseIteration(useOuterEventList);

    std::cout << " PASSED\n";
}