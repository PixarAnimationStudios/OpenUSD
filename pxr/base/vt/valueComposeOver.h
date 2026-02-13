//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_VT_VALUE_COMPOSE_OVER_H
#define PXR_BASE_VT_VALUE_COMPOSE_OVER_H

#include "pxr/pxr.h"

#include "pxr/base/vt/api.h"
#include "pxr/base/vt/traits.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/valueRef.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

/// Return true if the result of `VtValueCanComposeOver(val, x)` could be true
/// for some \p x, false otherwise.  The empty value can always compose-over.
/// This is a faster check than the two-argument form of
/// `VtValueCanComposeOver()`.
inline bool
VtValueCanComposeOver(VtValueRef val) {
    return val.CanComposeOver();
}

/// Return true if VtRegisterComposeOver() has been called with \p strongType as
/// the stronger type.
VT_API bool
VtValueTypeCanComposeOver(std::type_info const &strongType);

/// Return true if `stronger` composes non-trivially over `weaker`.
/// Specifically, if `stronger` can never compose-over (see the single-argument
/// form) or if no compose-over functionality has been defined for values of
/// `stronger`'s type over values of `weaker`'s type, return false.  Otherwise
/// return true. In other words, if this function returns false, then
/// `VtValueComposeOver(stronger, weaker)` always returns `stronger`.
VT_API bool
VtValueCanComposeOver(VtValueRef stronger, VtValueRef weaker);

/// If `VtValueCanComposeOver(stronger, weaker)`, then return
/// `VtValueComposeOver(stronger, weaker)`.  Otherwise return an empty optional.
VT_API std::optional<VtValue>
VtValueTryComposeOver(VtValueRef stronger, VtValueRef weaker);

/// Return the result of composing `stronger` over `weaker`.  If `stronger`
/// never composes-over, or if no compose-over functionality has been defined
/// for values of `stronger`'s type over `weaker`'s type, return `stronger`.
/// Otherwise invoke the compose-over function and return the result.  If
/// `stronger` is empty, return `weaker`.
VT_API VtValue
VtValueComposeOver(VtValueRef stronger, VtValueRef weaker);

/// A special sentinel type and singular value that can be used to "finalize" a
/// composing type.  For example, `VtArrayEdit<T>` can compose over
/// `VtBackground`.  It does so by applying its edits to an empty `VtArray<T>`
/// to produce a final array.  Using a single-typed sentinel lets type-agnostic
/// code "finalize" composing values uniformly.  The name "background" is by
/// analogy to image compositing.
struct VtBackgroundType {};
inline constexpr VtBackgroundType VtBackground;

// Required to store VtBackground in a VtValue.
inline bool
operator==(VtBackgroundType, VtBackgroundType) {
    return true;
}

// Private helper implementation.
VT_API void
Vt_RegisterComposeOver(std::type_info const &strongType,
                       std::type_info const &weakType,
                       void (*voidFn)(),
                       VtValue (*over)(VtValueRef, VtValueRef, void (*)()));

/// Register compose-over functionality for values of type `Strong` over `Weak`.
/// It is essential that the function be associative.  That is, ((`A` over `B`)
/// over `C`) must yield the same result as (`A` over (`B` over `C`)).
template <class Ret, class Strong, class Weak>
void
VtRegisterComposeOver(Ret (*fn)(Strong const &, Weak const &))
{
    static_assert(!VtIsKnownValueType<Strong>() || VtIsArrayEdit<Strong>::value,
                  "Unexpected compose-over registration for one of the "
                  "known value types");
    
    static_assert(VtValueTypeCanCompose<Strong>::value,
                  "Use VT_VALUE_TYPE_CAN_COMPOSE or specialize "
                  "VtValueTypeCanCompose<> before registering compose-over "
                  "functionality");

    using TypedFn = Ret (*)(Strong const &, Weak const &);
    using VoidFn = void (*)();
    
    VoidFn voidFn = reinterpret_cast<VoidFn>(fn);
    Vt_RegisterComposeOver(
        typeid(Strong), typeid(Weak), voidFn,
        [](VtValueRef strong, VtValueRef weak, void (*vFn)()) {
            TypedFn tFn = reinterpret_cast<TypedFn>(vFn);
            TF_DEV_AXIOM(strong.IsHolding<Strong>());
            TF_DEV_AXIOM(weak.IsHolding<Weak>());
            return VtValue {
                tFn(strong.UncheckedGet<Strong>(), weak.UncheckedGet<Weak>())
            };
        });
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_VALUE_COMPOSE_OVER_H
