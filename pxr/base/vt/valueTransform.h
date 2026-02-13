//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_VT_VALUE_TRANSFORM_H
#define PXR_BASE_VT_VALUE_TRANSFORM_H

#include "pxr/pxr.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/valueRef.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

class Vt_TransformImpl
{
private:
    template <class Xf>
    friend bool
    VtValueCanTransform(VtValueRef obj, Xf const &xfObj);
    
    template <class Xf>
    friend VtValue
    VtValueTryTransform(VtValueRef obj, Xf const &xfObj);

    VT_API
    static bool
    Can(VtValueRef obj, std::type_info const &xfType);
    
    VT_API
    static VtValue
    Try(VtValueRef x, std::type_info const &xfType, void const *xfObj,
        TfFunctionRef<VtValue (VtValueRef)>);
};

/// Return true if `obj` is not empty, views an object that supports value
/// transforms, and has value transform support registered for transforms of
/// type `Xf`.  Note that the type `Xf` does not have specific requirements, but
/// typically represents a possibly "polymorphic" transform that can apply to
/// many types.  For example, "Transpose" may apply to many matrix types.  Or
/// "Normalize" to many vector types.
template <class Xf>
bool
VtValueCanTransform(VtValueRef obj, Xf const &xfObj)
{
    if (obj.IsEmpty() || !obj.CanTransform()) {
        return false;
    }
    return Vt_TransformImpl::Can(obj, typeid(xfObj));
}

/// Attempt to transform `obj` by `xfObj` and return the result.  If no
/// transformation is done, return an empty value.  Strongly typed transforms
/// are preferred to type-erased transforms.
template <class Xf>
VtValue
VtValueTryTransform(VtValueRef obj, Xf const &xfObj)
{
    if (obj.IsEmpty() || !obj.CanTransform()) {
        return {};
    }
    // Helper for type-erased transforms.
    auto erasedXf = [&xfObj](VtValueRef in) -> VtValue {
        return VtValueTryTransform(in, xfObj);
    };
    return Vt_TransformImpl
        ::Try(obj, typeid(xfObj),
              static_cast<void const *>(std::addressof(xfObj)), erasedXf);
}

// Registration helper.
VT_API void
Vt_RegisterTypedTransform(
    std::type_info const &objType,
    std::type_info const &xfType,
    void (*voidFn)(),
    VtValue (*xform)(VtValueRef, void const *, void (*)()));

template <bool RegisterForArrays, class T, class Xf>
void
Vt_RegisterTransformImpl(T (*fn)(T const &obj, Xf const &xform))
{
    static_assert(!VtIsKnownValueType<T>(),
                  "Unexpected transform registration for one of the "
                  "known value types");

    static_assert(VtValueTypeCanTransform<T>::value,
                  "Use VT_VALUE_TYPE_CAN_TRANSFORM or directly specialize "
                  "VtValueTypeCanTransform<> before registering transform "
                  "functionality");

    using TypedFn = T (*)(T const &, Xf const &);
    using VoidFn = void (*)();

    VoidFn voidFn = reinterpret_cast<VoidFn>(fn);
    Vt_RegisterTypedTransform(
        typeid(T), typeid(Xf), voidFn,
        [](VtValueRef obj, void const *xf, void (*vFn)()) {
            TypedFn tFn = reinterpret_cast<TypedFn>(vFn);
            TF_DEV_AXIOM(obj.IsHolding<T>());
            return VtValue {
                tFn(obj.UncheckedGet<T>(), *static_cast<Xf const *>(xf))
            };
        });

    // Register array & array edit xforms of the same, if requested.
    if constexpr (RegisterForArrays) {
        // Arrays.
        Vt_RegisterTypedTransform(
            typeid(VtArray<T>), typeid(Xf), voidFn,
            [](VtValueRef obj, void const *xf, void (*vFn)()) {
                TypedFn tFn = reinterpret_cast<TypedFn>(vFn);
                TF_DEV_AXIOM(obj.IsHolding<VtArray<T>>());
                VtArray<T> const &src = obj.UncheckedGet<VtArray<T>>();
                VtArray<T> xformed;
                T const *srcPtr = src.cdata();
                Xf const &xform = *static_cast<Xf const *>(xf);
                xformed.resize(src.size(), [&](T *first, T *last) {
                    while (first != last) {
                        new (first++) T { tFn(*srcPtr++, xform) };
                    }
                });
                return VtValue::Take(xformed);
            });

        // Array Edits.
        Vt_RegisterTypedTransform(
            typeid(VtArrayEdit<T>), typeid(Xf), voidFn,
            [](VtValueRef obj, void const *xf, void (*vFn)()) {
                TypedFn tFn = reinterpret_cast<TypedFn>(vFn);
                TF_DEV_AXIOM(obj.IsHolding<VtArrayEdit<T>>());
                VtArrayEdit<T> const &src = obj.UncheckedGet<VtArrayEdit<T>>();
                VtArrayEdit<T> xformed = src;
                Xf const &xform = *static_cast<Xf const *>(xf);
                for (T &elem: xformed.GetMutableLiterals()) {
                    elem = tFn(elem, xform);
                }
                return VtValue::Take(xformed);
            });
    }
}
    
/// Register a strongly-typed transformation of `T` by `Xf` but do not register
/// transforms for `VtArray<T>` and `VtArrayEdit<T>`.  Call
/// VtRegisterTransform() to include arrays & array edits.
template <class T, class Xf>
void
VtRegisterTransformNoArrays(T (*fn)(T const &obj, Xf const &xform))
{
    Vt_RegisterTransformImpl</*RegisterForArrays=*/false>(fn);
}        

/// Register a strongly-typed transformation of `T` by `Xf`, plus `VtArray<T>`
/// and `VtArrayEdit<T>`, transforming their elements and literals respectively.
/// Call VtRegisterTransformNoArrays() to avoid registering for arrays & array
/// edits.
template <class T, class Xf>
void
VtRegisterTransform(T (*fn)(T const &obj, Xf const &xform))
{
    Vt_RegisterTransformImpl</*RegisterForArrays=*/true>(fn);
}

VT_API void
Vt_RegisterErasedTransform(
    std::type_info const &objType,
    void (*voidFn)(),
    VtValue (*xform)(VtValueRef, void (*)(),
                     TfFunctionRef<VtValue (VtValueRef)>));

/// Register a type-erased transform-forwarder for objects of type `T` that may
/// contain `VtValue`s (directly or indirectly).  For example, `VtDictionary`,
/// which is a map from string keys to `VtValue`s, registers type-erased
/// transform support to forward a transform to all of its values.
template <class T>
void
VtRegisterErasedTransform(
    std::optional<T> (*fn)(T const &obj, TfFunctionRef<VtValue (VtValueRef)>))
{
    static_assert(!VtIsKnownValueType<T>(),
                  "Unexpected transform registration for one of the "
                  "known value types");

    static_assert(VtValueTypeCanTransform<T>::value,
                  "Use VT_VALUE_TYPE_CAN_TRANSFORM or specialize "
                  "VtValueTypeCanTransform<> before registering transform "
                  "functionality");

    using TypedFn = std::optional<T> (*)(
        T const &, TfFunctionRef<VtValue (VtValueRef)>);
    using VoidFn = void (*)();

    VoidFn voidFn = reinterpret_cast<VoidFn>(fn);
    Vt_RegisterErasedTransform(
        typeid(T), voidFn,
        [](VtValueRef obj, VoidFn vFn,
           TfFunctionRef<VtValue (VtValueRef)> xform) {
            TypedFn tFn = reinterpret_cast<TypedFn>(vFn);
            TF_DEV_AXIOM(obj.IsHolding<T>());
            std::optional<T> optT = tFn(obj.UncheckedGet<T>(), xform);
            VtValue ret;
            if (optT) {
                ret = std::move(*optT);
            }
            return ret;
        });
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_VALUE_TRANSFORM_H
