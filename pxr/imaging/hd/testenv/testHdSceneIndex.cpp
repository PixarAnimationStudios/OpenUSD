//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <algorithm>
#include <iostream>
#include <unordered_set>

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/flatteningSceneIndex.h"
#include "pxr/imaging/hd/prefixingSceneIndex.h"
#include "pxr/imaging/hd/mergingSceneIndex.h"
#include "pxr/imaging/hd/dependencyForwardingSceneIndex.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/flattenedDataSourceProviders.h"

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
        for (const AddedPrimEntry &entry : entries) {
            std::cout << _prefix << "PrimAdded: " << entry.primPath << ", "
                << entry.primType << std::endl;
        }
    }

    void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override
    {
        for (const RemovedPrimEntry &entry : entries) {
            std::cout << _prefix << "PrimRemoved: " << entry.primPath << ", "
                << std::endl;
        }
    }

    void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override
    {
        for (const DirtiedPrimEntry &entry : entries) {
            std::cout << _prefix << "PrimDirtied: " << entry.primPath << ", ";

            for (const HdDataSourceLocator &locator : entry.dirtyLocators) {
                std::cout << locator.GetString() << ",";
            }

            std::cout << std::endl;
        }
    }

    void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override
    {
        ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
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
        HdFlatteningSceneIndex::New(
            sceneIndex_, HdFlattenedDataSourceProviders());

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
                        SdfPath("F/G")),
                TfToken("pathArray"),
                HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                        {SdfPath("/A/B/C/D"), SdfPath("/A/B")})
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

    if (!_CompareValue("COMPARING PATH ARRAY",
            GetTypedValueFromScene<VtArray<SdfPath>>(
                    prefixingSceneIndex,
                    SdfPath("/E/F/G/A/C"),
                    HdDataSourceLocator(
                            TfToken("someContainer"),
                            TfToken("pathArray"))),
            VtArray<SdfPath>{
                SdfPath("/E/F/G/A/B/C/D"),
                SdfPath("/E/F/G/A/B")})) {
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

static bool _CompareSceneValue(
    const std::string &label,
    HdSceneIndexBaseRefPtr scene,
    const SdfPath &primPath,
    const HdDataSourceLocator &locator,
    const VtValue &value)
{
    if (auto sampledDataSource = HdSampledDataSource::Cast(
            scene->GetDataSource(primPath, locator))) {
        if (sampledDataSource->GetValue(0.0) == value) {
            std::cout << label << " matches." << std::endl;
            return true;
        } else {
            std::cerr << label << " doesn't match. Expecting " << value 
                << " got " << sampledDataSource->GetValue(0.0) << std::endl;
            return false;
        }
    } else {
        std::cerr << label << " value not found. Expecting " << value
            << std::endl;
        return false;
    }
}

bool TestMergingSceneIndex()
{
    HdRetainedSceneIndexRefPtr retainedSceneA = HdRetainedSceneIndex::New();

    retainedSceneA->AddPrims({
        {SdfPath("/A"), TfToken("group"),
            HdRetainedContainerDataSource::New(
                TfToken("uniqueToA"),
                HdRetainedTypedSampledDataSource<int>::New(0),
                TfToken("common"),
                HdRetainedTypedSampledDataSource<int>::New(0))
        },
        {SdfPath("/A/AA"), TfToken("group"),
            HdRetainedContainerDataSource::New(
                TfToken("value"),
                HdRetainedTypedSampledDataSource<int>::New(1)
            )},
    });

    HdRetainedSceneIndexRefPtr retainedSceneB = HdRetainedSceneIndex::New();

    retainedSceneB->AddPrims({
        {SdfPath("/A"), TfToken("group"),
            HdRetainedContainerDataSource::New(
                TfToken("uniqueToB"),
                HdRetainedTypedSampledDataSource<int>::New(1),
                TfToken("common"),
                HdRetainedTypedSampledDataSource<int>::New(1))
        },
        {SdfPath("/A/BB"), TfToken("group"),
            HdRetainedContainerDataSource::New(
                TfToken("value"),
                HdRetainedTypedSampledDataSource<int>::New(1)
            )},
        {SdfPath("/B"), TfToken("group"),
            HdRetainedContainerDataSource::New(
                TfToken("value"),
                HdRetainedTypedSampledDataSource<int>::New(1)
            )},
    });

    HdMergingSceneIndexRefPtr mergingSceneIndex = HdMergingSceneIndex::New();
    mergingSceneIndex->AddInputScene(retainedSceneA,
        SdfPath::AbsoluteRootPath());
    mergingSceneIndex->AddInputScene(retainedSceneB,
        SdfPath::AbsoluteRootPath());

    PrintSceneIndexPrim(
        *mergingSceneIndex,
        SdfPath::AbsoluteRootPath(),
        true);

    if (!_CompareSceneValue("testing common value:", mergingSceneIndex,
        SdfPath("/A"), HdDataSourceLocator(TfToken("common")), VtValue(0))) {
        return false;
    }
    if (!_CompareSceneValue("testing uniqueToA value:", mergingSceneIndex,
        SdfPath("/A"), HdDataSourceLocator(TfToken("uniqueToA")), VtValue(0))) {
        return false;
    }
    if (!_CompareSceneValue("testing uniqueToB value:", mergingSceneIndex,
        SdfPath("/A"), HdDataSourceLocator(TfToken("uniqueToB")), VtValue(1))) {
        return false;
    }
    if (!_CompareSceneValue("testing /A/AA value:", mergingSceneIndex,
        SdfPath("/A/AA"), HdDataSourceLocator(TfToken("value")), VtValue(1))) {
        return false;
    }
    if (!_CompareSceneValue("testing /A/BB value:", mergingSceneIndex,
        SdfPath("/A/AA"), HdDataSourceLocator(TfToken("value")), VtValue(1))) {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

namespace
{

TF_DECLARE_REF_PTRS(_RepopulatingSceneIndex);

// utility for testing PrimAdded messages
class _RepopulatingSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:

    static _RepopulatingSceneIndexRefPtr New(
            const HdSceneIndexBaseRefPtr &inputScene) {
        return TfCreateRefPtr(new _RepopulatingSceneIndex(inputScene));
    }


    HdSceneIndexPrim GetPrim(const SdfPath &path) const override {
        return _GetInputSceneIndex()->GetPrim(path);
    }

    SdfPathVector GetChildPrimPaths(const SdfPath &path) const override {
        return _GetInputSceneIndex()->GetChildPrimPaths(path);
    }

    void Repopulate(const SdfPath &fromRoot = SdfPath::AbsoluteRootPath())
    {
        HdSceneIndexBaseRefPtr input =  _GetInputSceneIndex();

        HdSceneIndexObserver::AddedPrimEntries entries;

        std::vector<SdfPath> queue = {
            fromRoot,
        };

        while (!queue.empty()) {
            const SdfPath path = queue.back();
            queue.pop_back();

            HdSceneIndexPrim prim = input->GetPrim(path);
            entries.emplace_back(path, prim.primType);

            const SdfPathVector childPaths = input->GetChildPrimPaths(path);
            queue.insert(queue.end(), childPaths.begin(), childPaths.end());
        }

        _SendPrimsAdded(entries);
    }

protected:
    _RepopulatingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputScene)
    : HdSingleInputFilteringSceneIndexBase(inputScene)
    {
    }

    void _PrimsAdded(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::AddedPrimEntries &entries) override
    {
        _SendPrimsAdded(entries);
    }

    void _PrimsRemoved(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::RemovedPrimEntries &entries) override
    {
        _SendPrimsRemoved(entries);
    }

    void _PrimsDirtied(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::DirtiedPrimEntries &entries) override
    {
        _SendPrimsDirtied(entries);
    }
};

}


bool TestMergingSceneIndexPrimAddedNotices()
{
    HdRetainedSceneIndexRefPtr retainedSceneA = HdRetainedSceneIndex::New();
    retainedSceneA->AddPrims({
        {SdfPath("/A"), TfToken("chicken"), nullptr},
        {SdfPath("/A/B"), TfToken("group"),
            HdRetainedContainerDataSource::New(
                TfToken("value"),
                HdRetainedTypedSampledDataSource<int>::New(1)
            )},
        {SdfPath("/A/C"), TfToken(), //provides a data source but no type
            HdRetainedContainerDataSource::New(
                TfToken("value"),
                HdRetainedTypedSampledDataSource<int>::New(1)
            )},
    });

    HdRetainedSceneIndexRefPtr retainedSceneB = HdRetainedSceneIndex::New();
    retainedSceneB->AddPrims({
        {SdfPath("/A/B"), TfToken(), nullptr}, // no type
        {SdfPath("/A/C"), TfToken("taco"),
            HdRetainedContainerDataSource::New(
                TfToken("value"),
                HdRetainedTypedSampledDataSource<int>::New(2)
            )},
        {SdfPath("/A/D"), TfToken("salsa"), nullptr},
    });

    _RepopulatingSceneIndexRefPtr rpA =
        _RepopulatingSceneIndex::New(retainedSceneA);

    _RepopulatingSceneIndexRefPtr rpB =
        _RepopulatingSceneIndex::New(retainedSceneB);

    HdMergingSceneIndexRefPtr mergingSceneIndex = HdMergingSceneIndex::New();
    mergingSceneIndex->AddInputScene(rpA, SdfPath::AbsoluteRootPath());
    mergingSceneIndex->AddInputScene(rpB, SdfPath("/A"));

    RecordingSceneIndexObserver observer;
    mergingSceneIndex->AddObserver(HdSceneIndexObserverPtr(&observer));

    const TfDenseHashMap<SdfPath, TfToken, TfHash> expectedTypes = {
        {SdfPath("/"), TfToken()},
        {SdfPath("/A"), TfToken("chicken")},
        {SdfPath("/A/B"), TfToken("group")},
        {SdfPath("/A/C"), TfToken("taco")},
        {SdfPath("/A/D"), TfToken("salsa")},
    };

    auto _Compare = [&mergingSceneIndex, &observer, &expectedTypes]() {
        for (const RecordingSceneIndexObserver::Event& event :
                observer.GetEvents()) {

            if (event.eventType !=
                    RecordingSceneIndexObserver::EventType_PrimAdded) {
                std::cerr << "received unexpected event type for "
                    << event.primPath << std::endl;
                return false;
            }

            const auto it = expectedTypes.find(event.primPath);
            if (it ==  expectedTypes.end()) {
                std::cerr << "expected type is unknown for " << event.primPath
                    << std::endl;
                return false;
            }

            const TfToken &expectedType = it->second;
            if (event.primType != expectedType) {
                std::cerr << "expected '" << expectedType << "' but received '"
                    << event.primType <<  "' for " << event.primPath
                        << std::endl;
                return false;
            }

            HdSceneIndexPrim prim = mergingSceneIndex->GetPrim(event.primPath);
            if (prim.primType != expectedType) {
                std::cerr << "expected '" <<  expectedType << "' but received '"
                    <<  prim.primType << "' for GetPrim(" << event.primPath
                        << ")" << std::endl;
                return false;
            }
        }

        return true;
    };

    std::cout << "comparing repopulation from input b" << std::endl;
    rpB->Repopulate();
    if (!_Compare()) {
        return false;
    }

    observer.Clear();
    std::cout << "comparing repopulation from input a" << std::endl;
    rpA->Repopulate();
    if (!_Compare()) {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------


bool TestDependencyForwardingSceneIndex()
{
    using EventType = RecordingSceneIndexObserver::EventType;
    using Event = RecordingSceneIndexObserver::Event;
    using EventSet = RecordingSceneIndexObserver::EventSet;

    using RDS = HdRetainedTypedSampledDataSource<HdDataSourceLocator>;

    HdRetainedSceneIndexRefPtr retainedScene_ = HdRetainedSceneIndex::New();
    HdRetainedSceneIndex &retainedScene = *retainedScene_;
    HdDependencyForwardingSceneIndexRefPtr dependencyForwardingScene_ =
        HdDependencyForwardingSceneIndex::New(retainedScene_);
    HdDependencyForwardingSceneIndex &dependencyForwardingScene =
        *dependencyForwardingScene_;


    retainedScene.AddPrims({{SdfPath("/A"), TfToken("group"),
        HdRetainedContainerDataSource::New()}}
    );

    retainedScene.AddPrims({{SdfPath("/B"), TfToken("group"),
        HdRetainedContainerDataSource::New(
            HdDependenciesSchemaTokens->__dependencies,
            HdRetainedContainerDataSource::New(
                TfToken("test"),
                HdDependencySchema::Builder()
                    .SetDependedOnPrimPath(
                        HdRetainedTypedSampledDataSource<SdfPath>::New(
                            SdfPath("/A")))
                    .SetDependedOnDataSourceLocator(
                        RDS::New(HdDataSourceLocator(TfToken("taco"))))
                    .SetAffectedDataSourceLocator(
                        RDS::New(HdDataSourceLocator(TfToken("chicken"))))
                    .Build()
            )
        )}}
    );

    retainedScene.AddPrims({{SdfPath("/C"), TfToken("group"),
        HdRetainedContainerDataSource::New(
            HdDependenciesSchemaTokens->__dependencies,
            HdRetainedContainerDataSource::New(
                TfToken("test"),
                HdDependencySchema::Builder()
                    .SetDependedOnPrimPath(
                        HdRetainedTypedSampledDataSource<SdfPath>::New(
                            SdfPath("/B")))
                    .SetDependedOnDataSourceLocator(
                        RDS::New(HdDataSourceLocator(TfToken("chicken"))))
                    .SetAffectedDataSourceLocator(
                        RDS::New(HdDataSourceLocator(TfToken("salsa"))))
                    .Build()
            )
        )}}
    );

    // ...D->E->F->D->...
    retainedScene.AddPrims({{SdfPath("/D"), TfToken("group"),
            HdRetainedContainerDataSource::New(
                HdDependenciesSchemaTokens->__dependencies,
                HdRetainedContainerDataSource::New(
                    TfToken("test"),
                    HdDependencySchema::Builder()
                        .SetDependedOnPrimPath(
                            HdRetainedTypedSampledDataSource<SdfPath>::New(
                                SdfPath("/E")))
                        .SetDependedOnDataSourceLocator(
                            RDS::New(HdDataSourceLocator(TfToken("attr2"))))
                        .SetAffectedDataSourceLocator(
                            RDS::New(HdDataSourceLocator(TfToken("attr1"))))
                        .Build()
                )
            )}}
        );

    retainedScene.AddPrims({{SdfPath("/E"), TfToken("group"),
        HdRetainedContainerDataSource::New(
            HdDependenciesSchemaTokens->__dependencies,
            HdRetainedContainerDataSource::New(
                TfToken("test"),
                HdDependencySchema::Builder()
                    .SetDependedOnPrimPath(
                        HdRetainedTypedSampledDataSource<SdfPath>::New(
                            SdfPath("/F")))
                    .SetDependedOnDataSourceLocator(
                        RDS::New(HdDataSourceLocator(TfToken("attr3"))))
                    .SetAffectedDataSourceLocator(
                        RDS::New(HdDataSourceLocator(TfToken("attr2"))))
                    .Build()
            )
        )}}
    );

    retainedScene.AddPrims({{SdfPath("/F"), TfToken("group"),
        HdRetainedContainerDataSource::New(
            HdDependenciesSchemaTokens->__dependencies,
            HdRetainedContainerDataSource::New(
                TfToken("test"),
                HdDependencySchema::Builder()
                    .SetDependedOnPrimPath(
                        HdRetainedTypedSampledDataSource<SdfPath>::New(
                            SdfPath("/D")))
                    .SetDependedOnDataSourceLocator(
                        RDS::New(HdDataSourceLocator(TfToken("attr1"))))
                    .SetAffectedDataSourceLocator(
                        RDS::New(HdDataSourceLocator(TfToken("attr3"))))
                    .Build()
            )
        )}}
    );

    RecordingSceneIndexObserver recordingScene;
    dependencyForwardingScene.AddObserver(
        HdSceneIndexObserverPtr(&recordingScene));

    // pulling on the scene causes dependencies to be computed at the visited
    // prim
    PrintSceneIndexPrim(dependencyForwardingScene, SdfPath("/"), true);

    {
        recordingScene.Clear();
        retainedScene.DirtyPrims({{
            SdfPath("/A"),
            HdDataSourceLocator(TfToken("taco"))}}
        );

        auto baseline = EventSet{
            Event{
                EventType::EventType_PrimDirtied,
                SdfPath("/A"), TfToken(),
                HdDataSourceLocator(TfToken("taco"))
            },
            Event{
                EventType::EventType_PrimDirtied,
                SdfPath("/B"), TfToken(),
                HdDataSourceLocator(TfToken("chicken"))
            },
            Event{
                EventType::EventType_PrimDirtied,
                SdfPath("/C"), TfToken(),
                HdDataSourceLocator(TfToken("salsa"))
            },
        };

        if (!_CompareValue("DIRTYING \"/A @taco\" ->",
                recordingScene.GetEventsAsSet(), baseline)) {
            return false;
        }
    }

    {
        recordingScene.Clear();
        retainedScene.DirtyPrims({{
            SdfPath("/A"),
            HdDataSourceLocator()}}
        );

        auto baseline = EventSet{
            Event{
                EventType::EventType_PrimDirtied,
                SdfPath("/A"), TfToken(),
                HdDataSourceLocator()
            },
            Event{
                EventType::EventType_PrimDirtied,
                SdfPath("/B"), TfToken(),
                HdDataSourceLocator(TfToken("chicken"))
            },
            Event{
                EventType::EventType_PrimDirtied,
                SdfPath("/C"), TfToken(),
                HdDataSourceLocator(TfToken("salsa"))
            },
        };

        if (!_CompareValue("DIRTYING \"/A @(prim level)\" ->",
                recordingScene.GetEventsAsSet(), baseline)) {
            return false;
        }
    }


    // test cycles
    {
        auto baseline = EventSet{
            Event{
                EventType::EventType_PrimDirtied,
                SdfPath("/D"), TfToken(),
                HdDataSourceLocator(TfToken("attr1"))
            },
            Event{
                EventType::EventType_PrimDirtied,
                SdfPath("/E"), TfToken(),
                HdDataSourceLocator(TfToken("attr2"))
            },
            Event{
                EventType::EventType_PrimDirtied,
                SdfPath("/F"), TfToken(),
                HdDataSourceLocator(TfToken("attr3"))
            },
        };


        recordingScene.Clear();
        retainedScene.DirtyPrims({{
            SdfPath("/D"),
            HdDataSourceLocator(TfToken("attr1"))}}
        );

        if (!_CompareValue("CYCLE CHECK: DIRTYING \"/D @attr1\" ->",
                recordingScene.GetEventsAsSet(), baseline)) {
            return false;
        }

        recordingScene.Clear();
        retainedScene.DirtyPrims({{
            SdfPath("/E"),
            HdDataSourceLocator(TfToken("attr2"))}}
        );

        if (!_CompareValue("CYCLE CHECK: DIRTYING \"/E @attr2\" ->",
                recordingScene.GetEventsAsSet(), baseline)) {
            return false;
        }

        recordingScene.Clear();
        retainedScene.DirtyPrims({{
            SdfPath("/F"),
            HdDataSourceLocator(TfToken("attr3"))}}
        );

        if (!_CompareValue("CYCLE CHECK: DIRTYING \"/E @attr3\" ->",
                recordingScene.GetEventsAsSet(), baseline)) {
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------

void TestDependencyForwardingSceneIndexEviction_InitScenes(
    HdRetainedSceneIndexRefPtr * retainedSceneP,
    HdDependencyForwardingSceneIndexRefPtr * dependencyForwardingSceneP)
{
    using RDS = HdRetainedTypedSampledDataSource<HdDataSourceLocator>;


    HdRetainedSceneIndexRefPtr retainedScene = HdRetainedSceneIndex::New();

    retainedScene->AddPrims({{SdfPath("/A"), TfToken("group"),
        HdRetainedContainerDataSource::New()}}
    );

    retainedScene->AddPrims({{SdfPath("/B"), TfToken("group"),
        HdRetainedContainerDataSource::New(
            HdDependenciesSchemaTokens->__dependencies,
            HdRetainedContainerDataSource::New(
                TfToken("test"),
                HdDependencySchema::Builder()
                    .SetDependedOnPrimPath(
                        HdRetainedTypedSampledDataSource<SdfPath>::New(
                            SdfPath("/A")))
                    .SetDependedOnDataSourceLocator(
                        RDS::New(HdDataSourceLocator(TfToken("taco"))))
                    .SetAffectedDataSourceLocator(
                        RDS::New(HdDataSourceLocator(TfToken("chicken"))))
                    .Build()
            )
        )}}
    );

    retainedScene->AddPrims({{SdfPath("/C"), TfToken("group"),
        HdRetainedContainerDataSource::New()}}
    );

    HdDependencyForwardingSceneIndexRefPtr dependencyForwardingScene =
            HdDependencyForwardingSceneIndex::New(retainedScene);


    // pull on all prims to seed the cache
    PrintSceneIndexPrim(*dependencyForwardingScene, SdfPath("/"), true);


    *retainedSceneP = retainedScene;
    *dependencyForwardingSceneP = dependencyForwardingScene;
}


bool TestDependencyForwardingSceneIndexEviction()
{
    using EventType = RecordingSceneIndexObserver::EventType;
    using Event = RecordingSceneIndexObserver::Event;
    using EventSet = RecordingSceneIndexObserver::EventSet;

    HdRetainedSceneIndexRefPtr retainedScene;
    HdDependencyForwardingSceneIndexRefPtr dependencyForwardingScene;

    //---------------------
    {
        TestDependencyForwardingSceneIndexEviction_InitScenes(
                &retainedScene, &dependencyForwardingScene);
        
        RecordingSceneIndexObserver recordingScene;
        dependencyForwardingScene->AddObserver(
            HdSceneIndexObserverPtr(&recordingScene));

        retainedScene->RemovePrims({SdfPath("/B")});

        // Validate recorded events.
        // Since nothing depends on B, we should see just the removal event.
        {
            auto baseline = EventSet{
                Event{
                    EventType::EventType_PrimRemoved,
                    SdfPath("/B"), TfToken(),
                    HdDataSourceLocator()
                },
            };

            if (!_CompareValue("Removing \"/B\" ->",
                    recordingScene.GetEventsAsSet(), baseline)) {
                return false;
            }
        }

        // Validate bookkeeping.
        {
            SdfPathVector removedAffectedPrimPaths;
            SdfPathVector removedDependedOnPrimPaths;
            dependencyForwardingScene->RemoveDeletedEntries(
                    &removedAffectedPrimPaths,
                    &removedDependedOnPrimPaths);

            //std::sort(
            //    removedAffectedPrimPaths.begin(), removedAffectedPrimPaths.end());
            //std::sort(
            //    removedDependedOnPrimPaths.begin(), removedDependedOnPrimPaths.end());

            SdfPathVector baselineAffected = {SdfPath("/B")};
            SdfPathVector baselineDependedOn = {SdfPath("/A")};

            if (!_CompareValue("Remove Affected (affected paths): ",
                    removedAffectedPrimPaths, baselineAffected)) {
                return false;
            }
            if (!_CompareValue("Remove Affected (depended on paths): ",
                    removedDependedOnPrimPaths, baselineDependedOn)) {
                return false;
            }
        }
    }

    //---------------------
    {
        TestDependencyForwardingSceneIndexEviction_InitScenes(
                &retainedScene, &dependencyForwardingScene);

        RecordingSceneIndexObserver recordingScene;
        dependencyForwardingScene->AddObserver(
            HdSceneIndexObserverPtr(&recordingScene));

        retainedScene->RemovePrims({SdfPath("/A")});

        // Validate recorded events.
        // Since B depends on A, we should see it getting a dirty notice in
        // addition to A's removal.
        {
                auto baseline = EventSet{
                Event{
                    EventType::EventType_PrimRemoved,
                    SdfPath("/A"), TfToken(),
                    HdDataSourceLocator()
                },
                Event{
                    EventType::EventType_PrimDirtied,
                    SdfPath("/B"), TfToken(),
                    HdDataSourceLocator(TfToken("chicken"))
                },
            };

            if (!_CompareValue("Removing \"/A\" ->",
                    recordingScene.GetEventsAsSet(), baseline)) {
                return false;
            }
        }

        // Validate bookkeeping.
        {
            // NOTE: this should be removing /A from affected paths also!
            //       (since we pulled on it, it should have checked for
            //        dependencies and dirtied a group)
            SdfPathVector baselineAffected = {SdfPath("/B")};
            SdfPathVector baselineDependedOn = {SdfPath("/A")};
            SdfPathVector removedAffectedPrimPaths;
            SdfPathVector removedDependedOnPrimPaths;
            dependencyForwardingScene->RemoveDeletedEntries(
                &removedAffectedPrimPaths,
                &removedDependedOnPrimPaths);

            if (!_CompareValue("Remove Depended On (affected paths): ",
                    removedAffectedPrimPaths, baselineAffected)) {
                return false;
            }
            if (!_CompareValue("Remove Depended On (depended on paths): ",
                    removedDependedOnPrimPaths, baselineDependedOn)) {
                return false;
            }
        }
    }

    //---------------------
    TestDependencyForwardingSceneIndexEviction_InitScenes(
            &retainedScene, &dependencyForwardingScene);
    
    RecordingSceneIndexObserver recordingScene;
        dependencyForwardingScene->AddObserver(
            HdSceneIndexObserverPtr(&recordingScene));
    
    retainedScene->RemovePrims({SdfPath("/C")});

    // Validate recorded events.
    // Since nothing depends on C, we should see just the removal event.
    {
            auto baseline = EventSet{
            Event{
                EventType::EventType_PrimRemoved,
                SdfPath("/C"), TfToken(),
                HdDataSourceLocator()
            },
        };

        if (!_CompareValue("Removing \"/C\" ->",
                recordingScene.GetEventsAsSet(), baseline)) {
            return false;
        }
    }

    // Validate bookkeeping.
    {
        // expecting nothing as _UpdateDependencies exits early if there is no
        // dependency data source
        SdfPathVector baselineAffected = {};
        SdfPathVector baselineDependedOn = {};
        SdfPathVector removedAffectedPrimPaths;
        SdfPathVector removedDependedOnPrimPaths;

        dependencyForwardingScene->RemoveDeletedEntries(
            &removedAffectedPrimPaths,
            &removedDependedOnPrimPaths);

        if (!_CompareValue(
            "Remove Prim Without Dependencies (affected paths): ",
                removedAffectedPrimPaths, baselineAffected)) {
            return false;
        }
        if (!_CompareValue(
            "Remove Prim Without Dependencies  (depended on paths): ",
                removedDependedOnPrimPaths, baselineDependedOn)) {
            return false;
        }
    }

    return true;
}

//-----------------------------------------------------------------------------

// Setup a scene wherein the caller provides the prim container for "/Human".
// This is to allow mutations of the data sources, specifically the one
// corresponding to the __dependencies locator.
//
void TestDependencyForwardingSceneIndexForDependentDependencies_InitScenes(
    const HdContainerDataSourceHandle &humanPrimDs,
    HdRetainedSceneIndexRefPtr * retainedSceneP,
    HdDependencyForwardingSceneIndexRefPtr * dependencyForwardingSceneP)
{
    using LocatorDS = HdRetainedTypedSampledDataSource<HdDataSourceLocator>;
    using PathDS = HdRetainedTypedSampledDataSource<SdfPath>;
    using PathsDS = HdRetainedTypedSampledDataSource<VtArray<SdfPath>>;
    using BoolDS = HdRetainedTypedSampledDataSource<bool>;

    HdRetainedSceneIndexRefPtr retainedScene = HdRetainedSceneIndex::New();

    retainedScene->AddPrims(
        {{SdfPath("/Human"), TfToken("group"), humanPrimDs}});

    retainedScene->AddPrims({{
        SdfPath("/Dog"), TfToken("group"),
        HdRetainedContainerDataSource::New(
            TfToken("hungry"), BoolDS::New(false),
            TfToken("bark"), BoolDS::New(false))}});

    retainedScene->AddPrims({{
        SdfPath("/Cat"), TfToken("group"),
        HdRetainedContainerDataSource::New(
            TfToken("hungry"), BoolDS::New(false),
            TfToken("meow"), BoolDS::New(false))}});

    retainedScene->AddPrims({{
        SdfPath("/Tiger"), TfToken("group"),
        HdRetainedContainerDataSource::New(
            TfToken("hungry"), BoolDS::New(false),
            TfToken("growl"), BoolDS::New(false))}});

    HdDependencyForwardingSceneIndexRefPtr dependencyForwardingScene =
            HdDependencyForwardingSceneIndex::New(retainedScene);

    // pull on all prims to seed the cache
    PrintSceneIndexPrim(*dependencyForwardingScene, SdfPath("/"), true);

    *retainedSceneP = retainedScene;
    *dependencyForwardingSceneP = dependencyForwardingScene;
}

using _TokenDsPair = std::pair<TfToken, HdDataSourceBaseHandle>;
using _TokenDsPairVector = std::vector<_TokenDsPair>;

// A retained container data source that allows updates to the data source 
// corresponding to a locator token.
//
class _MutableRetainedContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_MutableRetainedContainerDataSource);

    TfTokenVector GetNames() override
    {
        TfTokenVector names;
        names.reserve(_entries.size());
        for (const auto &[name, value] : _entries) {
            names.push_back(name);
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &_name) override
    {
        for (const auto &[name, value] : _entries) {
            if (name == _name) {
                return value;
            }
        }
        return nullptr;
    }

    void Set(const TfToken &_name, const HdDataSourceBaseHandle &_value)
    {
        for (auto &[name, value] : _entries) {
            if (name == _name) {
                value = _value;
                break;
            }
        }
    }

private:
    _MutableRetainedContainerDataSource(
        const _TokenDsPairVector &entries)
    {
        _entries = entries;
    }

    _TokenDsPairVector _entries;
};

// A retained data source for strongly typed sampled data that allows mutations
// of the stored value.
// 
template <typename T>
class _MutableRetainedTypedSampledDataSource :
    public HdRetainedTypedSampledDataSource<T>
{
public:
    HD_DECLARE_DATASOURCE(_MutableRetainedTypedSampledDataSource<T>);

    void SetValue(const T &value)
    {
        this->_value = value;
    }

private:
    _MutableRetainedTypedSampledDataSource(const T &value)
    : HdRetainedTypedSampledDataSource<T>(value) {}
};

// Returns a mutable container with a dependency on each pet prim path and
// an entry to declare that the dependencies depend on the prim's 'pets' 
// locator.
//
_MutableRetainedContainerDataSource::Handle
_BuildHumanDependenciesDs(const HdPathArrayDataSourceHandle &petsDs)
{
    using PathDS = HdRetainedTypedSampledDataSource<SdfPath>;
    using LocatorDS = HdRetainedTypedSampledDataSource<HdDataSourceLocator>;

    _TokenDsPairVector deps;
    const VtArray<SdfPath> petPaths = petsDs->GetTypedValue(0.0);

    for (const SdfPath &petPath : petPaths) {
        deps.push_back({
            // .feed -> <Pet>.hungry
            TfToken(std::string("dep_feed_") + petPath.GetAsString()),
            HdDependencySchema::Builder()
                .SetDependedOnPrimPath(
                    PathDS::New(petPath))
                .SetDependedOnDataSourceLocator(
                    LocatorDS::New(HdDataSourceLocator(TfToken("hungry"))))
                .SetAffectedDataSourceLocator(
                    LocatorDS::New(HdDataSourceLocator(TfToken("feed"))))
                .Build()});
    }

    // .__dependencies -> .pets
    deps.push_back({
        TfToken("dep_deps"),
        HdDependencySchema::Builder()
            .SetDependedOnPrimPath(/*self */nullptr)
            .SetDependedOnDataSourceLocator(
                LocatorDS::New(HdDataSourceLocator(TfToken("pets"))))
            .SetAffectedDataSourceLocator(
                LocatorDS::New(HdDataSourceLocator(
                    HdDependenciesSchema::GetSchemaToken())))
            .Build()});
        
    return _MutableRetainedContainerDataSource::New(deps);
}

bool TestDependencyForwardingSceneIndexForDependentDependencies()
{
    using EventType = RecordingSceneIndexObserver::EventType;
    using Event = RecordingSceneIndexObserver::Event;
    using EventSet = RecordingSceneIndexObserver::EventSet;

    HdRetainedSceneIndexRefPtr retainedScene;
    HdDependencyForwardingSceneIndexRefPtr dependencyForwardingScene;

    //---------------------
    {
        using BoolDS = HdRetainedTypedSampledDataSource<bool>;
        using MutablePathsDS =
            _MutableRetainedTypedSampledDataSource<VtArray<SdfPath>>;

        MutablePathsDS::Handle petsDs = MutablePathsDS::New(
            VtArray<SdfPath>({SdfPath("/Dog"), SdfPath("/Cat")}));
        
        _MutableRetainedContainerDataSource::Handle humanContainer =
            _MutableRetainedContainerDataSource::New(
                _TokenDsPairVector({
                    {TfToken("pets"), petsDs},
                    
                    {TfToken("feed"), BoolDS::New(false)},

                    {HdDependenciesSchema::GetSchemaToken(),
                    _BuildHumanDependenciesDs(petsDs)}
        }));

        TestDependencyForwardingSceneIndexForDependentDependencies_InitScenes(
                humanContainer, &retainedScene, &dependencyForwardingScene);
        
        RecordingSceneIndexObserver recordingScene;
        dependencyForwardingScene->AddObserver(
            HdSceneIndexObserverPtr(&recordingScene));
        
        // We started with 2 pets, /Dog and /Cat.
        // /Human realizes that s/he is a cat person.
        // Add /Tiger and remove /Dog from the pets,
        // and update /Human.__dependencies to reflect this change.
        // This needs to be done before we send the dirty notice, below.
        petsDs->SetValue({SdfPath("/Cat"), SdfPath("/Tiger")});
        humanContainer->Set(
            HdDependenciesSchema::GetSchemaToken(),
            _BuildHumanDependenciesDs(petsDs));

        retainedScene->DirtyPrims({
            {SdfPath("/Human"), HdDataSourceLocator(TfToken("pets"))}});

        // Validate recorded events.
        // /Human.__dependencies depends on /Human.pets, so we should see
        // an additional dirty event.
        {
            auto baseline = EventSet{
                Event{
                    EventType::EventType_PrimDirtied,
                    SdfPath("/Human"), TfToken(),
                    HdDataSourceLocator(TfToken("pets"))
                },
                Event{
                    EventType::EventType_PrimDirtied,
                    SdfPath("/Human"), TfToken(),
                    HdDataSourceLocator(HdDependenciesSchema::GetSchemaToken())
                },
            };

            if (!_CompareValue("Dirty \"/Human.pets\" ->",
                    recordingScene.GetEventsAsSet(), baseline)) {
                return false;
            }
        }

        // Validate bookkeeping. /Human no longer depends on /Dog.
        {
            SdfPathVector removedAffectedPrimPaths;
            SdfPathVector removedDependedOnPrimPaths;
            dependencyForwardingScene->RemoveDeletedEntries(
                    &removedAffectedPrimPaths,
                    &removedDependedOnPrimPaths);

            SdfPathVector baselineAffected = {};
            SdfPathVector baselineDependedOn = {SdfPath("/Dog")};

            if (!_CompareValue("Remove Affected (affected paths): ",
                    removedAffectedPrimPaths, baselineAffected)) {
                return false;
            }
            if (!_CompareValue("Remove Affected (depended on paths): ",
                    removedDependedOnPrimPaths, baselineDependedOn)) {
                return false;
            }
        }

        // Validate that the tables have been updated correctly.
        // 1.
        {
            recordingScene.Clear();
            retainedScene->DirtyPrims({
                {SdfPath("/Tiger"), HdDataSourceLocator(TfToken("hungry"))}});
            
            auto baseline = EventSet{
                Event{
                    EventType::EventType_PrimDirtied,
                    SdfPath("/Tiger"), TfToken(),
                    HdDataSourceLocator(TfToken("hungry"))
                },
                Event{
                    EventType::EventType_PrimDirtied,
                    SdfPath("/Human"), TfToken(),
                    HdDataSourceLocator(TfToken("feed"))
                },
            };

            if (!_CompareValue("Dirty \"/Tiger.hungry\" ->",
                    recordingScene.GetEventsAsSet(), baseline)) {
                return false;
            }
        }
        // 2.
        {
            recordingScene.Clear();
            retainedScene->DirtyPrims({
                {SdfPath("/Dog"), HdDataSourceLocator(TfToken("hungry"))}});
            
            auto baseline = EventSet{
                Event{
                    EventType::EventType_PrimDirtied,
                    SdfPath("/Dog"), TfToken(),
                    HdDataSourceLocator(TfToken("hungry"))
                }
            };

            if (!_CompareValue("Dirty \"/Dog.hungry\" ->",
                    recordingScene.GetEventsAsSet(), baseline)) {
                return false;
            }
        }
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
    TEST(TestMergingSceneIndex);
    TEST(TestMergingSceneIndexPrimAddedNotices);
    TEST(TestDependencyForwardingSceneIndex);
    TEST(TestDependencyForwardingSceneIndexEviction);
    TEST(TestDependencyForwardingSceneIndexForDependentDependencies);

    //--------------------------------------------------------------------------
    std::cout << "DONE testHdSceneIndex" << std::endl;
    return 0;
}
