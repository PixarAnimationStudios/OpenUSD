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

#include "pxr/pxr.h"

#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/scopeDescriptionPrivate.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"

#include "pxr/base/arch/threads.h"

#include <tbb/spin_mutex.h>
#include <tbb/enumerable_thread_specific.h>

#include <algorithm>
#include <chrono>
#include <thread>

using std::vector;
using std::string;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// A 2 MB buffer for generating the crash message, and a lock.
tbb::spin_mutex messageMutex;
constexpr size_t MaxMessageBytes = 2*1024*1024;
char message[MaxMessageBytes];

// Called by Tf's _fatalSignalHandler in diagnostic.cpp.  This code must be
// async signal safe!  It must not invoke the heap allocator or anything like
// that.
struct _MessageWriter
{
    _MessageWriter() : cur(message), end(message + MaxMessageBytes - 1) {}

    inline void Write(char const *txt) {
        while (*txt && cur != end) {
            *cur++ = *txt++;
        }
        *cur = '\0';
    }

    inline void Write(size_t num) {
        if (cur == end)
            return;
        char *start = cur;
        do {
            size_t dig = num % 10;
            *cur++ = '0' + dig;
            num /= 10;
        } while (num && cur != end);
        std::reverse(start, cur);
        *cur = '\0';
    }
    
    char *cur, *end;
};

struct _Stack;

// Collects all present description stacks together.
struct _StackRegistry
{
    // Lock class used by clients that read stacks.
    friend class StackLock;
    class StackLock {
        friend struct _StackRegistry;
        StackLock(StackLock const &) = delete;
        StackLock &operator=(StackLock const &) = delete;
        inline StackLock(_Stack *stack, _StackRegistry *reg)
            : _stack(stack), _reg(reg) {}
        inline StackLock() : _stack(nullptr), _reg(nullptr) {}
    public:
        inline StackLock(StackLock &&o)
            : _stack(o._stack), _reg(o._reg) {
            o._stack = nullptr;
            o._reg = nullptr;
        }
        inline StackLock &operator=(StackLock &&o) {
            _stack = o._stack;
            _reg = o._reg;
            o._stack = nullptr;
            o._reg = nullptr;
            return *this;
        }
        inline ~StackLock() {
            if (_reg) {
                _reg->_UnlockThread();
            }
        }
        // Provide access to the requested stack.
        inline _Stack *Get() const {
            return _stack;
        }
    private:
        _Stack *_stack;
        _StackRegistry *_reg;
    };

    struct _StackEntry {
        std::thread::id id;
        std::string idStr;
        _Stack *stack;
    };

    // Register \p stack as the stack for \p id.
    void Add(std::thread::id id, _Stack *stack) {
        tbb::spin_mutex::scoped_lock lock(_stacksMutex);
        _stacks.push_back({ id, TfStringify(id), stack });
    }
    // Remove \p stack from the registry.
    void Remove(_Stack *stack) {
        tbb::spin_mutex::scoped_lock lock(_stacksMutex);
        auto it = std::find_if(
            _stacks.begin(), _stacks.end(),
            [stack](_StackEntry const &e) {
                return e.stack == stack;
            });
        TF_AXIOM(it != _stacks.end());
        std::swap(*it, _stacks.back());
        _stacks.pop_back();
    }
    // Acquire a lock on the registry and obtain a pointer to the stack
    // associated with \p id if one exists, otherwise return a StackLock with a
    // nullptr stack.
    StackLock LockThread(std::thread::id id) {
        _stacksMutex.lock();
        auto it = std::find_if(
            _stacks.begin(), _stacks.end(),
            [id](_StackEntry const &e) {
                return e.id == id;
            });
        return StackLock(it == _stacks.end() ? nullptr : it->stack, this);
    }    
private:
    // Give access to the crash reporter facility.
    friend char const *_ComputeAndLockScopeDescriptionStackMsg();
    
    void _UnlockThread() {
        _stacksMutex.unlock();
    }
    
    tbb::spin_mutex _stacksMutex;
    std::vector<_StackEntry> _stacks;
};

// Obtain the registry singleton instance.
static _StackRegistry &GetRegistry() {
    static _StackRegistry *theRegistry = new _StackRegistry;
    return *theRegistry;
}

// Per thread description stack representation.
struct _Stack
{
    // Add this stack to the registry.
    _Stack() : head(nullptr) {
        GetRegistry().Add(std::this_thread::get_id(), this);
    }
    // Remove this stack from the registry.
    ~_Stack() {
        GetRegistry().Remove(this);
    }
    // The head of the description stack (nullptr if empty);
    TfScopeDescription *head;
    // Thread-local lock for this stack.
    tbb::spin_mutex mutex;
};

// Force this out-of-line since modern compilers generate way too many calls to
// __tls_get_addr even if you cache a pointer to the thread_local yourself.
static tbb::enumerable_thread_specific<_Stack> _tlStack;
static _Stack &_GetLocalStack() ARCH_NOINLINE;
static _Stack &_GetLocalStack()
{
    return _tlStack.local();
}

static bool _TimedTryAcquire(tbb::spin_mutex::scoped_lock &lock,
                             tbb::spin_mutex &mutex,
                             int msecToTry = 10)
{
    if (lock.try_acquire(mutex))
        return true;

    auto start = std::chrono::high_resolution_clock::now();
    int msec = 0;
    do {
        std::this_thread::yield();
        if (lock.try_acquire(mutex))
            return true;
        msec = std::chrono::duration_cast<
            std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - start).count();
    } while (msec < msecToTry);
    return false;
}

char const *
_ComputeAndLockScopeDescriptionStackMsg()
{
    // Lock the message mutex.
    messageMutex.lock();
    
    _MessageWriter writer;

    // Try to lock the _StackRegistry mutex -- if we fail, bail.
    auto &reg = GetRegistry();
    tbb::spin_mutex::scoped_lock regLock;
    if (!_TimedTryAcquire(regLock, reg._stacksMutex)) {
        writer.Write("Error: cannot generate TfScopeDescription stacks - "
                     "failed to acquire lock on stack registry mutex.\n");
        return message;
    }

    // Now collect up as many entries as we can and sort.  This is number of
    // *stacks*, like number of threads -- not stack depth.
    constexpr size_t MaxStackEntries = 1024;
    _StackRegistry::_StackEntry *entries[MaxStackEntries];
    size_t numEntries = std::min(MaxStackEntries, reg._stacks.size());
    for (size_t i = 0; i != numEntries; ++i) {
        entries[i] = &reg._stacks[i];
    }
    // Sort -- always sort "main thread" first.
    std::thread::id mainThreadId = ArchGetMainThreadId();
    std::sort(
        entries, entries + numEntries,
        [mainThreadId](_StackRegistry::_StackEntry *l,
                       _StackRegistry::_StackEntry *r) {
            if (l->id == r->id)
                return false;
            if (l->id == mainThreadId)
                return true;
            if (r->id == mainThreadId)
                return false;
            return l->id < r->id;
        });

    // Now try to generate text for each of them.
    for (size_t i = 0; i != numEntries; ++i) {
        _StackRegistry::_StackEntry *e = entries[i];
        // Try to lock this entry's mutex, if we fail or if empty stack, bail.
        tbb::spin_mutex::scoped_lock stackLock;
        if (!_TimedTryAcquire(stackLock, e->stack->mutex)) {
            writer.Write("Error: cannot write TfScopeDescription stack "
                         "for thread ");
            writer.Write(e->idStr.c_str());
            writer.Write(" - failed to acquire stack lock.\n\n");
        }
        if (!e->stack->head) {
            continue;
        }

        writer.Write("Thread ");
        writer.Write(e->idStr.c_str());
        if (e->id == ArchGetMainThreadId()) {
            writer.Write(" (main)");
        }
        writer.Write(" Scope Descriptions\n");

        size_t frame = 1;
        TfScopeDescription *curScope = e->stack->head;
        while (curScope) {
            writer.Write("#");
            writer.Write(frame++);
            writer.Write(" ");
            writer.Write(Tf_GetScopeDescriptionText(curScope));
            if (TfCallContext const &ctx =
                Tf_GetScopeDescriptionContext(curScope)) {
                writer.Write(" (from "); writer.Write(ctx.GetFunction());
                writer.Write(" in "); writer.Write(ctx.GetFile());
                writer.Write("#"); writer.Write(ctx.GetLine());
                writer.Write(")");
            }
            writer.Write("\n");
            curScope = Tf_GetPreviousScopeDescription(curScope);
        }
        writer.Write("\n");
    }
    return message;
}

} // anon

void
TfScopeDescription::_Push()
{
    // No other thread can modify head, so we can read it without the lock
    // safely here.
    _Stack &stack = _GetLocalStack();
    _prev = stack.head;
    _localStack = &stack;
    tbb::spin_mutex::scoped_lock lock(stack.mutex);
    stack.head = this;
}

void
TfScopeDescription::_Pop() const
{
    // No other thread can modify head, so we can read it without the lock
    // safely here.
    _Stack &stack = *static_cast<_Stack *>(_localStack);
    TF_AXIOM(stack.head == this);
    tbb::spin_mutex::scoped_lock lock(stack.mutex);
    stack.head = _prev;
}

TfScopeDescription::TfScopeDescription(
    std::string const &msg, TfCallContext const &ctx)
    : _description(msg.c_str())
    , _context(ctx)
{
    _Push();
}

TfScopeDescription::TfScopeDescription(
    std::string &&msg, TfCallContext const &ctx)
    : _ownedString(std::move(msg))
    , _description(_ownedString->c_str())
    , _context(ctx)
{
    _Push();
}

TfScopeDescription::TfScopeDescription(
    char const *msg, TfCallContext const &ctx)
    : _description(msg)
    , _context(ctx)
{
    _Push();
}

TfScopeDescription::~TfScopeDescription()
{
    _Pop();
}

void
TfScopeDescription::SetDescription(std::string const &msg)
{
    _Stack &stack = *static_cast<_Stack *>(_localStack);
    {
        tbb::spin_mutex::scoped_lock lock(stack.mutex);
        _description = msg.c_str();
    }
    _ownedString = boost::none;
}

void
TfScopeDescription::SetDescription(std::string &&msg)
{
    _Stack &stack = *static_cast<_Stack *>(_localStack);
    tbb::spin_mutex::scoped_lock lock(stack.mutex);
    _ownedString = std::move(msg);
    _description = _ownedString->c_str();
}

void
TfScopeDescription::SetDescription(char const *msg)
{
    _Stack &stack = *static_cast<_Stack *>(_localStack);
    {
        tbb::spin_mutex::scoped_lock lock(stack.mutex);
        _description = msg;
    }
    _ownedString = boost::none;
}

static
std::vector<std::string>
_GetScopeDescriptionStack(std::thread::id id)
{
    std::vector<std::string> result;
    {
        auto lock = GetRegistry().LockThread(id);
        if (_Stack *stack = lock.Get()) {
            tbb::spin_mutex::scoped_lock lock(stack->mutex);
            for (TfScopeDescription *cur = stack->head; cur;
                 cur = Tf_GetPreviousScopeDescription(cur)) {
                result.emplace_back(Tf_GetScopeDescriptionText(cur));
            }
        }
    }
    // Callers expect most recent called last.
    std::reverse(result.begin(), result.end());
    return result;
}

std::vector<std::string>
TfGetCurrentScopeDescriptionStack()
{
    return _GetScopeDescriptionStack(ArchGetMainThreadId());
}

vector<string>
TfGetThisThreadScopeDescriptionStack()
{
    return _GetScopeDescriptionStack(std::this_thread::get_id());
}

// From scopeDescriptionPrivate.h

Tf_ScopeDescriptionStackReportLock::Tf_ScopeDescriptionStackReportLock()
    : _msg(_ComputeAndLockScopeDescriptionStackMsg())
{
}

Tf_ScopeDescriptionStackReportLock::~Tf_ScopeDescriptionStackReportLock()
{
    messageMutex.unlock();
}

PXR_NAMESPACE_CLOSE_SCOPE
