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
#ifndef PXR_BASE_TF_STACKED_H
#define PXR_BASE_TF_STACKED_H

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/arch/demangle.h"

#include <tbb/enumerable_thread_specific.h>

#include <atomic>
#include <type_traits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfStackedAccess
///
/// Classes that derive \a TfStacked may befriend TfStackedAccess if they wish
/// to customize aspects \a TfStacked's behavior.  See \a TfStacked
/// documentation for more details.
///
class TfStackedAccess {
public:
    template <class Derived>
    static void InitializeStack() {
        return Derived::_InitializeStack();
    }
};

// Detail for TfStacked storage types.
template <typename T, bool PerThread>
class Tf_StackedStorageType {
public:
    typedef std::vector<T const *> Stack;

private:
    /* This is a wrapper around Stack that makes sure we call InitializeStack    */
    /* once per stack instance.                                                  */
    class _StackHolder {
    public:
        _StackHolder() : _initialized(false) { }

        Stack &Get() {
            if (!_initialized) {
                _initialized = true;
                TfStackedAccess::InitializeStack<T>();
            }
            return _stack;
        }

    private:
        Stack _stack;
        bool _initialized;
    };

    struct _PerThreadStackStorage {
        tbb::enumerable_thread_specific<_StackHolder> stack;
        Stack &Get() {
            return stack.local().Get();
        }
    };

    struct _GlobalStackStorage {
        _StackHolder stack;
        Stack &Get() {
            return stack.Get();
        }
    };

public:
    /* Choose the stack storage type based on thea PerThread argument. */
    typedef typename std::conditional<
        PerThread, _PerThreadStackStorage, _GlobalStackStorage
    >::type Type;
};

// Detail for TfStacked storage.  This exists so we can specialize it
// with exported storage.
template <typename T, bool PerThread>
struct Tf_ExportedStackedStorage { };

// Detail for TfStacked storage.  This is for the case we don't need
// exported storage.  This is the default when simply subclassing
// TfStacked without using TF_DEFINE_STACKED.
template <typename T, bool PerThread>
struct Tf_StackedStorage {
    typedef typename Tf_StackedStorageType<T, PerThread>::Stack Stack;
    typedef typename Tf_StackedStorageType<T, PerThread>::Type Type;
    static std::atomic<Type*> value;
};

/// \class TfStacked
///
/// A TfStacked is used where a class needs to keep a stack of the objects
/// currently in existence.  This class follows the CRTP and is a base class
/// that is parameterized on its derived classes.
///
/// TfStacked is thread-safe by default and each thread will get its own stack.
/// This behavior may be disabled by passing \a false for the \a PerThread
/// template parameter.
///
/// Derived classes must instantiate the stack themselves by putting
///
///   TF_INSTANTIATE_STACKED(YourStackedClass)
///
/// in a single .cpp file.
///
/// Note that \a Stacked objects that differ only by \a PerThread will not
/// share stacks.
///
template <class Derived, bool PerThread = true,
          class Holder = Tf_StackedStorage<Derived, PerThread>>
class TfStacked {
    TfStacked(TfStacked const &) = delete;
    TfStacked& operator=(TfStacked const &) = delete;
    typedef typename Holder::Type _StorageType;
public:
    typedef Holder Storage;
    typedef typename Storage::Stack Stack;

    /// Pushes this stacked object onto the stack.
    TfStacked() {
        _Push(_AsDerived());
    }

    /// Pops this stacked object from the stack.
    ~TfStacked() {
        _Pop(_AsDerived());
    }

    /// Returns the top of the stack.  If the stack is empty, returns 0.
    /// Derived classes can befriend TfStackedAccess and hide (override)
    /// \a _InitializeStack() to pre-populate the stack if desired.  This way,
    /// a stack can be made never to be empty.
    static Derived const *GetStackTop() {
        Stack const &stack = GetStack();
        return stack.empty() ? 0 : stack.back();
    }

    /// Returns the element under the top of the stack.  If the stack contains
    /// only one element, or is empty, returns 0.  Derived classes can befriend
    /// TfStackedAccess and hide (override) \a _InitializeStack() to
    /// pre-populate the stack if desired.  This way, a stack can be made never
    /// to be empty.
    static Derived const *GetStackPrevious() {
        Stack const &stack = GetStack();
        size_t size = stack.size();
        return size <= 1 ? 0 : stack[size-2];
    }

    /// Returns a const reference to the entire stack.
    static Stack const &GetStack() {
        return _GetStack();
    }
    
    /// Returns true if \a p is the top of the stack.
    static bool IsStackTop(Derived const *p) {
        return GetStackTop() == p;
    }

private:
    friend class TfStackedAccess;

    // This function may be hidden (overridden) by derived classes to initialize
    // (pre-populate) the stack with some items.  One way to do this is to
    // allocate objects on the heap, never to be freed.  By default, no
    // initialization is performed.
    static void _InitializeStack() {}

    // Push p on the stack.  Only the constructor should call this.
    static void _Push(Derived const *p) {
        _GetStack().push_back(p);
    }

    // Pop p off the stack.  Only the destructor should call this.
    static void _Pop(Derived const *p) {
        // Make sure we pop in reverse order.
        if (ARCH_LIKELY(IsStackTop(p))) {
            _GetStack().pop_back();
        } else {
            // CODE_COVERAGE_OFF
            TF_FATAL_ERROR("Destroyed %s out of stack order.",
                ArchGetDemangled<Derived>().c_str());
            // CODE_COVERAGE_ON
        }
    }

    static Stack &_GetStack() {
        // Technically unsafe double-checked lock to initialize the stack.
        if (ARCH_UNLIKELY(Storage::value.load() == nullptr)) {
            // Make a new stack and try to set it.
            _StorageType *old = nullptr;
            _StorageType *tmp = new _StorageType;
            // Attempt to set the stack.
            if (!Storage::value.compare_exchange_strong(old, tmp)) {
                // Another caller won the race.
                delete tmp;
            }
        }
        return Storage::value.load(std::memory_order_relaxed)->Get();
    }

    Derived *_AsDerived() {
        return static_cast<Derived *>(this);
    }

    Derived const *_AsDerived() const {
        return static_cast<Derived const *>(this);
    }
};

/// Define the class \p Derived that subclasses from TfStacked.
/// \p IsPerThread selected thread safety and \p eiAPI is used to export
/// the storage for the stack.  Use \c TF_INSTANTIATE_DEFINED_STACKED
/// to define the storage.
#define TF_DEFINE_STACKED(Derived, IsPerThread, eiAPI)                         \
class Derived;                                                                 \
template <>                                                                    \
struct Tf_ExportedStackedStorage<Derived, IsPerThread> {                       \
    typedef typename Tf_StackedStorageType<Derived, IsPerThread>::Stack Stack; \
    typedef typename Tf_StackedStorageType<Derived, IsPerThread>::Type Type;   \
    static eiAPI std::atomic<Type*> value;                                     \
};                                                                             \
class Derived :                                                                \
    public TfStacked<Derived, IsPerThread,                                     \
                     Tf_ExportedStackedStorage<Derived, IsPerThread>>

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_STACKED_H
