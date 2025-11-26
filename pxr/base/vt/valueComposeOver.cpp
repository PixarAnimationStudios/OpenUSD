//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/vt/valueComposeOver.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/valueRef.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/spinRWMutex.h"

#include <typeindex>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// A named pair of type_index, used to key compose-over registrations.
struct Types
{
    Types(std::type_index const &s,
          std::type_index const &w) : strong(s), weak(w) {}

    template <class HashState>
    friend void TfHashAppend(HashState &h, Types const &t) {
        h.Append(t.strong.hash_code(), t.weak.hash_code());
    }

    friend bool operator==(Types const &lhs, Types const &rhs) {
        return lhs.strong == rhs.strong && lhs.weak == rhs.weak;
    }
    
    std::type_index strong;
    std::type_index weak;
};

using VoidFn = void (*)();
using CompFn = VtValue (*)(VtValueRef strong, VtValueRef weak, VoidFn);

// A pair of function pointers, representing a compose-over registration.  The
// function-call operator trampolines to the type-specific function.
struct ComposeFunc
{
    ComposeFunc(VoidFn vfn, CompFn cfn) : voidFn(vfn), compFn(cfn) {}

    VtValue operator()(VtValueRef stronger, VtValueRef weaker) const {
        return compFn(stronger, weaker, voidFn);
    }

    VoidFn voidFn;
    CompFn compFn;
};

// The singleton registry of compose-over functions.
struct Vt_ValueComposeRegistry
{
    // Look for a ComposeFunc for strong over weak and return a pointer to it if
    // found, else return nullptr.
    ComposeFunc const *
    Find(std::type_index const &strong,
         std::type_index const &weak) const {
        TfSpinRWMutex::ScopedLock lock(_rwMutex, /*write=*/false);
        // It's safe to return the address of the value here since the container
        // is an unordered_map and we never erase registrations.
        auto iter = _composeFns.find({ strong, weak });
        return iter == _composeFns.end() ? nullptr : &iter->second;
    }

    // Attempt to register a compose function for strong over weak.  If a
    // registration already exists, ignore this registration and return false.
    bool
    Register(std::type_index const &strong,
             std::type_index const &weak,
             VoidFn vfn, CompFn cfn) {
        TfSpinRWMutex::ScopedLock lock(_rwMutex);
        auto [iter, inserted] =
            _composeFns.emplace(Types { strong, weak },
                                ComposeFunc { vfn, cfn });
        return inserted;
    }

    mutable TfSpinRWMutex _rwMutex;
    std::unordered_map<Types, ComposeFunc, TfHash> _composeFns;
};

static Vt_ValueComposeRegistry &
_GetMutableComposeRegistry()
{
    // Intentionally "leak" this singleton to avoid possible static destruction
    // issues.
    static Vt_ValueComposeRegistry *registry = new Vt_ValueComposeRegistry;
    return *registry;
}

static Vt_ValueComposeRegistry const &
_GetComposeRegistry()
{
    return _GetMutableComposeRegistry();
}

} // anon

/// Return true if `stronger` composes non-trivially over `weaker`.
/// Specifically, if `stronger` cannot compose over (see
/// VtValueCanComposeOver()) or if no compose-over functionality has been
/// defined for values of `stronger`'s type over values of `weaker`'s type,
/// return false.  Otherwise return true. In other words, if this function
/// returns false, then `VtValueComposeOver(a, b)` always returns `stronger`.
bool
VtValueCanComposeOver(VtValueRef stronger, VtValueRef weaker)
{
    return stronger.IsEmpty() ||
        _GetComposeRegistry().Find(stronger.GetTypeid(), weaker.GetTypeid());
}

/// If `VtValueCanComposeOver(a, b)`, then return `VtValueComposeOver(a, b)`.
/// Otherwise return an empty optional.
std::optional<VtValue>
VtValueTryComposeOver(VtValueRef stronger, VtValueRef weaker)
{
    std::optional<VtValue> ret;
    if (!stronger.CanComposeOver()) {
        // leave ret empty.
    }
    else if (stronger.IsEmpty()) {
        ret = weaker;
    }
    else if (ComposeFunc const *
             fn = _GetComposeRegistry().Find(stronger.GetTypeid(),
                                             weaker.GetTypeid())) {
        ret = (*fn)(stronger, weaker);
    }
    return ret;
}

/// Return the result of composing `stronger` over `weaker`.  If `stronger` is
/// cannot compose-over, or if no compose-over functionality has been defined
/// for values of `stronger`'s type over `weaker`'s type, return `stronger`.
/// Otherwise invoke the compose-over function and return the result.  If
/// `stronger` is empty, return `weaker`.
VtValue
VtValueComposeOver(VtValueRef stronger, VtValueRef weaker)
{
    VtValue ret;
    if (!stronger.CanComposeOver()) {
        ret = stronger;
    }
    else if (stronger.IsEmpty()) {
        ret = weaker;
    }
    else if (ComposeFunc const *
             fn = _GetComposeRegistry().Find(stronger.GetTypeid(),
                                             weaker.GetTypeid())) {
        ret = (*fn)(stronger, weaker);
    }
    return ret;
}

void
Vt_RegisterComposeOver(std::type_info const &strongType,
                       std::type_info const &weakType,
                       VoidFn vfn, CompFn cfn)
{
    if (!_GetMutableComposeRegistry()
        .Register(strongType, weakType, vfn, cfn)) {
        TF_CODING_ERROR("Duplicate compose-over function registered for "
                        "'%s' over '%s' - ignoring registration",
                        ArchGetDemangled(strongType).c_str(),
                        ArchGetDemangled(weakType).c_str());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
