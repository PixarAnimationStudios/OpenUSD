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
#ifndef TF_MALLOCTAG_H
#define TF_MALLOCTAG_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include <cstdlib>
#include <iosfwd>
#include <stdint.h>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \file tf/mallocTag.h
/// \ingroup group_tf_MallocTag

struct Tf_MallocPathNode;

/// \class TfMallocTag
/// \ingroup group_tf_MallocTag
///
/// Top-down memory tagging system.
///
/// See \ref page_tf_MallocTag for a detailed description.
class TfMallocTag {
public:
    struct CallStackInfo;

    /// \struct CallTree
    /// Summary data structure for \c malloc statistics.
    ///
    /// The \c CallTree structure is used to deliver a snapshot of the current
    /// malloc usage.  It is accessible as publicly modifiable data because it
    /// is simply a returned snapshot of the current memory state.
    struct CallTree {
        /// \struct PathNode
        /// Node in the call tree structure.
        ///
        /// A \c PathNode captures the hierarchy of active \c TfAutoMallocTag
        /// objects that are pushed and popped during program execution.  Each
        /// \c PathNode thus describes a sequence of call-sites (i.e. a path
        /// down the call tree).  Repeated call sites (in the case of
        /// co-recursive function calls) can be skipped, e.g. pushing tags
        /// "A", "B", "C", "B", "C" leads to only three path-nodes,
        /// representing the paths "A", "AB", and "ABC".  Allocations done at
        /// the bottom (i.e. when tags "A", "B", "C", "B", "C" are all active)
        /// are billed to the longest path node in the sequence, which
        /// corresponds to the path "ABC").
        ///
        /// Path nodes track both the memory they incur directly (\c
        /// nBytesDirect) but more importantly, the total memory allocated by
        /// themselves and any of their children (\c nBytes).  The name of a
        /// node (\c siteName) corresponds to the tag name of the final tag in
        /// the path.
        struct PathNode {
            size_t nBytes,          ///< Allocated bytes by this or descendant nodes.
                   nBytesDirect;    ///< Allocated bytes (only for this node).
            size_t nAllocations;    ///< The number of allocations for this node.
            std::string siteName;   ///< Tag name.
            std::vector<PathNode>
                        children;   ///< Children nodes.
        };

        /// \struct CallSite
        /// Record of the bytes allocated under each different tag.
        ///
        /// Each construction of a \c TfAutoMallocTag object with a different
        /// argument produces a distinct \c CallSite record.  The total bytes
        /// outstanding for all memory allocations made under a given
        /// call-site are recorded in \c nBytes, while the name of the call
        /// site is available as \c name.
        struct CallSite {
            std::string name;       ///< Tag name.
            size_t nBytes;          ///< Allocated bytes.
        };

        // Note: enum below must be kept in sync with tfmodule/mallocCallTree.h

        /// Specify which parts of the report to print.
        enum PrintSetting {
            TREE = 0,                   ///< Print the full call tree
            CALLSITES,                  ///< Print just the call sites > 0.1%
            BOTH                        ///< Print both tree and call sites
        };

        /// Return the malloc report string.
        ///
        /// Get a malloc report of the tree and/or callsites.
        ///
        /// The columns in the report are abbreviated. Here are the definitions.
        ///
        /// \b TAGNAME : The name of the tag being tracked. This matches the
        /// string argument to TfAutoMallocTag constructor.
        ///
        /// \b BytesIncl : Bytes Inclusive. This includes all bytes allocated by
        /// this tag and any bytes of its children.
        ///
        /// \b BytesExcl : Bytes Exclusive. Only bytes allocated exclusively by
        /// this tag, not including any bytes of its children.
        ///
        /// \b %%Prnt : (%% Parent).  me.BytesIncl / parent.BytesIncl * 100
        ///
        /// \b %%Exc : BytesExcl / BytesIncl * 100
        ///
        /// \b %%Totl : (%% Total). BytesExcl / TotalBytes * 100
        TF_API
        std::string GetPrettyPrintString(PrintSetting setting = BOTH,
                                         size_t maxPrintedNodes = 100000) const;

        /// Generates a report to the ostream \p out.
        ///
        /// This report is printed in a way that is intended to be used by
        /// xxtracediff.  If \p rootName is provided and is non-empty it will
        /// replace the name of the tree root in the report.
        TF_API
        void Report(
            std::ostream &out,
            const boost::optional<std::string> &rootName =
                boost::optional<std::string>()) const;

        /// All call sites.
        std::vector<CallSite> callSites;

        /// Root node of the call-site hierarchy.
        PathNode root;

        /// The captured malloc stacks.
        std::vector<CallStackInfo> capturedCallStacks;
    };

    /// \struct CallStackInfo
    /// This struct is used to represent a call stack taken for an allocation
    /// that was  billed under a specific malloc tag.
    struct CallStackInfo
    {
        /// The stack frame pointers.
        std::vector<uintptr_t> stack;

        /// The amount of allocated memory (accumulated over all allocations
        /// sharing this call stack).
        size_t size;

        /// The number of allocations (always one unless stack frames have
        /// been combined to create unique stacks).
        size_t numAllocations;
    };

    /// Initialize the memory tagging system.
    ///
    /// This function returns \c true if the memory tagging system can be
    /// successfully initialized or it has already been initialized. Otherwise,
    /// \p *errMsg is set with an explanation for the failure.
    ///
    /// Until the system is initialized, the various memory reporting calls
    /// will indicate that no memory has been allocated.  Note also that
    /// memory allocated prior to calling \c Initialize() is not tracked i.e.
    /// all data refers to allocations that happen subsequent to calling \c
    /// Initialize().
    TF_API static bool Initialize(std::string* errMsg);

    /// Return true if the tagging system is active.
    ///
    /// If \c Initialize() has been successfully called, this function returns
    /// \c true.
    static bool IsInitialized() {
        return TfMallocTag::_doTagging;
    }

    /// Return total number of allocated bytes.
    ///
    /// The current total memory that has been allocated and not freed is
    /// returned. Memory allocated before calling \c Initialize() is not
    /// accounted for.
    TF_API static size_t GetTotalBytes();

    /// Return the maximum total number of bytes that have ever been allocated
    /// at one time.
    ///
    /// This is simply the maximum value of GetTotalBytes() since Initialize()
    /// was called.
    TF_API static size_t GetMaxTotalBytes();

    /// Return a snapshot of memory usage.
    ///
    /// Returns a snapshot by writing into \c *tree.  See the \c C *tree
    /// structure for documentation.  If \c Initialize() has not been called,
    /// \ *tree is set to a rather blank structure (empty vectors, empty
    /// strings, zero in all integral fields) and \c false is returned;
    /// otherwise, \p *tree is set with the contents of the current memory
    /// snapshot and \c true is returned. It is fine to call this function on
    /// the same \p *tree instance; each call simply overwrites the data from
    /// the last call. If /p skipRepeated is \c true, then any repeated
    /// callsite is skipped. See the \c CallTree documentation for more
    /// details.
    TF_API static bool GetCallTree(CallTree* tree, bool skipRepeated = true);

private:
    // Enum describing whether allocations are being tagged in an associated
    // thread.
    enum _Tagging {
        _TaggingEnabled,   // Allocations are being tagged
        _TaggingDisabled,  // Allocations are not being tagged

        _TaggingDormant    // Tagging has not been initialized in this
                           // thread as no malloc tags have been pushed onto
                           // the stack.
    };

    struct _ThreadData;

public:

    /// \class Auto
    /// \ingroup group_tf_MallocTag
    ///
    /// Scoped (i.e. local) object for creating/destroying memory tags.
    ///
    /// Note: \c TfAutoMallocTag is a typedef to \c TfMallocTag::Auto; the
    /// convention is to use \c TfAutoMallocTag to make it clear that the
    /// local object exists only because its constructor and destructor modify
    /// program state.
    ///
    /// A \c TfAutoMallocTag object is used to push a memory tag onto the
    /// current call stack; destruction of the object pops the call stack.
    /// Note that each thread has its own call-stack.
    ///
    /// There is no (measurable) cost to creating or destroying memory tags if
    /// \c TfMallocTag::Initialize() has not been called; if it has, then
    /// there is a small (but measurable) cost associated with pushing and
    /// popping memory tags on the local call stack.  Most of the cost is
    /// simply locking a mutex; typically, pushing or popping the call stack
    /// does not actually cause any memory allocation unless this is the first
    /// time that the given named tag has been encountered.
    class Auto : public boost::noncopyable {
    public:
        /// Push a memory tag onto the local-call stack with name \p name.
        ///
        /// If \c TfMallocTag::Initialize() has not been called, this
        /// constructor does essentially no (measurable) work, assuming \p
        /// name is a string literal or just a pointer to an existing string.
        ///
        /// Objects of this class should only be created as local variables;
        /// never as member variables, global variables, or via \c new.  If
        /// you can't create your object as a local variable, you can make
        /// manual calls to \c TfMallocTag::Push() and \c TfMallocTag::Pop(),
        /// though you should do this only as a last resort.
        Auto(const char* name) : _threadData(0) {
            if (TfMallocTag::_doTagging)
                _Begin(name);
        }

        /// Push a memory tag onto the local-call stack with name \p name.
        ///
        /// If \c TfMallocTag::Initialize() has not been called, this
        /// constructor does essentially no (measurable) work.  However, any
        /// work done in constructing the \c std::string object \p name will
        /// be incurred even if tagging is not active.  If this is an issue,
        /// you can query \c TfMallocTag::IsInitialized() to avoid unneeded
        /// work when tagging is inactive.  Note that the case when \p name is
        /// a string literal does not apply here: instead, the constructor that
        /// takes a \c const \c char* (above) will be called.
        ///
        /// Objects of this class should only be created as local variables;
        /// never as member variables, global variables, or via \c new.  If
        /// you can't create your object as a local variable, you can make
        /// manual calls to \c TfMallocTag::Push() and \c TfMallocTag::Pop(),
        /// though you should do this only as a last resort.
        Auto(const std::string& name) : _threadData(0) {
            if (TfMallocTag::_doTagging)
                _Begin(name);
        }

        /// Pop the tag from the stack before it is destructed.
        ///
        /// Normally you should not use this.  The normal destructor is
        /// preferable because it insures proper release order.  If you call
        /// \c Release(), make sure all tags are released in the opposite
        /// order they were declared in.  It is better to use sub-scopes to
        /// control the life span of tags, but if that won't work, \c
        /// Release() is still preferable to \c TfMallocTag::Push() and \c
        /// TfMallocTag::Pop() because it isn't vulnerable to early returns or
        /// exceptions.
        inline void Release() {
            if (_threadData) {
                _End();
                _threadData = NULL;
            }
        }

        /// Pop a memory tag from the local-call stack.
        ///
        /// If \c TfMallocTag::Initialize() was not called when this tag was
        /// pushed onto the stack, popping the tag from the stack does
        /// essentially no (measurable) work.
        inline ~Auto() {
            Release();
        }

    private:
        TF_API void _Begin(const char* name);
        TF_API void _Begin(const std::string& name);
        TF_API void _End();

        _ThreadData* _threadData;

        friend class TfMallocTag;
    };

    /// \class Auto2
    /// \ingroup group_tf_MallocTag
    ///
    /// Scoped (i.e. local) object for creating/destroying memory tags.
    ///
    /// Auto2 is just like Auto, except it pushes two tags onto the stack.
    class Auto2 {
    public:
        /// Push two memory tags onto the local-call stack.
        ///
        /// \see TfMallocTag::Auto(const char* name).
        Auto2(const char* name1, const char* name2) :
            _tag1(name1),
            _tag2(name2)
        {
        }

        /// Push two memory tags onto the local-call stack.
        ///
        /// \see TfMallocTag::Auto(const std::string& name).
        Auto2(const std::string& name1, const std::string& name2) :
            _tag1(name1),
            _tag2(name2)
        {
        }

        /// Pop two memory tags from the local-call stack.
        ///
        /// \see TfMallocTag::Auto(const char* name).
        void Release() {
            _tag2.Release();
            _tag1.Release();
        }

    private:
        Auto _tag1;
        Auto _tag2;
    };

    /// Manually push a tag onto the stack.
    ///
    /// This call has the same effect as the constructor for \c
    /// TfMallocTag::Auto (aka \c TfAutoMallocTag), however a matching call to
    /// \c Pop() is required.
    ///
    /// Note that initializing the tagging system between matching calls to \c
    /// Push() and \c Pop() is ill-advised, which is yet another reason to
    /// prefer using \c TfAutoMallocTag whenever possible.
    static void Push(const std::string& name) {
        TfMallocTag::Auto noname(name);
        noname._threadData = NULL;  // disable destructor
    }

    /// \overload
    static void Push(const char* name) {
        TfMallocTag::Auto noname(name);
        noname._threadData = NULL;  // disable destructor
    }

    /// Manually pop a tag from the stack.
    ///
    /// This call has the same effect as the destructor for \c
    /// TfMallocTag::Auto; it must properly nest with a matching call to \c
    /// Push(), of course.
    ///
    /// If \c name is supplied and does not match the tag at the top of the
    /// stack, a warning message is issued.
    TF_API static void Pop(const char* name = NULL);

    /// \overload
    static void Pop(const std::string& name) {
        Pop(name.c_str());
    }

    /// Sets the tags to trap in the debugger.
    ///
    /// When memory is allocated or freed for any tag that matches \p
    /// matchList the debugger trap is invoked. If a debugger is attached the
    /// program will stop in the debugger, otherwise the program will continue
    /// to run. See \c ArchDebuggerTrap() and \c ArchDebuggerWait().
    ///
    /// \p matchList is a comma, tab or newline separated list of malloc tag
    /// names. The names can have internal spaces but leading and trailing
    /// spaces are stripped. If a name ends in '*' then the suffix is
    /// wildcarded. A name can have a leading '-' or '+' to prevent or allow a
    /// match. Each name is considered in order and later matches override
    /// earlier matches. For example, 'Csd*, -CsdScene::_Populate*,
    /// +CsdScene::_PopulatePrimCacheLocal' matches any malloc tag starting
    /// with 'Csd' but nothing starting with 'CsdScene::_Populate' except
    /// 'CsdScene::_PopulatePrimCacheLocal'. Use the empty string to disable
    /// debugging traps.
    TF_API static void SetDebugMatchList(const std::string& matchList);

    /// Sets the tags to trace.
    ///
    /// When memory is allocated for any tag that matches \p matchList a stack
    /// trace is recorded.  When that memory is released the stack trace is
    /// discarded.  Clients can call \c GetCapturedMallocStacks() to get a
    /// list of all recorded stack traces.  This is useful for finding leaks.
    ///
    /// Traces recorded for any tag that will no longer be matched are
    /// discarded by this call.  Traces recorded for tags that continue to be
    /// matched are retained.
    ///
    /// \p matchList is a comma, tab or newline separated list of malloc tag
    /// names.  The names can have internal spaces but leading and trailing
    /// spaces are stripped.  If a name ends in '*' then the suffix is
    /// wildcarded.  A name can have a leading '-' or '+' to prevent or allow
    /// a match.  Each name is considered in order and later matches override
    /// earlier matches.  For example, 'Csd*, -CsdScene::_Populate*,
    /// +CsdScene::_PopulatePrimCacheLocal' matches any malloc tag starting
    /// with 'Csd' but nothing starting with 'CsdScene::_Populate' except
    /// 'CsdScene::_PopulatePrimCacheLocal'.  Use the empty string to disable
    /// stack capturing.
    TF_API static void SetCapturedMallocStacksMatchList(const std::string& matchList);

    /// Returns the captured malloc stack traces for allocations billed to the
    /// malloc tags passed to SetCapturedMallocStacksMatchList().
    ///
    /// \note This method also clears the internally held set of captured
    /// stacks.
    TF_API static std::vector<std::vector<uintptr_t> > GetCapturedMallocStacks();

private:
    friend struct Tf_MallocGlobalData;
    
    class _TemporaryTaggingState : public boost::noncopyable {
    public:
        explicit _TemporaryTaggingState(_Tagging state);
        ~_TemporaryTaggingState();

    private:
        _Tagging _oldState;
    };

    static void _SetTagging(_Tagging state);
    static _Tagging _GetTagging();

    static bool _Initialize(std::string* errMsg);

    static inline bool _ShouldNotTag(_ThreadData**, _Tagging* t = NULL);
    static inline Tf_MallocPathNode* _GetCurrentPathNodeNoLock(
        const _ThreadData* threadData);

    static void* _MallocWrapper_ptmalloc(size_t, const void*);
    static void* _ReallocWrapper_ptmalloc(void*, size_t, const void*);
    static void* _MemalignWrapper_ptmalloc(size_t, size_t, const void*);
    static void  _FreeWrapper_ptmalloc(void*, const void*);

    static void* _MallocWrapper(size_t, const void*);
    static void* _ReallocWrapper(void*, size_t, const void*);
    static void* _MemalignWrapper(size_t, size_t, const void*);
    static void  _FreeWrapper(void*, const void*);

    friend class TfMallocTag::Auto;
    class Tls;
    friend class TfMallocTag::Tls;
    TF_API static bool _doTagging;
};

/// Top-down memory tagging system.
typedef TfMallocTag::Auto TfAutoMallocTag;

/// Top-down memory tagging system.
typedef TfMallocTag::Auto2 TfAutoMallocTag2;

/// Enable lib/tf memory management.
///
/// Invoking this macro inside a class body causes the class operator \c new to push
/// two \c TfAutoMallocTag objects onto the stack before actually allocating memory for the
/// class.  The names passed into the tag are used for the two tags; pass NULL if you
/// don't need the second tag.  For example,
/// \code
/// class MyBigMeshVertex {
/// public:
///     TF_MALLOC_TAG_NEW("MyBigMesh", "Vertex");
///     ...
/// }
/// \endcode
/// will cause dynamic allocations of \c MyBigMeshVertex to be grouped under
/// the tag \c Vertex which is in turn grouped under \c MyBigMesh.  However,
/// \code
/// class MyBigMesh {
/// public:
///     TF_MALLOC_TAG_NEW("MyBigMesh", NULL);
///     ...
/// }
/// \endcode
/// specifies \c NULL for the second tag because the first tag is sufficient.
///
/// Normally, this macro should be placed in the public section of a class.
/// Note that you cannot specify both this and \c TF_FIXED_SIZE_ALLOCATOR()
/// for the same class.
///
/// Also, note that allocations of a class inside an STL datastructure will
/// not be grouped under the indicated tags.
/// \remark Placed in .h files.
///
/// \hideinitializer
//
PXR_NAMESPACE_CLOSE_SCOPE                                                 

#define TF_MALLOC_TAG_NEW(name1, name2)                                       \
    /* this is for STL purposes */                                            \
    inline void* operator new(::std::size_t, void* ptr) {                     \
        return ptr;                                                           \
    }                                                                         \
                                                                              \
    inline void* operator new(::std::size_t s) {                              \
        PXR_NS::TfAutoMallocTag tag1(name1);                                  \
        PXR_NS::TfAutoMallocTag tag2(name2);                                  \
        return malloc(s);                                                     \
    }                                                                         \
                                                                              \
    inline void* operator new[](::std::size_t s) {                            \
        PXR_NS::TfAutoMallocTag tag1(name1);                                  \
        PXR_NS::TfAutoMallocTag tag2(name2);                                  \
        return malloc(s);                                                     \
    }                                                                         \
                                                                              \
    /* Required due to the placement-new override above. */                   \
    inline void operator delete(void* ptr, void* place) {}                    \
                                                                              \
    inline void operator delete(void* ptr, size_t) {                          \
        free(ptr);                                                            \
    }                                                                         \
                                                                              \
    inline void operator delete[] (void* ptr, size_t) {                       \
        free(ptr);                                                            \
    }                                                                         \

#endif
