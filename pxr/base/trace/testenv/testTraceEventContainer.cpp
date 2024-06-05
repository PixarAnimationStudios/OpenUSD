//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

static std::shared_ptr<TraceEventList>
CreateAppendedList()
{
    const TraceEvent::TimeStamp ms = 1;
    std::shared_ptr<TraceEventList> events(new TraceEventList);

    int numEvents = 200;

    for (int i = 0; i < numEvents; i++) {
        events->EmplaceBack(
            TraceEvent::Timespan,
            events->CacheKey("Timespan " + std::to_string(i)),
            i*ms,
            (i+1)*ms,
            TraceCategory::Default
        );
    }

    int numAppends = 7;

    for (int j = 0; j < numAppends; ++j) {
        TraceEventList otherEvents;

        for (int i = 0; i < numEvents; i++) {
            otherEvents.EmplaceBack(
                TraceEvent::Timespan,
                otherEvents.CacheKey("Timespan " + std::to_string(j) +
                                     ", " + std::to_string(i)),
                i*ms,
                (i+1)*ms,
                TraceCategory::Default
                );
        }

        events->Append(std::move(otherEvents));
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

    std::cout << "Appended list:" << std::endl;
    std::shared_ptr<TraceEventList> appendedEventList = 
        CreateAppendedList();
    _TestForwardIteration(appendedEventList);
    _TestReverseIteration(appendedEventList);

    std::cout << " PASSED\n";
}
