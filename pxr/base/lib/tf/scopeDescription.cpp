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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"

#include "pxr/base/arch/threads.h"

#include <tbb/spin_mutex.h>

#include <thread>

using std::vector;
using std::string;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

struct _Stack;

// Collects all present description stacks together.
struct _StackRegistry
{
    // Lock class used by clients that read stacks.
    friend class StackLock;
    class StackLock {
        friend class _StackRegistry;
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

    // Register \p stack as the stack for \p id.
    void Add(std::thread::id id, _Stack *stack) {
        tbb::spin_mutex::scoped_lock lock(_stacksMutex);
        _stacks.emplace_back(id, stack);
    }
    // Remove \p stack from the registry.
    void Remove(_Stack *stack) {
        tbb::spin_mutex::scoped_lock lock(_stacksMutex);
        auto it = std::find_if(
            _stacks.begin(), _stacks.end(),
            [stack](std::pair<std::thread::id, _Stack *> const &p) {
                return p.second == stack;
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
            [id](std::pair<std::thread::id, _Stack *> const &p) {
                return p.first == id;
            });
        return StackLock(it == _stacks.end() ? nullptr : it->second, this);
    }    
private:
    void _UnlockThread() {
        _stacksMutex.unlock();
    }
    
    tbb::spin_mutex _stacksMutex;
    std::vector<std::pair<std::thread::id, _Stack *>> _stacks;
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
static thread_local _Stack _tlStack;
static _Stack &_GetLocalStack() ARCH_NOINLINE;
static _Stack &_GetLocalStack()
{
    return _tlStack;
}

} // anon

void
TfScopeDescription::_Push()
{
    // No other thread can modify head, so we can read it without the lock
    // safely here.
    _Stack &stack = _GetLocalStack();
    _prev = stack.head;
    tbb::spin_mutex::scoped_lock lock(stack.mutex);
    stack.head = this;
}

void
TfScopeDescription::_Pop() const
{
    // No other thread can modify head, so we can read it without the lock
    // safely here.
    _Stack &stack = _GetLocalStack();
    TF_AXIOM(stack.head == this);
    tbb::spin_mutex::scoped_lock lock(stack.mutex);
    stack.head = _prev;
}

TfScopeDescription::TfScopeDescription(std::string const &msg)
    : _description(msg.c_str())
{
    _Push();
}

TfScopeDescription::TfScopeDescription(std::string &&msg)
    : _ownedString(std::move(msg))
    , _description(_ownedString->c_str())
{
    _Push();
}

TfScopeDescription::TfScopeDescription(char const *msg)
    : _description(msg)
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
    _Stack &stack = _GetLocalStack();
    {
        tbb::spin_mutex::scoped_lock lock(stack.mutex);
        _description = msg.c_str();
    }
    _ownedString = boost::none;
}

void
TfScopeDescription::SetDescription(std::string &&msg)
{
    _Stack &stack = _GetLocalStack();
    tbb::spin_mutex::scoped_lock lock(stack.mutex);
    _ownedString = std::move(msg);
    _description = _ownedString->c_str();
}

void
TfScopeDescription::SetDescription(char const *msg)
{
    _Stack &stack = _GetLocalStack();
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

PXR_NAMESPACE_CLOSE_SCOPE
