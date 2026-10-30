//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_OBJ_WRAPPER_H
#define PXR_BASE_TF_PY_OBJ_WRAPPER_H

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/arch/pragmas.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
// Include this header first to pick up additional mitigations
// for build issues when including Python.h
#include "pxr/base/tf/pySafePython.h"

#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace nanobind {
class handle;
class object;
namespace detail {
template <typename Impl> class accessor;
} // namespace detail
} // namespace nanobind

#else

#include <type_traits>

#endif

PXR_NAMESPACE_OPEN_SCOPE

// We define the empty stub for ABI compatibility even if Python support is
// enabled so we can make sure size and alignment is the same.
class TfPyObjWrapperStub
{
public:
    static constexpr std::size_t Size = 16;
    static constexpr std::size_t Align = 8;

private:
    ARCH_PRAGMA_PUSH
    ARCH_PRAGMA_UNUSED_PRIVATE_FIELD
    std::aligned_storage<Size, Align>::type _stub;
    ARCH_PRAGMA_POP
};


/// \class TfPyObjWrapper
///
/// Python object wrapper.
///
/// Provides a wrapper around nanobind::object that works correctly for the
/// following basic operations regardless of the GIL state: default construction,
/// copy construction, assignment, (in)equality comparison, hash_value(), and
/// destruction.
///
/// None of those work correctly in the presence of an unlocked GIL for
/// nanobind::object.  This class only actually acquires the GIL for default
/// construction, destruction and for some (in)equality comparisons.  The other
/// operations do not require taking the GIL.
///
/// This is primarily useful in cases where a nanobind::object might be
/// destroyed without a locked GIL by a client blind to that fact.  This occurs
/// when a registry, for example, holds type-erased objects.  If one
/// of the type-erased objects in the registry happens to hold a
/// nanobind::object, that type-erased object must be destroyed while the
/// GIL is held but it's unreasonable to require that the registry know that.
/// This class helps solve that problem.
#ifdef PXR_PYTHON_SUPPORT_ENABLED
class TfPyObjWrapper
{
    typedef nanobind::object object;

public:

    /// Default construct a TfPyObjWrapper holding a reference to python None.
    /// The GIL need not be held by the caller.
    TF_API TfPyObjWrapper();

    /// Construct a TfPyObjectWrapper wrapping \a obj.
    /// The GIL must be held by the caller.  Note, allowing the implicit
    /// conversion is intended here.
    TF_API TfPyObjWrapper(object obj);

    /// Construct a TfPyObjWrapper borrowing a reference from \a h.
    /// The GIL must be held by the caller.
    TF_API TfPyObjWrapper(nanobind::handle h);

    /// Construct a TfPyObjWrapper from a nanobind accessor.
    template <typename Impl, typename Obj = object>
    TfPyObjWrapper(nanobind::detail::accessor<Impl> const &acc)
        : TfPyObjWrapper(Obj(acc)) {}

    /// Underlying object access.
    /// This method returns a reference, so technically, the GIL need not be
    /// held to call this.  However, the caller is strongly advised to ensure
    /// the GIL is held, since assigning this object to another or otherwise
    /// operating on the returned object requires it.
    object const &Get() const {
        return *_objectPtr;
    }

    /// Implicit conversion to nanobind::object const &.
    operator object const &() const {
        return Get();
    }

    /// Return true if the wrapped Python object is valid and not None.
    explicit operator bool() const {
        PyObject *p = ptr();
        return p && p != Py_None;
    }

    /// Underlying PyObject* access.
    /// This method returns a pointer, so technically, the GIL need not be
    /// held to call this. However, the caller is strongly advised to ensure
    /// the GIL is held, since assigning this object to another or otherwise
    /// operating on the returned object requires it.  The returned PyObject *
    /// is a "borrowed reference", meaning that the underlying object's
    /// reference count has not been incremented on behalf of the caller.
    TF_API PyObject *ptr() const;

    /// Call operator forwarding to the underlying Python object.
    /// The GIL must be held by the caller.
    template <typename... Args, typename Obj = object>
    Obj operator()(Args&&... args) const {
        return static_cast<Obj const &>(Get())(std::forward<Args>(args)...);
    }

    /// Subscript operator forwarding to the underlying Python object.
    /// The GIL must be held by the caller.
    template <typename Key, typename Obj = object>
    auto operator[](Key&& key) const {
        return static_cast<Obj const &>(Get())[std::forward<Key>(key)];
    }

    /// Attribute access forwarding to the underlying Python object.
    /// The GIL must be held by the caller.
    template <typename Obj = object>
    auto attr(const char *key) const {
        return static_cast<Obj const &>(Get()).attr(key);
    }

    template <typename Handle = nanobind::handle, typename Obj = object>
    auto attr(Handle key) const {
        return static_cast<Obj const &>(Get()).attr(key);
    }

    /// Produce a hash code for this object.
    /// Note that this does not attempt to hash the underlying python object,
    /// it returns a hash code that's suitable for hash-table lookup of
    /// TfPyObjWrapper instances, and does not require taking the python lock.
    friend inline size_t hash_value(TfPyObjWrapper const &o) {
        return (size_t) o.ptr();
    }

    /// Equality.
    /// Returns true if \a other refers to the same python object.
    TF_API bool operator==(TfPyObjWrapper const &other) const;

    /// Inequality.
    /// Returns false if \a other refers to the same python object.
    TF_API bool operator!=(TfPyObjWrapper const &other) const;

private:

    // Store a shared_ptr to a python object.
    std::shared_ptr<object> _objectPtr;
};

static_assert(sizeof(TfPyObjWrapper) == sizeof(TfPyObjWrapperStub),
              "ABI break: Incompatible class sizes.");
static_assert(alignof(TfPyObjWrapper) == alignof(TfPyObjWrapperStub),
              "ABI break: Incompatible class alignments.");

class TfAnyWeakPtr;
class TfEnum;
class TfRefBase;
class TfToken;
class TfWeakBase;
template <class T> class TfWeakPtr;
template <class T> class TfRefPtr;
template <typename Type, uint32_t N> class TfSmallVector;
template <class Key, class HashFn, class EqualKey, class Alloc> class TfHashSet;
template <class Key, class Value, class HashFn, class EqualKey, class Alloc>
class TfHashMap;
namespace pxr_half {
class half;
}

template <typename T>
struct Tf_IsWeakPtr : std::false_type {};
template <typename U>
struct Tf_IsWeakPtr<TfWeakPtr<U>> : std::true_type {};

template <typename T>
struct Tf_IsRefPtr : std::false_type {};
template <typename U>
struct Tf_IsRefPtr<TfRefPtr<U>> : std::true_type {};

template <typename P, typename = void>
struct Tf_PointeeHasWeakBase : std::false_type {};
template <typename P>
struct Tf_PointeeHasWeakBase<
    P, std::void_t<decltype(std::declval<P const &>().__GetTfWeakBase__())>>
    : std::true_type {};

template <typename T>
struct Tf_IsStdOptional : std::false_type {};
template <typename U>
struct Tf_IsStdOptional<std::optional<U>> : std::true_type {};

template <typename T>
struct Tf_IsStdPair : std::false_type {};
template <typename A, typename B>
struct Tf_IsStdPair<std::pair<A, B>> : std::true_type {};

template <typename T>
struct Tf_IsStdTuple : std::false_type {};
template <typename... Ts>
struct Tf_IsStdTuple<std::tuple<Ts...>> : std::true_type {};

template <typename T>
struct Tf_IsSequenceContainer : std::false_type {};
template <typename Type, typename Alloc>
struct Tf_IsSequenceContainer<std::vector<Type, Alloc>> : std::true_type {};
template <typename Type, uint32_t N>
struct Tf_IsSequenceContainer<TfSmallVector<Type, N>> : std::true_type {};

template <typename T>
struct Tf_IsSetContainer : std::false_type {};
template <typename Key, typename Compare, typename Alloc>
struct Tf_IsSetContainer<std::set<Key, Compare, Alloc>> : std::true_type {};
template <typename Key, typename Hash, typename Equal, typename Alloc>
struct Tf_IsSetContainer<std::unordered_set<Key, Hash, Equal, Alloc>>
    : std::true_type {};
template <class Key, class HashFn, class EqualKey, class Alloc>
struct Tf_IsSetContainer<TfHashSet<Key, HashFn, EqualKey, Alloc>>
    : std::true_type {};

template <typename T>
struct Tf_IsMapContainer : std::false_type {};
template <typename Key, typename Value, typename Compare, typename Alloc>
struct Tf_IsMapContainer<std::map<Key, Value, Compare, Alloc>>
    : std::true_type {};
template <typename Key, typename Value, typename Hash, typename Equal,
          typename Alloc>
struct Tf_IsMapContainer<std::unordered_map<Key, Value, Hash, Equal, Alloc>>
    : std::true_type {};
template <class Key, class Value, class HashFn, class EqualKey, class Alloc>
struct Tf_IsMapContainer<TfHashMap<Key, Value, HashFn, EqualKey, Alloc>>
    : std::true_type {};

TF_API TfPyObjWrapper Tf_PyObjSteal(PyObject *obj);
TF_API TfPyObjWrapper Tf_PyGetNone();
TF_API void Tf_PyThrowNoToPythonConverterError(std::type_info const &ti);
TF_API TfPyObjWrapper Tf_PyObjFromBool(bool val);
TF_API TfPyObjWrapper Tf_PyObjFromInt64(int64_t val);
TF_API TfPyObjWrapper Tf_PyObjFromUInt64(uint64_t val);
TF_API TfPyObjWrapper Tf_PyObjFromDouble(double val);
TF_API TfPyObjWrapper Tf_PyObjFromString(char const *str, size_t size);
TF_API TfPyObjWrapper Tf_PyObjFromCStr(char const *str);
TF_API TfPyObjWrapper Tf_PyObjFromToken(TfToken const &val);
TF_API TfPyObjWrapper Tf_PyObjFromTfEnum(TfEnum const &val);
TF_API TfPyObjWrapper Tf_PyObjFromEnum(std::type_info const &ti, long val);
TF_API TfPyObjWrapper Tf_PyObjFromAnyWeakPtr(TfAnyWeakPtr const &val);
TF_API TfPyObjWrapper Tf_PyObjFromWeakPtr(
    void const *uniqueId,
    void const *rawPtr,
    void const *mostDerivedPtr,
    std::type_info const &staticTi,
    std::type_info const *dynTi,
    void (*enableNotification)(void const *),
    void const *weakPtrAddr);
TF_API TfPyObjWrapper Tf_PyObjFromRefPtrWithWeakBase(
    TfWeakBase const *weakBase,
    TfRefBase *refBase,
    void const *rawPtr,
    void const *mostDerivedPtr,
    std::type_info const &staticTi,
    std::type_info const *dynTi,
    PyObject *(*makeOwnerCapsule)(void const *),
    void const *refPtrAddr);
TF_API TfPyObjWrapper Tf_PyObjFromCppByTypeInfo(
    std::type_info const &ti,
    std::type_info const *dynTi,
    void const *val,
    bool isPointer);
TF_API TfPyObjWrapper Tf_PyMakeTuple(TfPyObjWrapper const *items, size_t size);
TF_API TfPyObjWrapper Tf_PyNewList(size_t size);
TF_API void Tf_PyListSetItem(
    TfPyObjWrapper &list, size_t index, TfPyObjWrapper const &item);
TF_API TfPyObjWrapper Tf_PyNewSet();
TF_API void Tf_PySetAdd(TfPyObjWrapper &set, TfPyObjWrapper const &item);
TF_API TfPyObjWrapper Tf_PyNewDict();
TF_API void Tf_PyDictSetItem(
    TfPyObjWrapper &dict,
    TfPyObjWrapper const &key,
    TfPyObjWrapper const &val);

using Tf_PyCustomToPythonConverter = PyObject *(*)(void const *);
TF_API void Tf_PyRegisterCustomToPythonConverter(
    std::type_info const &ti,
    Tf_PyCustomToPythonConverter fn);

template <typename T, typename Enable = void>
struct Tf_PyCustomCppToPy {
    static constexpr bool value = false;
};

template <typename T>
TfPyObjWrapper
Tf_PyObjFromCpp(T const &val)
{
    if constexpr (Tf_PyCustomCppToPy<T>::value) {
        return Tf_PyCustomCppToPy<T>::Convert(val);
    } else if constexpr (std::is_pointer_v<T> &&
                         std::is_function_v<std::remove_pointer_t<T>>) {
        Tf_PyThrowNoToPythonConverterError(typeid(T));
        return TfPyObjWrapper();
    } else if constexpr (std::is_same_v<T, TfPyObjWrapper>) {
        return val;
    } else if constexpr (std::is_same_v<T, bool>) {
        return Tf_PyObjFromBool(val);
    } else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
        return Tf_PyObjFromInt64(static_cast<int64_t>(val));
    } else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) {
        return Tf_PyObjFromUInt64(static_cast<uint64_t>(val));
    } else if constexpr (std::is_floating_point_v<T> ||
                         std::is_same_v<T, pxr_half::half>) {
        return Tf_PyObjFromDouble(static_cast<double>(val));
    } else if constexpr (std::is_same_v<T, std::string>) {
        return Tf_PyObjFromString(val.data(), val.size());
    } else if constexpr (std::is_same_v<T, const char *> ||
                         std::is_same_v<T, char *>) {
        return Tf_PyObjFromCStr(val);
    } else if constexpr (std::is_same_v<T, TfToken>) {
        return Tf_PyObjFromToken(val);
    } else if constexpr (std::is_same_v<T, TfEnum>) {
        return Tf_PyObjFromTfEnum(val);
    } else if constexpr (std::is_enum_v<T>) {
        return Tf_PyObjFromEnum(typeid(T), static_cast<long>(val));
    } else if constexpr (std::is_same_v<T, TfAnyWeakPtr>) {
        return Tf_PyObjFromAnyWeakPtr(val);
    } else if constexpr (Tf_IsWeakPtr<T>::value) {
        using Pointee = typename T::DataType;
        using NonConstPointee = std::remove_const_t<Pointee>;
        void const *uniqueId = val.GetUniqueIdentifier();
        if (!uniqueId) {
            return Tf_PyGetNone();
        }
        Pointee *rawPtr = get_pointer(val);
        void const *instPtr = rawPtr;
        std::type_info const *dynTi = nullptr;
        if constexpr (std::is_polymorphic_v<NonConstPointee>) {
            if (rawPtr) {
                dynTi = &typeid(*rawPtr);
                instPtr = dynamic_cast<void const *>(rawPtr);
            }
        }
        return Tf_PyObjFromWeakPtr(
            uniqueId, rawPtr, instPtr, typeid(NonConstPointee), dynTi,
            +[](void const *p) {
                static_cast<T const *>(p)->EnableExtraNotification();
            },
            &val);
    } else if constexpr (Tf_IsRefPtr<T>::value) {
        using Pointee = typename T::DataType;
        using NonConstPointee = std::remove_const_t<Pointee>;
        using NonConstRefPtr = TfRefPtr<NonConstPointee>;
        if (!val) {
            return Tf_PyGetNone();
        }
        Pointee *rawPtr = get_pointer(val);
        void const *instPtr = rawPtr;
        std::type_info const *dynTi = nullptr;
        if constexpr (std::is_polymorphic_v<NonConstPointee>) {
            if (rawPtr) {
                dynTi = &typeid(*rawPtr);
                instPtr = dynamic_cast<void const *>(rawPtr);
            }
        }
        if constexpr (Tf_PointeeHasWeakBase<NonConstPointee>::value) {
            TfWeakBase const *weakBase = &(rawPtr->__GetTfWeakBase__());
            TfRefBase *refBase = nullptr;
            if constexpr (std::is_base_of_v<TfRefBase, NonConstPointee>) {
                refBase = static_cast<TfRefBase *>(
                    const_cast<NonConstPointee *>(rawPtr));
            }
            NonConstRefPtr nonConstRef(const_cast<NonConstPointee *>(rawPtr));
            return Tf_PyObjFromRefPtrWithWeakBase(
                weakBase, refBase, rawPtr, instPtr,
                typeid(NonConstPointee), dynTi,
                +[](void const *refPtrAddr) -> PyObject * {
                    return PyCapsule_New(
                        new NonConstRefPtr(
                            *static_cast<NonConstRefPtr const *>(refPtrAddr)),
                        "refptr",
                        +[](PyObject *capsule) {
                            delete static_cast<NonConstRefPtr *>(
                                PyCapsule_GetPointer(capsule, "refptr"));
                        });
                },
                &nonConstRef);
        } else {
            return Tf_PyObjFromCppByTypeInfo(
                typeid(NonConstPointee), dynTi, rawPtr, /*isPointer=*/true);
        }
    } else if constexpr (Tf_IsStdOptional<T>::value) {
        return val.has_value() ? Tf_PyObjFromCpp(*val) : Tf_PyGetNone();
    } else if constexpr (Tf_IsStdPair<T>::value) {
        TfPyObjWrapper items[2] = {
            Tf_PyObjFromCpp(val.first),
            Tf_PyObjFromCpp(val.second)
        };
        return Tf_PyMakeTuple(items, 2);
    } else if constexpr (Tf_IsStdTuple<T>::value) {
        return std::apply([](auto const &... elems) {
            TfPyObjWrapper items[] = {
                Tf_PyObjFromCpp(elems)..., TfPyObjWrapper()
            };
            return Tf_PyMakeTuple(items, sizeof...(elems));
        }, val);
    } else if constexpr (Tf_IsSequenceContainer<T>::value) {
        TfPyObjWrapper list = Tf_PyNewList(val.size());
        size_t i = 0;
        for (auto const &elem : val) {
            Tf_PyListSetItem(list, i++, Tf_PyObjFromCpp(elem));
        }
        return list;
    } else if constexpr (Tf_IsSetContainer<T>::value) {
        TfPyObjWrapper set = Tf_PyNewSet();
        for (auto const &elem : val) {
            Tf_PySetAdd(set, Tf_PyObjFromCpp(elem));
        }
        return set;
    } else if constexpr (Tf_IsMapContainer<T>::value) {
        TfPyObjWrapper dict = Tf_PyNewDict();
        for (auto const &kv : val) {
            Tf_PyDictSetItem(
                dict, Tf_PyObjFromCpp(kv.first), Tf_PyObjFromCpp(kv.second));
        }
        return dict;
    } else {
        using Pointee = std::remove_cv_t<std::remove_pointer_t<T>>;
        void const *ptr = nullptr;
        if constexpr (std::is_pointer_v<T>) {
            ptr = val;
        } else {
            ptr = &val;
        }
        std::type_info const *dynTi = nullptr;
        if constexpr (std::is_polymorphic_v<Pointee>) {
            if (ptr) {
                dynTi = &typeid(*static_cast<Pointee const *>(ptr));
            }
        }
        return Tf_PyObjFromCppByTypeInfo(
            typeid(Pointee), dynTi, ptr, std::is_pointer_v<T>);
    }
}

#else // PXR_PYTHON_SUPPORT_ENABLED

class TfPyObjWrapper : TfPyObjWrapperStub
{
};

#endif // PXR_PYTHON_SUPPORT_ENABLED

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_OBJ_WRAPPER_H
