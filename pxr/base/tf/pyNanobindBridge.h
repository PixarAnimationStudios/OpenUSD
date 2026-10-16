//
// Copyright 2026 Google LLC
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_NANOBIND_BRIDGE_H
#define PXR_BASE_TF_PY_NANOBIND_BRIDGE_H

#include "pxr/base/tf/pySafePython.h"
#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/detail/nb_dict.h>
#include <nanobind/stl/detail/nb_list.h>
#include <nanobind/stl/detail/nb_set.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/unordered_set.h>
#include <nanobind/stl/vector.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>
#include <typeinfo>
#include <utility>

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/anyWeakPtr.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyIdentity.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_API nanobind::module_ TfPyGetCurrentNanobindModule();

PXR_NAMESPACE_CLOSE_SCOPE

namespace nanobind {
namespace detail {

template <typename Type, uint32_t N>
struct type_caster<PXR_NS::TfSmallVector<Type, N>>
    : list_caster<PXR_NS::TfSmallVector<Type, N>, Type> {};

template <typename Key, class HashFn, class EqualKey, class Alloc>
struct type_caster<PXR_NS::TfHashSet<Key, HashFn, EqualKey, Alloc>>
    : set_caster<PXR_NS::TfHashSet<Key, HashFn, EqualKey, Alloc>, Key> {};

template <typename Key, typename Value, class HashFn, class EqualKey,
          class Alloc>
struct type_caster<PXR_NS::TfHashMap<Key, Value, HashFn, EqualKey, Alloc>>
    : dict_caster<PXR_NS::TfHashMap<Key, Value, HashFn, EqualKey, Alloc>, Key,
                  Value> {};

template <>
struct type_caster<PXR_NS::TfPyObjWrapper> {
  NB_TYPE_CASTER(PXR_NS::TfPyObjWrapper, const_name("object"))

  bool from_python(handle src, uint8_t, cleanup_list*) noexcept {
    if (!src.is_valid()) {
      return false;
    }
    value = PXR_NS::TfPyObjWrapper(nanobind::borrow<nanobind::object>(src));
    return true;
  }

  static handle from_cpp(const PXR_NS::TfPyObjWrapper& v, rv_policy,
                         cleanup_list*) noexcept {
    PyObject* p = v.ptr();
    if (!p) {
      Py_RETURN_NONE;
    }
    Py_INCREF(p);
    return handle(p);
  }
};

template <typename T>
struct type_caster<T, enable_if_t<std::is_same_v<T, PXR_NS::TfAnyWeakPtr>>> {
  NB_TYPE_CASTER(T, const_name("Tf.AnyWeakPtr"))

  bool from_python(handle src, uint8_t flags, cleanup_list*) noexcept {
    if (!src.is_valid()) {
      return false;
    }
    if (src.is_none()) {
      if (flags & static_cast<uint8_t>(cast_flags::none_disallowed)) {
        return false;
      }
      value = T();
      return true;
    }
    return PXR_NS::Tf_PyExtractAnyWeakPtr(src.ptr(), &value);
  }

  static handle from_cpp(const T& v, rv_policy,
                         cleanup_list*) noexcept {
    PXR_NS::TfPyObjWrapper obj = Tf_GetPythonObjectFromAnyWeakPtr(v);
    PyObject* p = obj.ptr();
    if (!p) {
      Py_RETURN_NONE;
    }
    Py_INCREF(p);
    return handle(p);
  }
};

template <typename T>
struct type_caster<T, enable_if_t<std::is_same_v<T, PXR_NS::TfEnum>>> {
  NB_TYPE_CASTER(T, const_name("Tf.Enum"))

  bool from_python(handle src, uint8_t, cleanup_list*) noexcept {
    if (!src.is_valid()) {
      return false;
    }
    return PXR_NS::Tf_PyEnumRegistry::GetInstance().FindEnum(src.ptr(), &value);
  }

  static handle from_cpp(const T& src, rv_policy,
                         cleanup_list*) noexcept {
    return PXR_NS::Tf_PyEnumRegistry::GetInstance().GetPyObject(src);
  }
};

template <typename T>
struct type_caster<PXR_NS::TfWeakPtr<T>> {
  using WeakPtrType = PXR_NS::TfWeakPtr<T>;
  using Pointee = typename WeakPtrType::DataType;
  using NonConstPointee = std::remove_const_t<Pointee>;
  using NonConstWeakPtr = PXR_NS::TfWeakPtr<NonConstPointee>;

  NB_TYPE_CASTER(WeakPtrType, const_name<NonConstPointee>())

  bool from_python(handle src, uint8_t flags, cleanup_list* cleanup) noexcept {
    if (!src.is_valid()) {
      return false;
    }
    if (src.is_none()) {
      if ((flags & static_cast<uint8_t>(cast_flags::construct)) ||
          (flags & static_cast<uint8_t>(cast_flags::none_disallowed))) {
        return false;
      }
      value = WeakPtrType();
      return true;
    }
    NonConstPointee* ptr = nullptr;
    if (!NB_CALL(nb_type_get)(NB_CTX_C(cleanup), &typeid(NonConstPointee),
                              src.ptr(), flags, cleanup,
                              reinterpret_cast<void**>(&ptr)) ||
        !ptr) {
      return false;
    }
    value = WeakPtrType(ptr);
    if (value) {
      PXR_NS::Tf_PySetPythonIdentity(value, src.ptr());
    }
    return true;
  }

  static handle from_cpp(const WeakPtrType& p, rv_policy,
                         cleanup_list*) noexcept {
    if (!p.GetUniqueIdentifier()) {
      return none().release();
    }
    get_pointer(p);
    if (PyObject* id = PXR_NS::Tf_PyGetPythonIdentity(p)) {
      return handle(id);
    }
    nanobind::handle pyType = nanobind::type<NonConstPointee>();
    if (pyType.is_valid()) {
      Pointee* rawPtr = get_pointer(p);
      if (!rawPtr) {
        nanobind::object nbObj = nanobind::inst_alloc_zero(pyType);
        if (nbObj.is_valid()) {
          return nbObj.release();
        }
        return none().release();
      }
      void* instPtr = const_cast<NonConstPointee*>(rawPtr);
      if constexpr (std::is_polymorphic_v<NonConstPointee>) {
        if (PyObject* dynType =
                NB_CALL(nb_type_lookup)(NB_CTX, &typeid(*rawPtr))) {
          pyType = dynType;
          instPtr = const_cast<void*>(PXR_NS::TfCastToMostDerivedType(rawPtr));
        }
      }
      nanobind::object nbObj = nanobind::inst_reference(pyType, instPtr);
      if (nbObj.is_valid()) {
        PyObject* res = nbObj.release().ptr();
        PXR_NS::Tf_PySetPythonIdentity(
            PXR_NS::TfConst_cast<NonConstWeakPtr>(p), res);
        return handle(res);
      }
    }
    return none().release();
  }
};

template <typename T>
struct type_caster<PXR_NS::TfRefPtr<T>> {
  using RefPtrType = PXR_NS::TfRefPtr<T>;
  using Pointee = typename RefPtrType::DataType;
  using NonConstPointee = std::remove_const_t<Pointee>;
  using NonConstRefPtr = PXR_NS::TfRefPtr<NonConstPointee>;
  using WeakPtrType = PXR_NS::TfWeakPtr<NonConstPointee>;

  NB_TYPE_CASTER(RefPtrType, const_name<NonConstPointee>())

  bool from_python(handle src, uint8_t flags, cleanup_list* cleanup) noexcept {
    if (!src.is_valid()) {
      return false;
    }
    if (src.is_none()) {
      if ((flags & static_cast<uint8_t>(cast_flags::construct)) ||
          (flags & static_cast<uint8_t>(cast_flags::none_disallowed))) {
        return false;
      }
      value = RefPtrType();
      return true;
    }
    NonConstPointee* ptr = nullptr;
    if (!NB_CALL(nb_type_get)(NB_CTX_C(cleanup), &typeid(NonConstPointee),
                              src.ptr(), flags, cleanup,
                              reinterpret_cast<void**>(&ptr)) ||
        !ptr) {
      return false;
    }
    NonConstRefPtr nonConstPtr(ptr);
    PXR_NS::Tf_PyRemovePythonOwnership(nonConstPtr, src.ptr());
    if constexpr (PXR_NS::Tf_SupportsWeakPtr<NonConstPointee>::value) {
      WeakPtrType weakPtr(ptr);
      if (weakPtr) {
        PXR_NS::Tf_PySetPythonIdentity(weakPtr, src.ptr());
      }
    }
    value = RefPtrType(ptr);
    return true;
  }

  static handle from_cpp(const RefPtrType& src, rv_policy policy,
                         cleanup_list* cleanup) noexcept {
    if (!src) {
      return none().release();
    }
    if constexpr (PXR_NS::Tf_SupportsWeakPtr<NonConstPointee>::value) {
      WeakPtrType weakPtr(const_cast<NonConstPointee*>(get_pointer(src)));
      handle h = type_caster<WeakPtrType>::from_cpp(weakPtr, policy, cleanup);
      if (h.is_valid() && h.ptr() != Py_None &&
          !PyObject_HasAttrString(h.ptr(), "__owner")) {
        NonConstRefPtr nonConstRef(
            const_cast<NonConstPointee*>(get_pointer(src)));
        PXR_NS::Tf_PyAddPythonOwnership(
            nonConstRef, weakPtr.GetUniqueIdentifier(), h.ptr());
      }
      return h;
    } else {
      return type_caster_base<NonConstPointee>::from_cpp(
          const_cast<NonConstPointee*>(get_pointer(src)),
          rv_policy::reference, cleanup);
    }
  }
};

template <>
struct type_caster<PXR_NS::TfToken> {
  NB_TYPE_CASTER(PXR_NS::TfToken, const_name("str"))

  bool from_python(handle src, uint8_t, cleanup_list*) noexcept {
    Py_ssize_t size = 0;
    const char* str = PyUnicode_AsUTF8AndSize(src.ptr(), &size);
    if (!str) {
      PyErr_Clear();
      return false;
    }
    value = PXR_NS::TfToken(std::string(str, size));
    return true;
  }

  static handle from_cpp(const PXR_NS::TfToken& src, rv_policy,
                         cleanup_list*) noexcept {
    return PyUnicode_FromStringAndSize(src.GetText(), src.size());
  }
};

template <typename T>
struct has_arg_defaults<PXR_NS::TfWeakPtr<T>> : std::true_type {};
template <typename T>
struct has_arg_defaults<PXR_NS::TfRefPtr<T>> : std::true_type {};
template <>
struct has_arg_defaults<PXR_NS::TfAnyWeakPtr> : std::true_type {};
template <>
struct has_arg_defaults<PXR_NS::TfPyObjWrapper> : std::true_type {};

}  // namespace detail
}  // namespace nanobind

PXR_NAMESPACE_OPEN_SCOPE

template <typename T, bool IsScopedEnum = !std::is_convertible<T, int>::value>
void TfNbWrapEnum(nanobind::handle scope,
                  std::string const& name = std::string()) {
  (void)TfPyWrapEnum<T, IsScopedEnum>(scope, name);
}

template <typename T, bool IsScopedEnum = !std::is_convertible<T, int>::value>
void TfNbWrapEnum(std::string const& name = std::string()) {
  (void)TfPyWrapEnum<T, IsScopedEnum>(name);
}

template <typename Seq>
nanobind::tuple TfNbSequenceToTuple(Seq const& seq) {
  nanobind::list l;
  for (auto const& item : seq) {
    l.append(nanobind::cast(item));
  }
  return nanobind::tuple(l);
}

template <typename Seq>
nanobind::list TfNbSequenceToList(Seq const& seq) {
  nanobind::list l;
  for (auto const& item : seq) {
    l.append(nanobind::cast(item));
  }
  return l;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_BASE_TF_PY_NANOBIND_BRIDGE_H
