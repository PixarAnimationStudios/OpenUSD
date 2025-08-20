//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_VALUE_H
#define PXR_BASE_VT_VALUE_H

#include "pxr/pxr.h"

// XXX: Include pyLock.h after pyObjWrapper.h to work around
// Python include ordering issues.
#include "pxr/base/tf/pyObjWrapper.h"

#include "pxr/base/tf/pyLock.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/anyUniquePtr.h"
#include "pxr/base/tf/delegatedCountPtr.h"
#include "pxr/base/tf/meta.h"
#include "pxr/base/tf/pointerAndBits.h"
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

#include <iosfwd>
#include <typeinfo>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// Make a default value.
/// VtValue uses this to create values to be returned from failed calls to \a
/// Get. Clients may specialize this for their own types.
template <class T>
struct Vt_DefaultValueFactory;

// This is a helper class used by Vt_DefaultValueFactory to return a value with
// its type erased and only known at runtime via a std::type_info.
struct Vt_DefaultValueHolder
{
    // Creates a value-initialized object and stores the type_info for the
    // static type.
    template <typename T>
    static Vt_DefaultValueHolder Create() {
        return Vt_DefaultValueHolder(TfAnyUniquePtr::New<T>(), typeid(T));
    }

    // Creates a copy of the object and stores the type_info for the static
    // type.
    template <typename T>
    static Vt_DefaultValueHolder Create(T const &val) {
        return Vt_DefaultValueHolder(TfAnyUniquePtr::New(val), typeid(T));
    }

    // Return the runtime type of the held object.
    std::type_info const &GetType() const {
        return *_type;
    }

    // Return a pointer to the held object.  This may be safely cast to the
    // static type corresponding to the type_info returned by GetType.
    void const *GetPointer() const {
        return _ptr.Get();
    }

private:
    Vt_DefaultValueHolder(TfAnyUniquePtr &&ptr, std::type_info const &type)
        : _ptr(std::move(ptr)), _type(&type) {}

    TfAnyUniquePtr _ptr;
    std::type_info const *_type;
};

class VtValue;

// Overload VtStreamOut for vector<VtValue>.  Produces output like [value1,
// value2, ... valueN].
VT_API std::ostream &VtStreamOut(std::vector<VtValue> const &val, std::ostream &);

#define VT_VALUE_SET_STORED_TYPE(SRC, DST)                      \
    template <> struct Vt_ValueStoredType<SRC> { typedef DST Type; }

template <class T> struct Vt_ValueStoredType { typedef T Type; };
VT_VALUE_SET_STORED_TYPE(char const *, std::string);
VT_VALUE_SET_STORED_TYPE(char *, std::string);

template <size_t N>
struct Vt_ValueStoredType<char [N]> {
    using Type = std::string;
};

#ifdef PXR_PYTHON_SUPPORT_ENABLED
VT_VALUE_SET_STORED_TYPE(pxr_boost::python::object, TfPyObjWrapper);
#endif // PXR_PYTHON_SUPPORT_ENABLED

#undef VT_VALUE_SET_STORED_TYPE

// A metafunction that gives the type VtValue should store for a given type T.
template <class T>
using Vt_ValueGetStored = Vt_ValueStoredType<std::decay_t<T>>;

/// Provides a container which may hold any type, and provides introspection
/// and iteration over array types.  See \a VtIsArray for more info.
///
/// \section VtValue_Casting Held-type Conversion with VtValue::Cast
///
/// VtValue provides a suite of "Cast" methods that convert or create a
/// VtValue holding a requested type (via template parameter, typeid, or
/// type-matching to another VtValue) from the type of the currently-held
/// value.  Clients can add conversions between their own types using the
/// RegisterCast(), RegisterSimpleCast(), and
/// RegisterSimpleBidirectionalCast() methods.  Conversions from plugins can
/// be guaranteed to be registered before they are needed by registering them
/// from within a
/// \code
/// TF_REGISTRY_FUNCTION(VtValue) {
/// }
/// \endcode
/// block.
///
/// \subsection VtValue_builtin_conversions Builtin Type Conversion
///
/// Conversions between most of the basic "value types" that are intrinsically
/// convertible are builtin, including all numeric types (including Gf's \c
/// half), std::string/TfToken, GfVec* (for vecs of the same dimension), and
/// VtArray<T> for floating-point POD and GfVec of the preceding.
///
/// \subsection VtValue_numeric_conversion Numeric Conversion Safety
///
/// The conversions between all scalar numeric types are performed with range
/// checks such as provided by boost::numeric_cast(), and will fail, returning
/// an empty VtValue if the source value is out of range of the destination
/// type.
///
/// Conversions between GfVec and other compound-numeric types provide no more
/// or less safety or checking than the conversion constructors of the types
/// themselves.  This includes VtArray, even VtArray<T> for T in scalar types
/// that are range-checked when held singly.
class VtValue
{
    static const unsigned int _LocalFlag       = 1 << 0;
    static const unsigned int _TrivialCopyFlag = 1 << 1;
    static const unsigned int _ProxyFlag       = 1 << 2;

    template <class T>
    struct _Counted {
        explicit _Counted(T const &obj) : _obj(obj) {
            _refCount = 0;
        }
        bool IsUnique() const { return _refCount == 1; }
        T const &Get() const { return _obj; }
        T &GetMutable() { return _obj; }

    private:
        T _obj;
        mutable std::atomic<int> _refCount;

        friend inline void TfDelegatedCountIncrement(_Counted const *d) {
            d->_refCount.fetch_add(1, std::memory_order_relaxed);
        }
        friend inline void TfDelegatedCountDecrement(_Counted const *d) noexcept {
            if (d->_refCount.fetch_sub(1, std::memory_order_release) == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete d;
            }
        }
    };

    // Hold objects up to 1 word large locally.  This makes the total structure
    // 16 bytes when compiled 64 bit (1 word type-info pointer, 1 word storage
    // space).
    static const size_t _MaxLocalSize = sizeof(void*);
    typedef std::aligned_storage<
        /* size */_MaxLocalSize, /* alignment */_MaxLocalSize>::type _Storage;

    template <class T>
    using _IsTriviallyCopyable = std::integral_constant<bool,
        std::is_trivially_default_constructible_v<T> &&
        std::is_trivially_copyable_v<T> &&
        std::is_trivially_copy_assignable_v<T> &&
        std::is_trivially_destructible_v<T>>;

    // Metafunction that returns true if T should be stored locally, false if it
    // should be stored remotely.
    template <class T>
    using _UsesLocalStore = std::integral_constant<bool,
        (sizeof(T) <= sizeof(_Storage)) &&
        VtValueTypeHasCheapCopy<T>::value &&
        std::is_nothrow_move_constructible<T>::value &&
        std::is_nothrow_move_assignable<T>::value>;

    // Type information base class.
    struct _TypeInfo {
    private:
        using _CopyInitFunc = void (*)(_Storage const &, _Storage &);
        using _DestroyFunc = void (*)(_Storage &);
        using _MoveFunc = void (*)(_Storage &, _Storage &);
        using _CanHashFunc = bool (*)(_Storage const &);
        using _HashFunc = size_t (*)(_Storage const &);
        using _EqualFunc = bool (*)(_Storage const &, _Storage const &);
        using _EqualPtrFunc = bool (*)(_Storage const &, void const *);
        using _MakeMutableFunc = void (*)(_Storage &);
        using _GetPyObjFunc = TfPyObjWrapper (*)(_Storage const &);
        using _StreamOutFunc =
            std::ostream & (*)(_Storage const &, std::ostream &);
        using _GetTypeidFunc = std::type_info const & (*)(_Storage const &);
        using _IsArrayValuedFunc = bool (*)(_Storage const &);
        using _GetElementTypeidFunc =
            std::type_info const & (*)(_Storage const &);
        using _GetShapeDataFunc = const Vt_ShapeData* (*)(_Storage const &);
        using _GetNumElementsFunc = size_t (*)(_Storage const &);
        using _ProxyHoldsTypeFunc = bool (*)(_Storage const &, std::type_info const &);
        using _GetProxiedTypeFunc = TfType (*)(_Storage const &);
        using _GetProxiedTypeidFunc =
            std::type_info const & (*)(_Storage const &);
        using _GetProxiedObjPtrFunc = void const *(*)(_Storage const &);
        using _GetProxiedAsVtValueFunc = VtValue (*)(_Storage const &);

    protected:
        constexpr _TypeInfo(const std::type_info &ti,
                            const std::type_info &elementTi,
                            int knownTypeIndex,
                            bool isArray,
                            bool isHashable,
                            bool isProxy,
                            _CopyInitFunc copyInit,
                            _DestroyFunc destroy,
                            _MoveFunc move,
                            _CanHashFunc canHash,
                            _HashFunc hash,
                            _EqualFunc equal,
                            _EqualPtrFunc equalPtr,
                            _MakeMutableFunc makeMutable,
                            _GetPyObjFunc getPyObj,
                            _StreamOutFunc streamOut,
                            _GetTypeidFunc getTypeid,
                            _IsArrayValuedFunc isArrayValued,
                            _GetElementTypeidFunc getElementTypeid,
                            _GetShapeDataFunc getShapeData,
                            _GetNumElementsFunc getNumElements,
                            _ProxyHoldsTypeFunc proxyHoldsType,
                            _GetProxiedTypeFunc getProxiedType,
                            _GetProxiedTypeidFunc getProxiedTypeid,
                            _GetProxiedObjPtrFunc getProxiedObjPtr,
                            _GetProxiedAsVtValueFunc getProxiedAsVtValue)
            : typeInfo(ti)
            , elementTypeInfo(elementTi)
            , knownTypeIndex(knownTypeIndex)
            , isProxy(isProxy)
            , isArray(isArray)
            , isHashable(isHashable)
            // Function table
            , _copyInit(copyInit)
            , _destroy(destroy)
            , _move(move)
            , _canHash(canHash)
            , _hash(hash)
            , _equal(equal)
            , _equalPtr(equalPtr)
            , _makeMutable(makeMutable)
            , _getPyObj(getPyObj)
            , _streamOut(streamOut)
            , _getTypeid(getTypeid)
            , _isArrayValued(isArrayValued)
            , _getElementTypeid(getElementTypeid)
            , _getShapeData(getShapeData)
            , _getNumElements(getNumElements)
            , _proxyHoldsType(proxyHoldsType)
            , _getProxiedType(getProxiedType)
            , _getProxiedTypeid(getProxiedTypeid)
            , _getProxiedObjPtr(getProxiedObjPtr)
            , _getProxiedAsVtValue(getProxiedAsVtValue)
        {}

    public:
        void CopyInit(_Storage const &src, _Storage &dst) const {
            _copyInit(src, dst);
        }
        void Destroy(_Storage &storage) const {
            _destroy(storage);
        }
        void Move(_Storage &src, _Storage &dst) const noexcept {
            _move(src, dst);
        }
        bool CanHash(_Storage const &storage) const {
            return _canHash(storage);
        }
        size_t Hash(_Storage const &storage) const {
            return _hash(storage);
        }
        bool Equal(_Storage const &lhs, _Storage const &rhs) const {
            return _equal(lhs, rhs);
        }
        bool EqualPtr(_Storage const &lhs, void const *rhs) const {
            return _equalPtr(lhs, rhs);
        }
        void MakeMutable(_Storage &storage) const {
            _makeMutable(storage);
        }
        TfPyObjWrapper GetPyObj(_Storage const &storage) const {
            return _getPyObj(storage);
        }
        std::ostream &StreamOut(_Storage const &storage,
                                std::ostream &out) const {
            return _streamOut(storage, out);
        }
        bool IsArrayValued(_Storage const &storage) const {
            return _isArrayValued(storage);
        }
        std::type_info const &GetElementTypeid(_Storage const &storage) const {
            return _getElementTypeid(storage);
        }
        std::type_info const &GetTypeid(_Storage const &storage) const {
            return _getTypeid(storage);
        }
        const Vt_ShapeData* GetShapeData(_Storage const &storage) const {
            return _getShapeData(storage);
        }
        size_t GetNumElements(_Storage const &storage) const {
            return _getNumElements(storage);
        }
        bool ProxyHoldsType(_Storage const &storage,
                            std::type_info const &t) const {
            return _proxyHoldsType(storage, t);
        }
        TfType GetProxiedType(_Storage const &storage) const {
            return _getProxiedType(storage);
        }
        std::type_info const &GetProxiedTypeid(_Storage const &storage) const {
            return _getProxiedTypeid(storage);
        }
        VtValue GetProxiedAsVtValue(_Storage const &storage) const {
            return _getProxiedAsVtValue(storage);
        }
        void const *GetProxiedObjPtr(_Storage const &storage) const {
            return _getProxiedObjPtr(storage);
        }

        const std::type_info &typeInfo;
        const std::type_info &elementTypeInfo;
        int knownTypeIndex;
        bool isProxy;
        bool isArray;
        bool isHashable;

    private:
        _CopyInitFunc _copyInit;
        _DestroyFunc _destroy;
        _MoveFunc _move;
        _CanHashFunc _canHash;
        _HashFunc _hash;
        _EqualFunc _equal;
        _EqualPtrFunc _equalPtr;
        _MakeMutableFunc _makeMutable;
        _GetPyObjFunc _getPyObj;
        _StreamOutFunc _streamOut;
        _GetTypeidFunc _getTypeid;
        _IsArrayValuedFunc _isArrayValued;
        _GetElementTypeidFunc _getElementTypeid;
        _GetShapeDataFunc _getShapeData;
        _GetNumElementsFunc _getNumElements;
        _ProxyHoldsTypeFunc _proxyHoldsType;
        _GetProxiedTypeFunc _getProxiedType;
        _GetProxiedTypeidFunc _getProxiedTypeid;
        _GetProxiedObjPtrFunc _getProxiedObjPtr;
        _GetProxiedAsVtValueFunc _getProxiedAsVtValue;
    };

    // Type-dispatching overloads.

    // Array type helpers.  Non-array types have no shape data, no elements and
    // report `void` for their element types.
    template <class T>
    struct _NonArrayHelper
    {
        static const Vt_ShapeData* GetShapeData(T const &) { return NULL; }
        static size_t GetNumElements(T const &) { return 0; }
        constexpr static std::type_info const &GetElementTypeid() {
            return typeid(void);
        }
    };
    // VtArray types report their qualities.
    template <class Array>
    struct _IsArrayHelper
    {
        static const Vt_ShapeData* GetShapeData(Array const &obj) {
            return obj._GetShapeData();
        }
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

    // Function used in case T has equality comparison.
    template <class T>
    static inline auto
    _TypedProxyEqualityImpl(T const &a, T const &b, int) -> decltype(a == b) {
        return a == b;
    }
    // Function used in case T does not have equality comparison.
    template <class NoEqual>
    static inline bool
    _TypedProxyEqualityImpl(NoEqual const &a, NoEqual const &b, long) {
        return VtGetProxiedObject(a) == VtGetProxiedObject(b);
    }

    template <class T>
    static inline auto
    _ErasedProxyEqualityImpl(T const &a, T const &b, int) -> decltype(a == b) {
        return a == b;
    }
    // Function used in case T does not have equality comparison.
    template <class NoEqual>
    static inline bool
    _ErasedProxyEqualityImpl(NoEqual const &a, NoEqual const &b, long) {
        return *VtGetErasedProxiedVtValue(a) == *VtGetErasedProxiedVtValue(b);
    }

    // Proxy type helper. This version handles non-proxies and typed proxies.
    template <class T>
    struct _TypedProxyHelper
    {
        using ProxiedType = typename VtGetProxiedType<T>::type;

        static bool CanHash(T const &) { return VtIsHashable<ProxiedType>(); }
        static size_t Hash(T const &obj) {
            return VtHashValue(VtGetProxiedObject(obj));
        }
        static bool Equal(T const &a, T const &b) {
            // We use the traditional int/long = 0 arg technique to disambiguate
            // overloads, so we can invoke equality comparison on the *proxy*
            // type if it provides one, or if it doesn't then invoke equality
            // comparison on the *proxied* type instead.
            return _TypedProxyEqualityImpl(a, b, 0);
        }
        static TfPyObjWrapper GetPyObj(T const &obj) {
#ifdef PXR_PYTHON_SUPPORT_ENABLED
            ProxiedType const &p = VtGetProxiedObject(obj);
            TfPyLock lock;
            return pxr_boost::python::api::object(p);
#else
            return {};
#endif //PXR_PYTHON_SUPPORT_ENABLED
        }
        static std::ostream &StreamOut(T const &obj, std::ostream &out) {
            return VtStreamOut(VtGetProxiedObject(obj), out);
        }
        static Vt_ShapeData const *GetShapeData(T const &obj) {
            return _ArrayHelper<ProxiedType>::GetShapeData(
                VtGetProxiedObject(obj));
        }
        static size_t GetNumElements(T const &obj) {
            return _ArrayHelper<ProxiedType>::GetNumElements(
                VtGetProxiedObject(obj));
        }
        static bool IsArrayValued(T const &) {
            return VtIsArray<ProxiedType>::value;
        }
        static std::type_info const &GetTypeid(T const &) {
            return typeid(ProxiedType);
        }
        static std::type_info const &GetElementTypeid(T const &) {
            return _ArrayHelper<ProxiedType>::GetElementTypeid();
        }
        static VtValue GetProxiedAsVtValue(T const &obj) {
            return VtValue(VtGetProxiedObject(obj));
        }
        static bool HoldsType(T const &tp, std::type_info const &query) {
            return TfSafeTypeCompare(typeid(ProxiedType), query);
        }
        static TfType GetTfType(T const &tp) {
            return TfType::Find<ProxiedType>();
        }
        static void const *GetObjPtr(T const &tp) {
            return static_cast<void const *>(&VtGetProxiedObject(tp));
        }
    };

    // Proxy type helper. This version handles type-erased proxies.
    template <class ErasedProxy>
    struct _ErasedProxyHelper
    {
        static bool CanHash(ErasedProxy const &proxy) {
            return VtGetErasedProxiedVtValue(proxy)->CanHash();
        }
        static size_t Hash(ErasedProxy const &proxy) {
            return VtGetErasedProxiedVtValue(proxy)->GetHash();
        }
        static bool Equal(ErasedProxy const &a, ErasedProxy const &b) {
            // We use the traditional int/long = 0 arg technique to disambiguate
            // overloads, so we can invoke equality comparison on the *proxy*
            // type if it provides one, or if it doesn't then invoke equality
            // comparison on the VtValue containing the *proxied* type instead.
            return _ErasedProxyEqualityImpl(a, b, 0);
        }
        static TfPyObjWrapper GetPyObj(ErasedProxy const &obj) {
#ifdef PXR_PYTHON_SUPPORT_ENABLED
            VtValue const *val = VtGetErasedProxiedVtValue(obj);
            TfPyLock lock;
            return pxr_boost::python::api::object(*val);
#else
            return {};
#endif //PXR_PYTHON_SUPPORT_ENABLED
        }
        static std::ostream &
        StreamOut(ErasedProxy const &obj, std::ostream &out) {
            return VtStreamOut(obj, out);
        }
        static Vt_ShapeData const *GetShapeData(ErasedProxy const &obj) {
            return VtGetErasedProxiedVtValue(obj)->_GetShapeData();
        }
        static size_t GetNumElements(ErasedProxy const &obj) {
            return VtGetErasedProxiedVtValue(obj)->_GetNumElements();
        }
        static bool IsArrayValued(ErasedProxy const &obj) {
            return VtGetErasedProxiedVtValue(obj)->IsArrayValued();
        }
        static std::type_info const &GetTypeid(ErasedProxy const &obj) {
            return VtGetErasedProxiedVtValue(obj)->GetTypeid();
        }
        static std::type_info const &GetElementTypeid(ErasedProxy const &obj) {
            return VtGetErasedProxiedVtValue(obj)->GetElementTypeid();
        }
        static VtValue GetProxiedAsVtValue(ErasedProxy const &ep) {
            return *VtGetErasedProxiedVtValue(ep);
        }
        static bool
        HoldsType(ErasedProxy const &ep, std::type_info const &query) {
            return VtErasedProxyHoldsType(ep, query);
        }
        static TfType GetTfType(ErasedProxy const &ep) {
            return VtGetErasedProxiedTfType(ep);
        }
        static void const *GetObjPtr(ErasedProxy const &ep) {
            VtValue const *val = VtGetErasedProxiedVtValue(ep);
            return val ? val->_GetProxiedObjPtr() : nullptr;
        }
    };

    // _TypeInfo implementation helper.  This is a CRTP base that the
    // _LocalTypeInfo and _RemoteTypeInfo types derive.  It wraps their
    // type-specific implementations with type-generic interfaces.
    template <class T, class Container, class Derived>
    struct _TypeInfoImpl : public _TypeInfo
    {
        static const bool IsLocal = _UsesLocalStore<T>::value;
        static const bool HasTrivialCopy = _IsTriviallyCopyable<T>::value;
        static const bool IsProxy = VtIsValueProxy<T>::value;

        using ProxyHelper = TfConditionalType<
            VtIsErasedValueProxy<T>::value,
            _ErasedProxyHelper<T>, _TypedProxyHelper<T>>;

        using This = _TypeInfoImpl;

        constexpr _TypeInfoImpl()
            : _TypeInfo(typeid(T),
                        _ArrayHelper<T>::GetElementTypeid(),
                        Vt_KnownValueTypeDetail::GetIndex<T>(),
                        VtIsArray<T>::value,
                        VtIsHashable<T>(),
                        IsProxy,
                        &This::_CopyInit,
                        &This::_Destroy,
                        &This::_Move,
                        &This::_CanHash,
                        &This::_Hash,
                        &This::_Equal,
                        &This::_EqualPtr,
                        &This::_MakeMutable,
                        &This::_GetPyObj,
                        &This::_StreamOut,

                        &This::_GetTypeid,

                        // Array support.
                        &This::_IsArrayValued,
                        &This::_GetElementTypeid,
                        &This::_GetShapeData,
                        &This::_GetNumElements,

                        // Proxy support.
                        &This::_ProxyHoldsType,
                        &This::_GetProxiedType,
                        &This::_GetProxiedTypeid,
                        &This::_GetProxiedObjPtr,
                        &This::_GetProxiedAsVtValue)
        {}

        ////////////////////////////////////////////////////////////////////
        // Typed API for client use.
        static T const &GetObj(_Storage const &storage) {
            return Derived::_GetObj(_Container(storage));
        }

        static T &GetMutableObj(_Storage &storage) {
            return Derived::_GetMutableObj(_Container(storage));
        }

        static void CopyInitObj(T const &objSrc, _Storage &dst) {
            Derived::_PlaceCopy(&_Container(dst), objSrc);
        }

    private:
        static_assert(sizeof(Container) <= sizeof(_Storage),
                      "Container size cannot exceed storage size.");

        ////////////////////////////////////////////////////////////////////
        // _TypeInfo interface function implementations.
        static void _CopyInit(_Storage const &src, _Storage &dst) {
            new (&_Container(dst)) Container(_Container(src));
        }

        static void _Destroy(_Storage &storage) {
            _Container(storage).~Container();
        }

        static bool _CanHash(_Storage const &storage) {
            return ProxyHelper::CanHash(GetObj(storage));
        }

        static size_t _Hash(_Storage const &storage) {
            return ProxyHelper::Hash(GetObj(storage));
        }

        static bool _Equal(_Storage const &lhs, _Storage const &rhs) {
            // Equal is only ever invoked with an object of this specific type.
            // That is, we only ever ask a proxy to compare to a proxy; we never
            // ask a proxy to compare to the proxied object.
            return ProxyHelper::Equal(GetObj(lhs), GetObj(rhs));
        }

        static bool _EqualPtr(_Storage const &lhs, void const *rhs) {
            // Equal is only ever invoked with an object of this specific type.
            // That is, we only ever ask a proxy to compare to a proxy; we never
            // ask a proxy to compare to the proxied object.
            return ProxyHelper::Equal(
                GetObj(lhs), *static_cast<T const *>(rhs));
        }

        static void _Move(_Storage &src, _Storage &dst) noexcept {
            new (&_Container(dst)) Container(std::move(_Container(src)));
            _Destroy(src);
        }

        static void _MakeMutable(_Storage &storage) {
            GetMutableObj(storage);
        }

        static TfPyObjWrapper _GetPyObj(_Storage const &storage) {
            return ProxyHelper::GetPyObj(GetObj(storage));
        }

        static std::ostream &_StreamOut(
            _Storage const &storage, std::ostream &out) {
            return ProxyHelper::StreamOut(GetObj(storage), out);
        }

        static std::type_info const &_GetTypeid(_Storage const &storage) {
            return ProxyHelper::GetTypeid(GetObj(storage));
        }

        static bool _IsArrayValued(_Storage const &storage) {
            return ProxyHelper::IsArrayValued(GetObj(storage));
        }

        static std::type_info const &
        _GetElementTypeid(_Storage const &storage) {
            return ProxyHelper::GetElementTypeid(GetObj(storage));
        }

        static const Vt_ShapeData* _GetShapeData(_Storage const &storage) {
            return ProxyHelper::GetShapeData(GetObj(storage));
        }

        static size_t _GetNumElements(_Storage const &storage) {
            return ProxyHelper::GetNumElements(GetObj(storage));
        }

        static bool
        _ProxyHoldsType(_Storage const &storage, std::type_info const &t) {
            return ProxyHelper::HoldsType(GetObj(storage), t);
        }

        static TfType
        _GetProxiedType(_Storage const &storage) {
            return ProxyHelper::GetTfType(GetObj(storage));
        }

        static std::type_info const &
        _GetProxiedTypeid(_Storage const &storage) {
            return ProxyHelper::GetTypeid(GetObj(storage));
        }

        static void const *
        _GetProxiedObjPtr(_Storage const &storage) {
            return ProxyHelper::GetObjPtr(GetObj(storage));
        }

        static VtValue
        _GetProxiedAsVtValue(_Storage const &storage) {
            return ProxyHelper::GetProxiedAsVtValue(GetObj(storage));
        }

        ////////////////////////////////////////////////////////////////////
        // Internal helper -- cast type-generic storage to type-specific
        // container.
        static Container &_Container(_Storage &storage) {
            // XXX Will need std::launder in c++17.
            return *reinterpret_cast<Container *>(&storage);
        }
        static Container const &_Container(_Storage const &storage) {
            // XXX Will need std::launder in c++17.
            return *reinterpret_cast<Container const *>(&storage);
        }
    };

    ////////////////////////////////////////////////////////////////////////
    // Local-storage type info implementation.  The container and the object are
    // the same -- there is no distinct container.
    template <class T>
    struct _LocalTypeInfo : _TypeInfoImpl<
        T,                 // type
        T,                 // container
        _LocalTypeInfo<T>  // CRTP
        >
    {
        constexpr _LocalTypeInfo()
            : _TypeInfoImpl<T, T, _LocalTypeInfo<T>>()
        {}

        // Get returns object directly.
        static T &_GetMutableObj(T &obj) { return obj; }
        static T const &_GetObj(T const &obj) { return obj; }
        // Place placement new's object directly.
        static void _PlaceCopy(T *dst, T const &src) { new (dst) T(src); }
    };

    ////////////////////////////////////////////////////////////////////////
    // Remote-storage type info implementation.  The container is an
    // TfDelegatedCountPtr to an object holder: _Counted<T>.
    template <class T>
    struct _RemoteTypeInfo : _TypeInfoImpl<
        T,                                   // type
        TfDelegatedCountPtr<_Counted<T>>, // container
        _RemoteTypeInfo<T>                   // CRTP
        >
    {
        constexpr _RemoteTypeInfo()
            : _TypeInfoImpl<
                  T, TfDelegatedCountPtr<_Counted<T>>, _RemoteTypeInfo<T>>()
        {}

        using Ptr = TfDelegatedCountPtr<_Counted<T>>;
        // Get returns object stored in the pointed-to _Counted<T>.
        static T &_GetMutableObj(Ptr &ptr) {
            if (!ptr->IsUnique()) {
                ptr = TfMakeDelegatedCountPtr<_Counted<T>>(ptr->Get());
            }
            return ptr->GetMutable();
        }
        static T const &_GetObj(Ptr const &ptr) { return ptr->Get(); }
        // PlaceCopy() allocates a new _Counted<T> with a copy of the object.
        static void _PlaceCopy(Ptr *dst, T const &src) {
            new (dst) Ptr(TfDelegatedCountIncrementTag, new _Counted<T>(src));
        }
    };

    // Metafunction that returns the specific _TypeInfo subclass for T.
    template <class T>
    using _TypeInfoFor =
        TfConditionalType<_UsesLocalStore<T>::value,
                          _LocalTypeInfo<T>, _RemoteTypeInfo<T>>;

    // A helper that moves a held value to temporary storage, but keeps it alive
    // until the _HoldAside object is destroyed.  This is used when assigning
    // over a VtValue that might own the object being assigned.  For instance,
    // if I have a VtValue holding a map<string, VtValue>, and I reassign this
    // VtValue with one of the elements from the map, we must ensure that the
    // map isn't destroyed until after the assignment has taken place.
    friend struct _HoldAside;
    struct _HoldAside {
        explicit _HoldAside(VtValue *val)
            : info((val->IsEmpty() || val->_IsLocalAndTriviallyCopyable())
                   ? static_cast<_TypeInfo const *>(NULL) : val->_info.Get()) {
            if (info)
                info->Move(val->_storage, storage);
        }
        ~_HoldAside() {
            if (info)
                info->Destroy(storage);
        }
        _Storage storage;
        _TypeInfo const *info;
    };

    template <class T>
    struct _Init {
        using StoredType = typename Vt_ValueGetStored<T>::Type;
        using TypeInfo = _TypeInfoFor<StoredType>;

        static TfPointerAndBits<const _TypeInfo> _GetTypeInfo() {
            static const TypeInfo ti;
            static constexpr unsigned int flags =
                (TypeInfo::IsLocal ? _LocalFlag : 0) |
                (TypeInfo::HasTrivialCopy ? _TrivialCopyFlag : 0) |
                (TypeInfo::IsProxy ? _ProxyFlag : 0);
            return TfPointerAndBits<const _TypeInfo>(&ti, flags);
        }

        static void Init(VtValue *val, T const &obj) {
            val->_info = _GetTypeInfo();
            if constexpr (std::is_same_v<T, StoredType>) {
                TypeInfo::CopyInitObj(obj, val->_storage);
            }
            else {
                TypeInfo::CopyInitObj(StoredType {obj}, val->_storage);
            }
        }
    };

public:

    /// Default ctor gives empty VtValue.
    VtValue() {}

    /// Copy construct with \p other.
    VtValue(VtValue const &other) {
        _Copy(other, *this);
    }

    /// Move construct with \p other.
    VtValue(VtValue &&other) noexcept {
        _Move(other, *this);
    }

    /// Construct a VtValue holding a copy of \p obj.
    /// 
    /// If T is a char pointer or array, produce a VtValue holding a
    /// std::string. If T is pxr_boost::python::object, produce a VtValue holding
    /// a TfPyObjWrapper.
    template <class T>
    explicit VtValue(T const &obj) {
        _Init<T>::Init(this, obj);
    }

    /// Create a new VtValue, taking its contents from \p obj.
    /// 
    /// This is equivalent to creating a VtValue holding a value-initialized
    /// \p T instance, then invoking swap(<held-value>, obj), leaving obj in a
    /// default-constructed (value-initialized) state.  In the case that \p
    /// obj is expensive to copy, it may be significantly faster to use this
    /// idiom when \p obj need not retain its contents:
    ///
    /// \code
    /// MyExpensiveObject obj = CreateObject();
    /// return VtValue::Take(obj);
    /// \endcode
    ///
    /// Rather than:
    ///
    /// \code
    /// MyExpensiveObject obj = CreateObject();
    /// return VtValue(obj);
    /// \endcode
    template <class T>
    static VtValue Take(T &obj) {
        VtValue ret;
        ret.Swap(obj);
        return ret;
    }

    /// Destructor.
    ~VtValue() { _Clear(); }

    /// Copy assignment from another \a VtValue.
    VtValue &operator=(VtValue const &other) {
        if (ARCH_LIKELY(this != &other))
            _Copy(other, *this);
        return *this;
    }

    /// Move assignment from another \a VtValue.
    VtValue &operator=(VtValue &&other) noexcept {
        if (ARCH_LIKELY(this != &other))
            _Move(other, *this);
        return *this;
    }

    /// Assignment operator from any type.
    template <class T>
    VtValue &
    operator=(T const &obj) {
        if constexpr (_TypeInfoFor<T>::IsLocal &&
                      _TypeInfoFor<T>::HasTrivialCopy) {
            _Clear();
            _Init<T>::Init(this, obj);
            return *this;
        }
        else {
            _HoldAside tmp(this);
            _Init<T>::Init(this, obj);
            return *this;
        }
    }

    /// Swap this with \a rhs.
    VtValue &Swap(VtValue &rhs) noexcept {
        // Do nothing if both empty.  Otherwise general swap.
        if (!IsEmpty() || !rhs.IsEmpty()) {
            VtValue tmp;
            _Move(*this, tmp);
            _Move(rhs, *this);
            _Move(tmp, rhs);
        }
        return *this;
    }

    /// Overloaded swap() for generic code/stl/etc.
    friend void swap(VtValue &lhs, VtValue &rhs) { lhs.Swap(rhs); }

    /// Swap the held value with \a rhs.  If this value is holding a T, make an
    /// unqualified call to swap(<held-value>, rhs).  If this value is not
    /// holding a T, replace the held value with a value-initialized T instance
    /// first, then swap.
    template <class T>
    void
    Swap(T &rhs) {
        static_assert(std::is_same_v<T, typename Vt_ValueGetStored<T>::Type>,
                      "Can only VtValue::Swap with a type T that stores as T");
        if (!IsHolding<T>())
            *this = T();
        UncheckedSwap(rhs);
    }

    /// Swap the held value with \a rhs.  This VtValue must be holding an
    /// object of type \p T.  If it does not, this invokes undefined behavior.
    /// Use Swap() if this VtValue is not known to contain an object of type
    /// \p T.
    template <class T>
    void
    UncheckedSwap(T &rhs) {
        static_assert(std::is_same_v<T, typename Vt_ValueGetStored<T>::Type>,
                      "Can only VtValue::Swap with a type T that stores as T");
        using std::swap;
        swap(_GetMutable<T>(), rhs);
    }

    /// \overload
    void UncheckedSwap(VtValue &rhs) { Swap(rhs); }

    /// Make this value empty and return the held \p T instance.  If
    /// this value does not hold a \p T instance, make this value empty and
    /// return a default-constructed \p T.
    template <class T>
    T Remove() {
        T result;
        Swap(result);
        _Clear();
        return result;
    }

    /// Make this value empty and return the held \p T instance.  If this
    /// value does not hold a \p T instance, this method invokes undefined
    /// behavior.
    template <class T>
    T UncheckedRemove() {
        T result;
        UncheckedSwap(result);
        _Clear();
        return result;
    }

    /// If this value holds an object of type \p T, invoke \p mutateFn, passing
    /// it a non-const reference to the held object and return true.  Otherwise
    /// do nothing and return false.
    template <class T, class Fn>
    bool
    Mutate(Fn &&mutateFn) {
        static_assert(std::is_same_v<T, typename Vt_ValueGetStored<T>::Type>,
                      "Can only VtValue::Mutate a type T that stores as T");
        if (!IsHolding<T>()) {
            return false;
        }
        UncheckedMutate<T>(std::forward<Fn>(mutateFn));
        return true;
    }

    /// Invoke \p mutateFn, passing it a non-const reference to the held object
    /// which must be of type \p T.  If the held object is not of type \p T,
    /// this function invokes undefined behavior.
    template <class T, class Fn>
    void
    UncheckedMutate(Fn &&mutateFn) {
        static_assert(std::is_same_v<T, typename Vt_ValueGetStored<T>::Type>,
                      "Can only VtValue::Mutate a type T that stores as T");
        // We move to a temporary, mutate the temporary, then move back.  This
        // prevents callers from escaping a mutable reference to the held object
        // via a side-effect of mutateFn.
        T &stored =_GetMutable<T>();
        T tmp = std::move(stored);
        std::forward<Fn>(mutateFn)(tmp);
        stored = std::move(tmp);
    }

    /// Return true if this value is holding an object of type \p T, false
    /// otherwise.
    template <class T>
    bool IsHolding() const {
        return _info.GetLiteral() && _TypeIs<T>();
    }

    /// Return true if this holds a VtArray instance, false otherwise.
    VT_API bool IsArrayValued() const;

    /// Return the number of elements in the held value if IsArrayValued(),
    /// return 0 otherwise.
    size_t GetArraySize() const { return _GetNumElements(); }

    /// Return the typeid of the type held by this value.
    VT_API std::type_info const &GetTypeid() const;

    /// If this value holds a VtArray or VtArrayEdit instance, return the typeid
    /// of its element type.  For example, if this value holds a VtIntArray or a
    /// VtIntArrayEdit, return typeid(int).  Otherwise return typeid(void).
    VT_API std::type_info const &GetElementTypeid() const;

    /// Returns the TfType of the type held by this value.
    VT_API TfType GetType() const;

    /// Return the type name of the held typeid.
    VT_API std::string GetTypeName() const;

    /// Return VtKnownValueTypeIndex<T> for the held type T.  If this value
    /// holds a proxy type, resolve the proxy and return the proxied type's
    /// index.  If this value is empty or holds a type that is not 'known',
    /// return -1.
    int GetKnownValueTypeIndex() const {
        if (ARCH_UNLIKELY(_IsProxy())) {
            return _info->GetProxiedAsVtValue(
                _storage).GetKnownValueTypeIndex();
        }
        return _info.GetLiteral() ? _info->knownTypeIndex : -1;
    }

    /// Returns a const reference to the held object if the held object
    /// is of type \a T.  Invokes undefined behavior otherwise.  This is the
    /// fastest \a Get() method to use after a successful \a IsHolding() check.
    template <class T>
    T const &UncheckedGet() const & { return _Get<T>(); }

    /// \overload 
    /// In case *this is an rvalue, move the held value out and return
    /// by value.
    template <class T>
    T UncheckedGet() && { return UncheckedRemove<T>(); }

    /// Returns a const reference to the held object if the held object
    /// is of type \a T.  Issues an error and returns a const reference to a
    /// default value if the held object is not of type \a T.  Use \a IsHolding
    /// to verify correct type before calling this function.  The default value
    /// returned in case of type mismatch is constructed using
    /// Vt_DefaultValueFactory<T>.  That may be specialized for client types.
    /// The default implementation of the default value factory produces a
    /// value-initialized T.
    template <class T>
    T const &Get() const & {
        typedef Vt_DefaultValueFactory<T> Factory;

        // In the unlikely case that the types don't match, we obtain a default
        // value to return and issue an error via _FailGet.
        if (ARCH_UNLIKELY(!IsHolding<T>())) {
            return *(static_cast<T const *>(
                         _FailGet(Factory::Invoke, typeid(T))));
        }

        return _Get<T>();
    }

    /// \overload 
    /// In case *this is an rvalue, move the held value out and return
    /// by value.
    template <class T>
    T Get() && {
        typedef Vt_DefaultValueFactory<T> Factory;

        // In the unlikely case that the types don't match, we obtain a default
        // value to return and issue an error via _FailGet.
        if (ARCH_UNLIKELY(!IsHolding<T>())) {
            return *(static_cast<T const *>(
                         _FailGet(Factory::Invoke, typeid(T))));
        }

        return UncheckedRemove<T>();
    }    

    /// Return a copy of the held object if the held object is of type T.
    /// Return a copy of the default value \a def otherwise.  Note that this
    /// always returns a copy, as opposed to \a Get() which always returns a
    /// reference.
    template <class T>
    T GetWithDefault(T const &def = T()) const {
        return IsHolding<T>() ? UncheckedGet<T>() : def;
    }

    /// Register a cast from VtValue holding From to VtValue holding To.
    template <typename From, typename To>
    static void RegisterCast(VtValue (*castFn)(VtValue const &)) {
        _RegisterCast(typeid(From), typeid(To), castFn);
    }

    /// Register a simple cast from VtValue holding From to VtValue
    // holding To.
    template <typename From, typename To>
    static void RegisterSimpleCast() {
        _RegisterCast(typeid(From), typeid(To), _SimpleCast<From, To>);
    }

    /// Register a two-way cast from VtValue holding From to VtValue
    /// holding To.
    template <typename From, typename To>
    static void RegisterSimpleBidirectionalCast() {
        RegisterSimpleCast<From, To>();
        RegisterSimpleCast<To, From>();
    }

    /// Return a VtValue holding \c val cast to hold T.  Return empty VtValue
    /// if cast fails.
    ///
    /// This Cast() function is safe to call in multiple threads as it does
    /// not mutate the operant \p val.
    ///
    /// \sa \ref VtValue_Casting
    template <typename T>
    static VtValue Cast(VtValue const &val) {
        VtValue ret = val;
        ret.Cast<T>();
        return ret;
    }

    /// Return a VtValue holding \c val cast to same type that \c other is
    /// holding.  Return empty VtValue if cast fails.
    ///
    /// This Cast() function is safe to call in multiple threads as it does not
    /// mutate the operant \p val.
    ///
    /// \sa \ref VtValue_Casting
    VT_API static VtValue
    CastToTypeOf(VtValue const &val, VtValue const &other);

    /// Return a VtValue holding \a val cast to \a type.  Return empty VtValue
    /// if cast fails.
    ///
    /// This Cast() function is safe to call in multiple threads as it does not
    /// mutate the operant \p val.
    ///
    /// \sa \ref VtValue_Casting
    VT_API static VtValue
    CastToTypeid(VtValue const &val, std::type_info const &type);

    /// Return if a value of type \a from can be cast to type \a to.
    ///
    /// \sa \ref VtValue_Casting
    static bool CanCastFromTypeidToTypeid(std::type_info const &from,
                                          std::type_info const &to) {
        return _CanCast(from, to);
    }

    /// Return \c this holding value type cast to T.  This value is left
    /// empty if the cast fails.
    ///
    /// \note Since this method mutates this value, it is not safe to invoke on
    /// the same VtValue in multiple threads simultaneously.
    ///
    /// \sa \ref VtValue_Casting
    template <typename T>
    VtValue &Cast() {
        if (IsHolding<T>())
            return *this;
        return *this = _PerformCast(typeid(T), *this);
    }

    /// Return \c this holding value type cast to same type that
    /// \c other is holding.  This value is left empty if the cast fails.
    ///
    /// \note Since this method mutates this value, it is not safe to invoke on
    /// the same VtValue in multiple threads simultaneously.
    ///
    /// \sa \ref VtValue_Casting
    VtValue &CastToTypeOf(VtValue const &other) {
        return CastToTypeid(other.GetTypeid());
    }

    /// Return \c this holding value type cast to \a type.  This value is
    /// left empty if the cast fails.
    ///
    /// \note Since this method mutates this value, it is not safe to invoke on
    /// the same VtValue in multiple threads simultaneously.
    ///
    /// \sa \ref VtValue_Casting
    VtValue &CastToTypeid(std::type_info const &type) {
        if (!TfSafeTypeCompare(GetTypeid(), type)) {
            *this = _PerformCast(type, *this);
        }
        return *this;
    }

    /// Return if \c this can be cast to \a T.
    ///
    /// \sa \ref VtValue_Casting
    template <typename T>
    bool CanCast() const {
        return _CanCast(GetTypeid(), typeid(T));
    }

    /// Return if \c this can be cast to \a type.
    ///
    /// \sa \ref VtValue_Casting
    bool CanCastToTypeOf(VtValue const &other) const {
        return _CanCast(GetTypeid(), other.GetTypeid());
    }

    /// Return if \c this can be cast to \a type.
    ///
    /// \sa \ref VtValue_Casting
    bool CanCastToTypeid(std::type_info const &type) const {
        return _CanCast(GetTypeid(), type);
    }

    /// Returns true iff this value is empty.
    bool IsEmpty() const { return _info.GetLiteral() == 0; }

    /// Return true if the held object provides a hash implementation.
    VT_API bool CanHash() const;

    /// Return a hash code for the held object by calling VtHashValue() on it.
    VT_API size_t GetHash() const;

    friend inline size_t hash_value(VtValue const &val) {
        return val.GetHash();
    }

    /// Tests for equality.
    template <typename T>
    friend bool operator == (VtValue const &lhs, T const &rhs) {
        typedef typename Vt_ValueGetStored<T>::Type Stored;
        return lhs.IsHolding<Stored>() && lhs.UncheckedGet<Stored>() == rhs;
    }
    template <typename T>
    friend bool operator == (T const &lhs, VtValue const &rhs) {
        return rhs == lhs;
    }

    /// Tests for inequality.
    template <typename T>
    friend bool operator != (VtValue const &lhs, T const &rhs) {
        return !(lhs == rhs);
    }
    template <typename T>
    friend bool operator != (T const &lhs, VtValue const &rhs) {
        return !(lhs == rhs);
    }

    /// Test two values for equality.
    bool operator == (const VtValue &rhs) const {
        bool empty = IsEmpty(), rhsEmpty = rhs.IsEmpty();
        if (empty || rhsEmpty) {
            // Either one or both empty -- only equal if both empty.
            return empty == rhsEmpty;
        }
        if (_info.GetLiteral() == rhs._info.GetLiteral()) {
            // Holding identical types -- compare directly.
            return _info.Get()->Equal(_storage, rhs._storage);
        }
        return _EqualityImpl(rhs);
    }
    bool operator != (const VtValue &rhs) const { return !(*this == rhs); }

    /// Calls through to operator << on the held object.
    VT_API friend std::ostream &
    operator << (std::ostream &out, const VtValue &self);

private:
    VT_API const Vt_ShapeData* _GetShapeData() const;
    VT_API size_t _GetNumElements() const;
    friend struct Vt_ValueShapeDataAccess;

    static inline void _Copy(VtValue const &src, VtValue &dst) {
        if (src.IsEmpty()) {
            dst._Clear();
            return;
        }

        _HoldAside tmp(&dst);
        dst._info = src._info;
        if (src._IsLocalAndTriviallyCopyable()) {
            dst._storage = src._storage;
        } else {
            dst._info->CopyInit(src._storage, dst._storage);
        }
    }

    static inline void _Move(VtValue &src, VtValue &dst) noexcept {
        if (src.IsEmpty()) {
            dst._Clear();
            return;
        }

        _HoldAside tmp(&dst);
        dst._info = src._info;
        if (src._IsLocalAndTriviallyCopyable()) {
            dst._storage = src._storage;
        } else {
            dst._info->Move(src._storage, dst._storage);
        }

        src._info.Set(nullptr, 0);
    }

    template <class T>
    inline bool
    _TypeIs() const {
        if constexpr (VtIsKnownValueType_Workaround<T>::value) {
            return _info->knownTypeIndex == VtGetKnownValueTypeIndex<T>() ||
                ARCH_UNLIKELY(_IsProxy() && _TypeIsImpl(typeid(T)));
        }
        else {
            std::type_info const &t = typeid(T);
            return TfSafeTypeCompare(_info->typeInfo, t) ||
                ARCH_UNLIKELY(_IsProxy() && _TypeIsImpl(t));
        }
    }

    VT_API bool _TypeIsImpl(std::type_info const &queriedType) const;

    VT_API bool _EqualityImpl(VtValue const &rhs) const;

    template <class T>
    T &
    _GetMutable() {
        using TypeInfo = _TypeInfoFor<T>;
        if constexpr (!VtIsValueProxy<T>::value) {
            // The request is not for a proxy type, so if we are holding a
            // proxy, collapse it out to the proxied value first.
            if (ARCH_UNLIKELY(_IsProxy())) {
                *this = _info->GetProxiedAsVtValue(_storage);
            }
        }
        return TypeInfo::GetMutableObj(_storage);
    }

    template <class T>
    T const &
    _Get() const {
        using TypeInfo = _TypeInfoFor<T>;
        if constexpr (!VtIsValueProxy<T>::value) {
            // The request is not for a proxy type, so if we are holding a
            // proxy, fetch the underlying proxied object pointer.
            if (ARCH_UNLIKELY(_IsProxy())) {
                return *static_cast<T const *>(_GetProxiedObjPtr());
            }
        }
        return TypeInfo::GetObj(_storage);
    }

    void const *_GetProxiedObjPtr() const {
        return _info->GetProxiedObjPtr(_storage);
    }

    // Helper invoked in case Get fails.  Reports an error and returns a default
    // value for \a queryType.
    VT_API void const *
    _FailGet(Vt_DefaultValueHolder (*factory)(),
             std::type_info const &queryType) const;

    inline void _Clear() {
        // optimize for local types not to deref _info.
ARCH_PRAGMA_PUSH
// XXX: http://bug/DEV-16695
ARCH_PRAGMA_MAYBE_UNINITIALIZED
        if (_info.GetLiteral() && !_IsLocalAndTriviallyCopyable())
            _info.Get()->Destroy(_storage);
ARCH_PRAGMA_POP
        _info.Set(nullptr, 0);
    }

    inline bool _IsLocalAndTriviallyCopyable() const {
        unsigned int bits = _info.BitsAs<unsigned int>();
        return (bits & (_LocalFlag | _TrivialCopyFlag)) ==
            (_LocalFlag | _TrivialCopyFlag);
    }

    inline bool _IsProxy() const {
        return _info.BitsAs<unsigned int>() & _ProxyFlag;
    }

    VT_API static void _RegisterCast(std::type_info const &from,
                                     std::type_info const &to,
                                     VtValue (*castFn)(VtValue const &));

    // Cast \p value to the type \p to.  Caller must ensure that val's type is
    // not already \p to.
    VT_API static VtValue
    _PerformCast(std::type_info const &to, VtValue const &val);
 
    // Return true if \p from == \p to or if there is a registered cast to
    // convert VtValues holding \p from to \p to.
    VT_API static bool
    _CanCast(std::type_info const &from, std::type_info const &to);

    // helper template function for simple casts from From to To.
    template <typename From, typename To>
    static VtValue _SimpleCast(VtValue const &val) {
        return VtValue(To(val.UncheckedGet<From>()));
    }

    // This grants friend access to a function in the wrapper file for this
    // class.  This lets the wrapper reach down into a value to get a
    // pxr_boost::python wrapped object corresponding to the held type.  This
    // facility is necessary to get the python API we want.
    friend TfPyObjWrapper
    Vt_GetPythonObjectFromHeldValue(VtValue const &self);

    VT_API TfPyObjWrapper _GetPythonObject() const;

    _Storage _storage;
    TfPointerAndBits<const _TypeInfo> _info;
};

#ifndef doxygen

struct Vt_ValueShapeDataAccess {
    static const Vt_ShapeData* _GetShapeData(const VtValue& value) {
        return value._GetShapeData();
    }

    static size_t _GetNumElements(const VtValue& value) {
        return value._GetNumElements();
    }
};

/// Make a default value.  VtValue uses this to create values to be returned
/// from failed calls to \a Get.  Clients may specialize this for their own
/// types.
template <class T>
struct Vt_DefaultValueFactory {
    static Vt_DefaultValueHolder Invoke();
};

template <class T>
inline Vt_DefaultValueHolder
Vt_DefaultValueFactory<T>::Invoke() {
    return Vt_DefaultValueHolder::Create<T>();
}

// For performance reasons, the default constructors for vectors,
// matrices, and quaternions do *not* initialize the data of the
// object.  This greatly improves the performance of creating large
// arrays of objects.  However, for consistency and to avoid
// errors complaining about uninitialized values, we use VtZero
// to construct zeroed out vectors, matrices, and quaternions by
// explicitly instantiating the factory for these types. 
//
#define _VT_DECLARE_ZERO_VALUE_FACTORY(unused, elem)                    \
template <>                                                             \
VT_API Vt_DefaultValueHolder Vt_DefaultValueFactory<VT_TYPE(elem)>::Invoke();

TF_PP_SEQ_FOR_EACH(_VT_DECLARE_ZERO_VALUE_FACTORY, ~,
                   VT_VEC_VALUE_TYPES
                   VT_MATRIX_VALUE_TYPES
                   VT_QUATERNION_VALUE_TYPES
                   VT_DUALQUATERNION_VALUE_TYPES)

#undef _VT_DECLARE_ZERO_VALUE_FACTORY

//
// The Get()/IsHolding routines needs to be special-cased to handle getting a
// VtValue *as* a VtValue.
//

template <>
inline const VtValue&
VtValue::Get<VtValue>() const & {
    return *this;
}

template <>
inline VtValue
VtValue::Get<VtValue>() && {
    return std::move(*this);
}

template <>
inline const VtValue&
VtValue::UncheckedGet<VtValue>() const & {
    return *this;
}

template <>
inline VtValue
VtValue::UncheckedGet<VtValue>() && {
    return std::move(*this);
}

template <>
inline bool
VtValue::IsHolding<VtValue>() const {
    return true;
}

// Specialize VtValue::IsHolding<void>() to always return false.
template <>
inline bool
VtValue::IsHolding<void>() const {
    return false;
}



#endif // !doxygen

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_VALUE_H
