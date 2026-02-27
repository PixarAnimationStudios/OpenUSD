//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_VISIT_VALUE_H
#define PXR_BASE_VT_VISIT_VALUE_H

#include "pxr/pxr.h"

#include "pxr/base/vt/value.h"

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


/// Invoke \p visitor with \p value's held object if \p value holds an object of
/// one of the "known" value types (those in VT_VALUE_TYPES, see vt/types.h).
/// If \p value does not hold a known type, or if it is empty, or if \p visitor
/// cannot be called with an object of the held type, then call \p visitor with
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
///
/// \endcode
///
/// Any additional passed arguments following the visitor are forwarded to it.
///
/// \code
///
/// auto multiply = TfOverloads {
///     [](int val, int scl) { return val * scl; },
///     [](double val, int scl) { return static_cast<int>(rint(val * scl)); },
///     [](VtValue const &val, int scl) { return -1; }
/// };
///
/// VtVisitValue(VtValue(123), multiply, 2);  -> 246 // multiply by 2.
/// VtVisitValue(VtValue(1.23), multiply, 3); -> 4   // multiply by 3 & round.
///
/// \endcode
template <class Visitor, class ...Args>
auto VtVisitValue(VtValue const &value, Visitor &&visitor, Args&&...args)
{
    // This generally gets the compiler to emit a jump table to dispatch
    // directly to the code for each known value type.
    switch (value.GetKnownValueTypeIndex()) {

// Cases for known types.
#define VT_CASE_FOR_TYPE_INDEX(unused, elem)                                   \
        case VtGetKnownValueTypeIndex<VT_TYPE(elem)>():                        \
            return Vt_ValueVisitDetail::Visit<VT_TYPE(elem)>(                  \
                value, std::forward<Visitor>(visitor), 0,                      \
                std::forward<Args>(args)...);                                  \
            break;
VT_FOR_EACH_VALUE_TYPE(VT_CASE_FOR_TYPE_INDEX)
#undef VT_CASE_FOR_TYPE_INDEX

        default:
            // Invoke visitor with value itself.
            return Vt_ValueVisitDetail::Visit<VtValue>(
                value, std::forward<Visitor>(visitor), 0,
                std::forward<Args>(args)...);
            break;
    };
}

/// Invoke `Visitor<T, ...>::Visit(...)` with \p T being \p value 's held
/// object's type if \p value holds an object of one of the "known" value types
/// (those in VT_VALUE_TYPES, see vt/types.h).  If \p value does not hold a
/// known type or if it is empty, then invoke \p Visitor with `T == VtValue`.
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
/// function are forwarded to Visit().
///
/// \code
/// // Return true if the common_type<T, U> is T.
/// template <class T, class U>
/// struct IsCommonTypeWith {
///     return std::is_same_v<std::common_type_t<T, U>, T>;
/// };
///
/// VtVisitValueType<IsCommonTypeWith, float>(VtValue(123)) -> false
/// VtVisitValueType<IsCommonTypeWith, int>(VtValue(1.23)) -> true
/// \endcode
template <
    template <class T, class ...> class Visitor,
    typename ...TypeArgs,
    typename ...FnArgs
    >
auto VtVisitValueType(VtValue const &value, FnArgs&&...args)
{
    // This generally gets the compiler to emit a jump table to dispatch
    // directly to the code for each known value type.
    switch (value.GetKnownValueTypeIndex()) {

// Cases for known types.
#define VT_CASE_FOR_TYPE_INDEX(unused, elem)                                   \
        case VtGetKnownValueTypeIndex<VT_TYPE(elem)>():                        \
            return Visitor<VT_TYPE(elem), TypeArgs...>::Visit(                 \
                std::forward<FnArgs>(args)...);                                \
            break;
VT_FOR_EACH_VALUE_TYPE(VT_CASE_FOR_TYPE_INDEX)
#undef VT_CASE_FOR_TYPE_INDEX

        default:
            // Invoke visitor with VtValue itself.
            return Visitor<VtValue, TypeArgs...>::Visit(
                std::forward<FnArgs>(args)...);
    };
}

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
/// return VtVisitValue<MakeNew, MyWrapper>(val, count);
/// \endcode
template <
    template <class T, template <class...> class, class ...> class Visitor,
    template <class...> class Tmpl,
    typename ...TypeArgs,
    typename ...FnArgs
    >
auto VtVisitValueType(VtValue const &value, FnArgs&&...args)
{
    // This generally gets the compiler to emit a jump table to dispatch
    // directly to the code for each known value type.
    switch (value.GetKnownValueTypeIndex()) {

// Cases for known types.
#define VT_CASE_FOR_TYPE_INDEX(unused, elem)                                   \
        case VtGetKnownValueTypeIndex<VT_TYPE(elem)>():                        \
            return Visitor<VT_TYPE(elem), Tmpl, TypeArgs...>::Visit(           \
                std::forward<FnArgs>(args)...);                                \
            break;
VT_FOR_EACH_VALUE_TYPE(VT_CASE_FOR_TYPE_INDEX)
#undef VT_CASE_FOR_TYPE_INDEX

        default:
            // Invoke visitor with VtValue itself.
            return Visitor<VtValue, Tmpl, TypeArgs...>::Visit(
                std::forward<FnArgs>(args)...);
    };
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_VISIT_VALUE_H
