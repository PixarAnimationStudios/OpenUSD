//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_VALUEREF_H
#define PXR_BASE_VT_VALUEREF_H

#include "pxr/pxr.h"

// XXX: Include pyLock.h after pyObjWrapper.h to work around
// Python include ordering issues.
#include "pxr/base/tf/pyObjWrapper.h"

#include "pxr/base/tf/pyLock.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/anyUniquePtr.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/vt/api.h"
#include "pxr/base/vt/hash.h"
#include "pxr/base/vt/streamOut.h"
#include "pxr/base/vt/traits.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/valueCommon.h"

#include <iosfwd>
#include <typeinfo>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

class VtValue;
class VtValueRef;

// Overload VtStreamOut for vector<VtValueRef>.  Produces output like [value1,
// value2, ... valueN].
VT_API std::ostream &
VtStreamOut(std::vector<VtValueRef> const &val, std::ostream &);

/// A non-owning type-erased view of a value, interoperating with VtValue.
/// Since VtValueRef is non-owning it must not persist beyond the lifetime of
/// the value it views, and so it is typically best used as a function argument
/// or an automatic variable.
/// 
/// Both ordinary typed values and VtValues are implicitly convertible to
/// VtValueRef.  This makes it convenient to use when writing a function that
/// can take an object of any type.  Since it's non-owning, it is relatively
/// light-weight, incurring no heap allocations or reference counting
/// operations.  In addition, VtValueRef remembers if it is viewing an
/// rvalue-reference, in which case the underlying object can be removed by a
/// move() operation rather than a copy.  See Remove().
///
/// VtValueRef supports much of the same API as VtValue, including implicitly
/// converting back to VtValue if needed.
class VtValueRef
{
protected:

    // This unfortunately needs to be a union since there are some callers that
    // place function pointers in VtValue.  The specific _TypeInfo
    // instantiations know which union member to use at based on their type
    // parameter.
    union _RefdObjPtr {
        void const *objPtr;
        void (*fnPtr)();
    };
    
    // Type information base class.
    // We force alignment here in order to ensure that TfPointerAndBits has
    // enough room to store all TypeInfo related flags.
    struct alignas(8) _TypeInfo {
    private:
        using _CanHashFunc = bool (*)(_RefdObjPtr);
        using _HashFunc = size_t (*)(_RefdObjPtr);
        using _EqualFunc = bool (*)(_RefdObjPtr, _RefdObjPtr);
        using _GetPyObjFunc = TfPyObjWrapper (*)(_RefdObjPtr);
        using _GetVtValueFunc = VtValue (*)(_RefdObjPtr);
        using _StreamOutFunc = std::ostream & (*)(_RefdObjPtr, std::ostream &);
        using _IsArrayValuedFunc = bool (*)(_RefdObjPtr);
        using _GetNumElementsFunc = size_t (*)(_RefdObjPtr);

    protected:
        constexpr _TypeInfo(const std::type_info &ti,
                            const std::type_info &elementTi,
                            int knownTypeIndex,
                            bool isArray,
                            bool isHashable,
                            bool canComposeOver,
                            bool canTransform,
                            bool isMutable,
                            _CanHashFunc canHash,
                            _HashFunc hash,
                            _EqualFunc equal,
                            _GetPyObjFunc getPyObj,
                            _GetVtValueFunc getVtValue,
                            _StreamOutFunc streamOut,
                            _IsArrayValuedFunc isArrayValued,
                            _GetNumElementsFunc getNumElements)
            : typeInfo(ti)
            , elementTypeInfo(elementTi)
            , knownTypeIndex(knownTypeIndex)
            , isArray(isArray)
            , isHashable(isHashable)
            , canComposeOver(canComposeOver)
            , canTransform(canTransform)
            , isMutable(isMutable)
            // Function table
            , _canHash(canHash)
            , _hash(hash)
            , _equal(equal)
            , _getPyObj(getPyObj)
            , _getVtValue(getVtValue)
            , _streamOut(streamOut)
            , _isArrayValued(isArrayValued)
            , _getNumElements(getNumElements)
        {}

    public:
        bool CanHash(_RefdObjPtr storage) const {
            return _canHash(storage);
        }
        size_t Hash(_RefdObjPtr storage) const {
            return _hash(storage);
        }
        bool Equal(_RefdObjPtr lhs, _RefdObjPtr rhs) const {
            return _equal(lhs, rhs);
        }
        TfPyObjWrapper GetPyObj(_RefdObjPtr storage) const {
            return _getPyObj(storage);
        }
        // Defined below due to circular VtValue <-> VtValueRef includes.
        inline VtValue GetVtValue(_RefdObjPtr storage) const;
        std::ostream &StreamOut(_RefdObjPtr storage,
                                std::ostream &out) const {
            return _streamOut(storage, out);
        }
        bool IsArrayValued(_RefdObjPtr storage) const {
            return _isArrayValued(storage);
        }
        size_t GetNumElements(_RefdObjPtr storage) const {
            return _getNumElements(storage);
        }

        const std::type_info &typeInfo;
        const std::type_info &elementTypeInfo;
        const int knownTypeIndex;
        const bool isArray;
        const bool isHashable;
        const bool canComposeOver;
        const bool canTransform;
        const bool isMutable;

    private:
        _CanHashFunc _canHash;
        _HashFunc _hash;
        _EqualFunc _equal;
        _GetPyObjFunc _getPyObj;
        _GetVtValueFunc _getVtValue;
        _StreamOutFunc _streamOut;
        _IsArrayValuedFunc _isArrayValued;
        _GetNumElementsFunc _getNumElements;
    };

    // Type-dispatching overloads.

    // Array type helpers.  Non-array types have no shape data, no elements and
    // report `void` for their element types.
    template <class T>
    struct _NonArrayHelper
    {
        template <class U>
        static size_t GetNumElements(U &&) { return 0; }
        constexpr static std::type_info const &GetElementTypeid() {
            return typeid(void);
        }
    };
    // VtArray types report their qualities.
    template <class Array>
    struct _IsArrayHelper
    {
        static size_t GetNumElements(Array const &obj) {
            return obj.size();
        }
        constexpr static std::type_info const &GetElementTypeid() {
            return typeid(typename Array::ElementType);
        }
    };
    // VtArrayEdit types are identical to non-array types except that they do
    // report their underlying element type.
    template <class ArrayEdit>
    struct _IsArrayEditHelper : _NonArrayHelper<ArrayEdit>
    {
        constexpr static std::type_info const &GetElementTypeid() {
            return typeid(typename ArrayEdit::ElementType);
        }
    };

    // Select which flavor of array helper to use -- VtArray uses
    // _IsArrayHelper, VtArrayEdit uses _IsArrayEditHelper, all other types use
    // _NonArrayHelper.
    template <class T>
    using _ArrayHelper = TfConditionalType<
        VtIsArray<T>::value, _IsArrayHelper<T>,
        TfConditionalType<VtIsArrayEdit<T>::value,
                          _IsArrayEditHelper<T>, _NonArrayHelper<T>>
        >;

    // _TypeInfo implementation.
    template <class T, bool IsMutable>
    struct _TypeInfoImpl : public _TypeInfo
    {
        using Type = T;
        using This = _TypeInfoImpl;

        static constexpr bool IsPointer = std::is_pointer_v<Type>;
        static constexpr bool IsFunctionPointer =
            IsPointer && std::is_function_v<std::remove_pointer_t<Type>>;
        
        using ConstType = std::conditional_t<
            IsPointer,
            const std::remove_pointer_t<Type> *,
            const Type>;

        using GetObjResultType = std::conditional_t<
            IsPointer, ConstType, ConstType &>;

        using GetMutableObjResultType = std::conditional_t<
            IsPointer, Type, Type &>;
        
        //using RefType = Type &;
        // using ObjType = std::conditional_t<
        //     std::is_pointer_v<Type>, Type, Type &>;
        //using ConstRefType = ConstType const &;

        constexpr _TypeInfoImpl()
            : _TypeInfo(typeid(T),
                        _ArrayHelper<T>::GetElementTypeid(),
                        Vt_KnownValueTypeDetail::GetIndex<T>(),
                        VtIsArray<T>::value,
                        VtIsHashable<T>(),
                        VtValueTypeCanCompose<T>::value,
                        VtValueTypeCanTransform<T>::value,
                        IsMutable,
                        &This::_CanHash,
                        &This::_Hash,
                        &This::_Equal,
                        &This::_GetPyObj,
                        &This::_GetVtValue,
                        &This::_StreamOut,

                        // Array support.
                        &This::_IsArrayValued,
                        &This::_GetNumElements)

        {}

        ////////////////////////////////////////////////////////////////////
        // Typed API for client use.
        static GetObjResultType
        GetObj(_RefdObjPtr const &storage) {
            if constexpr (IsFunctionPointer) {
                return reinterpret_cast<GetObjResultType>(storage.fnPtr);
            }
            else if constexpr (IsPointer) {
                return reinterpret_cast<GetObjResultType>(storage.objPtr);
            }
            else {
                return *static_cast<ConstType *>(storage.objPtr);
            }
        }

        static GetMutableObjResultType
        GetMutableObj(_RefdObjPtr &storage) {
            // This const cast is safe since the original VtValueRef must've
            // been constructed with a mutable object.
            if constexpr (IsFunctionPointer) {
                return reinterpret_cast<GetMutableObjResultType>(storage.fnPtr);
            }                
            else if constexpr (IsPointer) {
                return reinterpret_cast<
                    GetMutableObjResultType>(storage.objPtr);
            }
            else {
                return *static_cast<Type *>(const_cast<void *>(storage.objPtr));
            }
        }

        static void SetObj(T const &obj, _RefdObjPtr &storage) {
            if constexpr (IsFunctionPointer) {
                storage.fnPtr = reinterpret_cast<void (*)()>(obj);
            }
            else if constexpr (IsPointer) {
                storage.objPtr = static_cast<void const *>(obj);
            }
            else {
                storage.objPtr = static_cast<void const *>(std::addressof(obj));
            }
        }

    private:
        static bool _CanHash(_RefdObjPtr) {
            return VtIsHashable<T>();
        }

        static size_t _Hash(_RefdObjPtr ptr) {
            return VtHashValue(GetObj(ptr));
        }

        static bool _Equal(_RefdObjPtr lhs, _RefdObjPtr rhs) {
            return GetObj(lhs) == GetObj(rhs);
        }

        static TfPyObjWrapper _GetPyObj(_RefdObjPtr storage) {
#ifdef PXR_PYTHON_SUPPORT_ENABLED
            TfPyLock lock;
            return pxr_boost::python::api::object(GetObj(storage));
#else
            return {};
#endif
        }

        // Defined below.
        static VtValue _GetVtValue(_RefdObjPtr storage);

        static std::ostream &_StreamOut(
            _RefdObjPtr storage, std::ostream &out) {
            return VtStreamOut(GetObj(storage), out);
        }

        static bool _IsArrayValued(_RefdObjPtr storage) {
            return VtIsArray<T>::value;
        }

        static std::type_info const &
        _GetElementTypeid(_RefdObjPtr ptr) {
            return _ArrayHelper<T>::GetElementTypeid();
        }

        static size_t _GetNumElements(_RefdObjPtr ptr) {
            return _ArrayHelper<T>::GetNumElements(GetObj(ptr));
        }

    };

    // Metafunction that returns the specific _TypeInfo subclass for T or T&.
    // The passed `T` should be as-if deduced by a forwarding reference, so a
    // reference type if the referenced object is mutable, otherwise a
    // non-reference type.
    template <class T>
    using _TypeInfoFor = _TypeInfoImpl<
        std::decay_t<T>,
        std::is_reference_v<T> && std::is_const_v<T>>;

public:

    /// Default ctor gives empty VtValueRef.
    VtValueRef() : _info(nullptr) {}

    VtValueRef(VtValueRef const &) = default;
    VtValueRef(VtValueRef &&) = default;

    /// Implicitly convert or construct a VtValueRef referring to \p obj.  The
    /// passed \p obj must outlive this VtValueRef instance.
    template <class T>
    VtValueRef(T &&obj, std::enable_if_t<
               !std::is_same_v<std::decay_t<T>, VtValueRef> &&
               !std::is_same_v<std::decay_t<T>, VtValue>> * = 0) {
        using TypeInfo = _TypeInfoFor<T>;
        using TypeInfoType = typename TypeInfo::Type;
        static_assert(!VtIsValueProxy<TypeInfoType>::value,
                      "VtValueRef does not support proxies");
        static const TypeInfo ti;
        ti.SetObj(obj, _ptr);
        _info = std::addressof(ti);
    }

    /// Rebind to view the same object as another VtValueRef.
    VtValueRef &operator=(VtValueRef const &ref) = default;
    VtValueRef &operator=(VtValueRef &&ref) = default;

    // Disallow implicit conversion assignments.
    template <class T>
    VtValueRef &operator=(T &&) = delete;

    /// Overloaded swap() for generic code/stl/etc.  Swaps the viewed objects.
    friend void swap(VtValueRef &lhs, VtValueRef &rhs) {
        std::swap(lhs._ptr, rhs._ptr);
        std::swap(lhs._info, rhs._info);
    }

    /// Return true if this value is viewing an object of type \p T, false
    /// otherwise.
    template <class T>
    bool IsHolding() const {
        return _info && _TypeIs<T>();
    }

    /// Return true if this refers to an rvalue, and thus can be `Remove()`d by
    /// a move construction operation instead of a copy.
    bool IsRValue() const {
        return _info && _info->isMutable;
    }

    /// Return true if this views a VtArray instance, false otherwise.
    VT_API bool IsArrayValued() const;

    /// Return true if this views a VtArrayEdit instance, false otherwise.
    VT_API bool IsArrayEditValued() const;

    /// Return the number of elements in the viewed value if IsArrayValued(),
    /// return 0 otherwise.
    size_t GetArraySize() const { return _GetNumElements(); }

    /// Return the typeid of the type viewed by this value.
    std::type_info const &GetTypeid() const {
        return _info ? _info->typeInfo : typeid(void);
    }

    /// If this value views a VtArray or VtArrayEdit instance, return the typeid
    /// of its element type.  For example, if this value views a VtIntArray or a
    /// VtIntArrayEdit, return typeid(int).  Otherwise return typeid(void).
    std::type_info const &GetElementTypeid() const {
        return _info ? _info->elementTypeInfo : typeid(void);
    }
    
    /// Returns the TfType of the type viewed by this value.
    VT_API TfType GetType() const;

    /// Return the type name of the viewed typeid.
    VT_API std::string GetTypeName() const;

    /// Return VtKnownValueTypeIndex<T> for the viewed type T.  If this value is
    /// empty or views a type that is not 'known', return -1.
    int GetKnownValueTypeIndex() const {
        return _info ? _info->knownTypeIndex : -1;
    }

    /// Return a const reference to the viewed object if the viewed object is of
    /// type \a T.  Invokes undefined behavior otherwise.  This is the fastest
    /// \a Get() method to use after a successful \a IsHolding() check.
    template <class T>
    typename _TypeInfoFor<T>::GetObjResultType
    UncheckedGet() const { return _Get<T>(); }

    /// Return a const reference to the viewed object if the viewed object is of
    /// type \a T.  Issues an error and return a const reference to a default
    /// value if the viewed object is not of type \a T.  Use \a IsHolding to
    /// verify correct type before calling this function.  The default value
    /// returned in case of type mismatch is constructed using
    /// Vt_DefaultValueFactory<T>.  That may be specialized for client types.
    /// The default implementation of the default value factory produces a
    /// value-initialized T.
    template <class T>
    typename _TypeInfoFor<T>::GetObjResultType
    Get() const {
        using Factory = Vt_DefaultValueFactory<T>;

        // In the unlikely case that the types don't match, we obtain a default
        // value to return and issue an error via _FailGet.
        if (ARCH_UNLIKELY(!IsHolding<T>())) {
            return *(static_cast<T const *>(
                         _FailGet(Factory::Invoke, typeid(T))));
        }

        return _Get<T>();
    }

    /// Return a copy of the viewed object if the viewed object is of type T.
    /// Return a copy of the default value \a def otherwise.  Note that this
    /// always returns a copy, as opposed to \a Get() which always returns a
    /// reference.
    template <class T>
    T GetWithDefault(T const &def = T()) const {
        return IsHolding<T>() ? UncheckedGet<T>() : def;
    }

    /// Return a move-constructed (if the viewed object is an rvalue) or
    /// copy-constructed \p T instance from the viewed object.  If the viewed
    /// object is an rvalue then it is left in its moved-from state.  If this
    /// value does not view a \p T instance, issue an error and return a
    /// default-constructed \p T.
    template <class T>
    T Remove() {
        if (IsHolding<T>()) {
            return UncheckedRemove<T>();
        }
        else {
            _FailRemove(typeid(T));
        }
        return {};
    }

    /// Return a move-constructed (if the viewed object is an rvalue) or
    /// copy-constructed \p T instance from the viewed object.  If the viewed
    /// object is an rvalue then it is left in its moved-from state.  If this
    /// value does not view a \p T instance, invoke undefined behavior.
    template <class T>
    T UncheckedRemove() {
        if (IsRValue()) {
            return std::move(_GetMutable<T>());
        }
        return _Get<T>();
    }

    /// Returns true iff this value is empty.
    bool IsEmpty() const { return !_info; }

    /// Implicitly convert to a VtValue by copying the viewed object.
    VT_API operator VtValue() const;

    /// Return true if the viewed object provides a hash implementation.
    VT_API bool CanHash() const;

    /// Return a hash code for the viewed object by calling VtHashValue() on it.
    VT_API size_t GetHash() const;

    friend inline size_t hash_value(VtValueRef const &val) {
        return val.GetHash();
    }

    /// Return true if this value holds a type that has been declared at compile
    /// time to support composing over other types.  This is a fast check that
    /// can be used to avoid calling `VtValueComposeOver(strong, weak)` if
    /// `strong` does not support composing over.  Empty VtValue always can
    /// compose over.
    bool CanComposeOver() const {
        return !_info || _info->canComposeOver;
    }

    /// Return true if this value holds a type that has been declared to support
    /// value transforms at compile time.  This is a fast check that can be used
    /// to avoid calling the relatively slower `VtValueTryTransform(obj, xform)`
    /// if `obj` does not support transforms at all.  Empty VtValue does not
    /// support transforms.
    bool CanTransform() const {
        return _info && _info->canTransform;
    }

    /// Test two values for equality.
    bool operator == (const VtValueRef &rhs) const {
        bool empty = IsEmpty(), rhsEmpty = rhs.IsEmpty();
        if (empty || rhsEmpty) {
            // Either one or both empty -- only equal if both empty.
            return empty == rhsEmpty;
        }
        if (_info == rhs._info) {
            // Holding identical types -- compare directly.
            return _info->Equal(_ptr, rhs._ptr);
        }
        return false;
    }
    bool operator != (const VtValueRef &rhs) const { return !(*this == rhs); }

    /// Calls through to operator << on the viewed object.
    VT_API friend std::ostream &
    operator << (std::ostream &out, const VtValueRef &self);

protected:    
    VT_API size_t _GetNumElements() const;

    template <class T>
    inline bool
    _TypeIs() const {
        if constexpr (VtIsKnownValueType_Workaround<T>::value) {
            return _info->knownTypeIndex == VtGetKnownValueTypeIndex<T>();
        }
        else {
            std::type_info const &t = typeid(T);
            return TfSafeTypeCompare(_info->typeInfo, t);
        }
    }

    template <class T>
    typename _TypeInfoFor<T>::GetMutableObjResultType
    _GetMutable() {
        using TypeInfo = _TypeInfoFor<T>;
        return TypeInfo::GetMutableObj(_ptr);
    }

    template <class T>
    typename _TypeInfoFor<T>::GetObjResultType
    _Get() const {
        using TypeInfo = _TypeInfoFor<T>;
        return TypeInfo::GetObj(_ptr);
    }

    // Helper invoked in case Get fails.  Reports an error and returns a default
    // value for \a queryType.
    VT_API void const *
    _FailGet(Vt_DefaultValueHolder (*factory)(),
             std::type_info const &queryType) const;

    VT_API void _FailRemove(std::type_info const &);

    // This grants friend access to a function in the wrapper file for this
    // class.  This lets the wrapper reach down into a value to get a
    // pxr_boost::python wrapped object corresponding to the viewed type.  This
    // facility is necessary to get the python API we want.
    friend TfPyObjWrapper
    Vt_GetPythonObjectFromHeldValue(VtValueRef const &self);

    VT_API TfPyObjWrapper _GetPythonObject() const;

    _RefdObjPtr _ptr;
    _TypeInfo const *_info;
};

#ifndef doxygen

//
// The Get()/IsHolding routines needs to be special-cased to handle getting a
// VtValueRef *as* a VtValueRef.
//

template <>
inline VtValueRef const &
VtValueRef::Get<VtValueRef>() const {
    return *this;
}

template <>
inline VtValueRef const &
VtValueRef::UncheckedGet<VtValueRef>() const {
    return *this;
}

template <>
inline bool
VtValueRef::IsHolding<VtValueRef>() const {
    return true;
}

// Specialize VtValueRef::IsHolding<void>() to always return false.
template <>
inline bool
VtValueRef::IsHolding<void>() const {
    return false;
}

#endif // !doxygen

/// A non-owning type-erased view of a mutable lvalue, interoperating with
/// VtValue.  Since VtMutableValueRef is non-owning it must not persist beyond
/// the lifetime of the value it views, and so it is typically best used as a
/// function argument or an automatic variable.
/// 
/// Ordinary typed values are implicitly convertible to VtMutableValueRef.  This
/// makes it convenient to use when writing a function that can take an object
/// of any type.  Since it's non-owning, it is relatively light-weight,
/// incurring no heap allocations or reference counting operations.
///
class VtMutableValueRef : public VtValueRef
{
public:
    /// Default ctor gives empty VtMutableValueRef.
    VtMutableValueRef() = default;

    template <class T>
    VtMutableValueRef(T &obj) : VtValueRef(obj) {}

    /// Assign to the viewed value.
    template <class T>
    VtMutableValueRef &operator=(T &&obj) {
        static_assert(std::is_same_v<T, typename Vt_ValueGetStored<T>::Type>,
                      "Can only assign to VtMutableValueRef with a type T "
                      "that stores as T");
        if (IsHolding<T>()) {
            _GetMutable<T>() = std::forward<T>(obj);
        }
        else {
            _FailAssign(typeid(T));
        }
        return *this;
    }

    template <class T>
    VtMutableValueRef &UncheckedAssign(T &&obj) {
        static_assert(std::is_same_v<T, typename Vt_ValueGetStored<T>::Type>,
                      "Can only assign to VtMutableValueRef with a type T "
                      "that stores as T");
        _GetMutable<T>() = std::forward<T>(obj);
        return *this;
    }

    /// Swap the viewed value with \a rhs.  If this value is holding a T, make an
    /// unqualified call to swap(<viewed-value>, rhs).  If this value is not
    /// holding a T, replace the viewed value with a value-initialized T instance
    /// first, then swap.
    template <class T>
    void
    Swap(T &rhs) {
        static_assert(std::is_same_v<T, typename Vt_ValueGetStored<T>::Type>,
                      "Can only VtValueRef::Swap with a type T "
                      "that stores as T");
        if (IsHolding<T>()) {
            UncheckedSwap(rhs);
        }
        else {
            _FailSwap(typeid(T));
        }
    }

    /// Swap the viewed value with \a rhs.  This VtValue must be holding an
    /// object of type \p T.  If it does not, this invokes undefined behavior.
    /// Use Swap() if this VtValue is not known to contain an object of type
    /// \p T.
    template <class T>
    void
    UncheckedSwap(T &rhs) {
        static_assert(std::is_same_v<T, typename Vt_ValueGetStored<T>::Type>,
                      "Can only VtValueRef::Swap with a type T "
                      "that stores as T");
        using std::swap;
        swap(_GetMutable<T>(), rhs);
    }

    /// If this value holds an object of type \p T, invoke \p mutateFn, passing
    /// it a non-const reference to the viewed object and return true.  Otherwise
    /// do nothing and return false.
    template <class T, class Fn>
    bool
    Mutate(Fn &&mutateFn) {
        static_assert(std::is_same_v<T, typename Vt_ValueGetStored<T>::Type>,
                      "Can only VtValueRef::Mutate a type T that stores as T");
        if (!IsHolding<T>()) {
            return false;
        }
        UncheckedMutate<T>(std::forward<Fn>(mutateFn));
        return true;
    }

    /// Invoke \p mutateFn, passing it a non-const reference to the viewed
    /// object which must be of type \p T.  If the viewed object is not of type
    /// \p T, this function invokes undefined behavior.
    template <class T, class Fn>
    void
    UncheckedMutate(Fn &&mutateFn) {
        static_assert(std::is_same_v<T, typename Vt_ValueGetStored<T>::Type>,
                      "Can only VtValueRef::Mutate a type T that stores as T");
        // We move to a temporary, mutate the temporary, then move back.  This
        // prevents callers from escaping a mutable reference to the viewed
        // object via a side-effect of mutateFn.
        T &stored = _GetMutable<T>();
        T tmp = std::move(stored);
        std::forward<Fn>(mutateFn)(tmp);
        stored = std::move(tmp);
    }

private:
    VT_API void _FailAssign(std::type_info const &);
    VT_API void _FailSwap(std::type_info const &);

};

PXR_NAMESPACE_CLOSE_SCOPE

// This unusual arrangement of closing the namespace, including value.h, then
// reopening the namespace exists because valueRef.h and value.h are
// interdependent.  A similar symmetric construct exists in value.h.  If
// valueRef.h is included first, then value.h will be included here.  Otherwise
// if value.h is included first then it will have included valueRef.h.  Either
// way all the necessary declarations from both types are present prior to the
// appearance of the following defintions.

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

// Defined here due to circular dependencies.
VtValue
VtValueRef::_TypeInfo::GetVtValue(_RefdObjPtr storage) const {
    return _getVtValue(storage);
}

template <class T, bool B>
VtValue
VtValueRef::_TypeInfoImpl<T, B>::_GetVtValue(_RefdObjPtr storage) {
    return VtValue { GetObj(storage) };
}

PXR_NAMESPACE_CLOSE_SCOPE

#ifdef PXR_PYTHON_SUPPORT_ENABLED

#include "pxr/external/boost/python/converter/arg_from_python.hpp"
#include <optional>

namespace PXR_BOOST_NAMESPACE::python::converter {

// We specialize arg_rvalue_from_python<VtValueRef> to derive
// arg_rvalue_from_python<VtValue>.  This way we get storage for both -- an
// `rvalue_from_python_data<VtValue &> m_data` in the base and a VtValueRef in
// this class.  There's no other way afaict to "add space" in an rvalue
// converter to support a "view" type where we need to also conjure a temporary
// for that view.  This way we can create the `VtValue` in the base as usual and
// let the `VtValueRef` here refer to it for the lifetime of the arg.
template <>
struct arg_rvalue_from_python<PXR_NS::VtValueRef>
    : public arg_rvalue_from_python<PXR_NS::VtValue>
{
    using base_type = arg_rvalue_from_python<PXR_NS::VtValue>;
    using base_type::convertible;
    using result_type = PXR_NS::VtValueRef &; // boost.python convention.

    VT_API arg_rvalue_from_python(PyObject* obj);

    VT_API result_type operator()();
    
    std::optional<PXR_NS::VtValueRef> optValueRef;
};

} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_PYTHON_SUPPORT_ENABLED

#endif // PXR_BASE_VT_VALUEREF_H

