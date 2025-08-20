//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hdsi/prefixPathPruningSceneIndex.h"

#include "pxr/base/tf/errorMark.h"

#include <iostream>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

//
// Recording observer lofted from testHdSceneIndex.cpp
//
class RecordingSceneIndexObserver : public HdSceneIndexObserver
{
public:

    enum EventType {
        EventType_PrimAdded = 0,
        EventType_PrimRemoved,
        EventType_PrimDirtied,
    };

    struct Event
    {
        EventType eventType;
        SdfPath primPath;
        TfToken primType;
        HdDataSourceLocator locator;

        inline bool operator==(Event const &rhs) const noexcept
        {
            return (
                eventType == rhs.eventType
                && primPath == rhs.primPath
                && primType == rhs.primType
                && locator == rhs.locator);
        }

        template <class HashState>
        friend void TfHashAppend(HashState &h, Event const &myObj) {
            h.Append(myObj.eventType);
            h.Append(myObj.primPath);
            h.Append(myObj.primType);
            h.Append(myObj.locator);
        }

        inline size_t Hash() const;
        struct HashFunctor {
            size_t operator()(Event const& event) const {
                return event.Hash();
            }
        };

    };

    using EventVector = std::vector<Event>;
    using EventSet = std::unordered_set<Event, Event::HashFunctor>;

    void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override
    {
        for (const AddedPrimEntry &entry : entries) {
            _events.emplace_back(Event{
                EventType_PrimAdded, entry.primPath, entry.primType});
        }
    }

    void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override
    {
        for (const RemovedPrimEntry &entry : entries) {
            _events.emplace_back(Event{EventType_PrimRemoved, entry.primPath});
        }
    }

    void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override
    {
        for (const DirtiedPrimEntry &entry : entries) {
            for (const HdDataSourceLocator &locator : entry.dirtyLocators) {
                _events.emplace_back(Event{EventType_PrimDirtied,
                    entry.primPath, TfToken(), locator});
            }
        }
    }

    void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override
    {
        ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
    }

    EventVector GetEvents()
    {
        return _events;
    }

    EventSet GetEventsAsSet()
    {
        return EventSet(_events.begin(), _events.end());
    }

    void Clear()
    {
        _events.clear();
    }

private:
    EventVector _events;
};

inline size_t
RecordingSceneIndexObserver::Event::Hash() const
{
    return TfHash()(*this);
}

std::ostream & operator<<(
        std::ostream &out, const RecordingSceneIndexObserver::Event &event)
{
    switch (event.eventType) {
    case RecordingSceneIndexObserver::EventType_PrimAdded:
        out << "PrimAdded: " << event.primPath << ", " << event.primType;
        break;
    case RecordingSceneIndexObserver::EventType_PrimRemoved:
        out << "PrimRemoved: " << event.primPath;
        break;
    case RecordingSceneIndexObserver::EventType_PrimDirtied:
        out << "PrimDirtied: " << event.primPath << ", "
                << event.locator.GetString();
        break;
    default:
        out << "<unknown event type";
    }
    return out;
}

std::ostream & operator<<(
        std::ostream &out,
        const RecordingSceneIndexObserver::EventVector &events)
{
    out << "{" << std::endl;
    for (const auto & event : events) {
        out << event << std::endl;
    }
    out << "}" << std::endl;
    return out;
}

std::ostream & operator<<(
        std::ostream &out,
        const RecordingSceneIndexObserver::EventSet &events)
{
    return out << RecordingSceneIndexObserver::EventVector(
            events.begin(), events.end());
}

template<typename T>
bool 
_CompareValue(const char *msg, const T &v1, const T &v2)
{
    if (v1 == v2) {
        std::cout << msg << " matches." << std::endl;
    } else {
        std::cerr << msg << " doesn't match. Expecting " << v2 << " got " << v1 
                  << std::endl;
        return false;
    }
    return true;
}

// ----------------------------------------------------------------------------

std::ostream&
operator<<(std::ostream &out, const SdfPathSet &v)
{
    out << "{";
    for (const SdfPath &path : v) {
        out << path << ", ";
    }
    out << "}" << std::endl;

    return out;
}

bool
operator==(const HdSceneIndexPrim &lhs, const HdSceneIndexPrim &rhs)
{
    return lhs.primType == rhs.primType && lhs.dataSource == rhs.dataSource;
}

bool
_Compare(const SdfPathVector &computedVec, const SdfPathVector &expectedVec)
{
    const SdfPathSet computed(computedVec.begin(), computedVec.end());
    const SdfPathSet expected(expectedVec.begin(), expectedVec.end());
    
    if (computed.size() != expected.size() || computed != expected) {
        std::cerr << "FAILED."
                  << "\n  Expected: " << expected
                  << "\n  Got: " << computed << std::endl;

        return false;
    }
    return true;
}

HdRetainedSceneIndexRefPtr
_PopulateTestScene()
{
    static const TfToken primType("test");
    static const HdContainerDataSourceHandle primDs =
        HdRetainedContainerDataSource::New(
            TfToken("loc0"),
            HdRetainedTypedSampledDataSource<int>::New(23),
            TfToken("loc1"),
            HdRetainedTypedSampledDataSource<bool>::New(false));

    HdRetainedSceneIndexRefPtr scene = HdRetainedSceneIndex::New();
    scene->AddPrims(
        {
            { SdfPath("/A"), primType, primDs },
            { SdfPath("/A/B"), primType, primDs },
            { SdfPath("/A/B/C0"), primType, primDs },
            { SdfPath("/A/B/C1"), primType, primDs },
            { SdfPath("/A/C"), primType, primDs },
            { SdfPath("/A/C/D0"), primType, primDs },
            { SdfPath("/A/C/D0/E0"), primType, primDs },
            { SdfPath("/A/C/D1"), primType, primDs },
            { SdfPath("/A/D"), primType, primDs },
            { SdfPath("/B"), primType, primDs },
            { SdfPath("/B/A"), primType, primDs },
            { SdfPath("/B/C"), primType, primDs },
            { SdfPath("/B/C/D"), primType, primDs },
            { SdfPath("/B/C/D/E"), primType, primDs },
            { SdfPath("/B/D"), primType, primDs },
        }
    );
    return scene;
}

bool
TestPrefixPathPruning()
{
    bool success = true;
        
    HdRetainedSceneIndexRefPtr testSi = _PopulateTestScene();
   
    // Chain a scene index that prunes some prefix paths and verify that
    // these paths are pruned.
    HdsiPrefixPathPruningSceneIndexRefPtr pruningSi =
        HdsiPrefixPathPruningSceneIndex::New(
            testSi,
            HdRetainedContainerDataSource::New(
                HdsiPrefixPathPruningSceneIndexTokens->excludePathPrefixes,
                HdRetainedTypedSampledDataSource<SdfPathVector>::New(
                { 
                    SdfPath("/A/B"), 
                    SdfPath("/A/B/C1"), // Redundant since we prune the parent.
                    SdfPath("/A/C/D0"),
                    SdfPath("/B/C/D")
                 })));
    
    
    {
        success &=
            // "/A/B" should be pruned.
               _Compare(
                    pruningSi->GetChildPrimPaths(SdfPath("/A")),
                    SdfPathVector{ SdfPath("/A/C"), SdfPath("/A/D") })
            && _Compare(
                    pruningSi->GetChildPrimPaths(SdfPath("/A/B")),
                    SdfPathVector{})
    
            // "/A/C/D0" should be pruned.
            && _Compare(
                pruningSi->GetChildPrimPaths(SdfPath("/A/C")),
                SdfPathVector{ SdfPath("/A/C/D1") })
            
            // No children of "/B" should be pruned.
            && _Compare(
                pruningSi->GetChildPrimPaths(SdfPath("/B")),
                SdfPathVector{ SdfPath("/B/A"), SdfPath("/B/C"), SdfPath("/B/D") })
        
            // "/B/C/D" should be pruned leaving "/B/C" with no children.
            && _Compare(pruningSi->GetChildPrimPaths(SdfPath("/B/C")),
                SdfPathVector{});
    }

    // Edit the exclude paths to prune "/A/C" and "/B".
    {
        using EventType = RecordingSceneIndexObserver::EventType;
        using Event = RecordingSceneIndexObserver::Event;
        using EventSet = RecordingSceneIndexObserver::EventSet;

        RecordingSceneIndexObserver observer;
        pruningSi->AddObserver(HdSceneIndexObserverPtr(&observer));

        pruningSi->SetExcludePathPrefixes(
            { SdfPath("/A/B"), SdfPath("/B") });
        
        auto baseline = EventSet{
            Event{
                EventType::EventType_PrimAdded,
                SdfPath("/A/C/D0"), TfToken("test"), {}
            },
            Event{
                EventType::EventType_PrimAdded,
                SdfPath("/A/C/D0/E0"), TfToken("test"), {}
            },
            Event{
                EventType::EventType_PrimRemoved,
                SdfPath("/B"), {}, {}
            },
        };

        success &=
            _CompareValue("Setting exclude paths to {\"/A/B\", \"/B\"} ->",
                observer.GetEventsAsSet(), baseline);
    }
    
    // Verify that querying pruned prims gives us an empty prim.
    {
        success &=
               pruningSi->GetPrim(SdfPath("/B/C")) == HdSceneIndexPrim()
            && pruningSi->GetPrim(SdfPath("/B/C/D")) == HdSceneIndexPrim()
            && pruningSi->GetPrim(SdfPath("/A/B")) == HdSceneIndexPrim()
            && pruningSi->GetPrim(SdfPath("/A/B/C1")) == HdSceneIndexPrim();
    }

    return success;
}

//-----------------------------------------------------------------------------

int main(int argc, char ** argv)
{
    TfErrorMark mark;

    bool success = true;
    success &= TestPrefixPathPruning();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
