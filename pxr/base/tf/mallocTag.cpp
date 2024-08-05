//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/mallocTag.h"

#include "pxr/base/tf/bigRWMutex.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_set.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/tf.h"

#include "pxr/base/arch/align.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/hash.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/arch/mallocHook.h"
#include "pxr/base/arch/stackTrace.h"

#include <tbb/concurrent_hash_map.h>

#include <algorithm>
#include <atomic>
#include <istream>
#include <ostream>
#include <regex>
#include <stack>
#include <string>
#include <stdlib.h>
#include <thread>
#include <type_traits>
#include <vector>

using std::map;
using std::make_pair;
using std::pair;
using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

// Change the following line and recompile this file to disable decrementing
// the allocation counts when freeing memory.
#define _DECREMENT_ALLOCATION_COUNTS true

// The max number of captured unique malloc stacks printed out in the report.
static const size_t _MaxReportedMallocStacks = 100;

// The max number of call stack frames stored when malloc stack capturing
// is enabled.  Note that two malloc stacks are considered identical if all
// their frames up to this depth are matching (the uncaptured parts of the
// stacks can still differ).
static const size_t _MaxMallocStackDepth = 64;

// The number of top stack frames to ignore when saving frames for a
// malloc stack.  Currently these frames are:
// #0   ArchGetStackFrames(unsigned long, vector<unsigned long, allocator<unsigned long> >*)
// #1   Tf_MallocGlobalData::_MaybeCaptureStackOrDebug(Tf_MallocPathNode const*, void const*, unsigned long)
// #2   TfMallocTag::_MallocWrapper(unsigned long, void const*)
static const size_t _IgnoreStackFramesCount = 3;

struct Tf_MallocPathNode;
struct Tf_MallocGlobalData;

static ArchMallocHook _mallocHook;      // zero-initialized POD
static Tf_MallocGlobalData* _mallocGlobalData = nullptr;
std::atomic<bool> TfMallocTag::_isInitialized { false };

static bool Tf_MatchesMallocTagDebugName(const string& name);
static bool Tf_MatchesMallocTagTraceName(const string& name);
static void Tf_MallocTagDebugHook(const void* ptr, size_t size) ARCH_NOINLINE;

static void Tf_MallocTagDebugHook(const void* ptr, size_t size)
{
    // Clients don't call this directly so the debugger can conveniently
    // see the pointer and size in the stack trace.
    ARCH_DEBUGGER_TRAP;
}

static inline size_t
Tf_GetMallocBlockSize(void* ptr, size_t requestedSize)
{
    // The allocator-agnostic implementation keeps track of the exact memory
    // block sizes requested by consumers. This ignores allocator-specific
    // overhead, such as alignment, associated metadata, etc. We believe this
    // is the right thing to be measuring, as malloc tags are intended to
    // allow consumers to bill memory requests to their originating subsystem.
    //
    // Uncomment the following line to enable tracking of 'actual' block sizes.
    // Be sure that the allocator in use provides this function! If not, this
    // will call the default glibc implementation, which will likely return
    // the wrong value (unless you're using the glibc allocator).
    // return malloc_usable_size(ptr);

    return requestedSize;
}

struct Tf_MallocBlockInfo {
    Tf_MallocBlockInfo() = default;
    Tf_MallocBlockInfo(size_t sz, Tf_MallocPathNode *pn)
        : blockSize(sz)
        , pathNode(pn) {}
    size_t blockSize = 0;
    Tf_MallocPathNode *pathNode = nullptr;
};

#if !defined(ARCH_OS_WINDOWS)
static_assert(sizeof(Tf_MallocBlockInfo) == 16, 
              "Unexpected size for Tf_MallocBlockInfo");
#endif

/*
 * Utility for checking a const char* against a table of match strings.
 * Each string is tested against each item in the table in order.  Each
 * item can either allow or deny the string, with later entries overriding
 * earlier results.  Match strings can end in '*' to wildcard the suffix
 * and can start with '-' to deny or '+' or nothing to allow.
 *
 * Match strings are concatenated into lists using commas, newlines or tabs.
 * Spaces are not delimiters but they are trimmed from each end.
 */
class Tf_MallocTagStringMatchTable {
public:
    Tf_MallocTagStringMatchTable();
    explicit Tf_MallocTagStringMatchTable(const std::string& matchList);

    // Replace the list of matches.
    void SetMatchList(const std::string& matchList);

    // Return \c true iff \p s matches the most recently set match list.
    bool Match(const char* s) const;

private:
    struct _MatchString {
        _MatchString(const std::string&);

        std::string str;    // String to match.
        bool allow;         // New result if str matches.
        bool wildcard;      // str has a suffix wildcard.
    };
    std::vector<_MatchString> _matchStrings;
};

Tf_MallocTagStringMatchTable::_MatchString::_MatchString(const std::string& s) :
    str(s),
    allow(true),
    wildcard(false)
{
    if (!str.empty()) {
        if (str[str.size() - 1] == '*') {
            wildcard = true;
            str.resize(str.size() - 1);
        }
        if (!str.empty()) {
            if (str[0] == '-') {
                allow = false;
                str.erase(0, 1);
            }
            else if (str[0] == '+') {
                str.erase(0, 1);
            }
        }
    }
}

Tf_MallocTagStringMatchTable::Tf_MallocTagStringMatchTable()
{
    // Do nothing
}

Tf_MallocTagStringMatchTable::Tf_MallocTagStringMatchTable(
    const std::string& matchList)
{
    SetMatchList(matchList);
}

void
Tf_MallocTagStringMatchTable::SetMatchList(const std::string& matchList)
{
    _matchStrings.clear();
    std::vector<std::string> items = TfStringTokenize(matchList, ",\t\n");
    TF_FOR_ALL(i, items) {
        _matchStrings.push_back(_MatchString(TfStringTrim(*i, " ")));
    }
}

bool
Tf_MallocTagStringMatchTable::Match(const char* s) const
{
    // The last match defines the overall result.  If the last match had
    // a '-' prefix then we don't match, otherwise we do.
    TF_REVERSE_FOR_ALL(i, _matchStrings) {
        if (i->wildcard) {
            // Check prefix match.
            const char* m = i->str.c_str();
            while (*m && *m == *s) {
                ++m, ++s;
            }
            if (*m != '\0') {
                continue;
            }
        }
        else {
            // Check exact match.
            if (i->str != s) {
                continue;
            }
        }

        // Matched.
        return i->allow;
    }

    // No match.
    return false;
}

/*
 * There is a different call-site object associated with each different
 * tag string used to construct a TfAutoMallocTag.
 */
struct Tf_MallocCallSite
{
    Tf_MallocCallSite(const string& name)
        : _name(std::make_unique<char []>(strlen(name.c_str()) + 1))
        , _totalBytes(0)
        , _flags(
            (Tf_MatchesMallocTagDebugName(name) ? _DebugFlag : 0) |
            (Tf_MatchesMallocTagTraceName(name) ? _TraceFlag : 0))
        {
            strcpy(_name.get(), name.c_str());
        }
    
    std::unique_ptr<char []> _name;
    
    std::atomic<int64_t> _totalBytes;

    static constexpr unsigned _TraceFlag = 1u;
    static constexpr unsigned _DebugFlag = 1u << 1;

    // If _TraceFlag bit is set, then capture a stack trace when allocating at
    // this site.  If _DebugFlag bit is set, then invoke the debugger trap when
    // allocating or freeing at this site.  This field is only written to when
    // the full global data write lock is held, and is only read when the read
    // lock is held, so it need not be atomic.
    unsigned _flags;
};

namespace {

struct _HashEqCStr
{
    inline bool equal(char const *l, char const *r) const {
        return !strcmp(l, r);
    }
    inline size_t hash(char const *k) const {
        return TfHashCString()(k);
    }
};

using Tf_MallocCallSiteTable =
    tbb::concurrent_hash_map<
    const char*, struct Tf_MallocCallSite *, _HashEqCStr>;

static inline
Tf_MallocCallSite *
Tf_GetOrCreateCallSite(Tf_MallocCallSiteTable* table,
                       const char* name) {

    // Callsites persist after first insertion, so optimistically assume
    // presence.
    {
        Tf_MallocCallSiteTable::const_accessor acc;
        if (table->find(acc, name)) {
            return acc->second;
        }
    }

    // Otherwise new up a site and attempt to insert.  If we lose a race here
    // we'll drop the site we created.
    auto newSite = std::make_unique<Tf_MallocCallSite>(name);

    Tf_MallocCallSiteTable::accessor acc;
    if (table->emplace(acc, newSite->_name.get(), newSite.get())) {
        // We emplaced the new site, so release it from the unique_ptr.
        return newSite.release();
    }
    else {
        // We lost the race, this site has been created in the meantime.  Just
        // return the table's pointer, and let the unique_ptr dispose of the one
        // we created.
        return acc->second;
    }
}


struct _HashEqPathNodeTable
{
    inline bool
    equal(std::pair<Tf_MallocPathNode *, Tf_MallocCallSite *> const &l,
          std::pair<Tf_MallocPathNode *, Tf_MallocCallSite *> const &r) const {
        return l == r;
    }
    inline size_t
    hash(std::pair<Tf_MallocPathNode *, Tf_MallocCallSite *> const &p) const {
        return TfHash()(p);
    }    
};


using Tf_MallocPathNodeTable =
    tbb::concurrent_hash_map<
    std::pair<Tf_MallocPathNode *, Tf_MallocCallSite *>,
    Tf_MallocPathNode *, _HashEqPathNodeTable>;

static inline
Tf_MallocPathNode *
Tf_GetOrCreateChild(
    Tf_MallocPathNodeTable *table,
    std::pair<Tf_MallocPathNode *, Tf_MallocCallSite *> parentAndCallSite)
{
    // Children persist after first insertion, so optimistically assume
    // presence.
    {
        Tf_MallocPathNodeTable::const_accessor acc;
        if (table->find(acc, parentAndCallSite)) {
            return acc->second;
        }
    }

    // Otherwise new up a child node and attempt to insert.  If we lose a race
    // here we'll drop the node we created.
    auto newChild =
        std::make_unique<Tf_MallocPathNode>(parentAndCallSite.second);

    Tf_MallocPathNodeTable::accessor acc;
    if (table->emplace(acc, parentAndCallSite, newChild.get())) {
        // We emplaced the new node, so release it from the unique_ptr.
        return newChild.release();
    }
    else {
        // We lost the race, this node has been created in the meantime.  Just
        // return the table's pointer, and let the unique_ptr dispose of the one
        // we created.
        return acc->second;
    }
}

using Tf_PathNodeChildrenTable = pxr_tsl::robin_map<
    Tf_MallocPathNode const *, std::vector<Tf_MallocPathNode const *>
    >;

} // anon

/*
 * This is a singleton.  Because access to this structure is gated via checks to
 * TfMallocTag::_isInitialized, we forego the usual TfSingleton pattern and just
 * use a single static-scoped pointer (_mallocGlobalData) to point to the
 * singleton instance.
 *
 * The member data in this class is guarded by a _mutex member variable.
 * However, the way this works is a bit different from ordinary mutex-protected
 * data.
 *
 * Since TfMallocTag intercepts all malloc/free routines, it is important for it
 * to be reasonably fast and thread-scalable in order to provide good user
 * experience when tagging is enabled.  To that end, the data structures in this
 * class are mostly concurrent containers and atomics.  This way different
 * threads can concurrently modify these without blocking each other.  But
 * that's not the end of the story.
 *
 * To support queries of the entire malloc tags state to generate reports
 * (e.g. TfMallocTag::GetCallTree()) or to modify global malloc tags behavior
 * (e.g. TfMallocTag::SetCapturedMallocStacksMatchList()) concurrently with
 * other threads doing malloc/free, we must have a way to halt all other reading
 * or mutation of the global state.  Only then can we do concurrency-unsafe
 * operations with the concurrent containers, like iterate over them, and
 * present a consistent result to callers.
 *
 * We do this by employing a readers-writer lock (in _mutex).  Ordinary
 * operations that read or modify the concurrent data structures and atomics in
 * thread-safe ways (such as during malloc/free handling, and tag push/pop) take
 * a "read" (or "shared") lock on _mutex: they can proceed concurrently
 * relatively unfettered.  Operations that read or modify the concurrent data
 * structures and atomics in a thread-unsafe way (such as iterating the data for
 * report generation, or modifying the stack capture match rules or debug match
 * rules) take a "write" (or "exclusive") lock on _mutex: they block all other
 * access until they complete.
 */
struct Tf_MallocGlobalData
{
    Tf_MallocGlobalData()
        : _rootNode(nullptr)
        , _totalBytes(0)
        , _maxTotalBytes(0) {}

    Tf_MallocCallSite *_GetOrCreateCallSite(const char* name) {
        return Tf_GetOrCreateCallSite(&_callSiteTable, name);
    }

    Tf_MallocPathNode *
    _GetOrCreateChild(
        std::pair<Tf_MallocPathNode *,
        Tf_MallocCallSite *> parentAndCallSite) {
        return Tf_GetOrCreateChild(&_pathNodeTable, parentAndCallSite);
    }

    inline void
    _RegisterBlock(const void* block, size_t blockSize,
                   Tf_MallocPathNode* pathNode);
    inline void
    _UnregisterBlock(const void* block);

    Tf_PathNodeChildrenTable _BuildPathNodeChildrenTable() const;

    void _GetStackTrace(size_t skipFrames, std::vector<uintptr_t>* stack);

    void _SetTraceNames(const std::string& matchList);
    bool _MatchesTraceName(const std::string& name);

    void _CaptureStackOrDebug(
        const Tf_MallocPathNode* node, const void *ptr, size_t size);
    inline void _MaybeCaptureStackOrDebug(
        const Tf_MallocPathNode* node, const void *ptr, size_t size);

    void _ReleaseStackOrDebug(
        const Tf_MallocPathNode* node, const void *ptr, size_t size);
    inline void _MaybeReleaseStackOrDebug(
        const Tf_MallocPathNode* node, const void *ptr, size_t size);

    void _BuildUniqueMallocStacks(TfMallocTag::CallTree* tree);

    void _SetDebugNames(const std::string& matchList);
    bool _MatchesDebugName(const std::string& name);

    using _CallStackTableType =
        tbb::concurrent_hash_map<const void *, TfMallocTag::CallStackInfo>;

    TfBigRWMutex _mutex;
    
    Tf_MallocPathNode* _rootNode;

    std::atomic<int64_t> _totalBytes;
    std::atomic<int64_t> _maxTotalBytes;

    // Mapping from memory block to information about that block.
    // Used by allocator-agnostic implementation.
    using _BlockInfoTable =
        tbb::concurrent_hash_map<const void *, Tf_MallocBlockInfo>;

    _BlockInfoTable _blockInfo;
    Tf_MallocCallSiteTable _callSiteTable;
    Tf_MallocPathNodeTable _pathNodeTable;

    Tf_MallocTagStringMatchTable _debugMatchTable;
    Tf_MallocTagStringMatchTable _traceMatchTable;
    _CallStackTableType _callStackTable;
    
};

/*
 * Each node describes a sequence (i.e. path) of call sites.
 * However, a given call-site can occur only once in a given path -- recursive
 * call loops are excised.
 */
struct Tf_MallocPathNode
{
    explicit Tf_MallocPathNode(Tf_MallocCallSite* callSite)
        : _callSite(callSite)
        , _totalBytes(0)
        , _numAllocations(0)
        , _repeated(false)
    {
    }

    void _BuildTree(Tf_PathNodeChildrenTable const &nodeChildren,
                    TfMallocTag::CallTree::PathNode* node,
                    bool skipRepeated) const;

    Tf_MallocCallSite* _callSite;
    std::atomic<int64_t> _totalBytes;
    std::atomic<int64_t> _numAllocations;
    std::atomic<bool> _repeated;    // repeated node
};

// Enum describing whether allocations are being tagged in an associated
// thread.
enum _TaggingState {
    _TaggingEnabled,   // Allocations are being tagged
    _TaggingDisabled,  // Allocations are not being tagged
};

// Per-thread data for TfMallocTag.
struct TfMallocTag::_ThreadData {
    _ThreadData() : _taggingState(_TaggingEnabled) {}
    _ThreadData(const _ThreadData &) = delete;
    _ThreadData(_ThreadData&&) = delete;
    _ThreadData& operator=(const _ThreadData &rhs) = delete;
    _ThreadData& operator=(_ThreadData&&) = delete;

    inline bool TaggingEnabled() const {
        return _taggingState == _TaggingEnabled;
    }

    inline void Push(Tf_MallocCallSite *site,
                     Tf_MallocPathNode *node) {
        if (!_callSitesOnStack.insert(site).second) {
            node->_repeated = true;
            // Push a nullptr onto the _nodeStack preceding repeated nodes.
            // This lets Pop() know not to erase node's site from
            // _callSitesOnStack.
            _nodeStack.push_back(nullptr);
        }
        _nodeStack.push_back(node);
    }

    inline void Pop() {
        Tf_MallocPathNode *node = _nodeStack.back();
        _nodeStack.pop_back();
        // If _nodeStack is not empty check to see if there's a nullptr.  If so,
        // this is a repeated node, so just pop the nullptr.  Otherwise we need
        // to erase this node's site from _callSitesOnStack.
        if (!_nodeStack.empty() && !_nodeStack.back()) {
            // Pop the nullptr, leave the repeated node in _callSitesOnStack.
            _nodeStack.pop_back();
        }
        else {
            // Remove from _callSitesOnStack.
            _callSitesOnStack.erase(node->_callSite);
        }
    }

    inline Tf_MallocPathNode *GetCurrentPathNode() const {
        return !_nodeStack.empty()
            ? _nodeStack.back()
            : _mallocGlobalData->_rootNode;
    }

    _TaggingState _taggingState;
    std::vector<Tf_MallocPathNode*> _nodeStack;
    pxr_tsl::robin_set<Tf_MallocCallSite *, TfHash> _callSitesOnStack;
};

class TfMallocTag::Tls {
public:
    static
    TfMallocTag::_ThreadData &Find() {
#if defined(ARCH_HAS_THREAD_LOCAL)
        // This thread_local must be placed in static TLS to prevent re-entry.
        // Starting in glibc 2.25, dynamic TLS allocation uses malloc.  Making
        // this allocation after malloc tags have been initialized results in
        // infinite recursion.
        static thread_local _ThreadData* data = nullptr;
        if (ARCH_LIKELY(data)) {
            return *data;
        }
        // This weirdness is so we don't re-enter malloc tags and don't call the
        // destructor of _ThreadData when the thread is exiting.  We can't do
        // the latter because we don't know in what order objects will be
        // destroyed and objects destroyed after the _ThreadData may do heap
        // (de)allocation, which requires the _ThreadData object.  We leak the
        // heap allocated blocks in the _ThreadData.
        void *dataBuffer = _mallocHook.IsInitialized()
            ? _mallocHook.Memalign(alignof(_ThreadData), sizeof(_ThreadData))
            : ArchAlignedAlloc(alignof(_ThreadData), sizeof(_ThreadData));
        data = new (dataBuffer) _ThreadData();
        return *data;
#else
        TF_FATAL_ERROR("TfMallocTag not supported on platforms "
                       "without thread_local");
#endif
    }
};

// Helper to temporarily disable tagging operations, so that TfMallocTag
// facilities can use the heap for bookkeeping without recursively invoking
// itself.  Note that these classes do not nest!  The reason is that we expect
// disabling to be done in very specific, carefully considered places, not
// willy-nilly, and not within any recursive contexts.
struct _TemporaryDisabler {
public:
    explicit _TemporaryDisabler(TfMallocTag::_ThreadData *threadData = nullptr)
        : _tls(threadData ? *threadData : TfMallocTag::Tls::Find()) {
        TF_AXIOM(_tls._taggingState == _TaggingEnabled);
        _tls._taggingState = _TaggingDisabled;
    }
        
    ~_TemporaryDisabler() {
        _tls._taggingState = _TaggingEnabled;
    }

private:
    TfMallocTag::_ThreadData &_tls;
};

inline void
Tf_MallocGlobalData::_RegisterBlock(
    const void* block, size_t blockSize, Tf_MallocPathNode* node)
{
    // Disable tagging for this thread so any allocations caused
    // here do not get intercepted and cause recursion.
    _TemporaryDisabler disable;

    TF_DEV_AXIOM(!_blockInfo.count(block));

    _MaybeCaptureStackOrDebug(node, block, blockSize);
    
    _blockInfo.emplace(block, Tf_MallocBlockInfo(blockSize, node));
    
    node->_totalBytes.fetch_add(blockSize, std::memory_order_relaxed);
    node->_callSite->_totalBytes.fetch_add(
        blockSize, std::memory_order_relaxed);
    
    int64_t newTotal = _totalBytes.fetch_add(
        blockSize, std::memory_order_relaxed) + blockSize;
    _maxTotalBytes.store(
        std::max(newTotal, _maxTotalBytes.load(std::memory_order_relaxed)),
        std::memory_order_relaxed);
    
    node->_numAllocations++;
}

inline void
Tf_MallocGlobalData::_UnregisterBlock(const void* block)
{
    // Disable tagging for this thread so any allocations caused
    // here do not get intercepted and cause recursion.
    _TemporaryDisabler disable;
    
    _BlockInfoTable::const_accessor acc;
    if (_blockInfo.find(acc, block)) {
        Tf_MallocBlockInfo bInfo = acc->second;
        _blockInfo.erase(acc);
        acc.release();

        _MaybeReleaseStackOrDebug(bInfo.pathNode, block, bInfo.blockSize);

        bInfo.pathNode->_totalBytes.fetch_sub(
            bInfo.blockSize, std::memory_order_relaxed);
        if (_DECREMENT_ALLOCATION_COUNTS) {
            bInfo.pathNode->_numAllocations.fetch_sub(
                1, std::memory_order_relaxed);
        }
        bInfo.pathNode->_callSite->_totalBytes.fetch_sub(
            bInfo.blockSize, std::memory_order_relaxed);
        _totalBytes.fetch_sub(bInfo.blockSize, std::memory_order_relaxed);
    }
}

void
Tf_MallocGlobalData::_GetStackTrace(
    size_t skipFrames,
    std::vector<uintptr_t>* stack)
{
    uintptr_t buf[_MaxMallocStackDepth];
    
    // Get the stack trace.
    size_t numFrames =
        ArchGetStackFrames(_MaxMallocStackDepth, skipFrames, buf);

    // Copy into stack, reserving exactly enough space.
    stack->assign(buf, buf + numFrames);
}

void
Tf_MallocGlobalData::_SetTraceNames(const std::string& matchList)
{
    _TemporaryDisabler disable;

    _traceMatchTable.SetMatchList(matchList);

    // Update trace flag on every existing call site.
    TF_FOR_ALL(i, _callSiteTable) {
        if (_traceMatchTable.Match(i->second->_name.get())) {
            i->second->_flags |= Tf_MallocCallSite::_TraceFlag;
        }
        else {
            i->second->_flags &= ~Tf_MallocCallSite::_TraceFlag;
        }
    }
}

bool
Tf_MallocGlobalData::_MatchesTraceName(const std::string& name)
{
    return _traceMatchTable.Match(name.c_str());
}

static bool Tf_MatchesMallocTagTraceName(const string& name)
{
    return _mallocGlobalData->_MatchesTraceName(name);
}

void
Tf_MallocGlobalData::_CaptureStackOrDebug(
    const Tf_MallocPathNode* node, const void *ptr, size_t size)
{
    if (node->_callSite->_flags & Tf_MallocCallSite::_TraceFlag) {
        _CallStackTableType::accessor acc;
        _callStackTable.insert(acc, ptr);
        TfMallocTag::CallStackInfo &stackInfo = acc->second;
        _GetStackTrace(_IgnoreStackFramesCount, &stackInfo.stack);
        stackInfo.size = size;
        stackInfo.numAllocations = 1;
    }
    if (node->_callSite->_flags & Tf_MallocCallSite::_DebugFlag) {
        Tf_MallocTagDebugHook(ptr, size);
    }
}

inline void
Tf_MallocGlobalData::_MaybeCaptureStackOrDebug(
    const Tf_MallocPathNode* node, const void *ptr, size_t size)
{
    if (ARCH_UNLIKELY(node->_callSite->_flags)) {
        _CaptureStackOrDebug(node, ptr, size);
    }
}

void
Tf_MallocGlobalData::_ReleaseStackOrDebug(
    const Tf_MallocPathNode* node, const void *ptr, size_t size)
{
    if (node->_callSite->_flags & Tf_MallocCallSite::_TraceFlag) {
        _callStackTable.erase(ptr);
    }
    if (node->_callSite->_flags & Tf_MallocCallSite::_DebugFlag) {
        Tf_MallocTagDebugHook(ptr, size);
    }
}

inline void
Tf_MallocGlobalData::_MaybeReleaseStackOrDebug(
    const Tf_MallocPathNode* node, const void *ptr, size_t size)
{
    if (ARCH_UNLIKELY(node->_callSite->_flags)) {
        _ReleaseStackOrDebug(node, ptr, size);
    }
}

void
Tf_MallocGlobalData::_SetDebugNames(const std::string& matchList)
{
    _TemporaryDisabler disable;

    _debugMatchTable.SetMatchList(matchList);

    // Update debug flag on every existing call site.
    TF_FOR_ALL(i, _callSiteTable) {
        if (_debugMatchTable.Match(i->second->_name.get())) {
            i->second->_flags |= Tf_MallocCallSite::_DebugFlag;
        }
        else {
            i->second->_flags &= ~Tf_MallocCallSite::_DebugFlag;
        }
    }
}

bool
Tf_MallocGlobalData::_MatchesDebugName(const std::string& name)
{
    return _debugMatchTable.Match(name.c_str());
}

static bool Tf_MatchesMallocTagDebugName(const string& name)
{
    return _mallocGlobalData->_MatchesDebugName(name);
}

namespace {
// Hash functor for a malloc stack.
//
struct _HashMallocStack
{
    size_t operator()(const vector<uintptr_t> &stack) const {
        return ArchHash(
            (const char *)&stack[0], sizeof(uintptr_t) * stack.size());
    }
};

// The data associated with a malloc stack (a pointer to the malloc stack
// itself, and the allocation size and number of allocations).
//
struct _MallocStackData
{
    const vector<uintptr_t> *stack;
    size_t size;
    size_t numAllocations;
};
}

// Comparison for sorting TfMallocTag::CallStackInfo based on their
// allocation size.
//
static bool
_MallocStackDataLessThan(
    const _MallocStackData *lhs,
    const _MallocStackData *rhs)
{
    return lhs->size < rhs->size;
}

// Builds a vector of unique captured malloc stacks and stores the result
// in tree->capturedCallStacks.  The malloc stacks are sorted with the
// stacks that allocated the most memory at the front of the vector.
//
void
Tf_MallocGlobalData::_BuildUniqueMallocStacks(TfMallocTag::CallTree* tree)
{
    if (!_callStackTable.empty()) {
        // Create a map from malloc stacks to the malloc stack data.
        typedef TfHashMap<
            vector<uintptr_t>, _MallocStackData, _HashMallocStack> _Map;
        _Map map;

        TF_FOR_ALL(it, _callStackTable) {
            // Since _callStackTable does not change at this point it is
            // ok to store the address of the malloc stack in the data.
            const TfMallocTag::CallStackInfo &stackInfo = it->second;
            _MallocStackData data = { &stackInfo.stack, 0, 0 };

            pair<_Map::iterator, bool> insertResult = map.insert(
                make_pair(stackInfo.stack, data));

            _MallocStackData &updateData = insertResult.first->second;
            updateData.size += stackInfo.size;
            updateData.numAllocations += stackInfo.numAllocations;
        }

        // Sort the malloc stack data by allocation size.
        std::vector<const _MallocStackData *> sortedStackData;
        sortedStackData.reserve(map.size());
        TF_FOR_ALL(it, map) {
            sortedStackData.push_back(&it->second);
        }

        std::sort(
            sortedStackData.begin(),
            sortedStackData.end(),
            _MallocStackDataLessThan);

        tree->capturedCallStacks.reserve(sortedStackData.size());
        TF_REVERSE_FOR_ALL(it, sortedStackData) {
            const _MallocStackData &data = **it;

            tree->capturedCallStacks.push_back(TfMallocTag::CallStackInfo());
            TfMallocTag::CallStackInfo &stackInfo =
                tree->capturedCallStacks.back();

            // Take a copy of the malloc stack.
            stackInfo.stack = *data.stack;
            stackInfo.size = data.size;
            stackInfo.numAllocations = data.numAllocations;
        }
    }
}

Tf_PathNodeChildrenTable
Tf_MallocGlobalData::_BuildPathNodeChildrenTable() const
{
    Tf_PathNodeChildrenTable result;
    // Walk all of _pathNodeTable and populate result.
    for (auto const &item: _pathNodeTable) {
        result[item.first.first].push_back(item.second);
    }
    return result;
}

void
Tf_MallocPathNode::_BuildTree(Tf_PathNodeChildrenTable const &nodeChildren,
                              TfMallocTag::CallTree::PathNode* node,
                              bool skipRepeated) const
{
    std::vector<Tf_MallocPathNode const *> const &children =
        nodeChildren.count(this)
        ? nodeChildren.find(this).value()
        : std::vector<Tf_MallocPathNode const *>();
    node->children.reserve(children.size());
    node->nBytes = node->nBytesDirect = _totalBytes;
    node->nAllocations = _numAllocations;
    node->siteName = _callSite->_name.get();

    for (Tf_MallocPathNode const *child: children) {
        // The tree is built in a special way, if the repeated allocations
        // should be skipped. First, the full tree is built using temporary 
        // nodes for all allocations that should be skipped. Then tree is 
        // collapsed by copying the children of temporary nodes to their parents
        // in bottom-up fasion.
        if (skipRepeated && child->_repeated) {
            // Create a temporary node
            TfMallocTag::CallTree::PathNode childNode;
            child->_BuildTree(nodeChildren, &childNode, skipRepeated);
            // Add the direct contribution of this node to the parent.
            node->nBytesDirect += childNode.nBytesDirect;
            // Copy the children, if there are any
            if (!childNode.children.empty()) {
                node->children.insert(node->children.end(), 
                                      childNode.children.begin(),
                                      childNode.children.end());
            }
            node->nBytes += childNode.nBytes;
        } else {
            node->children.push_back(TfMallocTag::CallTree::PathNode());
            TfMallocTag::CallTree::PathNode& childNode = node->children.back();
            child->_BuildTree(nodeChildren, &childNode, skipRepeated);
            node->nBytes += childNode.nBytes;
        }
    }
}

void
TfMallocTag::SetDebugMatchList(const std::string& matchList)
{
    if (TfMallocTag::IsInitialized()) {
        TfBigRWMutex::ScopedLock lock(_mallocGlobalData->_mutex);
        _mallocGlobalData->_SetDebugNames(matchList);
    }
}

void
TfMallocTag::SetCapturedMallocStacksMatchList(const std::string& matchList)
{
    if (TfMallocTag::IsInitialized()) {
        TfBigRWMutex::ScopedLock lock(_mallocGlobalData->_mutex);
        _mallocGlobalData->_SetTraceNames(matchList);
    }
}

vector<vector<uintptr_t> >
TfMallocTag::GetCapturedMallocStacks()
{
    vector<vector<uintptr_t> > result;
    
    if (!TfMallocTag::IsInitialized())
        return result;

    // Push some malloc tags, so what we do here doesn't pollute the root
    // stacks.
    TfAutoMallocTag tag("Tf", "TfMallocTag::GetCapturedMallocStacks");

    // Copy off the stack traces, make sure to malloc outside.
    Tf_MallocGlobalData::_CallStackTableType traces;
    
    // Swap them out while holding the lock.
    {
        TfBigRWMutex::ScopedLock lock(_mallocGlobalData->_mutex);
        traces.swap(_mallocGlobalData->_callStackTable);
    }

    TF_FOR_ALL(i, traces) {
        result.push_back(i->second.stack);
    }

    return result;
}

void*
TfMallocTag::_MallocWrapper(size_t nBytes, const void*)
{
    void* ptr = _mallocHook.Malloc(nBytes);

    _ThreadData &td = Tls::Find();
    
    if (!td.TaggingEnabled() || ARCH_UNLIKELY(!ptr)) {
        return ptr;
    }

    Tf_MallocPathNode *node = td.GetCurrentPathNode();
    size_t blockSize = Tf_GetMallocBlockSize(ptr, nBytes);
    
    // Take a shared/read lock on the global data mutex.
    TfBigRWMutex::ScopedLock
        lock(_mallocGlobalData->_mutex, /*write=*/false);
    // Update malloc global data with bookkeeping information.
    _mallocGlobalData->_RegisterBlock(ptr, blockSize, node);
    return ptr;
}

void*
TfMallocTag::_ReallocWrapper(void* oldPtr, size_t nBytes, const void*)
{
    /*
     * If ptr is NULL, we want to make sure we don't double count,
     * because a call to _mallocHook.Realloc(ptr, nBytes) could call
     * through to our malloc.  To avoid this, we'll explicitly short-circuit
     * ourselves rather than trust that the malloc library will do it.
     */
    if (!oldPtr) {
        return _MallocWrapper(nBytes, nullptr);
    }

    _ThreadData &td = Tls::Find();

    // If tagging is explicitly disabled, just do the realloc and skip 
    // everything else. This avoids a deadlock if we get here while updating
    // Tf_MallocGlobalData::_pathNodeTable.
    if (!td.TaggingEnabled()) {
        return _mallocHook.Realloc(oldPtr, nBytes);
    }

    // Take a shared/read lock on the global data mutex.
    TfBigRWMutex::ScopedLock lock(_mallocGlobalData->_mutex, /*write=*/false);
    
    _mallocGlobalData->_UnregisterBlock(oldPtr);

    void *newPtr = _mallocHook.Realloc(oldPtr, nBytes);
        
    if (ARCH_UNLIKELY(!newPtr)) {
        return newPtr;
    }

    Tf_MallocPathNode* newNode = td.GetCurrentPathNode();
    size_t blockSize = Tf_GetMallocBlockSize(newPtr, nBytes);
    
    // Update malloc global data with bookkeeping information.
    _mallocGlobalData->_RegisterBlock(newPtr, blockSize, newNode);
    return newPtr;
}

void*
TfMallocTag::_MemalignWrapper(size_t alignment, size_t nBytes, const void*)
{
    void* ptr = _mallocHook.Memalign(alignment, nBytes);

    _ThreadData &td = Tls::Find();
    if (!td.TaggingEnabled() || ARCH_UNLIKELY(!ptr)) {
        return ptr;
    }
    
    Tf_MallocPathNode *node = td.GetCurrentPathNode();
    size_t blockSize = Tf_GetMallocBlockSize(ptr, nBytes);

    // Update malloc global data with bookkeeping information.
    TfBigRWMutex::ScopedLock lock(_mallocGlobalData->_mutex, /*write=*/false);
    _mallocGlobalData->_RegisterBlock(ptr, blockSize, node);

    return ptr;
}

void
TfMallocTag::_FreeWrapper(void* ptr, const void*)
{
    if (!ptr) {
        return;
    }

    _ThreadData &td = Tls::Find();

    // If tagging is explicitly disabled, just do the free and skip everything
    // else.
    if (!td.TaggingEnabled()) {
        _mallocHook.Free(ptr);
        return;
    }

    TfBigRWMutex::ScopedLock lock(_mallocGlobalData->_mutex, /*write=*/false);
    _mallocGlobalData->_UnregisterBlock(ptr);
    lock.Release();

    _mallocHook.Free(ptr);
}

bool
TfMallocTag::Initialize(string* errMsg)
{
    static bool status = _Initialize(errMsg);
    return status;
}

static void
_GetCallSites(TfMallocTag::CallTree::PathNode* node,
              Tf_MallocCallSiteTable* table) {
    TF_AXIOM(node);
    TF_AXIOM(table);

    Tf_MallocCallSite* site =
        Tf_GetOrCreateCallSite(table, node->siteName.c_str());
    site->_totalBytes += node->nBytesDirect;

    TF_FOR_ALL(pi, node->children) {
        _GetCallSites(&(*pi), table);
    }
}

bool
TfMallocTag::GetCallTree(CallTree* tree, bool skipRepeated)
{
    tree->callSites.clear();
    tree->root.nBytes = tree->root.nBytesDirect = 0;
    tree->root.nAllocations = 0;
    tree->root.siteName.clear();
    tree->root.children.clear();

    if (Tf_MallocGlobalData* gd = _mallocGlobalData) {

        _TemporaryDisabler disable;

        TfBigRWMutex::ScopedLock lock(gd->_mutex);

        // Build the snapshot call tree
        gd->_rootNode->_BuildTree(
            gd->_BuildPathNodeChildrenTable(), &tree->root, skipRepeated);
        
        // Build the snapshot callsites map based on the tree
        Tf_MallocCallSiteTable callSiteTable;
        _GetCallSites(&tree->root, &callSiteTable);

        // Copy the callsites into the calltree
        tree->callSites.reserve(callSiteTable.size());
        TF_FOR_ALL(csi, callSiteTable) {
            CallTree::CallSite cs = {
                csi->second->_name.get(),
                static_cast<size_t>(csi->second->_totalBytes)
            };
            tree->callSites.push_back(cs);
            delete csi->second;
        }

        gd->_BuildUniqueMallocStacks(tree);

        return true;
    }
    else
        return false;
}

size_t
TfMallocTag::GetTotalBytes()
{
    if (!_mallocGlobalData) {
        return 0;
    }

    return _mallocGlobalData->_totalBytes;
}

size_t
TfMallocTag::GetMaxTotalBytes()
{
    if (!_mallocGlobalData) {
        return 0;
    }

    return _mallocGlobalData->_maxTotalBytes;
}

bool
TfMallocTag::_Initialize(std::string* errMsg)
{
    /*
     * This is called from an EXECUTE_ONCE block, so no
     * need to lock anything.
     */
    TF_AXIOM(!_mallocGlobalData);

    _mallocGlobalData = new Tf_MallocGlobalData();
    Tf_MallocCallSite* site = _mallocGlobalData->_GetOrCreateCallSite("__root");
    Tf_MallocPathNode* rootNode = new Tf_MallocPathNode(site);
    _mallocGlobalData->_rootNode = rootNode;

    TfMallocTag::_isInitialized = true;

    _TemporaryDisabler disable;
    
    return _mallocHook.Initialize(_MallocWrapper, 
                                  _ReallocWrapper,
                                  _MemalignWrapper, 
                                  _FreeWrapper, 
                                  errMsg);
}

TfMallocTag::_ThreadData *
TfMallocTag::_Begin(const char* name, _ThreadData *threadData)
{
    if (!name || !name[0]) {
        return nullptr;
    }

    _ThreadData &tls = threadData ? *threadData : TfMallocTag::Tls::Find();

    _TemporaryDisabler disable(&tls);
    
    TfBigRWMutex::ScopedLock
        lock(_mallocGlobalData->_mutex, /*write=*/false);
    
    Tf_MallocCallSite *site = _mallocGlobalData->_GetOrCreateCallSite(name);
    Tf_MallocPathNode *thisNode = _mallocGlobalData->
        _GetOrCreateChild({ tls.GetCurrentPathNode(), site });

    lock.Release();

    tls.Push(site, thisNode);

    return &tls;
}

void
TfMallocTag::_End(int nTags, TfMallocTag::_ThreadData *tls)
{
    if (!tls) {
        tls = &TfMallocTag::Tls::Find();
    }

    while (nTags--) {
        tls->Pop();
    }
}

// Returns the given number as a string with commas used as thousands
// separators.
//
static string
_GetAsCommaSeparatedString(size_t number)
{
    string result;

    string str = TfStringPrintf("%ld", number);
    size_t n = str.size();

    TF_FOR_ALL(it, str) {
        if (n < str.size() && n%3 == 0) {
            result.push_back(',');
        }
        result.push_back(*it);
        n--;
    }
    return result;
}

static void
_PrintHeader(string *rpt)
{
    *rpt += "\n" + string(80, '-') + "\n";
    *rpt += TfStringPrintf("\nMalloc Tag Report\n\n\n");
    *rpt += TfStringPrintf("Total bytes = %s\n\n\n",
        _GetAsCommaSeparatedString(TfMallocTag::GetTotalBytes()).c_str());
}

static size_t
_PrintMallocNode(
    string* rpt,
    const TfMallocTag::CallTree::PathNode &node,
    size_t rootTotal,
    size_t parentTotal,
    size_t level,
    size_t &printedNodes,
    size_t maxPrintedNodes)
{
    if (!level) {
        // XXX:cleanup  We should pass in maxNameWidth and generate format
        //              strings like in _PrintMallocCallSites().
        *rpt += TfStringPrintf("%-72s %15s%15s %5s %5s %5s\n", "TAGNAME",
                               "BytesIncl", "BytesExcl", "%Prnt", "% Exc",
                               "%Totl");
        *rpt += TfStringPrintf("%-72s %12s%12s %5s %5s %5s\n\n",
                               string(72, '-').c_str(),
                               " --------------", " --------------",
                               "-----", "-----", "-----");

        rootTotal = node.nBytes;
    }

    size_t maxNameWidth = 72;
    size_t indent = level;

    if (printedNodes >= maxPrintedNodes) {
        return 0;
    }
    printedNodes++;

    string name = string(indent, ' ') +
        node.siteName.substr(0, maxNameWidth-indent);
    int postLen = static_cast<int>(maxNameWidth - name.length());
    if (postLen > 0) {
        name += string(postLen, ' ');
    }

    *rpt += TfStringPrintf(
        "%s %15s%15s ",
        name.c_str(),
        _GetAsCommaSeparatedString(node.nBytes).c_str(),
        _GetAsCommaSeparatedString(node.nBytesDirect).c_str());

    string curPercent;
    string curPercentDirect;
    string percentDirectOfRoot;

    if (parentTotal) {

        float percent = node.nBytes/(float)parentTotal*100;
        if (percent > 0.5) {
            curPercent = TfStringPrintf(" %.0f%%", percent);
        }
        percent = node.nBytesDirect/(float)node.nBytes*100;
        if (percent > 0.5) {
            curPercentDirect = TfStringPrintf(" %.0f%%", percent);
        }

        percent = node.nBytesDirect/(float)rootTotal*100;
        if (percent > 0.5) {
            percentDirectOfRoot = TfStringPrintf(" %.0f%%", percent);
        }
    }

    if (!level) {
        // For Root, take the bytesDirect as the rootPercentage

        float percent = 100*node.nBytesDirect/(float)rootTotal;
        if (percent > 0.5) {
            percentDirectOfRoot = TfStringPrintf(" %.0f%%", percent);
        }
    }
    *rpt += TfStringPrintf("%5s %5s %5s\n", curPercent.c_str(),
                           curPercentDirect.c_str(),
                           percentDirectOfRoot.c_str());

    vector<TfMallocTag::CallTree::PathNode>::const_iterator it;
    for (it = node.children.begin(); it != node.children.end(); ++it) {
        _PrintMallocNode(rpt, *it, rootTotal, node.nBytes, level+1,
                         printedNodes,
                         maxPrintedNodes);
    }

    return rootTotal;
}

static void
_PrintMallocCallSites(
   string* rpt,
   const vector<TfMallocTag::CallTree::CallSite>& callSites,
   size_t rootTotal)
{
    *rpt += TfStringPrintf("\n\nCall Sites\n\n");

    // Use a map to sort by allocation size.
    map<size_t, const string *> map;
    TF_FOR_ALL(csi, callSites) {
        map.insert(make_pair(csi->nBytes, &csi->name));
    }

    // XXX:cleanup We should pass in maxNameWidth.
    const size_t maxNameWidth = 72;
    const size_t maxBytesWidth = 15;
    const size_t maxPercentageWidth = 15;

    string fmt = TfStringPrintf(
        "%%-%lds %%%lds %%%lds\n",
        maxNameWidth, maxBytesWidth, maxPercentageWidth);

    *rpt += TfStringPrintf(fmt.c_str(), "NAME", "BYTES", "%ROOT");
    *rpt += string(maxNameWidth, '-') + ' ' + 
            string(maxBytesWidth, '-') + ' ' + 
            string(maxPercentageWidth, '-') + "\n\n";

    TF_REVERSE_FOR_ALL(it, map) {
        size_t nBytes = it->first;
        const string &name = *it->second;

        string curPercent;
        if (rootTotal) {
            double percent = 100.0*nBytes/rootTotal;
            // Don't print anything less than 0.1%.
            if (percent < 0.1) {
                break;
            }
            curPercent = TfStringPrintf("%.1f%%", percent);
        }

        *rpt += TfStringPrintf(
            fmt.c_str(),
            name.substr(0, maxNameWidth).c_str(),
            _GetAsCommaSeparatedString(nBytes).c_str(),
            curPercent.c_str());
    }
}

// Comparison for sorting CallTree::PathNodes.
//
static bool
_MallocPathNodeLessThan(
    const TfMallocTag::CallTree::PathNode *lhs,
    const TfMallocTag::CallTree::PathNode *rhs)
{
    return lhs->siteName < rhs->siteName;
}

#if !(_DECREMENT_ALLOCATION_COUNTS)
// Returns the total number of allocations in the given sub-tree.
//
static int64_t
_GetNumAllocationInSubTree(
    const TfMallocTag::CallTree::PathNode &node)
{
    int64_t nAllocations = node.nAllocations;
    TF_FOR_ALL(it, node.children) {
        nAllocations += _GetNumAllocationInSubTree(*it);
    }
    return nAllocations;
}
#endif

static void
_ReportMallocNode(
    std::ostream &out,
    const TfMallocTag::CallTree::PathNode &node,
    size_t level,
    const std::string *rootName = nullptr)
{
    // Prune empty branches.
    if (node.nBytes == 0) {
        #if _DECREMENT_ALLOCATION_COUNTS
            return;
        #else
            if (_GetNumAllocationInSubTree(node) == 0) {
                return;
            }
        #endif
    }

    string indent(2*level, ' ');

    // Insert '|' characters every 4 spaces.
    for (size_t i=0; i<(level + 1)/2; i++) {
        indent[4*i] = '|';
    }

    out << TfStringPrintf(
        "%13s B %13s B %7ld samples    ",
        _GetAsCommaSeparatedString(node.nBytes).c_str(),
        _GetAsCommaSeparatedString(node.nBytesDirect).c_str(),
        node.nAllocations);

    out << indent
        << (rootName && !rootName->empty() ? *rootName : node.siteName)
        << std::endl;

    // Sort the children by name.  The reason for doing this is that it is
    // the easiest way to provide stable results for diffing.  You could
    // argue that letting the diff program do the sorting is more correct
    // (i.e. that sorting is a view into the unaltered source data).
    std::vector<const TfMallocTag::CallTree::PathNode *> sortedChildren;
    sortedChildren.reserve(node.children.size());
    TF_FOR_ALL(it, node.children) {
        sortedChildren.push_back(&(*it));
    }

    std::sort(
        sortedChildren.begin(), sortedChildren.end(), _MallocPathNodeLessThan);

    TF_FOR_ALL(it, sortedChildren) {
        _ReportMallocNode(out, **it, level+1);
    }
}

static void
_ReportCapturedMallocStacks(
    std::ostream &out,
    const std::vector<TfMallocTag::CallStackInfo> &stackInfos)
{
    size_t numReportedStacks =
        TfMin(stackInfos.size(), _MaxReportedMallocStacks);

    size_t totalSize = 0;
    size_t totalNumAllocations = 0;
    size_t reportSize = 0;
    size_t reportNumAllocations = 0;

    for(size_t n=0; n<stackInfos.size(); n++) {
        const TfMallocTag::CallStackInfo &stackInfo = stackInfos[n];
        totalSize += stackInfo.size;
        totalNumAllocations += stackInfo.numAllocations;
        if (n < numReportedStacks) {
            reportSize += stackInfo.size;
            reportNumAllocations += stackInfo.numAllocations;
        }
    }

    out << "\n\n\n"
        << "Captured Malloc Stacks\n"
        << "\n"
        << "Number of unique captured malloc stacks:          "
            << _GetAsCommaSeparatedString(stackInfos.size()) << "\n"
        << "Total allocated memory by captured mallocs:       "
            << _GetAsCommaSeparatedString(totalSize) << "\n"
        << "Total number of allocations by captured mallocs:  "
            << _GetAsCommaSeparatedString(totalNumAllocations) << "\n"
        << "\n"
        << "Number of captured malloc stacks in report:       "
            << _GetAsCommaSeparatedString(numReportedStacks) << "\n"
        << "Allocated memory by mallocs in report:            "
            << _GetAsCommaSeparatedString(reportSize) << "\n"
        << "Number of allocations by mallocs in report:       "
            << _GetAsCommaSeparatedString(reportNumAllocations) << "\n"
        << "Percentage of allocated memory covered by report: "
            << TfStringPrintf("%.1f%%", 100.0*reportSize/totalSize) << "\n\n";

    for(size_t n=0; n<numReportedStacks; n++) {
        const TfMallocTag::CallStackInfo &stackInfo = stackInfos[n];

        out << string(100, '-') << "\n"
            << "Captured malloc stack #" << n << "\n"
            << "Size:            " <<
                _GetAsCommaSeparatedString(stackInfo.size) << "\n"
            << "Num allocations: " <<
                _GetAsCommaSeparatedString(stackInfo.numAllocations) << "\n";

        ArchPrintStackFrames(out, stackInfo.stack);
    }
}


string
TfMallocTag::CallTree::GetPrettyPrintString(PrintSetting setting,
                                            size_t maxPrintedNodes) const
{
    string rpt;

    _PrintHeader(&rpt);

    if (setting == TREE || setting == BOTH) {
        size_t printedNodes = 0;
        size_t reportedMem = 
            _PrintMallocNode(&rpt, this->root, 0, 0, 0, printedNodes,
                             maxPrintedNodes);
        if (printedNodes >= maxPrintedNodes
            && reportedMem != GetTotalBytes()) {
            rpt += TfStringPrintf("\nWARNING: limit of %zu nodes visted, but "
                                  "only %zu bytes of %zu accounted for.  "
                                  "Running with a larger maxPrintedNodes will "
                                  "produce more accurate results.\n",
                                  maxPrintedNodes, 
                                  reportedMem, 
                                  GetTotalBytes());

        }
    }

    if (setting == CALLSITES || setting == BOTH) {
        _PrintMallocCallSites(&rpt, this->callSites, this->root.nBytes);
    }

    return rpt;
}

void
TfMallocTag::CallTree::Report(
    std::ostream &out) const
{
    const std::string emptyRootName;
    Report(out, emptyRootName);
}

static const std::string &
_GetTreeHeader()
{
    static const std::string treeHeader("Tree view  ==============");
    return treeHeader;
}

void
TfMallocTag::CallTree::Report(
    std::ostream &out,
    const std::string &rootName) const
{
    out << "\n" << _GetTreeHeader() << "\n";
    out << "      inclusive       exclusive\n";

    _ReportMallocNode(out, this->root, 0, &rootName);

    // Also add the dominant call sites to the report.
    out << GetPrettyPrintString(CALLSITES);

    // And the captured malloc stacks if there are any.
    if (!this->capturedCallStacks.empty()) {
        _ReportCapturedMallocStacks(out, this->capturedCallStacks);
    }
}

bool
TfMallocTag::CallTree::LoadReport(
    std::istream &in)
{
    // The matches here are:
    // 1. Exclusive memory (in bytes)
    // 2. Inclusive memory (in bytes)
    // 3. The number of allocations recorded
    // 4. The indentation string, which we match exactly (excluding leading
    //    whitespace) so that we can use the length to compute the depth, below.
    // 5. The name of the callsite.
    static const std::regex re(
        R"( *([\d].*) B *([\d].*) B *([\d].*) samples    ([ |]*)(.*))");

    // State of the parser. 
    enum class State {
        // Tree header not yet found
        FindingTree,
        // Found root, reading scopes
        ReadingTree
    } state = State::FindingTree;

    // Initialize the stack with a synthetic root node, so that we can support
    // loading multiple report trees. (This root node will be elided later if we
    // end up loading a single tree.)
    root = {0, 0, 0, "root", {}};
    std::stack<PathNode *> nodes;
    nodes.push(&root);

    // Parse the file contents
    for (std::string line; std::getline(in, line);) {
        // When finding the tree, only parse for the tree header.
        if (state == State::FindingTree) {
            if (line == _GetTreeHeader()) {
                state = State::ReadingTree;
            }
            continue;
        }
        if (!TF_VERIFY(state == State::ReadingTree)) {
            break;
        }

        // When we see an empty line, that means we've gotten a full tree. Clear
        // the stack and switch back to tree finding.
        if (TfStringTrim(line).empty()) {
            state = State::FindingTree;
            nodes = {};
            nodes.push(&root);
            continue;
        }

        std::cmatch match;
        if (!std::regex_match(line.c_str(), match, re)) {
            continue;
        }

        const size_t nBytes =
            TfStringToULong(TfStringReplace(match[1].str(), ",", ""));
        const size_t nBytesDirect =
            TfStringToULong(TfStringReplace(match[2].str(), ",", ""));
        const size_t nAllocations = TfStringToULong(match[3].str());
        const size_t depth = match[4].length() / 2;
        const std::string& siteName = match[5].str();

        // Pop nodes off the stack until the top is the parent of the node we
        // just parsed. (We add one here to account for the synthetic root
        // node.)
        while (nodes.size() > depth + 1) {
            nodes.pop();
        }

        PathNode *const parent = nodes.top();

        // Add the current node as a child.
        parent->children.push_back(
            {nBytes, nBytesDirect, nAllocations, siteName, {}});

        // Push the child onto the stack.
        PathNode *const child = &(parent->children.back());
        nodes.push(child);
    }

    // If we only ended up with one tree, elide the root node, since we only
    // created it to support reading multiple trees. Otherwise, sum up the child
    // allocations, mainly so we have a non-zero value at the root, so that we
    // don't produce an empty report by pruning at the root.
    if (root.children.size() == 1) {
        // Here, we copy the root's child before overwriting the root. While we
        // squeaked by here on linux, on windows we didn't get so (un)lucky.
        const PathNode newRoot = root.children[0];
        root = newRoot;
    } else {
        for (const PathNode& child : root.children) {
            root.nBytes += child.nBytes;
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
