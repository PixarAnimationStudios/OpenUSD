//
// Copyright 2016 Pixar
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
#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/mallocTag.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/tf.h"

#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/hash.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/arch/mallocHook.h"
#include "pxr/base/arch/stackTrace.h"

#include <tbb/spin_mutex.h>

#include <algorithm>
#include <string>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <ostream>

using std::map;
using std::make_pair;
using std::pair;
using std::string;
using std::vector;

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
// #1   Tf_MallocGlobalData::_CaptureMallocStack(Tf_MallocPathNode const*, void const*, unsigned long)
// #2   TfMallocTag::_MallocWrapper(unsigned long, void const*)
static const size_t _IgnoreStackFramesCount = 3;

struct Tf_MallocPathNode;
struct Tf_MallocGlobalData;

static ArchMallocHook _mallocHook;      // zero-initialized POD
static Tf_MallocGlobalData* _mallocGlobalData = NULL;
bool TfMallocTag::_doTagging = false;

static bool
_UsePtmalloc()
{
    string impl = TfGetenv("TF_MALLOC_TAG_IMPL", "auto");
    vector<string> legalImpl = {"auto",     "agnostic",
                                "jemalloc", "jemalloc force",
                                "ptmalloc", "ptmalloc force",
                                "pxmalloc", "pxmalloc force"};

    if (std::find(legalImpl.begin(), legalImpl.end(), impl) == legalImpl.end()) {
        string values = TfStringJoin(legalImpl, "', '");
        TF_WARN("Invalid value '%s' for TF_MALLOC_TAG_IMPL: "
                "(not one of '%s')", impl.c_str(), values.c_str());
    }

    if (impl != "auto") {
        fprintf(stderr, "########################################################################\n"
                        "#  TF_MALLOC_TAG_IMPL is overridden to '%s'.  Default is 'auto'  #\n"
                        "########################################################################\n",
                impl.c_str());
    }

    if (impl == "agnostic")
        return false;

    if (ArchIsPtmallocActive()) {
        return true;
    }
    else if (TfStringStartsWith(impl, "ptmalloc")) {
        TF_WARN("TfMallocTag can only use ptmalloc-specific implementation "
                "when ptmalloc is active. Falling back to agnostic "
                "implementation.");
    }

    return false;
}

/*
 * We let malloc have BITS_FOR_MALLOC_SIZE instead of the usual 64.
 * That leaves us 64 - BITS_FOR_MALLOC_SIZE for storing our own index,
 * which effectively gives us a pointer to a Tf_MallocPathNode (but
 * only for MAX_PATH_NODES different nodes).
 */
static const unsigned BITS_FOR_MALLOC_SIZE = 40;
static const unsigned BITS_FOR_INDEX = 64 - BITS_FOR_MALLOC_SIZE;
static const size_t MAX_PATH_NODES = 1 << BITS_FOR_INDEX;
static const unsigned HIWORD_INDEX_BIT_OFFSET = BITS_FOR_MALLOC_SIZE - 32;
static const unsigned HIWORD_INDEX_MASK = ~(~0U << HIWORD_INDEX_BIT_OFFSET);  // (HIWORD_INDEX_BIT_OFFSET no. of 1 bits.)
static const unsigned long long MALLOC_SIZE_MASK = ~(~0ULL << BITS_FOR_MALLOC_SIZE) & ~0x7ULL;

static bool Tf_MatchesMallocTagDebugName(const string& name);
static bool Tf_MatchesMallocTagTraceName(const string& name);
static void Tf_MallocTagDebugHook(void* ptr, size_t size) ARCH_NOINLINE;

static void Tf_MallocTagDebugHook(void* ptr, size_t size)
{
    // Clients don't call this directly so the debugger can conveniently
    // see the pointer and size in the stack trace.
    ARCH_DEBUGGER_TRAP;
}

static size_t Tf_GetMallocBlockSize(void* ptr, size_t requestedSize)
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
    Tf_MallocBlockInfo()
        : blockSize(0), pathNodeIndex(0)
    { }

    Tf_MallocBlockInfo(size_t size, uint32_t index)
        : blockSize(size), pathNodeIndex(index)
    { }

    size_t       blockSize:BITS_FOR_MALLOC_SIZE;
    uint32_t pathNodeIndex:BITS_FOR_INDEX;
};

#if defined(ARCH_COMPILER_HAS_STATIC_ASSERT)

static_assert(sizeof(Tf_MallocBlockInfo) == 8, 
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
        bool allow:1;       // New result if str matches.
        bool wildcard:1;    // str has a suffix wildcard.
    };
    std::vector<_MatchString> _matchStrings;
};

Tf_MallocTagStringMatchTable::_MatchString::_MatchString(const std::string& s) :
    str(s),
    allow(true),
    wildcard(false)
{
    if (not str.empty()) {
        if (str[str.size() - 1] == '*') {
            wildcard = true;
            str.resize(str.size() - 1);
        }
        if (not str.empty()) {
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
            while (*m and *m == *s) {
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
    Tf_MallocCallSite(const string& name, uint32_t index)
        : _name(name), _totalBytes(0), _nPaths(0), _index(index)
    {
        _debug = Tf_MatchesMallocTagDebugName(_name);
        _trace = Tf_MatchesMallocTagTraceName(_name);
    }

    // Note: _name needs to be const since we call c_str() on it.
    const string _name;
    int64_t _totalBytes;
    size_t _nPaths;    
    uint32_t _index;

    // If true then invoke the debugger trap when allocating or freeing
    // at this site.
    bool _debug:1;

    // If true then capture a stack trace when allocating at this site.
    bool _trace:1;
};

namespace {

typedef TfHashMap<const char*, struct Tf_MallocCallSite*,
                             TfHashCString,
                             TfEqualCString> Tf_MallocCallSiteTable;

Tf_MallocCallSite* Tf_GetOrCreateCallSite(Tf_MallocCallSiteTable* table,
                                          const char* name,
                                          size_t* traceSiteCount) {
    TF_AXIOM(table);
    Tf_MallocCallSiteTable::iterator it = table->find(name);

    if (it == table->end()) {
        Tf_MallocCallSite* site =
            new Tf_MallocCallSite(name, static_cast<uint32_t>(table->size()));
        // site->_name is const so it is ok to use c_str() as the key.
        (*table)[site->_name.c_str()] = site;
        if (site->_trace) {
            ++*traceSiteCount;
        }
        return site;
    } else {
        return it->second;
    }
}
}

/*
 * This is a singleton.  Because access to this structure is gated via checks
 * to TfMallocTag::_doTagging, we forego the usual TfSingleton pattern and just
 * use a single static-scoped pointer (::_mallocGlobalData) to point to the
 * singleton instance.
 */
struct Tf_MallocGlobalData
{
    Tf_MallocGlobalData() {
        _allPathNodes.reserve(1024);
        _totalBytes = 0;
        _maxTotalBytes = 0;
        _warned = false;
        _captureCallSiteCount = 0;
        _captureStack.reserve(_MaxMallocStackDepth);
    }

    Tf_MallocCallSite* _GetOrCreateCallSite(const char* name) {
        return Tf_GetOrCreateCallSite(&_callSiteTable, name,
                                      &_captureCallSiteCount);
    }

    inline bool _RegisterPathNode(Tf_MallocPathNode*);
    inline bool _RegisterPathNodeForBlock(
        Tf_MallocPathNode* pathNode, void* block, size_t blockSize);
    inline bool _UnregisterPathNodeForBlock(
        void* block, Tf_MallocBlockInfo* blockInfo);

    bool _IsMallocStackCapturingEnabled() const {
        return _captureCallSiteCount != 0;
    }

    void _RunDebugHookForNode(const Tf_MallocPathNode* node, void*, size_t);

    void _GetStackTrace(size_t skipFrames, std::vector<uintptr_t>* stack);

    void _SetTraceNames(const std::string& matchList);
    bool _MatchesTraceName(const std::string& name);
    void _CaptureMallocStack(
        const Tf_MallocPathNode* node, const void *ptr, size_t size);
    void _ReleaseMallocStack(
        const Tf_MallocPathNode* node, const void *ptr);

    void _BuildUniqueMallocStacks(TfMallocTag::CallTree* tree);

    void _SetDebugNames(const std::string& matchList);
    bool _MatchesDebugName(const std::string& name);

    typedef TfHashMap<const void *, TfMallocTag::CallStackInfo, TfHash>
    _CallStackTableType;

    tbb::spin_mutex _mutex;
    Tf_MallocPathNode* _rootNode;
    Tf_MallocCallSiteTable _callSiteTable;

    // Vector of path nodes indicating location of an allocated block. 
    // Implementations associate indices into this vector with a block.
    vector<struct Tf_MallocPathNode*> _allPathNodes;

    // Mapping from memory block to information about that block.
    // Used by allocator-agnostic implementation.
    typedef TfHashMap<const void *, Tf_MallocBlockInfo, TfHash>
    _PathNodeTableType;
    _PathNodeTableType _pathNodeTable;

    size_t _captureCallSiteCount;
    _CallStackTableType _callStackTable;
    Tf_MallocTagStringMatchTable _traceMatchTable;

    int64_t _totalBytes;
    int64_t _maxTotalBytes;
    bool _warned;

    Tf_MallocTagStringMatchTable _debugMatchTable;

    // Pre-allocated space for getting stack traces.
    vector<uintptr_t> _captureStack;
};

/*
 * Each node describes a sequence (i.e. path) of call sites.
 * However, a given call-site can occur only once in a given path -- recursive
 * call loops are excised.
 */
struct Tf_MallocPathNode
{
    Tf_MallocPathNode(Tf_MallocCallSite* callSite)
        : _callSite(callSite),
          _totalBytes(0),
          _numAllocations(0),
          _index(0),
          _repeated(false)
    {
    }

    Tf_MallocPathNode* _GetOrCreateChild(Tf_MallocCallSite* site)
    {
        // Note: As long as the number of children is quite small, using a
        // vector is a good option here.  If this assumption changes we
        // should change this back to using a map (or TfHashMap).
        TF_FOR_ALL(it, _children) {
            if (it->first == site) {
                return it->second;
            }
        }
        Tf_MallocPathNode* pathNode = new Tf_MallocPathNode(site);
        if (!::_mallocGlobalData->_RegisterPathNode(pathNode)) {
            delete pathNode;
            return NULL;
        }

        _children.push_back(make_pair(site, pathNode));
        site->_nPaths++;
        return pathNode;
    }

    void _BuildTree(TfMallocTag::CallTree::PathNode* node,
                    bool skipRepeated);

    Tf_MallocCallSite* _callSite;
    int64_t _totalBytes;
    int64_t _numAllocations;
    vector<pair<Tf_MallocCallSite*, Tf_MallocPathNode*> > _children;
    uint32_t _index;    // only 24 bits
    bool _repeated;    // repeated node
};

inline bool
Tf_MallocGlobalData::_RegisterPathNode(Tf_MallocPathNode* pathNode)
{
    if (_allPathNodes.size() == MAX_PATH_NODES) {
        if (!_warned) {
            TF_WARN("maximum no. of TfMallocTag nodes has been reached!");
            _warned = true;
        }
        return false;

    }
    pathNode->_index = static_cast<uint32_t>(_allPathNodes.size());
    _allPathNodes.push_back(pathNode);
    return true;
}

inline bool
Tf_MallocGlobalData::_RegisterPathNodeForBlock(
    Tf_MallocPathNode* pathNode, void* block, size_t blockSize)
{
    // Disable tagging for this thread so any allocations caused
    // here do not get intercepted and cause recursion.
    TfMallocTag::_TemporaryTaggingState tmpState(TfMallocTag::_TaggingDisabled);

    const Tf_MallocBlockInfo blockInfo(blockSize, pathNode->_index);
    return _pathNodeTable.insert(std::make_pair(block, blockInfo)).second;
}

inline bool
Tf_MallocGlobalData::_UnregisterPathNodeForBlock(
    void* block, Tf_MallocBlockInfo* blockInfo)
{
    // Disable tagging for this thread so any allocations caused
    // here do not get intercepted and cause recursion.
    TfMallocTag::_TemporaryTaggingState tmpState(TfMallocTag::_TaggingDisabled);

    _PathNodeTableType::iterator it = _pathNodeTable.find(block);
    if (it != _pathNodeTable.end()) {
        *blockInfo = it->second;
        _pathNodeTable.erase(it);
        return true;
    }

    return false;
}

void
Tf_MallocGlobalData::_GetStackTrace(
    size_t skipFrames,
    std::vector<uintptr_t>* stack)
{
    // Get the stack trace.
    ArchGetStackFrames(_MaxMallocStackDepth, skipFrames, &_captureStack);

    // Copy into stack, reserving exactly enough space.
    stack->reserve(_captureStack.size());
    stack->insert(stack->end(), _captureStack.begin(), _captureStack.end());

    // Done with stack trace.
    _captureStack.clear();
}

void
Tf_MallocGlobalData::_SetTraceNames(const std::string& matchList)
{
    TfMallocTag::_TemporaryTaggingState tmpState(TfMallocTag::_TaggingDisabled);

    _traceMatchTable.SetMatchList(matchList);

    // Update trace flag on every existing call site.
    _captureCallSiteCount = 0;
    TF_FOR_ALL(i, _callSiteTable) {
        i->second->_trace = _traceMatchTable.Match(i->second->_name.c_str());
        if (i->second->_trace) {
            ++_captureCallSiteCount;
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
    return ::_mallocGlobalData->_MatchesTraceName(name);
}

void
Tf_MallocGlobalData::_CaptureMallocStack(
    const Tf_MallocPathNode* node, const void *ptr, size_t size)
{
    if (node->_callSite->_trace) {
        // Disable tagging for this thread so any allocations caused
        // here do not get intercepted and cause recursion.
        TfMallocTag::_TemporaryTaggingState 
            tmpState(TfMallocTag::_TaggingDisabled);

        TfMallocTag::CallStackInfo &stackInfo = _callStackTable[ptr];
        _GetStackTrace(_IgnoreStackFramesCount, &stackInfo.stack);
        stackInfo.size = size;
        stackInfo.numAllocations = 1;
    }
}

void
Tf_MallocGlobalData::_ReleaseMallocStack(
    const Tf_MallocPathNode* node, const void *ptr)
{
    if (node->_callSite->_trace) {
        _CallStackTableType::iterator i = _callStackTable.find(ptr);
        if (i != _callStackTable.end()) {
            // Disable tagging for this thread so any allocations caused
            // here do not get intercepted and cause recursion.
            TfMallocTag::_TemporaryTaggingState 
                tmpState(TfMallocTag::_TaggingDisabled);
            _callStackTable.erase(i);
        }
    }
}

void
Tf_MallocGlobalData::_RunDebugHookForNode(
    const Tf_MallocPathNode* node, void* ptr, size_t size)
{
    if (node->_callSite->_debug)
        Tf_MallocTagDebugHook(ptr, size);
}

void
Tf_MallocGlobalData::_SetDebugNames(const std::string& matchList)
{
    TfMallocTag::_TemporaryTaggingState tmpState(TfMallocTag::_TaggingDisabled);

    _debugMatchTable.SetMatchList(matchList);

    // Update debug flag on every existing call site.
    TF_FOR_ALL(i, _callSiteTable) {
        i->second->_debug = _debugMatchTable.Match(i->second->_name.c_str());
    }
}

bool
Tf_MallocGlobalData::_MatchesDebugName(const std::string& name)
{
    return _debugMatchTable.Match(name.c_str());
}

static bool Tf_MatchesMallocTagDebugName(const string& name)
{
    return ::_mallocGlobalData->_MatchesDebugName(name);
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
    if (not _callStackTable.empty()) {
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


void
Tf_MallocPathNode::_BuildTree(TfMallocTag::CallTree::PathNode* node,
                              bool skipRepeated)
{
    node->children.reserve(_children.size());
    node->nBytes = node->nBytesDirect = _totalBytes;
    node->nAllocations = _numAllocations;
    node->siteName = _callSite->_name;

    TF_FOR_ALL(pi, _children) {
        // The tree is built in a special way, if the repeated allocations
        // should be skipped. First, the full tree is built using temporary 
        // nodes for all allocations that should be skipped. Then tree is 
        // collapsed by copying the children of temporary nodes to their parents
        // in bottom-up fasion.
        if (skipRepeated && pi->second->_repeated) {
            // Create a temporary node
            TfMallocTag::CallTree::PathNode childNode;
            pi->second->_BuildTree(&childNode, skipRepeated);
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
            pi->second->_BuildTree(&childNode, skipRepeated);
            node->nBytes += childNode.nBytes;
        }
    }
}

namespace {
void Tf_GetCallSites(TfMallocTag::CallTree::PathNode* node, 
                     Tf_MallocCallSiteTable* table) {
    TF_AXIOM(node);
    TF_AXIOM(table);

    size_t dummy;
    Tf_MallocCallSite* site = 
        Tf_GetOrCreateCallSite(table, node->siteName.c_str(), &dummy);
    site->_totalBytes += node->nBytesDirect;

    TF_FOR_ALL(pi, node->children) {
        Tf_GetCallSites(&(*pi), table);
    }
}
}

/*
 * None of this is implemented for a 32-bit build.
 */

#define _HI_WORD(sptr) *(((int *)sptr) + 1)
#define _LO_WORD(sptr) *((int *)sptr)

#if defined(ARCH_BITS_64)

// This modifies the control word associated with \a ptr, removing the stored
// index, and returning the index and allocation size.
static inline void
_ExtractIndexAndGetSize(void *ptr, size_t *size, uint32_t *index)
{
    // Get the control word.
    size_t *sptr = static_cast<size_t *>(ptr) - 1;
    
    // Read the stored index.
    *index = _HI_WORD(sptr) >> HIWORD_INDEX_BIT_OFFSET;

    // Read the size.
    *size = *sptr & MALLOC_SIZE_MASK;

    // Remove the stored index from the word.
    _HI_WORD(sptr) &= HIWORD_INDEX_MASK;

}

// This modifies the control word associated with \a ptr, storing \a index, and
// returning the allocation size.
static inline void
_StoreIndexAndGetSize(void *ptr, size_t *size, uint32_t index)
{
    // Get the control word.
    size_t const *sptr = static_cast<size_t const *>(ptr) - 1;

    // Read the size.
    *size = *sptr & MALLOC_SIZE_MASK;

    // Write the index.
    _HI_WORD(sptr) |= (index << HIWORD_INDEX_BIT_OFFSET);
}    

#else

// Allow compilation, but just fatal error.  This code shouldn't ever be active

static inline void
_ExtractIndexAndGetSize(void *, size_t *, uint32_t *)
{
    TF_FATAL_ERROR("Attempting to use Malloc Tags on unsupported platform");
}

static inline void
_StoreIndexAndGetSize(void *, size_t *, uint32_t)
{
    TF_FATAL_ERROR("Attempting to use Malloc Tags on unsupported platform");
}

#endif

// Per-thread data for TfMallocTag.
struct TfMallocTag::_ThreadData {
    _ThreadData() : _tagState(_TaggingDormant) { }
    _ThreadData(const _ThreadData &) = delete;
    _ThreadData(_ThreadData&&) = delete;
    _ThreadData& operator=(const _ThreadData &rhs) = delete;
    _ThreadData& operator=(_ThreadData&&) = delete;

    _Tagging _tagState;
    std::vector<Tf_MallocPathNode*> _tagStack;
    std::vector<unsigned int> _callSiteOnStack;
};

class TfMallocTag::Tls {
public:
    static
    TfMallocTag::_ThreadData*
    Find()
    {
#if defined(ARCH_HAS_THREAD_LOCAL)
        static thread_local _ThreadData data;
        return &data;
#else
        TF_FATAL_ERROR("TfMallocTag not supported on platforms "
                       "without thread_local");
        return nullptr;
#endif
    }
};

/*
 * If this returns false, it sets *tptr.  Otherwise,
 * we don't need *tptr, so it may not be set.
 */
inline bool
TfMallocTag::_ShouldNotTag(TfMallocTag::_ThreadData** tptr, _Tagging* statePtr)
{
    if (!TfMallocTag::_doTagging) {
        if (statePtr) {
            *statePtr = _TaggingDormant;
        }
        return true;
    }
    else {
        *tptr = TfMallocTag::Tls::Find();
            if (statePtr) {
                *statePtr = (*tptr)->_tagState;
            }
            return (*tptr)->_tagState != _TaggingEnabled;
        }
}

// Helper function to retrieve the current path node from a _ThreadData
// object. Note that ::_mallocGlobalData->_mutex must be locked before calling
// this function.
inline Tf_MallocPathNode*
TfMallocTag::_GetCurrentPathNodeNoLock(const TfMallocTag::_ThreadData* tptr)
{
    if (not tptr->_tagStack.empty()) {
        return tptr->_tagStack.back();
    }

    // If the _ThreadData does not have any entries in its tag stack, return
    // the global root so that any memory allocations are assigned to that
    // node.
    return ::_mallocGlobalData->_rootNode;
}

void
TfMallocTag::SetDebugMatchList(const std::string& matchList)
{
    if (TfMallocTag::IsInitialized()) {
        tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);
        ::_mallocGlobalData->_SetDebugNames(matchList);
    }
}

void
TfMallocTag::SetCapturedMallocStacksMatchList(const std::string& matchList)
{
    if (TfMallocTag::IsInitialized()) {
        tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);
        ::_mallocGlobalData->_SetTraceNames(matchList);
    }
}

vector<vector<uintptr_t> >
TfMallocTag::GetCapturedMallocStacks()
{
    vector<vector<uintptr_t> > result;
    
    if (not TfMallocTag::IsInitialized())
        return result;

    // Push some malloc tags, so what we do here doesn't pollute the root
    // stacks.
    TfAutoMallocTag2 tag("Tf", "TfGetRootMallocStacks");

    // Copy off the stack traces, make sure to malloc outside.
    Tf_MallocGlobalData::_CallStackTableType traces;
    
    // Swap them out while holding the lock.
    {
        tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);
        traces.swap(::_mallocGlobalData->_callStackTable);
    }

    TF_FOR_ALL(i, traces)
        result.push_back(i->second.stack);

    return result;
}

void*
TfMallocTag::_MallocWrapper(size_t nBytes, const void*)
{
    void* ptr = ::_mallocHook.Malloc(nBytes);

    _ThreadData* td;
    if (_ShouldNotTag(&td) or ARCH_UNLIKELY(not ptr))
        return ptr;

    {
        tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);
    
        Tf_MallocPathNode* node = _GetCurrentPathNodeNoLock(td);
        size_t blockSize = Tf_GetMallocBlockSize(ptr, nBytes);

        // Update malloc global data with bookkeeping information. This has to
        // happen while the mutex is held.
        if (_mallocGlobalData->_RegisterPathNodeForBlock(node, ptr, blockSize)) {
            ::_mallocGlobalData->_CaptureMallocStack(node, ptr, blockSize);

            node->_totalBytes += blockSize;
            node->_numAllocations++;
            node->_callSite->_totalBytes += blockSize;
            ::_mallocGlobalData->_totalBytes += blockSize;

            _mallocGlobalData->_maxTotalBytes = 
                std::max(_mallocGlobalData->_totalBytes,
                         _mallocGlobalData->_maxTotalBytes);

            _mallocGlobalData->_RunDebugHookForNode(node, ptr, blockSize);
            
            return ptr;
        }
    }

    // Make sure we issue this error while the mutex is unlocked, as issuing
    // the error could cause more allocations, leading to a reentrant call.
    //
    // This should only happen if there's a bug with removing previously
    // allocated blocks from the path node table. This likely would cause us to
    // miscount memory usage, but the allocated pointer is still valid and the
    // system should continue to work. So, we issue a warning but continue on
    // instead of using an axiom.
    TF_VERIFY(!"Failed to register path for allocated block. "
               "Memory usage may be miscounted");

    return ptr;
}

void*
TfMallocTag::_ReallocWrapper(void* oldPtr, size_t nBytes, const void*)
{
    /*
     * If ptr is NULL, we want to make sure we don't double count,
     * because a call to ::_mallocHook.Realloc(ptr, nBytes) could call
     * through to our malloc.  To avoid this, we'll explicitly short-circuit
     * ourselves rather than trust that the malloc library will do it.
     */
    if (!oldPtr)
        return _MallocWrapper(nBytes, NULL);

    _ThreadData* td = NULL;
    _Tagging tagState;
    const bool shouldNotTag = _ShouldNotTag(&td, &tagState);

    // If tagging is explicitly disabled, just do the realloc and skip 
    // everything else. This avoids a deadlock if we get here while updating
    // Tf_MallocGlobalData::_pathNodeTable.
    //
    // If tagState is _TaggingDormant, we still need to unregister the oldPtr.
    // However, we won't need to register the newly realloc'd ptr later on.
    if (tagState == _TaggingDisabled) {
        return ::_mallocHook.Realloc(oldPtr, nBytes);
    }

    void* newPtr = NULL;
    {
        tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);

        Tf_MallocBlockInfo info;
        if (::_mallocGlobalData->_UnregisterPathNodeForBlock(oldPtr, &info)) {

            size_t bytesFreed = info.blockSize;
            Tf_MallocPathNode* oldNode = 
                ::_mallocGlobalData->_allPathNodes[info.pathNodeIndex];

            _mallocGlobalData->_RunDebugHookForNode(oldNode, oldPtr, bytesFreed);

            // Check if we should release a malloc stack.  This has to happen
            // while the mutex is held.
            ::_mallocGlobalData->_ReleaseMallocStack(oldNode, oldPtr);
            
            oldNode->_totalBytes -= bytesFreed;
            oldNode->_numAllocations -= (_DECREMENT_ALLOCATION_COUNTS) ? 1 : 0;
            oldNode->_callSite->_totalBytes -= bytesFreed;
            ::_mallocGlobalData->_totalBytes -= bytesFreed;
        }

        newPtr = ::_mallocHook.Realloc(oldPtr, nBytes);

        if (shouldNotTag or ARCH_UNLIKELY(not newPtr))
            return newPtr;

        Tf_MallocPathNode* newNode = _GetCurrentPathNodeNoLock(td);
        size_t blockSize = Tf_GetMallocBlockSize(newPtr, nBytes);

        // Update malloc global data with bookkeeping information. This has to
        // happen while the mutex is held.
        if (::_mallocGlobalData->_RegisterPathNodeForBlock(
                newNode, newPtr, blockSize)) {

            ::_mallocGlobalData->_CaptureMallocStack(
                newNode, newPtr, blockSize);
    
            newNode->_totalBytes += blockSize;
            newNode->_numAllocations++;
            newNode->_callSite->_totalBytes += blockSize;
            ::_mallocGlobalData->_totalBytes += blockSize;

            _mallocGlobalData->_maxTotalBytes = 
                std::max(_mallocGlobalData->_totalBytes,
                         _mallocGlobalData->_maxTotalBytes);

            _mallocGlobalData->_RunDebugHookForNode(
                newNode, newPtr, blockSize);
        }

        return newPtr;
    }

    // See comment in _MallocWrapper.
    TF_VERIFY(!"Failed to register path for allocated block. "
               "Memory usage may be miscounted");
    return newPtr;
}

void*
TfMallocTag::_MemalignWrapper(size_t alignment, size_t nBytes, const void*)
{
    void* ptr = ::_mallocHook.Memalign(alignment, nBytes);

    _ThreadData* td;
    if (_ShouldNotTag(&td) or ARCH_UNLIKELY(not ptr))
        return ptr;

    tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);

    Tf_MallocPathNode* node = _GetCurrentPathNodeNoLock(td);
    size_t blockSize = Tf_GetMallocBlockSize(ptr, nBytes);

    // Update malloc global data with bookkeeping information. This has to
    // happen while the mutex is held.
    ::_mallocGlobalData->_RegisterPathNodeForBlock(node, ptr, blockSize);
    ::_mallocGlobalData->_CaptureMallocStack(node, ptr, blockSize);
    
    node->_totalBytes += blockSize;
    node->_numAllocations++;
    node->_callSite->_totalBytes += blockSize;
    ::_mallocGlobalData->_totalBytes += blockSize;

    _mallocGlobalData->_maxTotalBytes = std::max(_mallocGlobalData->_totalBytes,
        _mallocGlobalData->_maxTotalBytes);

    _mallocGlobalData->_RunDebugHookForNode(node, ptr, blockSize);

    return ptr;
}

void
TfMallocTag::_FreeWrapper(void* ptr, const void*)
{
    if (!ptr)
        return;

    // If tagging is explicitly disabled, just do the free and skip 
    // everything else. This avoids a deadlock if we get here while updating
    // Tf_MallocGlobalData::_pathNodeTable.
    _ThreadData* td;
    _Tagging tagState;
    if (_ShouldNotTag(&td, &tagState) and tagState == _TaggingDisabled) {
        ::_mallocHook.Free(ptr);
        return;
    }

    tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);

    Tf_MallocBlockInfo info;
    if (::_mallocGlobalData->_UnregisterPathNodeForBlock(ptr, &info)) {
        size_t bytesFreed = info.blockSize;
        Tf_MallocPathNode* node = 
            ::_mallocGlobalData->_allPathNodes[info.pathNodeIndex];

        ::_mallocGlobalData->_RunDebugHookForNode(node, ptr, bytesFreed);

        // Check if we should release a malloc stack.  This has to happen
        // while the mutex is held.
        ::_mallocGlobalData->_ReleaseMallocStack(node, ptr);
        
        node->_totalBytes -= bytesFreed;
        node->_numAllocations -= (_DECREMENT_ALLOCATION_COUNTS) ? 1 : 0;
        node->_callSite->_totalBytes -= bytesFreed;
        ::_mallocGlobalData->_totalBytes -= bytesFreed;
    }

    ::_mallocHook.Free(ptr);
}

void*
TfMallocTag::_MallocWrapper_ptmalloc(size_t nBytes, const void*)
{
    void* ptr = ::_mallocHook.Malloc(nBytes);

    _ThreadData* td;
    if (_ShouldNotTag(&td))
        return ptr;

    tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);

    Tf_MallocPathNode* node = _GetCurrentPathNodeNoLock(td);
    size_t actualBytes;
    _StoreIndexAndGetSize(ptr, &actualBytes, node->_index);

    // Check if we should capture a malloc stack.  This has to happen while
    // the mutex is held.
    ::_mallocGlobalData->_CaptureMallocStack(node, ptr, actualBytes);

    node->_totalBytes += actualBytes;
    node->_numAllocations++;
    node->_callSite->_totalBytes += actualBytes;
    ::_mallocGlobalData->_totalBytes += actualBytes;

    _mallocGlobalData->_maxTotalBytes = std::max(_mallocGlobalData->_totalBytes,
        _mallocGlobalData->_maxTotalBytes);

    _mallocGlobalData->_RunDebugHookForNode(node, ptr, actualBytes);

    return ptr;
}

void*
TfMallocTag::_ReallocWrapper_ptmalloc(void* oldPtr, size_t nBytes, const void*)
{
    /*
     * If ptr is NULL, we want to make sure we don't double count,
     * because a call to ::_mallocHook.Realloc(ptr, nBytes) could call
     * through to our malloc.  To avoid this, we'll explicitly short-circuit
     * ourselves rather than trust that the malloc library will do it.
     */
    if (!oldPtr)
        return _MallocWrapper_ptmalloc(nBytes, NULL);

    /*
     * Account for the implicit free, and fix-up oldPtr
     * regardless of whether we're currently tagging or not:
     */
    uint32_t index;
    size_t bytesFreed;
    _ExtractIndexAndGetSize(oldPtr, &bytesFreed, &index);

    void* newPtr = ::_mallocHook.Realloc(oldPtr, nBytes);

    _ThreadData* td;
    if (_ShouldNotTag(&td))
        return newPtr;

    tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);

    Tf_MallocPathNode* newNode = _GetCurrentPathNodeNoLock(td);
    size_t actualBytes;
    _StoreIndexAndGetSize(newPtr, &actualBytes, newNode->_index);

    if (index) {
        Tf_MallocPathNode* oldNode = ::_mallocGlobalData->_allPathNodes[index];

        _mallocGlobalData->_RunDebugHookForNode(oldNode, oldPtr, bytesFreed);

        // Check if we should release a malloc stack.  This has to happen while
        // the mutex is held.
        ::_mallocGlobalData->_ReleaseMallocStack(oldNode, oldPtr);
        
        oldNode->_totalBytes -= bytesFreed;
        oldNode->_numAllocations -= (_DECREMENT_ALLOCATION_COUNTS) ? 1 : 0;
        oldNode->_callSite->_totalBytes -= bytesFreed;
        ::_mallocGlobalData->_totalBytes -= bytesFreed;
    }

    // Check if we should capture a malloc stack.  This has to happen while
    // the mutex is held.
    ::_mallocGlobalData->_CaptureMallocStack(newNode, newPtr, actualBytes);
    
    newNode->_totalBytes += actualBytes;
    newNode->_numAllocations++;
    newNode->_callSite->_totalBytes += actualBytes;
    ::_mallocGlobalData->_totalBytes += actualBytes;

    _mallocGlobalData->_maxTotalBytes = std::max(_mallocGlobalData->_totalBytes,
        _mallocGlobalData->_maxTotalBytes);

    _mallocGlobalData->_RunDebugHookForNode(newNode, newPtr, actualBytes);

    return newPtr;
}

void*
TfMallocTag::_MemalignWrapper_ptmalloc(size_t alignment, size_t nBytes, const void*)
{
    void* ptr = ::_mallocHook.Memalign(alignment, nBytes);

    _ThreadData* td;
    if (_ShouldNotTag(&td))
        return ptr;

    tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);

    Tf_MallocPathNode* node = _GetCurrentPathNodeNoLock(td);
    size_t actualBytes;
    _StoreIndexAndGetSize(ptr, &actualBytes, node->_index);

    // Check if we should capture a malloc stack.  This has to happen while
    // the mutex is held.
    ::_mallocGlobalData->_CaptureMallocStack(node, ptr, actualBytes);
    
    node->_totalBytes += actualBytes;
    node->_numAllocations++;
    node->_callSite->_totalBytes += actualBytes;
    ::_mallocGlobalData->_totalBytes += actualBytes;

    _mallocGlobalData->_maxTotalBytes = std::max(_mallocGlobalData->_totalBytes,
        _mallocGlobalData->_maxTotalBytes);

    _mallocGlobalData->_RunDebugHookForNode(node, ptr, actualBytes);

    return ptr;
}

void
TfMallocTag::_FreeWrapper_ptmalloc(void* ptr, const void*)
{
    if (!ptr)
        return;

    /*
     * Make ptr safe in case it has index bits set:
     */
    uint32_t index;
    size_t bytesFreed;
    _ExtractIndexAndGetSize(ptr, &bytesFreed, &index);

    if (index && TfMallocTag::_doTagging) {
        tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);
        Tf_MallocPathNode* node = ::_mallocGlobalData->_allPathNodes[index];

        ::_mallocGlobalData->_RunDebugHookForNode(node, ptr, bytesFreed);

        // Check if we should release a malloc stack.  This has to happen
        // while the mutex is held.
        ::_mallocGlobalData->_ReleaseMallocStack(node, ptr);
        
        node->_totalBytes -= bytesFreed;
        node->_numAllocations -= (_DECREMENT_ALLOCATION_COUNTS) ? 1 : 0;
        node->_callSite->_totalBytes -= bytesFreed;
        ::_mallocGlobalData->_totalBytes -= bytesFreed;
    }

    ::_mallocHook.Free(ptr);
}

bool
TfMallocTag::Initialize(string* errMsg)
{
    static bool status = _Initialize(errMsg);
    return status;
}


bool
TfMallocTag::GetCallTree(CallTree* tree, bool skipRepeated)
{
    tree->callSites.clear();
    tree->root.nBytes = tree->root.nBytesDirect = 0;
    tree->root.nAllocations = 0;
    tree->root.siteName.clear();
    tree->root.children.clear();

    if (Tf_MallocGlobalData* gd = ::_mallocGlobalData) {
        TfMallocTag::_TemporaryTaggingState tmpState(_TaggingDisabled);

        gd->_mutex.lock();

        // Build the snapshot call tree
        gd->_rootNode->_BuildTree(&tree->root, skipRepeated);
        
        // Build the snapshot callsites map based on the tree
        Tf_MallocCallSiteTable callSiteTable;
        Tf_GetCallSites(&tree->root, &callSiteTable);

        // Copy the callsites into the calltree
        tree->callSites.reserve(callSiteTable.size());
        TF_FOR_ALL(csi, callSiteTable) {
            CallTree::CallSite cs = {
                csi->second->_name,
                static_cast<size_t>(csi->second->_totalBytes)
            };
            tree->callSites.push_back(cs);
            delete csi->second;
        }

        gd->_BuildUniqueMallocStacks(tree);

        gd->_mutex.unlock();
        return true;
    }
    else
        return false;
}

size_t
TfMallocTag::GetTotalBytes()
{
    if (!::_mallocGlobalData)
        return 0;

    tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);
    return ::_mallocGlobalData->_totalBytes;
}

size_t
TfMallocTag::GetMaxTotalBytes()
{
    if (!::_mallocGlobalData)
        return 0;

    tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);
    return ::_mallocGlobalData->_maxTotalBytes;
}

void
TfMallocTag::_SetTagging(_Tagging status)
{
    TfMallocTag::Tls::Find()->_tagState = status;
}

TfMallocTag::_Tagging
TfMallocTag::_GetTagging()
{
    return TfMallocTag::Tls::Find()->_tagState;
}

bool
TfMallocTag::_Initialize(std::string* errMsg)
{
    /*
     * This is called from an EXECUTE_ONCE block, so no
     * need to lock anything.
     */
    TF_AXIOM(!::_mallocGlobalData);
    ::_mallocGlobalData = new Tf_MallocGlobalData();

    // Note that we are *not* using the _TemporaryTaggingState object
    // here. We explicitly want the tagging set to enabled as the end
    // of this function so that all subsequent memory allocations are captured.
    _SetTagging(_TaggingDisabled);

    bool usePtmalloc = ::_UsePtmalloc();
    
    if (usePtmalloc) {
        // index 0 is reserved for untracked malloc/free's:
        ::_mallocGlobalData->_allPathNodes.push_back(NULL);
    }

    Tf_MallocCallSite* site = ::_mallocGlobalData->_GetOrCreateCallSite("__root");
    Tf_MallocPathNode* rootNode = new Tf_MallocPathNode(site);
    ::_mallocGlobalData->_rootNode = rootNode;
    (void) ::_mallocGlobalData->_RegisterPathNode(rootNode);
    TfMallocTag::Tls::Find()->_tagStack.reserve(64);
    TfMallocTag::Tls::Find()->_tagStack.push_back(rootNode);

    _SetTagging(_TaggingEnabled);

    TfMallocTag::_doTagging = true;

    if (usePtmalloc) {
        return ::_mallocHook.Initialize(_MallocWrapper_ptmalloc, 
                                        _ReallocWrapper_ptmalloc,
                                        _MemalignWrapper_ptmalloc, 
                                        _FreeWrapper_ptmalloc, 
                                        errMsg);
    }
    else {
        return ::_mallocHook.Initialize(_MallocWrapper, 
                                        _ReallocWrapper,
                                        _MemalignWrapper, 
                                        _FreeWrapper, 
                                        errMsg);
    }
}

void
TfMallocTag::Auto::_Begin(const string& name)
{
    _Begin(name.c_str());
}

void
TfMallocTag::Auto::_Begin(const char* name)
{
    if (!name || !name[0])
        return;

    _threadData = TfMallocTag::Tls::Find();

    _threadData->_tagState = _TaggingDisabled;
    Tf_MallocPathNode* thisNode;
    Tf_MallocCallSite* site;

    {
        tbb::spin_mutex::scoped_lock lock(::_mallocGlobalData->_mutex);
        site = ::_mallocGlobalData->_GetOrCreateCallSite(name);

        if (_threadData->_callSiteOnStack.size() <= site->_index) {
            if (_threadData->_callSiteOnStack.capacity() == 0)
                _threadData->_callSiteOnStack.reserve(128);
            _threadData->_callSiteOnStack.resize(site->_index + 1, 0);
        }


        if (_threadData->_tagStack.empty())
            thisNode = ::_mallocGlobalData->_rootNode->_GetOrCreateChild(site);
        else
            thisNode = _threadData->_tagStack.back()->_GetOrCreateChild(site);

        if (_threadData->_callSiteOnStack[site->_index]) {
            thisNode->_repeated = true;
        }
    }

    if (thisNode) {
        _threadData->_tagStack.push_back(thisNode);
        _threadData->_callSiteOnStack[site->_index] += 1;
        _threadData->_tagState = _TaggingEnabled;
    }
    else {
        _threadData->_tagState = _TaggingEnabled;
        _threadData = NULL;
    }
}

void
TfMallocTag::Auto::_End()
{
    Tf_MallocPathNode* node = _threadData->_tagStack.back();
    TF_AXIOM(_threadData->_callSiteOnStack[node->_callSite->_index] > 0);
    _threadData->_callSiteOnStack[node->_callSite->_index] -= 1;
    _threadData->_tagStack.pop_back();
}

void
TfMallocTag::Pop(const char* name)
{
    if (!TfMallocTag::_doTagging)
        return;

    _ThreadData* threadData = TfMallocTag::Tls::Find();
    Tf_MallocPathNode* node = threadData->_tagStack.back();

    if (name && node->_callSite->_name != name) {
        TF_CODING_ERROR("mismatched call Pop(\"%s\"); top of stack is \"%s\"",
                        name, node->_callSite->_name.c_str());
    }

    TF_AXIOM(threadData->_callSiteOnStack[node->_callSite->_index] > 0);
    threadData->_callSiteOnStack[node->_callSite->_index] -= 1;
    threadData->_tagStack.pop_back();
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
        if (n < str.size() and n%3 == 0) {
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

#if not (_DECREMENT_ALLOCATION_COUNTS)
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
    const boost::optional<std::string> &rootName =
        boost::optional<std::string>())
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
        << (rootName and not rootName->empty() ? *rootName : node.siteName)
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
            and reportedMem != GetTotalBytes()) {
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
    std::ostream &out,
    const boost::optional<std::string> &rootName) const
{
    out << "\nTree view  ==============\n";
    out << "      inclusive       exclusive\n";

    _ReportMallocNode(out, this->root, 0, rootName);

    // Also add the dominant call sites to the report.
    out << GetPrettyPrintString(CALLSITES);

    // And the captured malloc stacks if there are any.
    if (not this->capturedCallStacks.empty()) {
        _ReportCapturedMallocStacks(out, this->capturedCallStacks);
    }
}

TfMallocTag::
_TemporaryTaggingState::_TemporaryTaggingState(_Tagging tempStatus)
    : _oldState(TfMallocTag::_GetTagging())
{
    TfMallocTag::_SetTagging(tempStatus);
}

TfMallocTag::_TemporaryTaggingState::~_TemporaryTaggingState()
{
    TfMallocTag::_SetTagging(_oldState);
}
