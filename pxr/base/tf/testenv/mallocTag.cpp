//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/eternalString.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/mallocHook.h"

#include <algorithm>
#include <chrono>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

// TfMallocTag intercepts malloc and free through ArchMallocHook, which is only
// wired up on Linux, so this test is built there only.
#if defined(ARCH_OS_LINUX)

constexpr size_t Unit = 10000000;

using std::pair;
using std::string;
using std::vector;

namespace {

// File-scope test structs for TestMallocTagNew.  Defined at file scope to avoid
// any compiler ambiguity around operator new defined inside a local class.

struct _NewTagOne {
    TF_MALLOC_TAG_NEW("newTagOne")
    char _data[Unit];
};

struct _NewTagTwo {
    TF_MALLOC_TAG_NEW("newTagOuter", "newTagInner")
    char _data[Unit];
};

struct _NewTagThree {
    TF_MALLOC_TAG_NEW("newTagA", "newTagB", "newTagC")
    char _data[Unit];
};

} // anon


static vector<void*> _requests;
static vector<pair<void *, void (*)(void *)>> _newRequests;
static std::mutex _mutex;
int64_t _total = 0;
int64_t _maxTotal = 0;

static void
MyMalloc(size_t n) {
    void* ptr = malloc(n);
    std::lock_guard<std::mutex> lock(_mutex);
    _requests.push_back(ptr);
    _total += n;
    if (_total > _maxTotal) {
        _maxTotal = _total;
    }
}

template <class T>
static void
MyNew() {
    void *objPtr = new T;
    void (*fnPtr)(void *);
    using ET = std::remove_extent_t<T>;
    if constexpr (std::is_array_v<T>) {
        fnPtr = [](void *p) { delete [] static_cast<ET *>(p); };
    }
    else {
        fnPtr = [](void *p) { delete static_cast<ET *>(p); };
    }
    std::lock_guard<std::mutex> lock(_mutex);
    _newRequests.emplace_back(objPtr, fnPtr);
    _total += sizeof(T);
    if (_total > _maxTotal) {
        _maxTotal = _total;
    }
}

static void
FreeAll() {
    for (size_t i = 0; i < _requests.size(); i++) {
        free(::_requests[i]);
    }
    _requests.clear();
    for (auto [ptr, deleter]: _newRequests) {
        deleter(ptr);
    }
    _newRequests.clear();
    _total = 0;
}

// Free all tracked allocations and reset the high-water mark to match.
// Use after TfMallocTag::Clear(), which also resets the max.
static void
ClearAndReset()
{
    FreeAll();
    TfMallocTag::Clear();
    _maxTotal = 0;
}

// Shut down TfMallocTag, free all tracked allocations, and reset all
// accounting to match.  TfMallocTag::Shutdown() zeros its own accounting
// (including the high-water mark), so we sync _maxTotal here.
static void
ShutdownAndReset()
{
    FreeAll();
    TfMallocTag::Shutdown();
    _maxTotal = 0;
}

static void
FreeTaskNoTag() {
    MyMalloc(Unit);
}
    
static void
FreeTaskWithTag() {
    TfAutoMallocTag noname("freeTaskWithTag");
    MyMalloc(Unit);
}

static void
RegularTask(bool tag, int n) {
    if (tag) {
        TfAutoMallocTag noname("threadTag");
        MyMalloc(n);
    }
    else {
        MyMalloc(n);
    }
}

static int64_t
GetBytesForCallSite(const char* name, bool skipRepeated = true)
{
    TfMallocTag::CallTree ct;
    TfMallocTag::GetCallTree(&ct, skipRepeated);
    
    for (size_t i = 0; i < ct.callSites.size(); i++)
        if (ct.callSites[i].name == name)
            return ct.callSites[i].nBytes;

    return -1;
}

// Helper to find a PathNode by following a path of site names from a
// given root node.  Returns nullptr if the path is not found.
static const TfMallocTag::CallTree::PathNode *
FindPathNode(const TfMallocTag::CallTree::PathNode &root,
             std::initializer_list<const char *> path)
{
    const TfMallocTag::CallTree::PathNode *node = &root;
    for (const char *name : path) {
        const TfMallocTag::CallTree::PathNode *found = nullptr;
        for (auto const &child : node->children) {
            if (child.siteName == name) {
                found = &child;
                break;
            }
        }
        if (!found) { return nullptr; }
        node = found;
    }
    return node;
}

// Overload that searches under "myRoot" automatically, since the entire
// test is wrapped in a TfAutoMallocTag("myRoot").
static const TfMallocTag::CallTree::PathNode *
FindPathNode(const TfMallocTag::CallTree &ct,
             std::initializer_list<const char *> path)
{
    const auto *myRoot = FindPathNode(ct.root, {"myRoot"});
    if (!myRoot) { return nullptr; }
    return FindPathNode(*myRoot, path);
}

static bool CloseEnough(int64_t a1, int64_t a2)
{
    // Account for various small allocations during this test that can get
    // billed to various sites.  We use fairly large allocations so we can drown
    // out the small stuff.
    const auto [small, big] = [&]() {
        return abs(a1) < abs(a2)
            ? std::make_pair(a1, a2)
            : std::make_pair(a2, a1);
    }();

    // If small is zero, big must be less than 5% of a unit, otherwise the
    // difference between big and small must be less than 5% of big's absolute
    // value.
    if ((small == 0 && abs(big) < (Unit * 0.05)) ||
        (abs(big - small) < (0.05 * abs(big)))) {
        return true;
    }
    fprintf(stderr, "%" PRId64 " not close to %" PRId64 "\n", a1, a2);
    return false;
}

static bool
MemCheck()
{
    int64_t mallocTagCurrent = TfMallocTag::GetTotalBytes();
    int64_t mallocTagMax = TfMallocTag::GetMaxTotalBytes();

    bool ok = CloseEnough(::_total, mallocTagCurrent);
    printf("Expected about %zd, mallocTag has %zd: %s\n",
           ::_total, mallocTagCurrent, ok ? "[close enough]" : "[not good]");

    bool maxOk = CloseEnough(::_maxTotal, mallocTagMax);
    printf("Expected max of about %zd, actual is %zd: %s\n",
           ::_maxTotal, mallocTagMax, maxOk ? "[close enough]" : "[not good]");

    return ok && maxOk;
}

static void
TestFreeThread()
{
    TfAutoMallocTag noname("site3");

    std::thread t(FreeTaskNoTag);
    t.join();

    printf("bytesForSite[site3] = %zd\n", GetBytesForCallSite("site3"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site3"), 0));
    FreeAll();
}

static void
TestFreeThreadWithTag()
{
    TfAutoMallocTag noname("site4");

    std::thread t(FreeTaskWithTag);
    t.join();

    printf("bytesForSite[freeTaskWithTag] = %zd\n",
           GetBytesForCallSite("freeTaskWithTag"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("freeTaskWithTag"), Unit));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site4"), 0));
    FreeAll();

    printf("bytesForSite[freeTaskWithTag] = %zd\n",
           GetBytesForCallSite("freeTaskWithTag"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("freeTaskWithTag"), 0));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site4"), 0));
}

static void
TestRegularTask()
{
    // Test that we can capture the current tag stack state and use it to bridge
    // allocations from a different thread.
    {
        TfAutoMallocTag noname("name");
        TfMallocTag::StackState curState = TfMallocTag::GetCurrentStackState();
        std::thread t([&curState]() {
            TfMallocTag::StackOverride tso(curState);
            RegularTask(false, Unit);
        });
        t.join();
    }

    printf("bytesForSite[%s] = %zd\n", "name", GetBytesForCallSite("name"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("name"), Unit));
    FreeAll();

    printf("bytesForSite[%s] = %zd\n", "name", GetBytesForCallSite("name"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("name"), 0));
}

static void
TestRegularTaskWithTag()
{
    // Test that we can capture the current tag stack state and use it to bridge
    // allocations from a different thread.
    {
        TfAutoMallocTag noname("site2");
        TfMallocTag::StackState curState = TfMallocTag::GetCurrentStackState();
        std::thread t([&curState]() {
            TfMallocTag::StackOverride tso(curState);
            RegularTask(true, Unit);
        });
        t.join();
    }

    printf("bytesForSite[%s] = %zd\n", "threadTag",
           GetBytesForCallSite("threadTag"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("threadTag"), Unit));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site2"), 0));
    FreeAll();

    printf("bytesForSite[%s] = %zd\n", "threadTag",
           GetBytesForCallSite("threadTag"));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("threadTag"), 0));
}

static void
TestRepeated() 
{
    TfAutoMallocTag noname1("site1");
    MyMalloc(Unit);
    TfAutoMallocTag noname2("site2");
    MyMalloc(2*Unit);
    TfAutoMallocTag noname3("site1");
    MyMalloc(Unit);
    TfAutoMallocTag noname4("site3");
    MyMalloc(Unit);

    TF_AXIOM(CloseEnough(GetBytesForCallSite("site2", false), 2*Unit));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site2", true ), 3*Unit));

    TF_AXIOM(CloseEnough(GetBytesForCallSite("site1", true), Unit));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site1", false), 2*Unit));

    TF_AXIOM(CloseEnough(GetBytesForCallSite("site3", true), Unit));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("site3", false), Unit));

    FreeAll();
}

static void
TestMultiTags()
{
    TfAutoMallocTag tags1("multi1", "multi2", "multi3", "multi4");
    MyMalloc(Unit);
    TfAutoMallocTag tags2("multi5", "multi6");
    MyMalloc(Unit);

    TF_AXIOM(CloseEnough(GetBytesForCallSite("multi1", false), 0));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("multi2", false), 0));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("multi3", false), 0));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("multi4", false), Unit));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("multi5", false), 0));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("multi6", false), Unit));

    FreeAll();
}

// One tag name, pushed by all three routes that reach a call site, must resolve
// to one Tf_MallocCallSite and therefore one path node.
//
// The three differ in where the characters live and how the site is keyed:
//
//   - a string literal, and
//   - a TfEternalString
//       both have immortal addresses, so both take the address-keyed fast path.
//       They are *different* addresses, so they occupy two separate fast-path
//       entries pointing at the one site.
//   - a std::string built at runtime
//       has a transient address, so it skips the fast path entirely and can
//       only resolve by content.
//
// The literal pushes first here, so it is the literal's characters the site
// adopts; the other two find that site rather than creating one.  (Whichever
// route arrives first decides -- had the std::string led, the site would hold a
// strdup'd copy instead, and neither immortal address would equal its _name.)
//
// Since address <=> content holds for a TfEternalString, it is tempting to
// conclude that first sight of one could skip the content-keyed call site
// table.  It cannot -- that table is the only thing all three routes share, and
// without it the report would grow sibling nodes with identical names.
static void
TestEternalStringTagName()
{
    // Intern outside any tag scope of interest -- Immortalize() allocates the
    // first time it sees this content, and we do not want that billed to the
    // tag under test.
    const TfEternalString eternalSame =
        TfEternalString::Immortalize("esSame");

    // Assembled at runtime so the characters cannot be a literal's: this is the
    // copying, content-keyed path.
    const std::string dynamicSame = std::string("esS") + "ame";

    TfAutoMallocTag esRoot("esRoot");

    {
        TfAutoMallocTag asLiteral("esSame");
        MyMalloc(Unit);
    }
    {
        TfAutoMallocTag asEternal(eternalSame);
        MyMalloc(Unit);
    }
    {
        TfAutoMallocTag asDynamic(dynamicSame);
        MyMalloc(Unit);
    }

    TfMallocTag::CallTree ct;
    TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
    const auto *root = FindPathNode(ct, {"esRoot"});
    TF_AXIOM(root);

    size_t numSame = 0;
    const TfMallocTag::CallTree::PathNode *same = nullptr;
    for (auto const &child : root->children) {
        if (child.siteName == "esSame") {
            ++numSame;
            same = &child;
        }
    }
    printf("children of esRoot named esSame = %zu (want 1)\n", numSame);
    TF_AXIOM(numSame == 1);

    // All three pushes billed to that one node.
    TF_AXIOM(same && CloseEnough(same->nBytes, 3 * Unit));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("esSame"), 3 * Unit));

    FreeAll();
    TF_AXIOM(CloseEnough(GetBytesForCallSite("esSame"), 0));
}

static void
TestMallocTagNew()
{
    // Single tag: operator new bills bytes to "newTagOne".
    {
        MyNew<_NewTagOne>();
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagOne"), Unit));
        FreeAll();
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagOne"), 0));
    }

    // Two tags: bytes bill to the innermost tag "newTagInner".  "newTagOuter"
    // must appear as an ancestor in the call tree with nBytesDirect == 0 but
    // nBytes == Unit (it owns the child's bytes).
    {
        MyNew<_NewTagTwo>();
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagInner"), Unit));
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagOuter"), 0));

        TfMallocTag::CallTree ct;
        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *outer = FindPathNode(ct, {"newTagOuter"});
        const auto *inner = FindPathNode(ct, {"newTagOuter", "newTagInner"});
        TF_AXIOM(outer && inner);
        TF_AXIOM(CloseEnough(outer->nBytesDirect, 0));
        TF_AXIOM(CloseEnough(outer->nBytes,       Unit));
        TF_AXIOM(CloseEnough(inner->nBytesDirect, Unit));
        TF_AXIOM(CloseEnough(inner->nBytes,       Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagInner"), 0));
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagOuter"), 0));
    }

    // Three tags (variadic form): bytes bill to innermost "newTagC".  "newTagA"
    // and "newTagB" are ancestors with no direct bytes.
    {
        MyNew<_NewTagThree>();
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagC"), Unit));
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagB"), 0));
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagA"), 0));

        TfMallocTag::CallTree ct;
        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *nodeC =
            FindPathNode(ct, {"newTagA", "newTagB", "newTagC"});
        TF_AXIOM(nodeC && CloseEnough(nodeC->nBytes, Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagC"), 0));
    }

    // operator new[]: array allocation also bills to the tag.
    {
        MyNew<_NewTagOne[2]>();
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagOne"), 2 * Unit));
        FreeAll();
        TF_AXIOM(CloseEnough(GetBytesForCallSite("newTagOne"), 0));
    }

    // Allocations via new/delete don't go through MyMalloc/FreeAll so _maxTotal
    // is not updated.  Reset both sides to keep MemCheck() clean.
    ClearAndReset();
}

static void
TestPauseControl()
{
    // Basic local pause -- allocations while paused don't contribute bytes.
    {
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        {
            auto pause = TfMallocTag::PauseThisThread();
            MyMalloc(Unit);
            // Paused allocation does not contribute.
            TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));
        }

        // After unpause, new allocations are tracked again.
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 2 * Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }

    // Local pause is nestable.
    {
        auto pause1 = TfMallocTag::PauseThisThread();
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
        {
            auto pause2 = TfMallocTag::PauseThisThread();
            MyMalloc(Unit);
            TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
        }
        // pause1 still active -- still paused.
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));

        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }

    // DeferredPause -- construct unpaused, pause manually, unpause manually.
    {
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        auto pause = TfMallocTag::DeferredPause();
        TF_AXIOM(!pause.IsPaused());
        // Not yet paused -- allocation is tracked.
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 2 * Unit));

        pause.PauseThisThread();
        TF_AXIOM(pause.IsPaused());
        // Paused -- not tracked.
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 2 * Unit));

        pause.Unpause();
        TF_AXIOM(!pause.IsPaused());
        // Unpaused -- tracked again.
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 3 * Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }

    // Global pause -- affects allocations on another thread.
    {
        auto pause = TfMallocTag::PauseAllThreads();

        std::thread t([]() {
            MyMalloc(Unit);
        });
        t.join();

        // Paused allocation on other thread not counted.
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));

        pause.Unpause();

        // After global unpause, other threads track normally again.
        std::thread t2([]() {
            MyMalloc(Unit);
        });
        t2.join();

        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }

    // Global pause is nestable across threads.
    {
        auto pause1 = TfMallocTag::PauseAllThreads();

        std::thread t([&]() {
            // Second global pause from a different thread.
            auto pause2 = TfMallocTag::PauseAllThreads();
            MyMalloc(Unit);
            TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
            // pause2 destructor decrements global count but pause1 still active.
        });
        t.join();

        // pause1 still active -- still paused.
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));

        pause1.Unpause();

        // Now fully unpaused -- tracked again.
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }

    // The _maxTotal that MyMalloc() tracks won't match what malloc tags report
    // since it was paused, so just clear and reset to keep MemCheck() happy.
    ClearAndReset();
}

static void
TestPauseControlMove()
{
    // Move construction transfers pause ownership -- original no longer
    // unpauses on destruction.
    {
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        auto pause1 = TfMallocTag::PauseThisThread();
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        // Move construct -- pause1 is now disengaged.
        auto pause2 = std::move(pause1);
        TF_AXIOM(!pause1.IsPaused());
        TF_AXIOM(pause2.IsPaused());

        // Still paused via pause2.
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        // pause1 destructor is a no-op, pause2 still holds the pause.
        { auto sink = std::move(pause1); }
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        // pause2 destructor unpauses.
        { auto sink = std::move(pause2); }
        TF_AXIOM(!pause2.IsPaused());

        // Now unpaused -- tracked again.
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 2 * Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }

    // Move assignment transfers pause ownership.
    {
        auto pause1 = TfMallocTag::PauseThisThread();
        TF_AXIOM(pause1.IsPaused());

        auto pause2 = TfMallocTag::DeferredPause();
        TF_AXIOM(!pause2.IsPaused());

        // Move assign -- pause2 takes ownership, pause1 disengaged.
        pause2 = std::move(pause1);
        TF_AXIOM(!pause1.IsPaused());
        TF_AXIOM(pause2.IsPaused());

        // Still paused via pause2.
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));

        // Move assign a deferred pause onto pause2 -- should unpause first.
        pause2 = TfMallocTag::DeferredPause();
        TF_AXIOM(!pause2.IsPaused());

        // Now unpaused.
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }

    // DeferredPause with PauseAllThreads().
    {
        auto pause = TfMallocTag::DeferredPause();
        TF_AXIOM(!pause.IsPaused());

        // Not paused -- other thread allocation tracked.
        std::thread t1([]() { MyMalloc(Unit); });
        t1.join();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        pause.PauseAllThreads();
        TF_AXIOM(pause.IsPaused());

        // Paused globally -- other thread allocation not tracked.
        std::thread t2([]() { MyMalloc(Unit); });
        t2.join();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        pause.Unpause();
        TF_AXIOM(!pause.IsPaused());

        // Unpaused -- other thread allocation tracked again.
        std::thread t3([]() { MyMalloc(Unit); });
        t3.join();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 2 * Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }
    // The _maxTotal that MyMalloc() tracks won't match what malloc tags report
    // since it was paused, so just clear and reset to keep MemCheck() happy.
    ClearAndReset();
}

static void
TestCrossThreadFree()
{
    // Allocate on this thread under a tag, free on another thread.
    // Verify that bytes correctly go to zero after the free.
    {
        TfAutoMallocTag tag("crossThreadFree");

        void *ptr = malloc(Unit);
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _requests.push_back(ptr);
            _total += Unit;
            if (_total > _maxTotal) { _maxTotal = _total; }
        }

        TF_AXIOM(CloseEnough(GetBytesForCallSite("crossThreadFree"), Unit));

        // Free on another thread.
        std::thread t([ptr]() {
            free(ptr);
        });
        t.join();

        // Remove from _requests since we freed it manually.
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _requests.erase(
                std::find(_requests.begin(), _requests.end(), ptr));
            _total -= Unit;
        }

        TF_AXIOM(CloseEnough(GetBytesForCallSite("crossThreadFree"), 0));
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }

    // Allocate on multiple threads under the same tag, free on different
    // threads.  Verify total bytes accounts correctly.
    {
        void *ptr1 = nullptr, *ptr2 = nullptr;

        std::thread t1([&ptr1]() {
            TfAutoMallocTag tag("crossThreadMulti");
            ptr1 = malloc(Unit);
            std::lock_guard<std::mutex> lock(_mutex);
            _requests.push_back(ptr1);
            _total += Unit;
            if (_total > _maxTotal) { _maxTotal = _total; }
        });
        t1.join();

        std::thread t2([&ptr2]() {
            TfAutoMallocTag tag("crossThreadMulti");
            ptr2 = malloc(Unit);
            std::lock_guard<std::mutex> lock(_mutex);
            _requests.push_back(ptr2);
            _total += Unit;
            if (_total > _maxTotal) { _maxTotal = _total; }
        });
        t2.join();

        TF_AXIOM(CloseEnough(GetBytesForCallSite("crossThreadMulti"),
                             2 * Unit));

        // Free both on a third thread.
        std::thread t3([ptr1, ptr2]() {
            free(ptr1);
            free(ptr2);
        });
        t3.join();

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _requests.erase(
                std::find(_requests.begin(), _requests.end(), ptr1));
            _requests.erase(
                std::find(_requests.begin(), _requests.end(), ptr2));
            _total -= 2 * Unit;
        }

        TF_AXIOM(CloseEnough(GetBytesForCallSite("crossThreadMulti"), 0));
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }
}

// A cancelled alloc/free pair must not leave an earlier allocation of the same
// address billed to its tag forever.
//
// This is a regression case for a bug.  The hazard was an interaction between
// two mechanisms.  Alloc() replaced a matching look-back event in place instead
// of appending, and it did so even when the event it found was a free -- which
// destroyed the only record that the address died.  Separately, Free() may
// cancel an alloc/free pair outright so that neither event is ever seen by the
// consolidator.  Put them together and the free that should have killed an
// earlier allocation gets destroyed by the alloc-side replacement, and then the
// replacement itself gets cancelled, so nothing is left to kill the earlier
// alloc.
//
// The sequence, all on one thread:
//
//   1. Allocate X under "phantomOld", then query the call tree.  The query
//      forces a demand consolidation, which drains the look-back window and
//      moves X's alloc event into the baseline -- putting it beyond the reach
//      of any later cancellation, which can only touch the current buffer.
//   2. Free X.  The window no longer holds anything for X, so this appends a
//      free event.
//   3. Allocate the same size under "phantomNew".  The allocator hands X
//      straight back.  This is the step the bug turned on: the old Alloc()
//      found the step-2 free event in the window and overwrote it in place,
//      destroying the only record that X had died.  The current Alloc() only
//      ever appends, so that free now survives.
//   4. Free X again.  The window holds the step-3 alloc and the free counter
//      has not moved, so the pair cancels and both events vanish.
//
// Under the bug, the only surviving event for X was then the step-1 alloc in
// the baseline, so "phantomOld" stayed billed for X even though X is not live.
// With the fix the step-2 free outlives step 4's cancellation and supersedes
// that step-1 alloc at the next consolidation, so neither tag keeps the bytes
// -- which is what the assertions below check.
//
// Note what makes this reachable in practice rather than a contrivance: step 3
// needs the allocator to return a just-freed address for a same-sized request,
// which is the single most common thing an allocator does.
static void
TestCancelDoesNotOrphanEarlierAlloc()
{
    // A small size recycles most predictably, since the allocator's per-size
    // free lists are LIFO.  A tag used by nothing else means its byte count is
    // exact, so this needs no CloseEnough() slack to see a leak this small.
    static constexpr size_t Sz = 512;

    // Repeat a few times.  A single attempt can fail to exercise the hazard for
    // two independent reasons -- the allocator may not recycle the address, and
    // a background free of a hash-equivalent address (the consolidator thread
    // frees constantly) can block the step-4 cancellation.  Both make the test
    // pass vacuously, so count the attempts that really landed.
    int exercised = 0;
    for (int attempt = 0; attempt != 8; ++attempt) {
        void *first = nullptr;
        {
            TfAutoMallocTag tag("phantomOld");
            first = malloc(Sz);
        }
        TF_AXIOM(first);

        // Step 1: force the consolidation.
        TF_AXIOM(GetBytesForCallSite("phantomOld") >= static_cast<int64_t>(Sz));

        // Steps 2 and 3.  Nothing may allocate between them, or the free event
        // could fall out of the look-back window.
        free(first);
        void *second = nullptr;
        {
            TfAutoMallocTag tag("phantomNew");
            second = malloc(Sz);
        }
        TF_AXIOM(second);

        if (second != first) {
            // The allocator did not hand the address back, so there is no
            // earlier allocation of this address to orphan.
            free(second);
            TF_AXIOM(GetBytesForCallSite("phantomOld") <= 0);
            continue;
        }

        // Step 4.
        free(second);
        ++exercised;

        // X is not live.  Neither tag may be billed for it.
        const int64_t oldBytes = GetBytesForCallSite("phantomOld");
        const int64_t newBytes = GetBytesForCallSite("phantomNew");
        if (oldBytes > 0 || newBytes > 0) {
            fprintf(stderr, "orphaned allocation after cancellation: "
                    "phantomOld has %" PRId64 " bytes, phantomNew has %"
                    PRId64 " bytes, expected 0 for both (attempt %d, "
                    "address %p)\n", oldBytes, newBytes, attempt, first);
        }
        TF_AXIOM(oldBytes <= 0);
        TF_AXIOM(newBytes <= 0);
    }

    printf("TestCancelDoesNotOrphanEarlierAlloc: exercised the address-reuse "
           "path on %d of 8 attempts\n", exercised);
    if (exercised == 0) {
        // Do not fail -- this is a statement about the allocator, not about
        // TfMallocTag -- but do not pass quietly either.
        fprintf(stderr, "WARNING: the allocator never recycled the freed "
                "address, so this test verified nothing.  If this persists, "
                "adjust Sz to a size the allocator does recycle.\n");
    }
}

static void
TestGetMaxTotalBytes()
{
    // Reset all accounting including max before testing the high-water mark.
    ClearAndReset();

    // Allocate a known peak amount.
    MyMalloc(3 * Unit);
    TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 3 * Unit));
    TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 3 * Unit));

    // Free everything -- current goes to zero but max should remain.
    FreeAll();
    TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 3 * Unit));

    // Allocate less than the previous peak -- max should not change.
    MyMalloc(Unit);
    TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));
    TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 3 * Unit));

    // Allocate past the previous peak -- max should update.
    MyMalloc(3 * Unit);
    TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 4 * Unit));
    TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 4 * Unit));

    FreeAll();
    TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
}

static void
TestTagStackBehavior()
{
    TfMallocTag::CallTree ct;

    // Allocations with no tags pushed (other than the test's myRoot)
    // are attributed directly to myRoot.
    {
        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *myRoot = FindPathNode(ct.root, {"myRoot"});
        TF_AXIOM(myRoot);
        int64_t rootBytesBefore = myRoot->nBytes;

        MyMalloc(Unit);

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        myRoot = FindPathNode(ct.root, {"myRoot"});
        TF_AXIOM(myRoot);
        TF_AXIOM(CloseEnough(myRoot->nBytes - rootBytesBefore, Unit));

        FreeAll();
    }

    // Pushing and popping restores attribution correctly.  Parent nodes
    // include bytes from children in their nBytes total.
    {
        TfAutoMallocTag tag1("stackA");
        MyMalloc(Unit);
        {
            TfAutoMallocTag tag2("stackB");
            MyMalloc(Unit);

            TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
            const auto *nodeA = FindPathNode(ct, {"stackA"});
            const auto *nodeB = FindPathNode(ct, {"stackA", "stackB"});
            TF_AXIOM(nodeA && nodeB);
            // stackB has only its own allocation.
            TF_AXIOM(CloseEnough(nodeB->nBytesDirect, Unit));
            TF_AXIOM(CloseEnough(nodeB->nBytes,       Unit));
            // stackA includes its own allocation plus stackB's.
            TF_AXIOM(CloseEnough(nodeA->nBytesDirect, Unit));
            TF_AXIOM(CloseEnough(nodeA->nBytes,       2 * Unit));
        }

        // After popping stackB, new allocations go to stackA directly.
        MyMalloc(Unit);

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *nodeA = FindPathNode(ct, {"stackA"});
        const auto *nodeB = FindPathNode(ct, {"stackA", "stackB"});
        TF_AXIOM(nodeA && nodeB);
        TF_AXIOM(CloseEnough(nodeB->nBytes,       Unit));
        TF_AXIOM(CloseEnough(nodeA->nBytesDirect, 2 * Unit));
        TF_AXIOM(CloseEnough(nodeA->nBytes,       3 * Unit));

        FreeAll();

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        nodeA = FindPathNode(ct, {"stackA"});
        TF_AXIOM(!nodeA || nodeA->nBytes == 0);
    }

    // Deep nesting attributes correctly at each level, with nBytes
    // including all descendants and nBytesDirect only the node's own.
    {
        TfAutoMallocTag tag1("deepA");
        MyMalloc(Unit);
        {
            TfAutoMallocTag tag2("deepB");
            MyMalloc(Unit);
            {
                TfAutoMallocTag tag3("deepC");
                MyMalloc(Unit);

                TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
                const auto *nodeA = FindPathNode(ct, {"deepA"});
                const auto *nodeB = FindPathNode(ct, {"deepA", "deepB"});
                const auto *nodeC =
                    FindPathNode(ct, {"deepA", "deepB", "deepC"});
                TF_AXIOM(nodeA && nodeB && nodeC);

                TF_AXIOM(CloseEnough(nodeC->nBytesDirect, Unit));
                TF_AXIOM(CloseEnough(nodeC->nBytes,       Unit));
                TF_AXIOM(CloseEnough(nodeB->nBytesDirect, Unit));
                TF_AXIOM(CloseEnough(nodeB->nBytes,       2 * Unit));
                TF_AXIOM(CloseEnough(nodeA->nBytesDirect, Unit));
                TF_AXIOM(CloseEnough(nodeA->nBytes,       3 * Unit));
            }
        }

        FreeAll();

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *nodeA = FindPathNode(ct, {"deepA"});
        TF_AXIOM(!nodeA || nodeA->nBytes == 0);
    }

    // Re-pushing the same tag name at a different stack depth creates
    // a distinct path node.
    {
        TfAutoMallocTag tag1("rePushOuter");
        MyMalloc(Unit);
        {
            TfAutoMallocTag tag2("rePushInner");
            MyMalloc(Unit);
            {
                // Re-push the outer tag name at a deeper level.
                TfAutoMallocTag tag3("rePushOuter");
                MyMalloc(Unit);

                TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
                const auto *outer =
                    FindPathNode(ct, {"rePushOuter"});
                const auto *inner =
                    FindPathNode(ct, {"rePushOuter", "rePushInner"});
                const auto *reOuter =
                    FindPathNode(ct, {"rePushOuter", "rePushInner",
                                      "rePushOuter"});
                TF_AXIOM(outer && inner && reOuter);

                // The re-pushed outer node is a distinct path node.
                TF_AXIOM(CloseEnough(reOuter->nBytesDirect, Unit));
                TF_AXIOM(CloseEnough(inner->nBytesDirect,   Unit));
                // Outer includes all descendants.
                TF_AXIOM(CloseEnough(outer->nBytes,         3 * Unit));
            }
        }

        FreeAll();

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *outer = FindPathNode(ct, {"rePushOuter"});
        TF_AXIOM(!outer || outer->nBytes == 0);
    }
}

static void
TestPauseWithTagAccounting()
{
    TfMallocTag::CallTree ct;

    // Allocations under a tag while paused don't contribute to that
    // tag's byte count.
    {
        TfAutoMallocTag tag("pausedTag");
        MyMalloc(Unit);

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *node = FindPathNode(ct, {"pausedTag"});
        TF_AXIOM(node);
        TF_AXIOM(CloseEnough(node->nBytes, Unit));

        {
            auto pause = TfMallocTag::PauseThisThread();
            MyMalloc(Unit);

            TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
            node = FindPathNode(ct, {"pausedTag"});
            TF_AXIOM(node);
            // Paused allocation doesn't contribute.
            TF_AXIOM(CloseEnough(node->nBytes, Unit));
        }

        // After unpause, new allocations are attributed normally.
        MyMalloc(Unit);

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        node = FindPathNode(ct, {"pausedTag"});
        TF_AXIOM(node);
        TF_AXIOM(CloseEnough(node->nBytes, 2 * Unit));

        FreeAll();

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        node = FindPathNode(ct, {"pausedTag"});
        TF_AXIOM(!node || node->nBytes == 0);
    }

    // Pausing affects only the paused thread's allocations -- another
    // thread's allocations under the same tag are still counted.
    {
        TfAutoMallocTag tag("sharedTag");
        MyMalloc(Unit);

        auto pause = TfMallocTag::PauseThisThread();

        // This thread's allocation is not counted.
        MyMalloc(Unit);

        // Other thread's allocation under the same tag is counted.
        {
            TfMallocTag::StackState curState =
                TfMallocTag::GetCurrentStackState();
            std::thread t([&curState]() {
                TfMallocTag::StackOverride tso(curState);
                MyMalloc(Unit);
            });
            t.join();
        }

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *node = FindPathNode(ct, {"sharedTag"});
        TF_AXIOM(node);
        // Only the pre-pause and other-thread allocations are counted.
        TF_AXIOM(CloseEnough(node->nBytes, 2 * Unit));

        pause.Unpause();
        FreeAll();

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        node = FindPathNode(ct, {"sharedTag"});
        TF_AXIOM(!node || node->nBytes == 0);
    }

    // Global pause affects all threads' tag accounting.
    {
        auto pause = TfMallocTag::PauseAllThreads();

        TfAutoMallocTag tag("globalPausedTag");
        MyMalloc(Unit);

        std::thread t([]() {
            TfAutoMallocTag tag("globalPausedTag");
            MyMalloc(Unit);
        });
        t.join();

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *node = FindPathNode(ct, {"globalPausedTag"});
        // Neither this thread nor the other thread's allocation counted.
        TF_AXIOM(!node || node->nBytes == 0);

        pause.Unpause();

        // After global unpause, allocations are attributed normally.
        MyMalloc(Unit);

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        node = FindPathNode(ct, {"globalPausedTag"});
        TF_AXIOM(node);
        TF_AXIOM(CloseEnough(node->nBytes, Unit));

        FreeAll();

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        node = FindPathNode(ct, {"globalPausedTag"});
        TF_AXIOM(!node || node->nBytes == 0);
    }

    // Pause interacts correctly with nested tags -- pausing mid-stack
    // doesn't affect the parent tag's already-counted bytes.
    {
        TfAutoMallocTag outer("pauseOuter");
        MyMalloc(Unit);

        {
            TfAutoMallocTag inner("pauseInner");
            MyMalloc(Unit);

            {
                auto pause = TfMallocTag::PauseThisThread();
                MyMalloc(Unit);  // not counted

                TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
                const auto *nodeOuter = FindPathNode(ct, {"pauseOuter"});
                const auto *nodeInner =
                    FindPathNode(ct, {"pauseOuter", "pauseInner"});
                TF_AXIOM(nodeOuter && nodeInner);
                // Paused allocation doesn't show up anywhere.
                TF_AXIOM(CloseEnough(nodeInner->nBytes,       Unit));
                TF_AXIOM(CloseEnough(nodeOuter->nBytesDirect, Unit));
                TF_AXIOM(CloseEnough(nodeOuter->nBytes,       2 * Unit));
            }

            // After unpause, allocations resume normally under inner.
            MyMalloc(Unit);

            TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
            const auto *nodeOuter = FindPathNode(ct, {"pauseOuter"});
            const auto *nodeInner =
                FindPathNode(ct, {"pauseOuter", "pauseInner"});
            TF_AXIOM(nodeOuter && nodeInner);
            TF_AXIOM(CloseEnough(nodeInner->nBytes,       2 * Unit));
            TF_AXIOM(CloseEnough(nodeOuter->nBytesDirect, Unit));
            TF_AXIOM(CloseEnough(nodeOuter->nBytes,       3 * Unit));
        }

        FreeAll();

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *nodeOuter = FindPathNode(ct, {"pauseOuter"});
        TF_AXIOM(!nodeOuter || nodeOuter->nBytes == 0);
    }
}

static void
TestClear()
{
    TfMallocTag::CallTree ct;

    // Basic clear -- resets total bytes and call tree.
    {
        TfAutoMallocTag tag("clearBasic");
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *node = FindPathNode(ct, {"clearBasic"});
        TF_AXIOM(node && CloseEnough(node->nBytes, Unit));

        ClearAndReset();

        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
        TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 0));

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        node = FindPathNode(ct, {"clearBasic"});
        TF_AXIOM(!node || node->nBytes == 0);
    }

    // Clear resets max -- allocate, clear, allocate less, verify max
    // reflects only post-clear activity.
    {
        MyMalloc(3 * Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 3 * Unit));
        TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 3 * Unit));

        ClearAndReset();

        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
        TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 0));

        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));
        TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
        // Max reflects post-clear peak only.
        TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), Unit));
    }

    // Clear discards in-flight thread buffer events.
    {
        // Allocate on another thread, then clear before consolidation.
        std::thread t([]() {
            TfAutoMallocTag tag("clearInFlight");
            MyMalloc(Unit);
        });
        t.join();

        ClearAndReset();

        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *node = FindPathNode(ct, {"clearInFlight"});
        TF_AXIOM(!node || node->nBytes == 0);
    }

    // Allocations after clear are tracked correctly.
    {
        ClearAndReset();

        TfAutoMallocTag tag("clearAfter");
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *node = FindPathNode(ct, {"clearAfter"});
        TF_AXIOM(node && CloseEnough(node->nBytes, Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }
}

static void
TestShutdownBasic()
{
    // Shutdown stops tracking -- allocations after shutdown don't appear.
    // GetTotalBytes() and GetMaxTotalBytes() return exactly 0 after shutdown
    // since Shutdown() zeros all accounting synchronously.
    {
        TfAutoMallocTag tag("shutdownBasic");
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(GetBytesForCallSite("shutdownBasic"), Unit));

        ShutdownAndReset();

        TF_AXIOM(!TfMallocTag::IsInitialized());
        TF_AXIOM(TfMallocTag::GetTotalBytes() == 0);
        TF_AXIOM(TfMallocTag::GetMaxTotalBytes() == 0);

        // Allocations while shut down are not tracked.
        MyMalloc(Unit);
        TF_AXIOM(TfMallocTag::GetTotalBytes() == 0);
        FreeAll();

        string errMsg;
        TF_AXIOM(TfMallocTag::Initialize(&errMsg));
        TF_AXIOM(TfMallocTag::IsInitialized());
    }

    // Allocations before shutdown don't bleed into new session.
    {
        TfAutoMallocTag tag("shutdownBleed");
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(GetBytesForCallSite("shutdownBleed"), Unit));

        ShutdownAndReset();

        string errMsg;
        TF_AXIOM(TfMallocTag::Initialize(&errMsg));

        // Prior session's allocation must not appear.
        TF_AXIOM(CloseEnough(GetBytesForCallSite("shutdownBleed"), 0));
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
        TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 0));
    }

    // GetTotalBytes() and GetMaxTotalBytes() return exactly 0 after shutdown
    // even without re-initializing -- Shutdown() zeros all accounting
    // synchronously.
    {
        MyMalloc(3 * Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 3 * Unit));
        TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 3 * Unit));

        ShutdownAndReset();

        // Exactly zero -- not just CloseEnough -- since Shutdown() is
        // synchronous and no tracking is possible while shut down.
        TF_AXIOM(TfMallocTag::GetTotalBytes() == 0);
        TF_AXIOM(TfMallocTag::GetMaxTotalBytes() == 0);
        
        string errMsg;
        TF_AXIOM(TfMallocTag::Initialize(&errMsg));
    }
    
    // Re-initialize resumes tracking correctly.
    {
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));

        ShutdownAndReset();

        string errMsg;
        TF_AXIOM(TfMallocTag::Initialize(&errMsg));

        TfAutoMallocTag tag("shutdownResume");
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));
        TF_AXIOM(CloseEnough(GetBytesForCallSite("shutdownResume"), Unit));

        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }
}

static void
TestShutdownIdempotency()
{
    // Shutdown() when not initialized is a no-op.
    {
        TF_AXIOM(TfMallocTag::IsInitialized());
        TfMallocTag::Shutdown();
        TF_AXIOM(!TfMallocTag::IsInitialized());

        // Second shutdown is safe.
        TfMallocTag::Shutdown();
        TF_AXIOM(!TfMallocTag::IsInitialized());

        string errMsg;
        TF_AXIOM(TfMallocTag::Initialize(&errMsg));
        TF_AXIOM(TfMallocTag::IsInitialized());
    }

    // Initialize() when already initialized returns true immediately
    // and leaves tracking intact.
    {
        TF_AXIOM(TfMallocTag::IsInitialized());
        string errMsg;
        TF_AXIOM(TfMallocTag::Initialize(&errMsg));
        TF_AXIOM(TfMallocTag::IsInitialized());

        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), Unit));
        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }
}

static void
TestShutdownEpoch()
{
    TfMallocTag::CallTree ct;

    // Allocations from before shutdown must not appear in post-reinit
    // call tree even if the memory was not freed.
    {
        TfAutoMallocTag tag("epochStale");
        MyMalloc(Unit);
        TF_AXIOM(CloseEnough(GetBytesForCallSite("epochStale"), Unit));

        ShutdownAndReset();

        string errMsg;
        TF_AXIOM(TfMallocTag::Initialize(&errMsg));

        // Stale allocation must not appear in new session.
        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *node = FindPathNode(ct, {"epochStale"});
        TF_AXIOM(!node || CloseEnough(node->nBytes, 0));
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
        TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 0));

        // The memory is still allocated -- free it now.
        FreeAll();
    }

    // TfAutoMallocTag (RAII) objects that straddle a Shutdown/Initialize
    // boundary behave correctly: the pre-shutdown tag remains on the node
    // stack (since we don't clear it on shutdown), so post-reinit allocations
    // under a new tag are nested under it.  Both tags unwind correctly when
    // their RAII objects are destroyed.
    {
        {
            TfAutoMallocTag tag("epochStack");
            MyMalloc(Unit);

            ShutdownAndReset();

            string errMsg;
            TF_AXIOM(TfMallocTag::Initialize(&errMsg));

            // "epochStack" is still on the node stack -- new allocations
            // under "epochPostReinit" are nested beneath it.
            TfAutoMallocTag tag2("epochPostReinit");
            MyMalloc(Unit);

            TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
            const auto *node =
                FindPathNode(ct, {"epochStack", "epochPostReinit"});
            TF_AXIOM(node && CloseEnough(node->nBytes, Unit));

            FreeAll();
        }
        // Both tag and tag2 destroyed here -- stack unwinds back to myRoot.
        // New allocations go directly under myRoot, not under epochStack.
        TfAutoMallocTag tag3("epochAfterUnwind");
        MyMalloc(Unit);
        TfMallocTag::GetCallTree(&ct, /*skipRepeated=*/false);
        const auto *node = FindPathNode(ct, {"epochAfterUnwind"});
        TF_AXIOM(node && CloseEnough(node->nBytes, Unit));
        FreeAll();
    }

    // Multiple shutdown/reinit cycles accumulate correctly within each
    // session and reset cleanly between sessions.
    {
        for (int cycle = 0; cycle < 5; ++cycle) {
            TfAutoMallocTag tag("epochCycle");
            MyMalloc(Unit);
            TF_AXIOM(CloseEnough(GetBytesForCallSite("epochCycle"), Unit));

            ShutdownAndReset();

            string errMsg;
            TF_AXIOM(TfMallocTag::Initialize(&errMsg));

            // Each new session starts clean.
            TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
            TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 0));
            TF_AXIOM(CloseEnough(GetBytesForCallSite("epochCycle"), 0));
        }
    }
}

static void
TestShutdownCrossThread()
{
    // Threads active during shutdown have their stale events discarded.
    {
        std::atomic<bool> startAlloc{false};

        std::thread t([&]() {
            TfAutoMallocTag tag("shutdownCrossThread");
            while (!startAlloc.load()) {
                std::this_thread::yield();
            }
            MyMalloc(Unit);
        });

        // Signal the thread to allocate, then immediately shut down.
        startAlloc.store(true);
        ShutdownAndReset();

        t.join();
        // The thread may have allocated after we called FreeAll() inside
        // ShutdownAndReset() -- free anything remaining.
        FreeAll();

        string errMsg;
        TF_AXIOM(TfMallocTag::Initialize(&errMsg));

        // Regardless of the race, after reinit the total must be clean.
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
        TF_AXIOM(CloseEnough(TfMallocTag::GetMaxTotalBytes(), 0));
    }

    // New threads created after reinit track correctly.
    {
        std::thread t([]() {
            TfAutoMallocTag tag("shutdownNewThread");
            MyMalloc(Unit);
        });
        t.join();

        TF_AXIOM(CloseEnough(GetBytesForCallSite("shutdownNewThread"), Unit));
        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }

    // Threads created between shutdown and reinit track correctly after
    // reinit.
    {
        ShutdownAndReset();

        std::atomic<bool> threadReady{false};
        std::atomic<bool> reinitDone{false};

        std::thread t([&]() {
            threadReady.store(true);
            while (!reinitDone.load()) {
                std::this_thread::yield();
            }
            TfAutoMallocTag tag("shutdownMidThread");
            MyMalloc(Unit);
        });

        while (!threadReady.load()) {
            std::this_thread::yield();
        }

        string errMsg;
        TF_AXIOM(TfMallocTag::Initialize(&errMsg));
        reinitDone.store(true);

        t.join();

        TF_AXIOM(CloseEnough(GetBytesForCallSite("shutdownMidThread"), Unit));
        FreeAll();
        TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));
    }
}

static void
TestShutdownConcurrent()
{
    // Hammer concurrent Initialize()/Shutdown() from multiple threads for
    // a few seconds, verifying no crashes or assertion failures.  We don't
    // make strong correctness assertions here since the whole point is
    // exercising the races -- we just verify the system remains stable.
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);

    std::atomic<bool> stop{false};

    // A few threads that repeatedly initialize and shut down.
    auto lifecycleBody = [&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            string errMsg;
            TfMallocTag::Initialize(&errMsg);
            std::this_thread::yield();
            TfMallocTag::Shutdown();
            std::this_thread::yield();
        }
    };

    // A few threads that continuously allocate and free with tags.
    auto allocBody = [&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            TfAutoMallocTag tag("concurrentAlloc");
            void *p = malloc(1024);
            free(p);
            std::this_thread::yield();
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 2; ++i) {
        threads.emplace_back(lifecycleBody);
    }
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(allocBody);
    }

    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    stop.store(true);

    for (auto &t : threads) {
        t.join();
    }

    // Leave the system in initialized state for subsequent tests.
    string errMsg;
    TfMallocTag::Initialize(&errMsg);
    TF_AXIOM(TfMallocTag::IsInitialized());
}

static bool
Test_TfMallocTag()
{
    _requests.reserve(1024);
    _newRequests.reserve(1024);
    
    TF_AXIOM(TfMallocTag::GetTotalBytes() == 0);
    TF_AXIOM(MemCheck());

    // this won't show up in the accounting
    void* mem1 = malloc(Unit);

    string errMsg;
    if (!TfMallocTag::Initialize(&errMsg)) {
        fprintf(stderr, "Unable to initialize malloc tags: %s\n",
                errMsg.c_str());
        fprintf(stderr, "Skipping test\n");
        return true;
    }

    TfAutoMallocTag topTag("myRoot");
    
    // and this free shouldn't either...
    free(mem1);
    printf("total: %zd\n", TfMallocTag::GetTotalBytes());
    TF_AXIOM(CloseEnough(TfMallocTag::GetTotalBytes(), 0));

    MyMalloc(3*Unit);
    TF_AXIOM(MemCheck());
    
    FreeAll();
    TF_AXIOM(MemCheck());

    TfMallocTag::Push(string("manualTag"));
    MyMalloc(Unit);
    TF_AXIOM(CloseEnough(GetBytesForCallSite("manualTag"), Unit));
    TfMallocTag::Push("manualTag2");
    MyMalloc(Unit);
    TF_AXIOM(CloseEnough(GetBytesForCallSite("manualTag"), Unit));
    TfMallocTag::Pop();
    TfMallocTag::Pop();
    TF_AXIOM(CloseEnough(GetBytesForCallSite("manualTag"), Unit));
    FreeAll();
    TF_AXIOM(CloseEnough(GetBytesForCallSite("manualTag"), 0));
    TF_AXIOM(CloseEnough(GetBytesForCallSite("manualTag2"), 0));
    
    FreeAll();
    TF_AXIOM(MemCheck());

    TestRegularTask();
    TF_AXIOM(MemCheck());

    TestRegularTaskWithTag();
    TF_AXIOM(MemCheck());

    TestFreeThread();
    TF_AXIOM(MemCheck());

    TestFreeThreadWithTag();
    TF_AXIOM(MemCheck());

    FreeAll();
    TF_AXIOM(MemCheck());

    TestRepeated();
    TF_AXIOM(MemCheck());

    TestMultiTags();
    TF_AXIOM(MemCheck());

    TestEternalStringTagName();
    TF_AXIOM(MemCheck());

    TestMallocTagNew();
    TF_AXIOM(MemCheck());

    TestPauseControl();
    TF_AXIOM(MemCheck());
    
    TestPauseControlMove();
    TF_AXIOM(MemCheck());

    TestCrossThreadFree();
    TF_AXIOM(MemCheck());

    TestCancelDoesNotOrphanEarlierAlloc();
    TF_AXIOM(MemCheck());

    TestGetMaxTotalBytes();
    TF_AXIOM(MemCheck());
    
    TestTagStackBehavior();
    TF_AXIOM(MemCheck());

    TestPauseWithTagAccounting();
    TF_AXIOM(MemCheck());

    TestClear();
    TF_AXIOM(MemCheck());

    TestShutdownBasic();
    TF_AXIOM(MemCheck());

    TestShutdownIdempotency();
    TF_AXIOM(MemCheck());

    TestShutdownEpoch();
    TF_AXIOM(MemCheck());

    TestShutdownCrossThread();
    TF_AXIOM(MemCheck());

    // Keep this last -- the concurrent stress test deliberately races
    // Initialize()/Shutdown() so accounting is indeterminate at the end.
    TestShutdownConcurrent();

    return true;
}

TF_ADD_REGTEST(TfMallocTag);

#endif
