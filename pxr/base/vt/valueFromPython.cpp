//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/valueFromPython.h"

#include "pxr/base/arch/attributes.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/instantiateSingleton.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(Vt_ValueFromPythonRegistry);

Vt_ValueFromPythonRegistry::~Vt_ValueFromPythonRegistry() = default;

TfPyObjWrapper
Vt_GetPythonObjectFromHeldValue(VtValue const &self)
{
    return self._GetPythonObject();
}

TfPyObjWrapper
Vt_GetPythonObjectFromHeldValue(VtValueRef const &self)
{
    return self._GetPythonObject();
}

ARCH_CONSTRUCTOR(Vt_RegisterCustomToPythonConverters, 200)
{
    Tf_PyRegisterCustomToPythonConverter(
        typeid(VtValue),
        +[](void const *p) -> PyObject * {
            VtValue const &val = *static_cast<VtValue const *>(p);
            if (val.IsEmpty()) {
                Py_RETURN_NONE;
            }
            TfPyObjWrapper obj = Vt_GetPythonObjectFromHeldValue(val);
            PyObject *res = obj.ptr();
            if (!res || PyErr_Occurred()) {
                return nullptr;
            }
            Py_INCREF(res);
            return res;
        });
    Tf_PyRegisterCustomToPythonConverter(
        typeid(VtValueRef),
        +[](void const *p) -> PyObject * {
            VtValueRef const &val = *static_cast<VtValueRef const *>(p);
            if (val.IsEmpty()) {
                Py_RETURN_NONE;
            }
            TfPyObjWrapper obj = Vt_GetPythonObjectFromHeldValue(val);
            PyObject *res = obj.ptr();
            if (!res || PyErr_Occurred()) {
                return nullptr;
            }
            Py_INCREF(res);
            return res;
        });
    Tf_PyRegisterCustomToPythonConverter(
        typeid(VtDictionary),
        +[](void const *p) -> PyObject * {
            return VtDictionaryToPythonObject(
                *static_cast<VtDictionary const *>(p));
        });
}

VtValue
Vt_ValueFromPythonRegistry::Invoke(PyObject *obj) {
    TfPyLock lock;
    // Iterate over the extractors in reverse registration order.  We walk the
    // lvalue extractors first, looking for an exact match.  If none match, we
    // walk the rvalue extractors, which allow conversions.
    Vt_ValueFromPythonRegistry &self = _GetInstance();

    // Check to see if we have a cached lvalue extractor for the python type
    // object.
    PyObject *typeObj = reinterpret_cast<PyObject *>(Py_TYPE(obj));
    _LValueExtractorCache::iterator i =
        self._lvalueExtractorCache.find(typeObj);
    if (i != self._lvalueExtractorCache.end()) {
        // attempt conversion.
        VtValue result = i->second.Invoke(obj);
        if (!result.IsEmpty())
            return result;
    }

    // Fall back to trying extractors in reverse registration order.
    for (size_t i = self._lvalueExtractors.size(); i != 0; --i) {
        VtValue result = self._lvalueExtractors[i-1].Invoke(obj);
        if (!result.IsEmpty()) {
            // Cache the result.
            self._lvalueExtractorCache.insert(
                {typeObj, self._lvalueExtractors[i-1]});
            return result;
        }
    }

    // No lvalue extraction worked -- try rvalue conversions.
    for (size_t i = self._rvalueExtractors.size(); i != 0; --i) {
        VtValue result = self._rvalueExtractors[i-1].Invoke(obj);
        if (!result.IsEmpty())
            return result;
    }
    return VtValue();
}

void
Vt_ValueFromPythonRegistry::_RegisterLValue(_Extractor const &e) {
    _lvalueExtractors.push_back(e);
}

void
Vt_ValueFromPythonRegistry::_RegisterRValue(_Extractor const &e) {
    _rvalueExtractors.push_back(e);
}

static bool
_VtValueFromPythonForDictOrList(PyObject *pVal, VtValue *result);

static bool
_VtValueArrayFromPythonObject(PyObject *p, std::vector<VtValue> *result)
{
    if (!PyList_Check(p)) {
        return false;
    }
    Py_ssize_t numElts = PyList_GET_SIZE(p);
    if (result) {
        result->reserve(numElts);
    }
    for (Py_ssize_t i = 0; i < numElts; ++i) {
        PyObject *elem = PyList_GET_ITEM(p, i);
        if (result) {
            result->emplace_back();
            if (!_VtValueFromPythonForDictOrList(elem, &result->back())) {
                return false;
            }
        } else {
            if (!_VtValueFromPythonForDictOrList(elem, nullptr)) {
                return false;
            }
        }
    }
    return true;
}

static bool
_VtValueFromPythonForDictOrList(PyObject *pVal, VtValue *result)
{
    if (PyDict_Check(pVal)) {
        VtDictionary dict;
        if (!VtDictionaryFromPythonObject(pVal, result ? &dict : nullptr)) {
            return false;
        }
        if (result) {
            result->Swap(dict);
        }
        return true;
    }
    if (PyList_Check(pVal)) {
        std::vector<VtValue> array;
        if (!_VtValueArrayFromPythonObject(pVal, result ? &array : nullptr)) {
            return false;
        }
        if (result) {
            result->Swap(array);
        }
        return true;
    }
    VtValue v;
    if (!VtValueFromPythonObject(pVal, &v) || v.IsHolding<TfPyObjWrapper>()) {
        return false;
    }
    if (result) {
        result->Swap(v);
    }
    return true;
}

bool
VtDictionaryFromPythonObject(PyObject *p, VtDictionary *result)
{
    if (!PyDict_Check(p)) {
        return false;
    }
    VtDictionary localDict;
    Py_ssize_t pos = 0;
    PyObject *pyKey = nullptr, *pyVal = nullptr;
    while (PyDict_Next(p, &pos, &pyKey, &pyVal)) {
        std::string key;
        if (PyUnicode_Check(pyKey)) {
            Py_ssize_t size = 0;
            char const *str = PyUnicode_AsUTF8AndSize(pyKey, &size);
            if (!str) {
                PyErr_Clear();
                return false;
            }
            key.assign(str, static_cast<size_t>(size));
        } else if (PyBytes_Check(pyKey)) {
            char *str = nullptr;
            Py_ssize_t size = 0;
            if (PyBytes_AsStringAndSize(pyKey, &str, &size) < 0) {
                PyErr_Clear();
                return false;
            }
            key.assign(str, static_cast<size_t>(size));
        } else {
            return false;
        }
        if (result) {
            VtValue &val = localDict[key];
            if (!_VtValueFromPythonForDictOrList(pyVal, &val)) {
                return false;
            }
        } else {
            if (!_VtValueFromPythonForDictOrList(pyVal, nullptr)) {
                return false;
            }
        }
    }
    if (result) {
        result->swap(localDict);
    }
    return true;
}

PyObject *
VtDictionaryToPythonObject(VtDictionary const &v)
{
    nanobind::dict result;
    for (auto const &kv : v) {
        TfPyObjWrapper o = Vt_GetPythonObjectFromHeldValue(kv.second);
        PyObject *valPtr = o.ptr() ? o.ptr() : Py_None;
        result[kv.first.c_str()] = nanobind::borrow(valPtr);
    }
    return result.release().ptr();
}

bool
VtValueFromPythonObject(PyObject *obj_ptr, VtValue *out)
{
    if (!obj_ptr) {
        return false;
    }
    Vt_ValueWrapper *wrapper = nullptr;
    if (NB_CALL(nb_type_get)(
            NB_CTX, &typeid(Vt_ValueWrapper), obj_ptr, 0, nullptr,
            reinterpret_cast<void **>(&wrapper)) && wrapper) {
        if (out) {
            *out = wrapper->GetValue();
        }
        return true;
    }
    if (PyDict_Check(obj_ptr)) {
        VtDictionary dict;
        if (VtDictionaryFromPythonObject(obj_ptr, &dict)) {
            if (out) {
                out->Swap(dict);
            }
            return true;
        }
    }
    if (PyList_Check(obj_ptr)) {
        std::vector<VtValue> array;
        if (_VtValueArrayFromPythonObject(obj_ptr, &array)) {
            if (out) {
                out->Swap(array);
            }
            return true;
        }
    }
    if (obj_ptr == Py_None) {
        if (out) {
            *out = VtValue();
        }
        return true;
    }
    if (PyBool_Check(obj_ptr)) {
        if (out) {
            *out = VtValue(bool(PyLong_AsLong(obj_ptr)));
        }
        return true;
    }
    if (PyLong_Check(obj_ptr)) {
        long long val = PyLong_AsLongLong(obj_ptr);
        if (!PyErr_Occurred()) {
            if (out) {
                if (std::numeric_limits<int>::min() <= val &&
                    val <= std::numeric_limits<int>::max()) {
                    *out = VtValue(int(val));
                } else if (std::numeric_limits<long>::min() <= val &&
                           val <= std::numeric_limits<long>::max()) {
                    *out = VtValue(long(val));
                } else {
                    *out = VtValue(val);
                }
            }
            return true;
        } else {
            PyErr_Clear();
            unsigned long long uval = PyLong_AsUnsignedLongLong(obj_ptr);
            if (!PyErr_Occurred()) {
                if (out) {
                    *out = VtValue(uval);
                }
                return true;
            } else {
                PyErr_Clear();
            }
        }
    }
    if (PyFloat_Check(obj_ptr)) {
        if (out) {
            *out = VtValue(double(PyFloat_AS_DOUBLE(obj_ptr)));
        }
        return true;
    }
    if (PyUnicode_Check(obj_ptr)) {
        Py_ssize_t size = 0;
        char const *str = PyUnicode_AsUTF8AndSize(obj_ptr, &size);
        if (str) {
            if (out) {
                *out = VtValue(std::string(str, static_cast<size_t>(size)));
            }
            return true;
        }
        PyErr_Clear();
    }
    if (PyBytes_Check(obj_ptr)) {
        char *str = nullptr;
        Py_ssize_t size = 0;
        if (PyBytes_AsStringAndSize(obj_ptr, &str, &size) >= 0) {
            if (out) {
                *out = VtValue(std::string(str, static_cast<size_t>(size)));
            }
            return true;
        }
        PyErr_Clear();
    }

    VtValue v = Vt_ValueFromPythonRegistry::Invoke(obj_ptr);
    if (!v.IsEmpty()) {
        if (out) {
            *out = std::move(v);
        }
        return true;
    }

    if (out) {
        *out = VtValue(TfPyObjWrapper(
            nanobind::borrow<nanobind::object>(obj_ptr)));
    }
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
