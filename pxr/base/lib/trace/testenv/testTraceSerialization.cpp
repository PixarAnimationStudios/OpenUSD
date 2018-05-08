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
#include <iostream>

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
        TraceEvent::End,
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

static std::unique_ptr<TraceCollection>
CreateTestCollection(float startTimeSec=0.0)
{
    std::unique_ptr<TraceCollection> collection(new TraceCollection);
    collection->AddToCollection(
        TraceThreadId("MainThread"),
        CreateTestEvents(ArchSecondsToTicks(startTimeSec)));
    collection->AddToCollection(TraceThreadId("Thread 1"), 
        CreateTestEvents(ArchSecondsToTicks(startTimeSec + 0.001)));
    return collection;
}

static void
_TestSerialization(
    const std::vector<std::shared_ptr<TraceCollection>>& testCols,
    std::string fileName)
{
    std::stringstream test;
    bool written = false;
    if (testCols.size() == 1 ) {
        written = TraceSerialization::Write(test, testCols[0]);
    } else {
        written = TraceSerialization::Write(test, testCols);
    }
    TF_AXIOM(written);

    // Write out the file
    {
        std::ofstream ostream(fileName);
        ostream << test.str();
    }
    // Read a collection from the file just written
    std::shared_ptr<TraceCollection> collection;
    {
        std::ifstream istream(fileName);
        collection = TraceSerialization::Read(istream);
        TF_AXIOM(collection);
    }

    std::string stringRepr = test.str();

    std::stringstream test2;
    bool written2 = TraceSerialization::Write(test2, collection);
    TF_AXIOM(written2);

    std::string stringRepr2 = test2.str();

    // This comparison might be too strict.
    if (stringRepr != stringRepr2) {
        std::cout << "Written:\n" << stringRepr << "\n";
        std::cout << "Reconstruction:\n" << stringRepr2 << "\n";
    }
    TF_AXIOM(stringRepr == stringRepr2);
}

int
main(int argc, char *argv[]) 
{
    TraceCategory::GetInstance().RegisterCategory(
        TestCategory, "Test Category");

    std::vector<std::shared_ptr<TraceCollection>> collections;
    std::cout << "Testing single collection\n";
    collections.emplace_back(CreateTestCollection());
    _TestSerialization(collections, "trace.json");
    std::cout << " PASSED\n";

    std::cout << "Testing multiple collections\n";
    collections.emplace_back(CreateTestCollection(20.0/1000.0));
    _TestSerialization(collections, "trace2.json");
    std::cout << " PASSED\n";
}
