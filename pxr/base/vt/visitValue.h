//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_VISIT_VALUE_H
#define PXR_BASE_VT_VISIT_VALUE_H

#include "pxr/pxr.h"

#include "pxr/base/tf/meta.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/vt/value.h"

#include <optional>
#include <typeinfo>

#ifndef VT_VISIT_VALUE_EXT_TYPES
#  define VT_VISIT_VALUE_EXT_TYPES PXR_NS_GLOBAL::TfMetaList<>
#endif

PXR_NAMESPACE_OPEN_SCOPE

namespace Vt_ValueVisitDetail {

// These two overloads do SFINAE to detect whether the visitor can be invoked
// with the given held type T.  If the visitor cannot be invoked with T, it is
// instead invoked with the VtValue itself.  We use the int/long trick and pass
// 0 in the caller to disambiguate in case both are viable.
template <class T, class Visitor, class ...Args>
auto
Visit(VtValue const &val, Visitor &&visitor, int, Args &&...args) ->
    decltype(std::forward<Visitor>(visitor)(
                 val.UncheckedGet<T>(), std::forward<Args>(args)...))
{
    return std::forward<Visitor>(visitor)(
        val.UncheckedGet<T>(), std::forward<Args>(args)...);
}

template <class T, class Visitor, class ...Args>
auto
Visit(VtValue const &val, Visitor &&visitor, long, Args &&...args) {
    return std::forward<Visitor>(visitor)(val, std::forward<Args>(args)...);
}

} // Vt_ValueVisitDetail

// Hash table mapping type_info -> index for extended type dispatch.
// Shared across all visitor types for the same type list.
template <class ...Ts>
struct Vt_ExtTypeIndexTable {
    static int Lookup(std::type_info const &ti) {
        static const Vt_ExtTypeIndexTable instance;
        auto it = instance._map.find(&ti);
        return (it != instance._map.end()) ? it->second : -1;
    }
private:
    struct _Hash {
        size_t operator()(std::type_info const *ti) const {
            return ti->hash_code();
        }
    };
    struct _Equal {
        bool operator()(std::type_info const *a,
                        std::type_info const *b) const {
            return TfSafeTypeCompare(*a, *b);
        }
    };
    Vt_ExtTypeIndexTable() {
        int idx = 0;
        ((_map.emplace(&typeid(Ts), idx++)), ...);
    }
    pxr_tsl::robin_map<std::type_info const *, int, _Hash, _Equal> _map;
};

// Helper: a single dispatch thunk for VtVisitValue, used in function pointer
// tables for large extended type lists.
template <class T, class Ret, class Visitor, class ...Args>
Ret
Vt_VisitValueExtDispatchOne(
    VtValue const &val, Visitor &&visitor, Args &&...args)
{
    return Vt_ValueVisitDetail::Visit<T>(
        val, std::forward<Visitor>(visitor), 0,
        std::forward<Args>(args)...);
}

constexpr size_t Vt_VisitValueHashThreshold = 8;

// Extended type dispatch for VtVisitValue.  For small type lists (<=
// Vt_VisitValueHashThreshold), uses a linear search via a short-circuiting fold
// expression.  For larger lists, uses a hash-table lookup + function-pointer
// dispatch.
template <class ...Ts, class Visitor, class ...Args>
auto
Vt_VisitValueExtDispatch(
    TfMetaList<Ts...>,
    VtValue const &val, Visitor &&visitor, Args &&...args)
{
    using Ret = decltype(
        Vt_ValueVisitDetail::Visit<VtValue>(
            val, std::forward<Visitor>(visitor), 0,
            std::forward<Args>(args)...));

    if constexpr (sizeof...(Ts) == 0) {
        return Vt_ValueVisitDetail::Visit<VtValue>(
            val, std::forward<Visitor>(visitor), 0,
            std::forward<Args>(args)...);
    }
    else if constexpr (sizeof...(Ts) <= Vt_VisitValueHashThreshold) {
        std::optional<Ret> result;
        ((!result && val.IsHolding<Ts>() &&
          (result.emplace(Vt_ValueVisitDetail::Visit<Ts>(
               val, std::forward<Visitor>(visitor), 0,
               std::forward<Args>(args)...)), true)) || ...);
        if (!result) {
            return Vt_ValueVisitDetail::Visit<VtValue>(
                val, std::forward<Visitor>(visitor), 0,
                std::forward<Args>(args)...);
        }
        return std::move(*result);
    }
    else {
        int idx = Vt_ExtTypeIndexTable<Ts...>::Lookup(val.GetTypeid());
        if (idx >= 0) {
            using FnPtr = Ret (*)(VtValue const &, Visitor &&, Args &&...);
            static constexpr FnPtr table[] = {
                &Vt_VisitValueExtDispatchOne<Ts, Ret, Visitor, Args...>...
            };
            return table[idx](
                val, std::forward<Visitor>(visitor),
                std::forward<Args>(args)...);
        }
        return Vt_ValueVisitDetail::Visit<VtValue>(
            val, std::forward<Visitor>(visitor), 0,
            std::forward<Args>(args)...);
    }
}

// Helper: dispatch thunk for VtVisitValueType.
template <class T, class Ret,
          template <class, class...> class Visitor, class ...TypeArgs,
          class ...FnArgs>
Ret
Vt_VisitValueTypeExtDispatchOne(FnArgs &&...args)
{
    return Visitor<T, TypeArgs...>::Visit(std::forward<FnArgs>(args)...);
}

// Extended type dispatch for VtVisitValueType.
template <template <class T, class ...> class Visitor, typename ...TypeArgs,
          class ...Ts, typename ...FnArgs>
auto
Vt_VisitValueTypeExtDispatch(
    TfMetaList<Ts...>,
    VtValue const &val, FnArgs &&...args)
{
    using Ret = decltype(
        Visitor<VtValue, TypeArgs...>::Visit(std::forward<FnArgs>(args)...));

    if constexpr (sizeof...(Ts) == 0) {
        return Visitor<VtValue, TypeArgs...>::Visit(
            std::forward<FnArgs>(args)...);
    }
    else if constexpr (sizeof...(Ts) <= Vt_VisitValueHashThreshold) {
        std::optional<Ret> result;
        ((!result && val.IsHolding<Ts>() &&
          (result.emplace(Visitor<Ts, TypeArgs...>::Visit(
               std::forward<FnArgs>(args)...)), true)) || ...);
        if (!result) {
            return Visitor<VtValue, TypeArgs...>::Visit(
                std::forward<FnArgs>(args)...);
        }
        return std::move(*result);
    }
    else {
        int idx = Vt_ExtTypeIndexTable<Ts...>::Lookup(val.GetTypeid());
        if (idx >= 0) {
            using FnPtr = Ret (*)(FnArgs &&...);
            static constexpr FnPtr table[] = {
                &Vt_VisitValueTypeExtDispatchOne<
                    Ts, Ret, Visitor, TypeArgs...>...
            };
            return table[idx](std::forward<FnArgs>(args)...);
        }
        return Visitor<VtValue, TypeArgs...>::Visit(
            std::forward<FnArgs>(args)...);
    }
}

// Helper: dispatch thunk for VtVisitValueType (template-template variant).
template <class T, class Ret,
          template <class, template <class...> class, class...> class Visitor,
          template <class...> class Tmpl, class ...TypeArgs,
          class ...FnArgs>
Ret
Vt_VisitValueTypeTmplExtDispatchOne(FnArgs &&...args)
{
    return Visitor<T, Tmpl, TypeArgs...>::Visit(std::forward<FnArgs>(args)...);
}

// Extended type dispatch for VtVisitValueType (template-template variant).
template <template <class T, template <class...> class, class ...> class Visitor,
          template <class...> class Tmpl, typename ...TypeArgs,
          class ...Ts, typename ...FnArgs>
auto
Vt_VisitValueTypeTmplExtDispatch(
    TfMetaList<Ts...>,
    VtValue const &val, FnArgs &&...args)
{
    using Ret = decltype(
        Visitor<VtValue, Tmpl, TypeArgs...>::Visit(
            std::forward<FnArgs>(args)...));

    if constexpr (sizeof...(Ts) == 0) {
        return Visitor<VtValue, Tmpl, TypeArgs...>::Visit(
            std::forward<FnArgs>(args)...);
    }
    else if constexpr (sizeof...(Ts) <= Vt_VisitValueHashThreshold) {
        std::optional<Ret> result;
        ((!result && val.IsHolding<Ts>() &&
          (result.emplace(Visitor<Ts, Tmpl, TypeArgs...>::Visit(
               std::forward<FnArgs>(args)...)), true)) || ...);
        if (!result) {
            return Visitor<VtValue, Tmpl, TypeArgs...>::Visit(
                std::forward<FnArgs>(args)...);
        }
        return std::move(*result);
    }
    else {
        int idx = Vt_ExtTypeIndexTable<Ts...>::Lookup(val.GetTypeid());
        if (idx >= 0) {
            using FnPtr = Ret (*)(FnArgs &&...);
            static constexpr FnPtr table[] = {
                &Vt_VisitValueTypeTmplExtDispatchOne<
                    Ts, Ret, Visitor, Tmpl, TypeArgs...>...
            };
            return table[idx](std::forward<FnArgs>(args)...);
        }
        return Visitor<VtValue, Tmpl, TypeArgs...>::Visit(
            std::forward<FnArgs>(args)...);
    }
}


/// Provides VtVisitValue() and VtVisitValueType() dispatch, parameterized on a
/// compile-time list of extended types to dispatch in addition to the "known"
/// types (VT_VALUE_TYPES).
///
/// Callers typically use the VtVisitValue and VtVisitValueType macros, which
/// inject VT_VISIT_VALUE_EXT_TYPES automatically.  For explicit control, call
/// methods on this class directly:
/// \code
/// VtVisitValueWithExtTypes<TfMetaList<MyType>>::VisitValue(val, visitor);
/// VtVisitValueWithExtTypes<TfMetaList<>>::VisitValue(val, visitor); // no ext
/// \endcode
template <class ExtTypes>
struct VtVisitValueWithExtTypes {

    /// Invoke \p visitor with \p value's held object if \p value holds a known
    /// or extended type.  If not, invoke \p visitor with \p value itself.
    ///
    /// \sa VtVisitValue (macro)
    template <class Visitor, class ...Args>
    static auto VisitValue(
        VtValue const &value, Visitor &&visitor, Args&&...args)
    {
        switch (value.GetKnownValueTypeIndex()) {
#define VT_CASE_FOR_TYPE_INDEX(unused, elem)                                   \
        case VtGetKnownValueTypeIndex<VT_TYPE(elem)>():                        \
            return Vt_ValueVisitDetail::Visit<VT_TYPE(elem)>(                  \
                value, std::forward<Visitor>(visitor), 0,                      \
                std::forward<Args>(args)...);                                  \
            break;
VT_FOR_EACH_VALUE_TYPE(VT_CASE_FOR_TYPE_INDEX)
#undef VT_CASE_FOR_TYPE_INDEX
        default:
            return Vt_VisitValueExtDispatch(
                ExtTypes{},
                value, std::forward<Visitor>(visitor),
                std::forward<Args>(args)...);
        };
    }

    /// Invoke Visitor<T, TypeArgs...>::Visit(args...) where T is the held type.
    ///
    /// \sa VtVisitValueType (macro)
    template <
        template <class T, class ...> class Visitor,
        typename ...TypeArgs,
        typename ...FnArgs
        >
    static auto VisitType(VtValue const &value, FnArgs&&...args)
    {
        switch (value.GetKnownValueTypeIndex()) {
#define VT_CASE_FOR_TYPE_INDEX(unused, elem)                                   \
        case VtGetKnownValueTypeIndex<VT_TYPE(elem)>():                        \
            return Visitor<VT_TYPE(elem), TypeArgs...>::Visit(                 \
                std::forward<FnArgs>(args)...);                                \
            break;
VT_FOR_EACH_VALUE_TYPE(VT_CASE_FOR_TYPE_INDEX)
#undef VT_CASE_FOR_TYPE_INDEX
        default:
            return Vt_VisitValueTypeExtDispatch<Visitor, TypeArgs...>(
                ExtTypes{}, value, std::forward<FnArgs>(args)...);
        };
    }

    /// \overload
    /// Accepts a leading class template argument passed to the Visitor.
    template <
        template <class T, template <class...> class, class ...> class Visitor,
        template <class...> class Tmpl,
        typename ...TypeArgs,
        typename ...FnArgs
        >
    static auto VisitType(VtValue const &value, FnArgs&&...args)
    {
        switch (value.GetKnownValueTypeIndex()) {
#define VT_CASE_FOR_TYPE_INDEX(unused, elem)                                   \
        case VtGetKnownValueTypeIndex<VT_TYPE(elem)>():                        \
            return Visitor<VT_TYPE(elem), Tmpl, TypeArgs...>::Visit(           \
                std::forward<FnArgs>(args)...);                                \
            break;
VT_FOR_EACH_VALUE_TYPE(VT_CASE_FOR_TYPE_INDEX)
#undef VT_CASE_FOR_TYPE_INDEX
        default:
            return Vt_VisitValueTypeTmplExtDispatch<Visitor, Tmpl, TypeArgs...>(
                ExtTypes{}, value, std::forward<FnArgs>(args)...);
        };
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#ifdef doxygen

/// Invoke \p visitor with \p value's held object if \p value holds an object of
/// one of the "known" value types (those in VT_VALUE_TYPES, see vt/types.h) or
/// any type registered via VT_ADD_EXT_VISIT_VALUE_TYPE.  If \p value does not
/// hold a known or registered type, or if it is empty, or if \p visitor cannot
/// be called with an object of the held type, then call \p visitor with
/// \p value itself.  Note this means that \p visitor must be callable with a
/// VtValue argument.
///
/// VtVisitValue() can be lower overhead compared to a chained-if of
/// VtValue::IsHolding() calls, or a hash-table-lookup dispatch.  Additionally,
/// visitors can handle related types with a single case, rather than calling
/// out all types individually.  For example:
///
/// \code
/// // If the value holds an array return its size, otherwise size_t(-1).
/// struct GetArraySize {
///     template <class T>
///     size_t operator()(VtArray<T> const &array) const {
///         return array.size();
///     }
///     size_t operator()(VtValue const &val) const {
///         return size_t(-1);
///     }
/// };
///
/// VtVisitValue(VtValue(VtIntArray(123)), GetArraySize()) -> 123
/// VtVisitValue(VtValue(VtDoubleArray(234)), GetArraySize()) -> 234
/// VtVisitValue(VtValue(VtVec3fArray(345)), GetArraySize()) -> 345
/// VtVisitValue(VtValue("not-a-vt-array"), GetArraySize()) -> size_t(-1)
/// \endcode
///
/// Note that the visitor is invoked as a normal C++ call expression, so
/// implicit conversions and standard overload resolution (including controlling
/// overload resolution via techniques like enable_if) can take place.  For
/// example, consider the following, where the double-specific overload is
/// invoked for VtValues holding double, float, and GfHalf.
///
/// \code
/// struct AsDouble {
///     double operator()(double val) const {
///         return val;
///     }
///     double operator()(VtValue const &) const {
///         return std::numeric_limits<double>::quiet_NaN();
///     }
/// };
///
/// VtVisitValue(VtValue(1.23), AsDouble()) -> 1.23
/// VtVisitValue(VtValue(float(0.5f)), AsDouble()) -> 0.5
/// VtVisitValue(VtValue(GfHalf(1.5f)), AsDouble()) -> 1.5
/// VtVisitValue(VtValue("not-convertible-to-double"), AsDouble()) -> NaN.
/// \endcode
///
/// Any additional passed arguments following the visitor are forwarded to it.
///
/// \code
/// auto multiply = TfOverloads {
///     [](int val, int scl) { return val * scl; },
///     [](double val, int scl) { return static_cast<int>(rint(val * scl)); },
///     [](VtValue const &val, int scl) { return -1; }
/// };
///
/// VtVisitValue(VtValue(123), multiply, 2);  -> 246 // multiply by 2.
/// VtVisitValue(VtValue(1.23), multiply, 3); -> 4   // multiply by 3 & round.
/// \endcode
///
/// \section VtVisitValue_ExtTypes Extended Types
///
/// Types defined outside of Vt can register for dispatch by using
/// VT_ADD_EXT_VISIT_VALUE_TYPE in their headers:
/// \code
/// #define VT_ADD_EXT_VISIT_VALUE_TYPE MyType
/// #include "pxr/base/vt/addExtVisitValueType.h"
/// \endcode
///
/// Once registered, VtVisitValue() automatically dispatches over all these
/// types registered in addition to the "known" types.  The set of extended
/// types available at a call site is determined by which registrations have
/// been performed at the point of the call to VtVisitValue() (i.e. which
/// headers have been included that perform registration).
///
/// For explicit control over the extended type list, call the underlying class
/// template directly:
/// \code
/// // Dispatch with a specific set of extended types:
/// VtVisitValueWithExtTypes<TfMetaList<MyType>>::VisitValue(val, vis);
///
/// // Dispatch with no extended types (known types only):
/// VtVisitValueWithExtTypes<TfMetaList<>>::VisitValue(val, vis);
///
/// // Dispatch with registered extended types plus additional types:
/// using MyExtTypes = TfMetaConcat<
///     VT_VISIT_VALUE_EXT_TYPES, TfMetaList<LocalTypeA, LocalTypeB>>;
/// VtVisitValueWithExtTypes<MyExtTypes>::VisitValue(val, vis);
/// \endcode
///
/// \note VtVisitValue is implemented as a macro for technical reasons (to
/// capture the current set of registered extended types at the call site).
/// It behaves as the function declared here.
template <class Visitor, class ...Args>
auto VtVisitValue(VtValue const &value, Visitor &&visitor, Args &&...args);

/// Invoke `Visitor<T, ...>::Visit(...)` with \p T being \p value's held
/// object's type if \p value holds an object of one of the "known" value types
/// (those in VT_VALUE_TYPES, see vt/types.h) or any type registered via
/// VT_ADD_EXT_VISIT_VALUE_TYPE.  If \p value does not hold a known or
/// registered type, or if it is empty, then invoke \p Visitor with
/// `T == VtValue`.
///
/// VtVisitValueType() can be lower overhead compared to a chained-if of
/// VtValue::IsHolding() calls, or a hash-table-lookup dispatch.  Additionally,
/// visitors can use partial template specialization to handle related types
/// with a single case, rather than calling out all types individually.  For
/// example:
///
/// \code
/// // Return single-value sizes or array element sizes.
/// template <class T>
/// struct GetElementSize {
///     static constexpr size_t size = sizeof(T);
/// };
///
/// template <class T>
/// struct GetElementSize<VtArray<T>> {
///     static constexpr size_t size = sizeof(T);
/// };
///
/// template <>
/// struct GetElementSize<VtValue> {
///     static constexpr size_t size = 0; // unknown type.
/// };
///
/// VtVisitValueType<GetElementSize>(VtValue(123)) -> 4
/// VtVisitValueType<GetElementSize>(VtValue(VtIntArray {})) -> 4
/// VtVisitValueType<GetElementSize>(VtValue(1.23)) -> 8
/// VtVisitValueType<GetElementSize>(VtValue(VtDoubleArray {})) -> 8
/// VtVisitValueType<GetElementSize>(VtValue(UnknownType {})) -> 0
/// \endcode
///
/// Any explicitly passed template arguments are passed to \p Visitor and
/// function arguments are forwarded to Visit().
///
/// \code
/// // Return true if the common_type<T, U> is T.
/// template <class T, class U>
/// struct IsCommonTypeWith {
///     static bool Visit() {
///         return std::is_same_v<std::common_type_t<T, U>, T>;
///     }
/// };
///
/// VtVisitValueType<IsCommonTypeWith, float>(VtValue(123)) -> false
/// VtVisitValueType<IsCommonTypeWith, int>(VtValue(1.23)) -> true
/// \endcode
///
/// \section VtVisitValueType_ExtTypes Extended Types
///
/// Extended types registered via VT_ADD_EXT_VISIT_VALUE_TYPE are dispatched
/// automatically, just as with VtVisitValue().  See \ref VtVisitValue_ExtTypes
/// for details on registration and explicit control via
/// VtVisitValueWithExtTypes.
///
/// \note VtVisitValueType is implemented as a macro for technical reasons (to
/// capture the current set of registered extended types at the call site).
/// It behaves as the function declared here.
template <
    template <class T, class ...> class Visitor,
    typename ...TypeArgs,
    typename ...FnArgs
    >
auto VtVisitValueType(VtValue const &value, FnArgs &&...args);

/// \overload
///
/// This overload accepts a leading class template argument that is passed along
/// to the Visitor as its second template argument.  This can be useful to
/// invoke factories that instantiate types dependent on the VtValue held-type.
/// For example:
///
/// \code
/// template <class T, template <class> class Template>
/// struct MakeNew {
///     static auto Visit(size_t count) {
///         return Template<T>::New(count);
///     }
/// };
///
/// class WrapperBase {
/// public:
///     virtual ~WrapperBase() = default;
/// };
///
/// template <class T>
/// class Wrapper : public WrapperBase {
///     std::unique_ptr<Wrapper> New(size_t count);
/// };
///
/// // Returns Wrapper<T>::New(count) where T is the held-type of `val`.
/// return VtVisitValueType<MakeNew, Wrapper>(val, count);
/// \endcode
template <
    template <class T, template <class...> class, class ...> class Visitor,
    template <class...> class Tmpl,
    typename ...TypeArgs,
    typename ...FnArgs
    >
auto VtVisitValueType(VtValue const &value, FnArgs &&...args);

#else // !doxygen

#define VtVisitValue \
    VtVisitValueWithExtTypes<VT_VISIT_VALUE_EXT_TYPES>::VisitValue

#define VtVisitValueType \
    VtVisitValueWithExtTypes<VT_VISIT_VALUE_EXT_TYPES>::VisitType

#endif // doxygen

#endif // PXR_BASE_VT_VISIT_VALUE_H
