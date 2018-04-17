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
#include "pxr/base/trace/collectionNotice.h"
#include "pxr/base/trace/serialization.h"

#include <fstream>

PXR_NAMESPACE_USING_DIRECTIVE

// Declare a custom Trace category
constexpr TraceCategoryId TestCategory 
    = TraceCategory::CreateTraceCategoryId("TestCategory");

static std::unique_ptr<TraceEventList>
CreateTestEvents(TraceEvent::TimeStamp timeStampOffset)
{
    const TraceEvent::TimeStamp ms = ArchSecondsToTicks(0.001);
    timeStampOffset += ms;
    std::unique_ptr<TraceEventList> events(new TraceEventList);

    static constexpr TraceStaticKeyData counterKey("Test Counter");
    {
        TraceEvent counterEvent(
            TraceEvent::CounterDelta,
            counterKey,
            1,
            TraceCategory::Default
        );
        counterEvent.SetTimeStamp(2*ms + timeStampOffset);
        events->EmplaceBack(counterEvent);
    }

    events->EmplaceBack(
        TraceEvent::Begin,
        events->CacheKey("Inner Scope 2"),
        3*ms + timeStampOffset,
        TestCategory
    );
    events->EmplaceBack(
        TraceEvent::Begin,
        events->CacheKey("Inner Scope 2"),
        4*ms + timeStampOffset,
        TestCategory
    );

    {
        bool data = true;
        TraceEvent dataEvent(
            TraceEvent::Data,
            events->CacheKey("Test Data 0"),
            data,
            TraceCategory::Default
        );
        dataEvent.SetTimeStamp(5*ms+timeStampOffset);
        events->EmplaceBack(dataEvent);
    }
    {
        int data = -2;
        TraceEvent dataEvent(
            TraceEvent::Data,
            events->CacheKey("Test Data 1"),
            data,
            TraceCategory::Default
        );
        dataEvent.SetTimeStamp(6*ms+timeStampOffset);
        events->EmplaceBack(dataEvent);
    }
    {
        uint64_t data = ~0;
        TraceEvent dataEvent(
            TraceEvent::Data,
            events->CacheKey("Test Data 2"),
            data,
            TraceCategory::Default
        );
        dataEvent.SetTimeStamp(7*ms+timeStampOffset);
        events->EmplaceBack(dataEvent);
    }
    {
        double data = 1.5;
        TraceEvent dataEvent(
            TraceEvent::Data,
            events->CacheKey("Test Data 3"),
            data,
            TraceCategory::Default
        );
        dataEvent.SetTimeStamp(8*ms+timeStampOffset);
        events->EmplaceBack(dataEvent);
    }
    {
        std::string data = "String Data";
        TraceEvent dataEvent(
            TraceEvent::Data,
            events->CacheKey("Test Data 4"),
            events->StoreData(data.c_str()),
            TraceCategory::Default
        );
        dataEvent.SetTimeStamp(9*ms+timeStampOffset);
        events->EmplaceBack(dataEvent);
    }

    static constexpr TraceStaticKeyData keyInner("InnerScope");
    events->EmplaceBack(TraceEvent::Timespan, keyInner,
        ms + timeStampOffset,
        10*ms + timeStampOffset,
        TraceCategory::Default);


    {
        TraceEvent counterEvent(
            TraceEvent::CounterDelta,
            counterKey,
            1,
            TraceCategory::Default
        );
        counterEvent.SetTimeStamp(11*ms + timeStampOffset);
        events->EmplaceBack(counterEvent);
    }
    {
        TraceEvent counterEvent(
            TraceEvent::CounterValue,
            counterKey,
            -1,
            TraceCategory::Default
        );
        counterEvent.SetTimeStamp(12*ms + timeStampOffset);
        events->EmplaceBack(counterEvent);
    }
    static constexpr TraceStaticKeyData keyOuter("OuterScope");
    events->EmplaceBack(TraceEvent::Timespan, keyOuter,
        0 + timeStampOffset,
        13*ms + timeStampOffset,
        TraceCategory::Default);
    return events;
}

static TraceCollection
CreateTestCollection()
{
    TraceCollection collection;
    collection.AddToCollection(TraceThreadId("MainThread"), CreateTestEvents(0));
    collection.AddToCollection(TraceThreadId("Thread 1"), 
        CreateTestEvents(ArchSecondsToTicks(0.001)));
    return collection;
}

int
main(int argc, char *argv[]) 
{
    TraceCategory::GetInstance().RegisterCategory(
        TestCategory, "Test Category");

    TraceCollection testCol = CreateTestCollection();
    std::stringstream test;
    bool written = TraceSerialization::Write(test, testCol);
    TF_AXIOM(written);

    // Write out the file
    {
        std::ofstream ostream("trace.json");
        ostream << test.str();
    }
    // Read a collection from the file just written
    std::unique_ptr<TraceCollection> collection;
    {
        std::ifstream istream("trace.json");
        collection = TraceSerialization::Read(istream);
        TF_AXIOM(collection);
    }

    std::string stringRepr = test.str();

    std::stringstream test2;
    bool written2 = TraceSerialization::Write(test2, *collection);
    TF_AXIOM(written2);

    std::string stringRepr2 = test2.str();

    // This comparison might be too strict.
    TF_AXIOM(stringRepr == stringRepr2);
}