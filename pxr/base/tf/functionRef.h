//
// Copyright 2020 Pixar
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
#ifndef PXR_BASE_TF_FUNCTION_REF_H
#define PXR_BASE_TF_FUNCTION_REF_H

#include "pxr/pxr.h"

#include <memory>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

template <class Sig>
class TfFunctionRef;

/// \class TfFunctionRef
///
/// This class provides a non-owning reference to a type-erased callable object
/// with a specified signature.  This is useful in cases where you want to write
/// a function that takes a user-provided callback, and that callback is used
/// only in the duration of the function call, and you want to keep your
/// function's implementation out-of-line.
///
/// The advantage over std::function is that TfFunctionRef is lighter-weight.
/// Since it is non-owning, it guarantees no heap allocation; a possibility with
/// std::function.  The cost to call a TfFunctionRef is an indirect function
/// call.
///
/// For example:
///
/// \code
///
/// // widgetUtil.h ////////////////////////////////
///
/// class WidgetUtil
/// {
///     template <class WidgetPredicate>
///     bool AnyWidgetsMatch(WidgetPredicate const &predicate) {
///         TfFunctionRef<bool (Widget const &)> predRef(predicate);
///         return _AnyWidgetsMatchImpl(predRef);
///     }
/// private:
///     bool _AnyWidgetsMatchImpl(
///         TfFunctionRef<bool (Widget const &)> const &pred);
/// };
///
/// // widgetUtil.cpp //////////////////////////////
///
/// #include "widgetUtil.h"
///
/// bool WidgetUtil::_AnyWidgetsMatchImpl(
///     TfFunctionRef<bool (Widget const &)> const &pred)
/// {
///     for (Widget const &widget: GetAllTheWidgets()) {
///         if (pred(widget)) {
///             return true;
///         }
///     }
///     return false;
/// }
///
/// \endcode
///
/// Here the implementation of _AnyWidgetsMatchImpl is kept out-of-line, callers
/// can pass their own function objects for the predicate, there is no heap
/// allocation, and the cost to invoke the predicate in the implementation is
/// just the cost of calling a function pointer.
///
template <class Ret, class... Args>
class TfFunctionRef<Ret (Args...)>
{
public:
    /// Construct with an lvalue callable \p fn.
    template <class Fn>
    constexpr TfFunctionRef(Fn &fn) noexcept
        : _fn(static_cast<void const *>(std::addressof(fn)))
        , _invoke(_InvokeFn<Fn>) {}

    /// Copy construct from another TfFunctionRef.  The constructed
    /// TfFunctionRef refers to the same callable as \p rhs.
    TfFunctionRef(TfFunctionRef const &rhs) noexcept = default;

    /// Assign from another TfFunctionRef.  After assignment this object refers
    /// to the same callable as \p rhs.
    TfFunctionRef &
    operator=(TfFunctionRef const &rhs) noexcept = default;

    /// Assign from an lvalue callable \p fn.
    template <class Fn>
    TfFunctionRef &
    operator=(Fn &fn) noexcept {
        *this = TfFunctionRef(fn);
        return *this;
    }
    
    /// Swap this and \p other.  After the swap, this refers to \p other's
    /// previous callable, and \p other refers to this's previous callable.
    void swap(TfFunctionRef &other) noexcept {
        std::swap(_fn, other._fn);
        std::swap(_invoke, other._invoke);
    }
    
    /// Invoke the callable that this object refers to with \p args.
    inline Ret operator()(Args... args) const {
        return _invoke(_fn, std::forward<Args>(args)...);
    }

private:
    template <class Fn>
    static Ret _InvokeFn(void const *fn, Args...args) {
        using FnPtr = typename std::add_pointer<
            typename std::add_const<Fn>::type>::type;
        return (*static_cast<FnPtr>(fn))(std::forward<Args>(args)...);
    }

    void const *_fn;
    Ret (*_invoke)(void const *, Args...);
};

/// Swap \p lhs and \p rhs.  Equivalent to lhs.swap(rhs).
template <class Sig>
inline void
swap(TfFunctionRef<Sig> &lhs, TfFunctionRef<Sig> &rhs)
{
    lhs.swap(rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_FUNCTION_REF_H
