//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_MALLOC_TAG_H
#define PXR_BASE_TF_MALLOC_TAG_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/eternalString.h"
#include "pxr/base/arch/hints.h"

#include <atomic>
#include <cstdlib>
#include <cstdint>
#include <iosfwd>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
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

    // Wrapper-type marking a tag name whose characters are immortal, so that
    // TfMallocTag may key on their address and adopt them without copying.
    struct _ImmortalName {
        char const *str;
    };
    
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
            size_t nBytes,        ///< Mem allocated including descendant nodes.
                   nBytesDirect;  ///< Mem allocated excluding descendant nodes.
            size_t nAllocations;  ///< The number of allocations for this node.
            std::string siteName; ///< Tag name.
            std::vector<PathNode>
                        children; ///< Children nodes.
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

        /// Specify which parts of the report to print.
        enum PrintSetting {
            TREE = 0,                   ///< Print the full call tree
            CALLSITES,                  ///< Print just the call sites > 0.1%
            BOTH                        ///< Print both tree and call sites
        };


        /// \name Input/Output
        /// @{

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
        /// If \p rootName is non-empty it will replace the name of the tree
        /// root in the report.
        TF_API
        void Report(
            std::ostream &out,
            const std::string &rootName) const;

        /// Generates a report to the ostream \p out.
        TF_API
        void Report(
            std::ostream &out) const;

        /// Load the contents of \p in into the root of the call tree.
        ///
        /// Returns true if the report loaded successfully, false otherwise.
        TF_API
        bool LoadReport(
            std::istream &in);

        /// @}


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
    /// This function returns \c true if the memory tagging system is
    /// initialized successfully, either by this call or a prior
    /// call. Otherwise, it returns \c false and \p *errMsg is set with an
    /// explanation for the failure if \p errMsg is not null.
    ///
    /// Until the system is initialized, the various memory reporting calls will
    /// indicate that no memory has been allocated.  Note also that memory
    /// allocated prior to calling \c Initialize() is not tracked i.e.  all data
    /// refers to allocations that happen subsequent to calling \c Initialize().
    TF_API static bool Initialize(std::string* errMsg = nullptr);

    /// Shutdown the memory tagging system.
    ///
    /// If the system is not initialized, do nothing and return.  Otherwise shut
    /// down the malloc tagging system.  Clear all recorded events and discard
    /// any in-flight event data.  After Shutdown(), performance overhead
    /// associated with the system returns to the lowest possible level and
    /// should be negligible.  After a call to Shutdown(), call Initialize() to
    /// begin tracking allocations again.
    TF_API static void Shutdown();

    /// Return true if the tagging system is active.
    ///
    /// If \c Initialize() has been successfully called, this function returns
    /// \c true.  Note that racing calls to Shutdown() or Initialize() can
    /// immediately obsolete the returned result.
    static inline bool IsInitialized() {
        return TfMallocTag::_initState
            .load(std::memory_order_acquire) == _Initialized;
    }

    /// Clear all recorded allocation events, in-flight events from all threads,
    /// and reset the total bytes and max total bytes counters.  Note that due
    /// to concurrent activity, by the time this function returns new allocation
    /// and free activity may have already occurred.
    TF_API static void Clear();

    /// Return total number of allocated bytes.
    ///
    /// The current total memory that has been allocated and not freed is
    /// returned. Memory allocated before calling \c Initialize() is not
    /// accounted for.
    ///
    /// This call brings accounting fully up to date first, which requires
    /// briefly stopping every thread that is recording allocations.  It is not
    /// a lightweight query; do not poll it.
    TF_API static size_t GetTotalBytes();

    /// Return an estimate of the maximum total number of bytes that have ever
    /// been allocated at one time.
    ///
    /// This is a high-water mark sampled during internal event consolidation
    /// and also at a regular timer interval as a backstop during periods of low
    /// allocator activity.  A background thread consolidates on every thread
    /// event buffer spill, so an actively-allocating thread's usage is
    /// reflected promptly.  A less-actively-allocating thread is sampled at
    /// regular intervals defined by the PXR_TF_MALLOC_TAG_CONSOLIDATE_PERIOD_MS
    /// env setting.  Change this value to increase or decrease the sampling
    /// interval.
    ///
    /// The value is fundamentally an estimate.  It can be either an
    /// underestimate or an overestimate.  A peak is captured only when the
    /// allocations composing it reach the background baseline together.  The
    /// residual limit is a peak that both forms and dissolves within a single
    /// sampling inteval.  Even without sampling, there is no global total
    /// ordering of heap allocations in a multithreaded program in general, so
    /// therefore there is no well-defined high-water mark in general either.
    ///
    /// Carries the same cost as GetTotalBytes().  Do not poll.
    TF_API static size_t GetMaxTotalBytes();

    /// Return a snapshot of memory usage.
    ///
    /// Returns a snapshot by writing into \c *tree.  See the \c CallTree
    /// structure for documentation.  If \c Initialize() has not been called, \p
    /// *tree is set to a rather blank structure (empty vectors, empty strings,
    /// zero in all integral fields) and \c false is returned; otherwise, \p
    /// *tree is set with the contents of the current memory snapshot and \c
    /// true is returned. It is fine to call this function on the same \p *tree
    /// instance; each call simply overwrites the data from the last call. If \p
    /// skipRepeated is \c true, then any repeated callsite is skipped. See the
    /// \c CallTree documentation for more details.
    ///
    /// Like GetTotalBytes(), this brings accounting fully up to date first,
    /// which requires briefly stopping every thread that is recording
    /// allocations, and then builds the tree.  Treat it as a profiling
    /// operation rather than a query.
    TF_API static bool GetCallTree(CallTree* tree, bool skipRepeated = true);

    /// Return a report of diagnostic information related to the performance of
    /// the TfMallocTag tracking internals.  This is likely only to be of
    /// interest to TfMallocTag's developers.
    TF_API static std::string GetPerfStats(bool includePerThread=false);

private:

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
    /// A \c TfAutoMallocTag object is used to push memory tags onto the current
    /// call stack; destruction of the object pops the tags.  Note that each
    /// thread has its own tag-stack.
    ///
    /// There is very little cost to creating or destroying memory tags if \c
    /// TfMallocTag::Initialize() has not been called: an inline read of a
    /// global variable and a branch.  If tagging has been initialized, then
    /// there is a small cost associated with pushing and popping memory tags on
    /// the local stack.  Pushing a name whose characters are immortal, like a
    /// string literal or a \c TfEternalString, normally takes no lock at all.
    /// Pushing and popping a tag without allocating anything under it does no
    /// table lookup whatsoever.  Popping never takes a lock.  Pushing or
    /// popping the call stack does not actually cause any memory allocation
    /// unless this is the first time that the given named tag is encountered.
    class Auto {
    public:
        Auto(const Auto &) = delete;
        Auto& operator=(const Auto &) = delete;

        Auto(Auto &&) = delete;
        Auto& operator=(Auto &&) = delete;

        /// Push one or more memory tags onto the local-call stack with names \p
        /// name1 ... \p nameN.  The passed names should be string literals, \c
        /// TfEternalString objects, const char pointers, or std::strings.  A \c
        /// TfEternalString (as \c TF_FUNC_NAME() returns) is treated exactly as
        /// a string literal is, since its characters are immortal too.
        ///
        /// If \c TfMallocTag::Initialize() has not been called, this
        /// constructor does essentially no work, assuming the names are string
        /// literals, \c TfEternalStrings, or a pointer to an existing c-string.
        /// However if any of the names are expressions that evaluate to \c
        /// std::string objects, the work done constructing those strings will
        /// still be incurred.  If this is an issue, you can query \c
        /// TfMallocTag::IsInitialized() to avoid unneeded work when tagging is
        /// inactive.
        ///
        /// Objects of this class should only be created as local variables;
        /// never as member variables, global variables, or via \c new.  If
        /// you can't create your object as a local variable, you can make
        /// manual calls to \c TfMallocTag::Push() and \c TfMallocTag::Pop(),
        /// though you should do this only as a last resort.
        template <class Str, class... Strs>
        explicit Auto(Str &&name1, Strs &&... nameN)
            : _threadData(
                TfMallocTag::_Push(_TagName(std::forward<Str>(name1))))
            , _nTags(0) {
            if (_threadData) {
                // Accumulate _nTags; _Begin will not push null or empty tags.
                _nTags = 1;
                (..., (_nTags += (TfMallocTag::_Begin(
                                      _TagName(std::forward<Strs>(nameN)),
                                      _threadData) ? 1 : 0)));
            }
        }

        /// Pop the tag from the stack before it is destructed.
        ///
        /// Normally you should not use this.  The normal destructor is
        /// preferable because it ensures proper release order.  If you call
        /// \c Release(), make sure all tags are released in the opposite
        /// order they were declared in.  It is better to use sub-scopes to
        /// control the life span of tags, but if that won't work, \c
        /// Release() is still preferable to \c TfMallocTag::Push() and \c
        /// TfMallocTag::Pop() because it isn't vulnerable to early returns or
        /// exceptions.
        inline void Release() {
            if (_threadData) {
                TfMallocTag::_End(_nTags, _threadData);
                _threadData = nullptr;
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

        // Is this tag argument a char array whose address we may treat as
        // immortal -- a string literal, or a static const char array?
        //
        // What we can actually test for is a const char array lvalue.  It is as
        // narrow as we can make it and deliberately not "any char array".
        // Unfortunately this can still accept objects that are not literals --
        // a `const char buf[N]` on the stack also passes, and no C++17
        // construct can tell that from a literal.  So this is the best we can
        // do, and remains a contract on callers rather than a check: a name
        // passed as a const char array must live as long as the program.
        template <class Str>
        static constexpr bool _IsImmortalCharArray =
            std::is_lvalue_reference_v<Str> &&
            std::is_array_v<std::remove_reference_t<Str>> &&
            std::is_const_v<
                std::remove_extent_t<std::remove_reference_t<Str>>>;

        template <class Str>
        static auto _TagName(Str &&name) {
            if constexpr (_IsImmortalCharArray<Str &&>) {
                return _ImmortalName { name };
            }
            else {
                return _TagNameNotArray(std::forward<Str>(name));
            }
        }
        // A TfEternalString's characters are immortal (and content-unique) so
        // route it exactly as a literal.  This exact match beats the
        // std::string const & overload below, which TfEternalString's implicit
        // conversion would otherwise reach.
        static _ImmortalName _TagNameNotArray(TfEternalString s) {
            return _ImmortalName { s.c_str() };
        }
        static char const *_TagNameNotArray(char const *cstr) { return cstr; }
        static char const *_TagNameNotArray(std::string const &str) {
            // Borrowed, not owned.  `str` may be a temporary in which case this
            // reference dangles and the c_str() pointer is invalidated at the
            // end of the full-expression that called us.  Both callers above
            // resolve the tag within that same full-expression, which makes
            // this safe.
            return str.c_str();
        }

        _ThreadData* _threadData;
        int _nTags;

        friend class TfMallocTag;
    };

    // An historical compatibility: in prior versions, Auto could accept only
    // one argument, so Auto2 existed to handle two arguments.  Now Auto can
    // accept any number of arguments, so Auto2 is just an alias for Auto.
    using Auto2 = Auto;

    // fwd for friendship.
    class StackOverride;

    /// \class StackState
    /// \ingroup group_tf_MallocTag
    ///
    /// An object that represents a snapshot of a thread's TfMallocTag stack
    /// state.  See TfMallocTag::GetCurrentStackState()
    ///
    class StackState {
    public:
        StackState() = default;

    private:
        explicit StackState(Tf_MallocPathNode *top) : _top(top) {}

        Tf_MallocPathNode *_top = nullptr;

        friend class TfMallocTag;
        friend class StackOverride;
    };

    /// \class StackOverride
    /// \ingroup group_tf_MallocTag
    ///
    /// Scoped (i.e. local) object for temporarily replacing the current
    /// thread's tag stack with a different StackState.
    ///
    /// This is typically used to bridge tag stacks in parallelism contexts.
    /// For example, a call site that spawns parallel tasks may capture its own
    /// tag stack state and pass it to the parallel tasks, that then use a
    /// StackOverride to ensure that memory allocations are billed to the same
    /// tags as the spawning thread.  When a StackOverride object is destroyed,
    /// the thread's previous tag stack is restored.
    class StackOverride {
    public:
        explicit StackOverride(StackState state)
            : _state(state)
            , _tls(state._top ? _Push() : nullptr) {}

        // Noncopyable, nonmovable.
        StackOverride(const StackOverride &) = delete;
        StackOverride &operator=(const StackOverride &) = delete;

        ~StackOverride() {
            if (ARCH_UNLIKELY(_tls)) {
                _Pop();
            }
        }
        
    private:
        TF_API _ThreadData *_Push() const;
        TF_API void _Pop() const;
    
        const StackState _state;
        _ThreadData *_tls;
    
        friend class TfMallocTag;
    };

    /// \class TfMallocTag::PauseControl
    ///
    /// Debugging-oriented RAII object that pauses malloc tag collection for
    /// either the current thread or all threads.  Construct via the TfMallocTag
    /// factory functions PauseThisThread(), PauseAllThreads(), or
    /// DeferredPause().  Pausing nests and a nested PauseControl cannot
    /// override a pause established by an enclosing one.
    ///
    /// A PauseControl that is paused records allocations with size=0 so they do
    /// not contribute to reported memory usage.
    ///
    /// A PauseControl object must be unpaused by the same thread that paused
    /// it.
    ///
    /// \note Use this only to limit collection to areas of interest for
    /// debugging and investigation.  Committed code that pauses memory tracking
    /// forever hides its allocations from TfMallocTag's view.
    ///
    class PauseControl {
    public:
        PauseControl(PauseControl const &) = delete;
        PauseControl &operator=(PauseControl const &) = delete;

        /// Move-construct from `other`, adopting its pause state and leaving it
        /// unpaused.
        PauseControl(PauseControl &&other) noexcept
            : _td(std::exchange(other._td, nullptr))
            , _scope(std::exchange(other._scope, _NotPaused)) {}

        /// Move-assign from `other`, adopting its pause state and leaving it
        /// unpaused.
        PauseControl &operator=(PauseControl &&other) noexcept {
            if (this != &other) {
                if (IsPaused()) {
                    Unpause();
                }
                _td = std::exchange(other._td, nullptr);
                _scope = std::exchange(other._scope, _NotPaused);
            }
            return *this;
        }

        /// Unpause if paused.
        ~PauseControl() {
            Unpause();
        }

        /// Pause collection on the current thread.  If this PauseControl
        /// IsPaused(), do nothing.
        void PauseThisThread() {
            if (IsPaused()) {
                return;
            }
            _Pause(_ThisThread);
        }

        /// Pause collection on all threads.  If this PauseControl IsPaused(),
        /// do nothing.
        void PauseAllThreads() {
            if (IsPaused()) {
                return;
            }
            _Pause(_AllThreads);
        }

        /// Resume collection.  This PauseControl must currently be paused.
        void Unpause() {
            if (IsPaused()) {
                _Unpause();
            }
        }

        /// Return true if this PauseControl is currently paused.
        bool IsPaused() const {
            return _scope != _NotPaused;
        }

    private:
        enum _Scope {
            _NotPaused, _ThisThread, _AllThreads
        };

        // Only constructible via TfMallocTag factory functions.
        explicit PauseControl(_Scope desired) {
            if (desired != _NotPaused) {
                _Pause(desired);
            }
        }

        TF_API
        void _Pause(_Scope desired);

        TF_API
        void _Unpause();

        _ThreadData *_td    = nullptr;
        _Scope       _scope = _NotPaused;

        friend class TfMallocTag;
    };
        
    /// Return a PauseControl that immediately pauses collection for the
    /// calling thread.
    [[nodiscard]] static PauseControl PauseThisThread() {
        return PauseControl(PauseControl::_ThisThread);
    }

    /// Return a PauseControl that immediately pauses collection for all
    /// threads.
    [[nodiscard]] static PauseControl PauseAllThreads() {
        return PauseControl(PauseControl::_AllThreads);
    }

    /// Return a PauseControl in the unpaused state, for later manual
    /// control via PauseThisThread() or PauseAllThreads().
    [[nodiscard]] static PauseControl DeferredPause() {
        return PauseControl(PauseControl::_NotPaused);
    }    
    

    /// Capture the current thread's TfMallocTag stack state and return it.
    /// Later, construct a TfMallocTag::StackOverride with a
    /// TfMallocTag::StackState to temporarily override the current thread's tag
    /// stack with the captured stack state.  This is especially useful to
    /// bridge allocations in scoped parallel tasks (that may be executed by
    /// worker threads) back to the initiating context.
    static StackState GetCurrentStackState() {
        return ARCH_UNLIKELY(TfMallocTag::IsInitialized())
            ? _GetCurrentStackState() : StackState {};
    }

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
        _Push(name.c_str());
    }

    /// \overload
    static void Push(const char* name) {
        _Push(name);
    }

    /// \overload
    static void Push(TfEternalString name) {
        _Push(_ImmortalName { name.c_str() });
    }

    /// Manually pop a tag from the stack.
    ///
    /// This call has the same effect as the destructor for \c
    /// TfMallocTag::Auto; it must properly nest with a matching call to \c
    /// Push(), of course.
    ///
    /// Note that unlike \c TfAutoMallocTag, this API cannot automatically
    /// ensure that a matching \c Push() occurred, so an unbalanced \c Pop()
    /// issues a coding error.
    static void Pop() {
        if (TfMallocTag::IsInitialized()) {
            _PopChecked();
        }
    }

    /// Sets the tags to trap in the debugger.
    ///
    /// When memory is allocated for any tag that matches \p matchList the
    /// debugger trap is invoked. If a debugger is attached the program will
    /// stop in the debugger, otherwise the program will continue to run. See \c
    /// ArchDebuggerTrap() and \c ArchDebuggerWait().
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
    TF_API static void
    SetCapturedMallocStacksMatchList(const std::string& matchList);

    /// Returns the captured malloc stack traces for allocations billed to the
    /// malloc tags passed to SetCapturedMallocStacksMatchList().
    ///
    /// \note This method also clears the internally held set of captured
    /// stacks.
    TF_API static std::vector<std::vector<uintptr_t> > GetCapturedMallocStacks();

private:
    // Atomic initialization state -- valid transitions are from N -> N+1
    // cyclically, and from _Initializing -> _NotInitialized if initialization
    // fails.
    enum _InitState {
        _NotInitialized,
        _Initializing,
        _Initialized,
        _ShuttingDown
    };
    
    friend struct _TemporaryDisabler;

    friend struct Tf_MallocGlobalData;

    static inline _ThreadData *_Push(char const *name) {
        if (TfMallocTag::IsInitialized()) {
            return _Begin(name);
        }
        return nullptr;
    }
    static inline _ThreadData *_Push(_ImmortalName name) {
        if (TfMallocTag::IsInitialized()) {
            return _Begin(name);
        }
        return nullptr;
    }

    TF_API static _ThreadData *
    _Begin(char const *name, _ThreadData *threadData = nullptr);

    TF_API static _ThreadData *
    _Begin(_ImmortalName name, _ThreadData *threadData = nullptr);
    
    // The Auto/StackOverride exit path.  `threadData` must be non-null and
    // `nTags` must match the number of pushes that were actually performed;
    // both hold by construction at the only call site (Auto::Release()).
    TF_API static void _End(int nTags, _ThreadData *threadData);

    // The manual Pop() path, which tolerates an unbalanced pop.  Kept separate
    // from _End() so that the Auto path pays nothing for the check.
    TF_API static void _PopChecked();

    TF_API static StackState _GetCurrentStackState();
    
    static void* _MallocWrapper(size_t, const void*);
    static void* _ReallocWrapper(void*, size_t, const void*);
    static void* _MemalignWrapper(size_t, size_t, const void*);
    static void  _FreeWrapper(void*, const void*);

    friend class TfMallocTag::Auto;
    class Tls;
    friend class TfMallocTag::Tls;
    TF_API static std::atomic<_InitState> _initState;
};

/// Top-down memory tagging system.
using TfAutoMallocTag = TfMallocTag::Auto;

/// Top-down memory tagging system.
using TfAutoMallocTag2 = TfMallocTag::Auto;

/// Enable lib/tf memory management.
///
/// Invoking this macro inside a class body causes the class operator \c new to
/// push malloc tags onto the stack before actually allocating memory for the
/// class.  You can pass as many tags as you like: they forward to the
/// TfAutoMallocTag constructor.  For example,
/// \code
/// class MyBigMeshVertex {
/// public:
///     TF_MALLOC_TAG_NEW("MyBigMesh", "Vertex");
///     ...
/// }
/// \endcode
/// will cause dynamic allocations of \c MyBigMeshVertex to be billed to the tag
/// \c Vertex which grouped under \c MyBigMesh.  However,
/// \code
/// class MyBigMesh {
/// public:
///     TF_MALLOC_TAG_NEW("MyBigMesh");
///     ...
/// }
/// \endcode
/// specifies only the single tag \c MyBigMesh.
///
/// Normally, this macro should be placed in the public section of a class.
///
/// Also, note that instances of such a class created inside an STL data
/// structure may not be grouped under the indicated tags.
/// \remark Placed in .h files.
///
/// \hideinitializer
//
PXR_NAMESPACE_CLOSE_SCOPE                                                 

#define TF_MALLOC_TAG_NEW(...)                                                \
    /* this is for STL purposes */                                            \
    ARCH_ALWAYS_INLINE inline void* operator new(::std::size_t, void* ptr) {  \
        return ptr;                                                           \
    }                                                                         \
                                                                              \
    ARCH_ALWAYS_INLINE inline void* operator new(::std::size_t s) {           \
        PXR_NS::TfAutoMallocTag tag(__VA_ARGS__);                             \
        return ::operator new(s, std::nothrow);                               \
    }                                                                         \
                                                                              \
    ARCH_ALWAYS_INLINE inline void* operator new[](::std::size_t s) {         \
        PXR_NS::TfAutoMallocTag tag(__VA_ARGS__);                             \
        return ::operator new[](s, std::nothrow);                             \
    }                                                                         \
                                                                              \
    /* Required due to the placement-new override above. */                   \
    ARCH_ALWAYS_INLINE inline void operator delete(void* ptr, void* place) {} \
                                                                              \
    ARCH_ALWAYS_INLINE inline void operator delete(void* ptr, size_t) {       \
        ::operator delete(ptr, std::nothrow);                                 \
    }                                                                         \
                                                                              \
    ARCH_ALWAYS_INLINE inline void operator delete[] (void* ptr, size_t) {    \
        ::operator delete[](ptr, std::nothrow);                               \
    }                                                                         \

#endif
