//
// Copyright 2021 Pixar
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
#include <algorithm>
#include <iostream>
#include <unordered_set>

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/flatteningSceneIndex.h"
#include "pxr/imaging/hd/prefixingSceneIndex.h"

#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"

#include "pxr/imaging/hd/dependenciesSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/xformSchema.h"

PXR_NAMESPACE_USING_DIRECTIVE

static std::ostream & 
operator<<(std::ostream &out, const SdfPathVector &v)
{
    out << "{" << std::endl;
    for (const SdfPath &path : v) {
        out << path << std::endl;
    }
    out << "}" << std::endl;

    return out;
}

// ----------------------------------------------------------------------------

class PrintingSceneIndexObserver : public HdSceneIndexObserver
{
public:

    PrintingSceneIndexObserver(const std::string & prefix)
    : _prefix(prefix)
    {}

    void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override
    {
        for (const AddedPrimEntry entry : entries) {
            std::cout << _prefix << "PrimAdded: " << entry.primPath << ", "
                << entry.primType << std::endl;
        }
    }

    void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override
    {
        for (const RemovedPrimEntry entry : entries) {
            std::cout << _prefix << "PrimRemoved: " << entry.primPath << ", "
                << std::endl;
        }
    }

    void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override
    {
        for (const DirtiedPrimEntry entry : entries) {
            std::cout << _prefix << "PrimDirtied: " << entry.primPath << ", ";

            for (const HdDataSourceLocator &locator : entry.dirtyLocators) {
                std::cout << locator.GetString() << ",";
            }

            std::cout << std::endl;
        }
    }

private:

    std::string _prefix;
};


// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------

void PrintContainer(
    HdContainerDataSourceHandle container,
    const std::string & prefix = std::string())
{
    if (!container) {
        return;
    }

    for (const TfToken & name : container->GetNames()) {

        HdDataSourceBaseHandle childSource = container->Get(name);
        if (!childSource) {
            std::cout << prefix << "(@" << name << ")" << std::endl;
            continue;
        }

        std::cout << prefix << "@" << name << ": ";

        if (HdContainerDataSourceHandle childContainer =
                HdContainerDataSource::Cast(childSource)) {
            std::cout << std::endl;
            PrintContainer(childContainer, prefix + "  ");
        } else if (HdSampledDataSourceHandle sampledChild = 
                HdSampledDataSource::Cast(childSource)) {
            std::cout << sampledChild->GetValue(0.0f) << std::endl;
        } else {
            std::cout << "(unknown)" << std::endl;
        }
    }

}

void PrintSceneIndexPrim(
    HdSceneIndexBase & sceneIndex,
    const SdfPath & primPath,
    bool includeChildren,
    const std::string & prefix = std::string())
{
    HdSceneIndexPrim prim = sceneIndex.GetPrim(primPath);
    std::cout << prefix << primPath << " (" << prim.primType << ")" << std::endl;
    PrintContainer(prim.dataSource, prefix + "  ");

    if (!includeChildren) {
        return;
    }

    for (const SdfPath &childPath : sceneIndex.GetChildPrimPaths(primPath)) {
        PrintSceneIndexPrim(sceneIndex, childPath, true, prefix);
    }
}

// ----------------------------------------------------------------------------

GfMatrix4d GetPrimTransform(
        const HdSceneIndexBase & sceneIndex, const SdfPath & primPath)
{
    HdSceneIndexPrim prim = sceneIndex.GetPrim(primPath);

    if (HdXformSchema xformSchema =
            HdXformSchema::GetFromParent(prim.dataSource)) {
        if (HdMatrixDataSourceHandle matrixSource =
                    xformSchema.GetMatrix()) {
            return matrixSource->GetTypedValue(0.0f);
        }
    }

    return GfMatrix4d().SetIdentity();
}

// ----------------------------------------------------------------------------

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

bool TestFlatteningSceneIndex()
{
    HdRetainedSceneIndexRefPtr sceneIndex_ = HdRetainedSceneIndex::New();
    HdFlatteningSceneIndexRefPtr flatteningSceneIndex_ =
        HdFlatteningSceneIndex::New(sceneIndex_);

    HdRetainedSceneIndex &sceneIndex = *sceneIndex_;
    HdFlatteningSceneIndex &flatteningSceneIndex = *flatteningSceneIndex_;

    PrintingSceneIndexObserver observer("");
    flatteningSceneIndex.AddObserver(HdSceneIndexObserverPtr(&observer));



    sceneIndex.AddPrims({{SdfPath("/A"), TfToken("huh"), nullptr}});
    sceneIndex.AddPrims({{SdfPath("/A/B"), TfToken("huh"),
        HdRetainedContainerDataSource::New(
            HdXformSchemaTokens->xform,
            HdXformSchema::Builder()
                .SetMatrix(
                   HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                           GfMatrix4d().SetTranslate({0.0, 0.0, 10.0})))
                .Build()
            )}}
    );
    sceneIndex.AddPrims({{SdfPath("/A/B/C"), TfToken("huh"),
        HdRetainedContainerDataSource::New(
            HdXformSchemaTokens->xform,
            HdXformSchema::Builder()
                .SetMatrix(
                   HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                           GfMatrix4d().SetTranslate({5.0, 0.0, 0.0})))
                .Build()
        )}}
    );

    std::cout << "\n-- SCENE -----------------------" << std::endl;
    PrintSceneIndexPrim(sceneIndex, SdfPath("/A"), true);

    std::cout << "\n-- FLATTENED SCENE ------------" << std::endl;
    PrintSceneIndexPrim(flatteningSceneIndex, SdfPath("/A"), true);


    if (!_CompareValue(
            "INITIAL LEAF SCENE XFORM",
            GetPrimTransform(sceneIndex, SdfPath("/A/B/C")),
            GfMatrix4d().SetTranslate({5.0, 0.0, 0.0}))) {
        return false;
    }

    if (!_CompareValue(
            "FLATTENED LEAF SCENE XFORM",
            GetPrimTransform(flatteningSceneIndex, SdfPath("/A/B/C")),
            GfMatrix4d().SetTranslate({5.0, 0.0, 10.0}))) {
        return false;
    }


    std::cout << "\n-- DIRTYING SCENE ------------" << std::endl;

    sceneIndex.AddPrims({{SdfPath("/A/B"), TfToken("huh"),
        HdRetainedContainerDataSource::New(
            HdXformSchemaTokens->xform,
            HdXformSchema::Builder()
                .SetMatrix(
                   HdRetainedTypedSampledDataSource<GfMatrix4d>::New(
                           GfMatrix4d().SetTranslate({0.0, 0.0, 20.0})))
                .Build()
            )}}
    );


    std::cout << "\n-- SCENE -----------------------" << std::endl;
    PrintSceneIndexPrim(sceneIndex, SdfPath("/A"), true);

    std::cout << "\n-- FLATTENED SCENE ------------" << std::endl;
    PrintSceneIndexPrim(flatteningSceneIndex, SdfPath("/A"), true);


    if (!_CompareValue(
            "UPDATED INITIAL LEAF SCENE XFORM",
            GetPrimTransform(sceneIndex, SdfPath("/A/B/C")),
            GfMatrix4d().SetTranslate({5.0, 0.0, 0.0}))) {
        return false;
    }

    if (!_CompareValue(
            "UPDATED FLATTENED LEAF SCENE XFORM",
            GetPrimTransform(flatteningSceneIndex, SdfPath("/A/B/C")),
            GfMatrix4d().SetTranslate({5.0, 0.0, 20.0}))) {
        return false;
    }



    std::cout << "\n-- REMOVING XFORM FROM A/B ON SCENE ----" << std::endl;
    sceneIndex.AddPrims({{SdfPath("/A/B"), TfToken("huh"), nullptr}});

    std::cout << "\n-- SCENE -----------------------" << std::endl;
    PrintSceneIndexPrim(sceneIndex, SdfPath("/A"), true);

    std::cout << "\n-- FLATTENED SCENE ------------" << std::endl;
    PrintSceneIndexPrim(flatteningSceneIndex, SdfPath("/A"), true);


    if (!_CompareValue(
            "FINAL LEAF SCENE XFORM",
            GetPrimTransform(sceneIndex, SdfPath("/A/B/C")),
            GfMatrix4d().SetTranslate({5.0, 0.0, 0.0}))) {
        return false;
    }

    if (!_CompareValue(
            "FINAL FLATTENED LEAF SCENE XFORM",
            GetPrimTransform(flatteningSceneIndex, SdfPath("/A/B/C")),
            GfMatrix4d().SetTranslate({5.0, 0.0, 0.0}))) {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

HdDataSourceBaseHandle GetDataSourceFromScene(
        HdSceneIndexBase & sceneIndex,
        const SdfPath & primPath,
        const HdDataSourceLocator & locator)
{
    HdSceneIndexPrim prim = sceneIndex.GetPrim(primPath);
    return HdContainerDataSource::Get(prim.dataSource, locator);
}

template <typename T>
T GetTypedValueFromScene(
        HdSceneIndexBase & sceneIndex,
        const SdfPath & primPath,
        const HdDataSourceLocator & locator)
{
    auto dataSource = HdTypedSampledDataSource<T>::Cast(
            GetDataSourceFromScene(sceneIndex, primPath, locator));

    if (!dataSource) {
        return T();
    }

    return dataSource->GetTypedValue(0.0f);
}



//-----------------------------------------------------------------------------

static bool 
TestPrefixingSceneIndex()
{
    HdRetainedSceneIndexRefPtr sceneIndex_ = HdRetainedSceneIndex::New();
    HdRetainedSceneIndex &sceneIndex = *sceneIndex_;

    HdPrefixingSceneIndexRefPtr prefixingSceneIndex_ =
        HdPrefixingSceneIndex::New(sceneIndex_, SdfPath("/E/F/G"));
    HdPrefixingSceneIndex &prefixingSceneIndex = *prefixingSceneIndex_;

    sceneIndex.AddPrims({{SdfPath("/A"), TfToken("huh"), nullptr}});
    sceneIndex.AddPrims({{SdfPath("/A/B"), TfToken("huh"), nullptr}});
    sceneIndex.AddPrims({{SdfPath("/A/C"), TfToken("huh"),
        HdRetainedContainerDataSource::New(
            TfToken("somePath"),
            HdRetainedTypedSampledDataSource<SdfPath>::New(
                    SdfPath("/A/B")),
            TfToken("someContainer"),
            HdRetainedContainerDataSource::New(
                TfToken("anotherPath"),
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                        SdfPath("/A/B/C/D")),
                TfToken("relativePath"),
                HdRetainedTypedSampledDataSource<SdfPath>::New(
                        SdfPath("F/G"))
            )
        )}}
    );

    std::cout << "\n-- SCENE -----------------------" << std::endl;
    PrintSceneIndexPrim(sceneIndex, SdfPath("/"), true);

    std::cout << "\n-- PREFIXED SCENE --------------" << std::endl;
    PrintSceneIndexPrim(prefixingSceneIndex, SdfPath("/"), true);

    if (!_CompareValue("COMPARING TOP-LEVEL ABSOLUTE PATH",
            GetTypedValueFromScene<SdfPath>(
                    prefixingSceneIndex,
                    SdfPath("/E/F/G/A/C"),
                    HdDataSourceLocator(TfToken("somePath"))),
            SdfPath("/E/F/G/A/B"))) {
        return false;
    }


    if (!_CompareValue("COMPARING NESTED ABSOLUTE PATH",
            GetTypedValueFromScene<SdfPath>(
                    prefixingSceneIndex,
                    SdfPath("/E/F/G/A/C"),
                    HdDataSourceLocator(
                            TfToken("someContainer"),
                            TfToken("anotherPath"))),
            SdfPath("/E/F/G/A/B/C/D"))) {
        return false;
    }

    if (!_CompareValue("COMPARING NESTED RELATIVED PATH",
            GetTypedValueFromScene<SdfPath>(
                    prefixingSceneIndex,
                    SdfPath("/E/F/G/A/C"),
                    HdDataSourceLocator(
                            TfToken("someContainer"),
                            TfToken("relativePath"))),
            SdfPath("F/G"))) {
        return false;
    }

    //
    // Testing GetChildPrimPaths
    // 
    if (!_CompareValue("TESTING GetChildPrimPaths('/E/F/G/A'))",
            prefixingSceneIndex.GetChildPrimPaths(SdfPath("/E/F/G/A")), 
            SdfPathVector({SdfPath("/E/F/G/A/C"), SdfPath("/E/F/G/A/B")})
            )) {
        return false;
    }

    if (!_CompareValue("TESTING GetChildPrimPaths('/E/X/Y/Z'))",
            prefixingSceneIndex.GetChildPrimPaths(SdfPath("/E/X/Y/Z")), 
            SdfPathVector())
            ) {
        return false;
    }

    if (!_CompareValue("TESTING GetChildPrimPaths('/E/F'))",
            prefixingSceneIndex.GetChildPrimPaths(SdfPath("/E/F")), 
            SdfPathVector({SdfPath("/E/F/G")}))
            ) {
        return false;
    }

    if (!_CompareValue("TESTING GetChildPrimPaths('/E'))",
            prefixingSceneIndex.GetChildPrimPaths(SdfPath("/E")), 
            SdfPathVector({SdfPath("/E/F")}))
            ) {
        return false;
    }

    if (!_CompareValue("TESTING GetChildPrimPaths('/E/X'))",
            prefixingSceneIndex.GetChildPrimPaths(SdfPath("/E/X")), 
            SdfPathVector())
            ) {
        return false;
    }

    if (!_CompareValue("TESTING GetChildPrimPaths(''))",
            prefixingSceneIndex.GetChildPrimPaths(SdfPath()), 
            SdfPathVector())
            ) {
        return false;
    }

    if (!_CompareValue("TESTING GetChildPrimPaths('/'))",
            prefixingSceneIndex.GetChildPrimPaths(SdfPath("/")), 
            SdfPathVector({SdfPath("/E")}))
            ) {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

#define xstr(s) str(s)
#define str(s) #s
#define TEST(X) std::cout << (++i) << ") " <<  str(X) << "..." << std::endl; \
if (!X()) { std::cout << "FAILED" << std::endl; return -1; } \
else std::cout << "...SUCCEEDED" << std::endl;

int
main(int argc, char**argv)
{
    //-------------------------------------------------------------------------
    std::cout << "STARTING testHdSceneIndex" << std::endl;

    int i = 0;
    TEST(TestFlatteningSceneIndex);
    TEST(TestPrefixingSceneIndex);

    //--------------------------------------------------------------------------
    std::cout << "DONE testHdSceneIndex" << std::endl;
    return 0;
}
