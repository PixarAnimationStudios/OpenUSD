//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_VALUE_FROM_PYTHON_H
#define PXR_BASE_VT_VALUE_FROM_PYTHON_H

/// \file vt/valueFromPython.h

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/gf/pyBufferUtils.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/tf/pySafePython.h"

#include <optional>
#include <string>
#include <type_traits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

template <typename T>
class VtArray;
class VtDictionary;
class VtValueRef;
VT_API bool VtDictionaryFromPythonObject(PyObject *obj, VtDictionary *out);
VT_API PyObject *VtDictionaryToPythonObject(VtDictionary const &dict);
VT_API bool VtValueFromPythonObject(PyObject *obj, VtValue *out);
VT_API TfPyObjWrapper Vt_GetPythonObjectFromHeldValue(VtValue const &self);
VT_API TfPyObjWrapper Vt_GetPythonObjectFromHeldValue(VtValueRef const &self);

// Lets python explicitly pass in values of C++ types which have no python
// equivalents.
struct Vt_ValueWrapper {
    template <class T>
    static Vt_ValueWrapper Create(T a) { return Vt_ValueWrapper(a); }

    template <typename T> explicit Vt_ValueWrapper(T val) : _val(val) {}

    VtValue const &GetValue() const { return _val; }

    bool operator==(const Vt_ValueWrapper &other) const {
        return _val == other._val;
    }

    bool operator!=(const Vt_ValueWrapper &other) const {
        return _val != other._val;
    }

    std::string GetAsString() const {
        return TfStringPrintf(
            "%s(%s)", _val.GetTypeName().c_str(), TfStringify(_val).c_str());
    }

private:
    VtValue _val;
};

/// \class Vt_ValueFromPythonRegistry
///
class Vt_ValueFromPythonRegistry {
public:

    static bool HasConversions() {
        return !_GetInstance()._lvalueExtractors.empty() &&
               !_GetInstance()._rvalueExtractors.empty();
    }
    
    VT_API static VtValue Invoke(PyObject *obj);

    template <class T>
    static void Register(bool registerRvalue) {
        if (!TfPyIsInitialized()) {
            TF_FATAL_ERROR("Tried to register a VtValue from python conversion "
                           "but python is not initialized!");
        }
        _GetInstance()._RegisterLValue(_Extractor::MakeLValue<T>());
        if (registerRvalue)
            _GetInstance()._RegisterRValue(_Extractor::MakeRValue<T>());
    }

    Vt_ValueFromPythonRegistry(Vt_ValueFromPythonRegistry const&) = delete;
    Vt_ValueFromPythonRegistry& operator=(
        Vt_ValueFromPythonRegistry const&) = delete;

    Vt_ValueFromPythonRegistry(Vt_ValueFromPythonRegistry &&) = delete;
    Vt_ValueFromPythonRegistry& operator=(
        Vt_ValueFromPythonRegistry &&) = delete;

private:
    Vt_ValueFromPythonRegistry() {}
    VT_API ~Vt_ValueFromPythonRegistry();

    friend class TfSingleton<Vt_ValueFromPythonRegistry>;

    class _Extractor {
    private:
        using _ExtractFunc = VtValue (*)(PyObject *);

        // _ExtractLValue will attempt to obtain an l-value T from the python
        // object it's passed.  This effectively disallows type conversions
        // (other than things like derived-to-base type conversions).
        template <class T>
        static VtValue _ExtractLValue(PyObject *);

        // _ExtractRValue will attempt to obtain an r-value T from the python
        // object it's passed.  This allows nanobind to invoke type
        // conversions to produce the T.
        template <class T>
        static VtValue _ExtractRValue(PyObject *);

    public:

        template <class T>
        static _Extractor MakeLValue() {
            return _Extractor(&_ExtractLValue<T>);
        }
        
        template <class T>
        static _Extractor MakeRValue() {
            return _Extractor(&_ExtractRValue<T>);
        }
        
        VtValue Invoke(PyObject *obj) const {
            return _extract(obj);
        }

    private:
        explicit _Extractor(_ExtractFunc extract) : _extract(extract) {}

        _ExtractFunc _extract;
    };

    VT_API static Vt_ValueFromPythonRegistry &_GetInstance() {
        return TfSingleton<Vt_ValueFromPythonRegistry>::GetInstance();
    }
    
    VT_API void _RegisterLValue(_Extractor const &e);
    VT_API void _RegisterRValue(_Extractor const &e);
    
    std::vector<_Extractor> _lvalueExtractors;
    std::vector<_Extractor> _rvalueExtractors;

    typedef TfHashMap<PyObject *, _Extractor, TfHash> _LValueExtractorCache;
    _LValueExtractorCache _lvalueExtractorCache;

};

VT_API_TEMPLATE_CLASS(TfSingleton<Vt_ValueFromPythonRegistry>);

template <class T>
VtValue Vt_ValueFromPythonRegistry::
_Extractor::_ExtractLValue(PyObject *obj) {
    if constexpr (!std::is_enum_v<T>) {
        T *ptr = nullptr;
        if (NB_CALL(nb_type_get)(
                NB_CTX, &typeid(T), obj, 0, nullptr,
                reinterpret_cast<void **>(&ptr)) && ptr) {
            return VtValue(*ptr);
        }
        if (PyErr_Occurred()) {
            PyErr_Clear();
        }
    }
    return VtValue();
}

template <class T>
VtValue Vt_ValueFromPythonRegistry::
_Extractor::_ExtractRValue(PyObject *obj) {
    nanobind::detail::make_caster<T> caster;
    nanobind::detail::cleanup_list cleanup(obj);
    bool ok = caster.from_python(
        nanobind::handle(obj),
        static_cast<uint8_t>(nanobind::detail::cast_flags::convert),
        &cleanup);
    if (ok) {
        VtValue result(
            static_cast<T>(caster.operator nanobind::detail::cast_t<T>()));
        cleanup.release();
        return result;
    }
    cleanup.release();
    if (PyErr_Occurred()) {
        PyErr_Clear();
    }
    return VtValue();
}

template <class T>
void VtValueFromPython() {
    Vt_ValueFromPythonRegistry::Register<T>(/* registerRvalue = */ true);
}

template <class T>
void VtValueFromPythonLValue() {
    Vt_ValueFromPythonRegistry::Register<T>(/* registerRvalue = */ false);
}

PXR_NAMESPACE_CLOSE_SCOPE

namespace nanobind {
namespace detail {

template <typename T>
struct type_caster<PXR_NS::VtArray<T>> : type_caster_base_tag {
    using ArrayType = PXR_NS::VtArray<T>;
    using Value = ArrayType;
    template <typename T_>
    using Cast = precise_cast_t<T_>;
    static constexpr auto Name = nanobind::detail::const_name<ArrayType>();

    bool from_python(handle src, uint32_t flags,
                     cleanup_list *cleanup) noexcept {
        if (src.is_none() &&
            !(flags & static_cast<uint32_t>(cast_flags::construct))) {
            if (!(flags & static_cast<uint32_t>(cast_flags::none_disallowed))) {
                ptr_ = nullptr;
                return true;
            }
            return false;
        }

        if (NB_CALL(nb_type_get)(NB_CTX_C(cleanup), &typeid(ArrayType),
                                 src.ptr(), flags, cleanup,
                                 reinterpret_cast<void **>(&ptr_))) {
            return true;
        }

        if ((flags & static_cast<uint32_t>(cast_flags::construct)) ||
            !(flags & static_cast<uint32_t>(cast_flags::convert))) {
            return false;
        }

        PyObject *obj_ptr = src.ptr();
        if (!obj_ptr || PyBytes_Check(obj_ptr) || PyUnicode_Check(obj_ptr) ||
            PyDict_Check(obj_ptr) || nanobind::inst_check(src)) {
            return false;
        }
        Py_ssize_t len = PyObject_Length(obj_ptr);
        if (len < 0) {
            PyErr_Clear();
            return false;
        }
        PyObject *iter = PyObject_GetIter(obj_ptr);
        if (!iter) {
            PyErr_Clear();
            return false;
        }
        auto &arr = holder_.emplace(static_cast<size_t>(len));
        T *dst = arr.data();
        size_t i = 0;
        while (PyObject *item = PyIter_Next(iter)) {
            make_caster<T> elem_caster;
            bool ok = elem_caster.from_python(
                item, static_cast<uint8_t>(cast_flags::convert), cleanup);
            Py_DECREF(item);
            if (!ok || i >= static_cast<size_t>(len)) {
                Py_DECREF(iter);
                PyErr_Clear();
                holder_.reset();
                return false;
            }
            dst[i++] = static_cast<T>(elem_caster.operator cast_t<T>());
        }
        Py_DECREF(iter);
        if (PyErr_Occurred() || i != static_cast<size_t>(len)) {
            PyErr_Clear();
            holder_.reset();
            return false;
        }
        ptr_ = &*holder_;
        return true;
    }

    template <typename T_>
    static handle from_cpp(T_ &&src, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        return type_caster_base<ArrayType>::from_cpp(std::forward<T_>(src),
                                                     policy, cleanup);
    }

    template <typename T_>
    bool can_cast() const noexcept {
        return std::is_pointer_v<T_> || (ptr_ != nullptr);
    }

    operator ArrayType *() { return ptr_; }
    operator ArrayType &() { return *ptr_; }
    operator ArrayType &&() && { return std::move(*ptr_); }

private:
    ArrayType *ptr_ = nullptr;
    std::optional<ArrayType> holder_;
};

template <typename T>
struct type_caster<T, enable_if_t<std::is_same_v<T, PXR_NS::VtDictionary>>> {
    NB_TYPE_CASTER(T, const_name("dict"))

    bool from_python(handle src, uint8_t, cleanup_list *) noexcept {
        if (!src.is_valid()) {
            return false;
        }
        return PXR_NS::VtDictionaryFromPythonObject(src.ptr(), &value);
    }

    static handle from_cpp(T const &val, rv_policy,
                           cleanup_list *) noexcept {
        return PXR_NS::VtDictionaryToPythonObject(val);
    }
};

template <typename T>
struct type_caster<T, enable_if_t<std::is_same_v<T, PXR_NS::VtValue>>> {
    NB_TYPE_CASTER(T, const_name("Any"))

    bool from_python(handle src, uint8_t, cleanup_list *) noexcept {
        if (!src.is_valid()) {
            return false;
        }
        return PXR_NS::VtValueFromPythonObject(src.ptr(), &value);
    }

    static handle from_cpp(T const &val, rv_policy,
                           cleanup_list *) noexcept {
        if (val.IsEmpty()) {
            Py_RETURN_NONE;
        }
        PXR_NS::TfPyObjWrapper obj =
            PXR_NS::Vt_GetPythonObjectFromHeldValue(val);
        PyObject *p = obj.ptr();
        if (!p || PyErr_Occurred()) {
            return handle();
        }
        Py_INCREF(p);
        return handle(p);
    }
};

template <typename T>
struct type_caster<T, enable_if_t<std::is_same_v<T, PXR_NS::VtValueRef>>> {
    NB_TYPE_CASTER(T, const_name("Any"))
    std::conditional_t<std::is_same_v<T, PXR_NS::VtValueRef>, PXR_NS::VtValue,
                       int>
        heldValue;

    bool from_python(handle src, uint8_t, cleanup_list *) noexcept {
        if (!src.is_valid()) {
            return false;
        }
        if (!PXR_NS::VtValueFromPythonObject(src.ptr(), &heldValue)) {
            return false;
        }
        value = T(heldValue);
        return true;
    }

    static handle from_cpp(T const &val, rv_policy,
                           cleanup_list *) noexcept {
        if (val.IsEmpty()) {
            Py_RETURN_NONE;
        }
        PXR_NS::TfPyObjWrapper obj =
            PXR_NS::Vt_GetPythonObjectFromHeldValue(val);
        PyObject *p = obj.ptr();
        if (!p || PyErr_Occurred()) {
            return handle();
        }
        Py_INCREF(p);
        return handle(p);
    }
};

template <>
struct has_arg_defaults<PXR_NS::VtValue> : std::true_type {};

} // namespace detail
} // namespace nanobind

#endif // PXR_BASE_VT_VALUE_FROM_PYTHON_H
