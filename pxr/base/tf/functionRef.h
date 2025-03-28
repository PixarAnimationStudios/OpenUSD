//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
/// For technical reasons, TfFunctionRef does not support function pointers;
/// only function objects.  Internally TfFunctionRef stores a void pointer to
/// the function object it's referencing, but C++ does not allow function
/// pointers to be cast to void pointers.  Supporting this case would increase
/// this class's size and add complexity to its implementation.  Instead,
/// callers may wrap function pointers in lambdas to sidestep the issue.
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
    // Type trait to detect when an argument is a potentially cv-qualified
    // TfFunctionRef.  This is used to disable the generic constructor and
    // assignment operator so that TfFunctionRef arguments are copied rather
    // than forming TfFunctionRefs pointing to TfFunctionRefs.
    template <typename Fn>
    using _IsFunctionRef = std::is_same<
        std::remove_cv_t<std::remove_reference_t<Fn>>, TfFunctionRef>;

public:
    /// Construct with an lvalue callable \p fn.
    template <class Fn, class = std::enable_if_t<!_IsFunctionRef<Fn>::value>>
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
    std::enable_if_t<!_IsFunctionRef<Fn>::value,
                     TfFunctionRef &>
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
