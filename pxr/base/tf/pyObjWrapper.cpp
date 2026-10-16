//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyObjWrapper.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/anyWeakPtr.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyIdentity.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/weakPtr.h"

#include <typeindex>
#include <unordered_map>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TfPyObjWrapper>();
}

namespace {

// A custom deleter for shared_ptr<nanobind::object> that takes the
// python lock before deleting the python object.  This is necessary since it's
// invalid to decrement the python refcount without holding the lock.
struct _DeleteObjectWithLock {
    void operator()(nanobind::object *obj) const {
        PXR_NS::TfPyLock lock;
        delete obj;
    }
};
}

TfPyObjWrapper::TfPyObjWrapper()
{
    TfPyLock lock;
    *this = TfPyObjWrapper(nanobind::none());
}

TfPyObjWrapper::TfPyObjWrapper(nanobind::object obj)
    : _objectPtr(
          new object(obj.is_valid() ? std::move(obj) : nanobind::none()),
          _DeleteObjectWithLock())
{
}

TfPyObjWrapper::TfPyObjWrapper(nanobind::handle h)
    : TfPyObjWrapper(h.is_valid()
                         ? nanobind::borrow<object>(h)
                         : nanobind::none())
{
}

PyObject *
TfPyObjWrapper::ptr() const
{
    return _objectPtr->ptr();
}

bool
TfPyObjWrapper::operator==(TfPyObjWrapper const &other) const
{
    // If they point to the exact same object instance, we know they're equal.
    if (_objectPtr == other._objectPtr)
        return true;

    // Otherwise lock and let python determine equality.
    TfPyLock lock;
    return Get().equal(other.Get());
}

bool
TfPyObjWrapper::operator!=(TfPyObjWrapper const &other) const
{
    return !(*this == other);
}

namespace {
using _CustomConverterMap =
    std::unordered_map<std::type_index, Tf_PyCustomToPythonConverter>;
TfStaticData<_CustomConverterMap> _customToPythonConverters;
} // namespace

void
Tf_PyRegisterCustomToPythonConverter(
    std::type_info const &ti,
    Tf_PyCustomToPythonConverter fn)
{
    (*_customToPythonConverters)[std::type_index(ti)] = fn;
}

TfPyObjWrapper
Tf_PyObjSteal(PyObject *obj)
{
    if (!obj) {
        return TfPyObjWrapper();
    }
    return TfPyObjWrapper(nanobind::steal<nanobind::object>(obj));
}

TfPyObjWrapper
Tf_PyGetNone()
{
    return TfPyObjWrapper(nanobind::none());
}

void
Tf_PyThrowNoToPythonConverterError(std::type_info const &ti)
{
    if (!PyErr_Occurred()) {
        PyErr_Format(
            PyExc_TypeError,
            "No to_python (by-value) converter found for C++ type: %s",
            ArchGetDemangled(ti).c_str());
    }
}

TfPyObjWrapper
Tf_PyObjFromBool(bool val)
{
    return Tf_PyObjSteal(PyBool_FromLong(val ? 1 : 0));
}

TfPyObjWrapper
Tf_PyObjFromInt64(int64_t val)
{
    return Tf_PyObjSteal(PyLong_FromLongLong(static_cast<long long>(val)));
}

TfPyObjWrapper
Tf_PyObjFromUInt64(uint64_t val)
{
    return Tf_PyObjSteal(
        PyLong_FromUnsignedLongLong(static_cast<unsigned long long>(val)));
}

TfPyObjWrapper
Tf_PyObjFromDouble(double val)
{
    return Tf_PyObjSteal(PyFloat_FromDouble(val));
}

TfPyObjWrapper
Tf_PyObjFromString(char const *str, size_t size)
{
    return Tf_PyObjSteal(
        PyUnicode_FromStringAndSize(str, static_cast<Py_ssize_t>(size)));
}

TfPyObjWrapper
Tf_PyObjFromCStr(char const *str)
{
    if (!str) {
        return Tf_PyGetNone();
    }
    return Tf_PyObjSteal(PyUnicode_FromString(str));
}

TfPyObjWrapper
Tf_PyObjFromToken(TfToken const &val)
{
    return Tf_PyObjSteal(
        PyUnicode_FromStringAndSize(
            val.GetText(), static_cast<Py_ssize_t>(val.size())));
}

TfPyObjWrapper
Tf_PyObjFromTfEnum(TfEnum const &val)
{
    return Tf_PyObjSteal(Tf_PyEnumRegistry::GetInstance().GetPyObject(val));
}

TfPyObjWrapper
Tf_PyObjFromEnum(std::type_info const &ti, long val)
{
    if (NB_CALL(nb_type_lookup)(NB_CTX, &ti)) {
        return Tf_PyObjSteal(
            NB_CALL(enum_from_cpp)(NB_CTX, &ti, static_cast<int64_t>(val)));
    }
    return Tf_PyObjSteal(
        Tf_PyEnumRegistry::GetInstance().GetPyObject(TfEnum(ti, val)));
}

TfPyObjWrapper
Tf_PyObjFromAnyWeakPtr(TfAnyWeakPtr const &val)
{
    return Tf_GetPythonObjectFromAnyWeakPtr(val);
}

TfPyObjWrapper
Tf_PyObjFromWeakPtr(
    void const *uniqueId,
    void const *rawPtr,
    void const *mostDerivedPtr,
    std::type_info const &staticTi,
    std::type_info const *dynTi,
    void (*enableNotification)(void const *),
    void const *weakPtrAddr)
{
    if (!uniqueId) {
        return Tf_PyGetNone();
    }
    if (PyObject *id = Tf_PyIdentityHelper::Get(uniqueId)) {
        return Tf_PyObjSteal(id);
    }
    PyObject *pyTypeObj = NB_CALL(nb_type_lookup)(NB_CTX, &staticTi);
    if (pyTypeObj) {
        nanobind::handle pyType(pyTypeObj);
        if (!rawPtr) {
            nanobind::object nbObj = nanobind::inst_alloc_zero(pyType);
            if (nbObj.is_valid()) {
                return TfPyObjWrapper(std::move(nbObj));
            }
            return Tf_PyGetNone();
        }
        void *instPtr = const_cast<void *>(rawPtr);
        if (dynTi) {
            if (PyObject *dynType = NB_CALL(nb_type_lookup)(NB_CTX, dynTi)) {
                pyType = dynType;
                instPtr = const_cast<void *>(mostDerivedPtr);
            }
        }
        nanobind::object nbObj = nanobind::inst_reference(pyType, instPtr);
        if (nbObj.is_valid()) {
            Tf_PyIdentityHelper::Set(uniqueId, nbObj.ptr());
            if (enableNotification) {
                enableNotification(weakPtrAddr);
            }
            return TfPyObjWrapper(std::move(nbObj));
        }
    }
    return Tf_PyGetNone();
}

TfPyObjWrapper
Tf_PyObjFromRefPtrWithWeakBase(
    TfWeakBase const *weakBase,
    TfRefBase *refBase,
    void const *rawPtr,
    void const *mostDerivedPtr,
    std::type_info const &staticTi,
    std::type_info const *dynTi,
    PyObject *(*makeOwnerCapsule)(void const *),
    void const *refPtrAddr)
{
    if (!weakBase) {
        return Tf_PyGetNone();
    }
    TfRefPtr<Tf_Remnant> rem = Tf_WeakBaseAccess::GetRemnant(*weakBase);
    void const *uniqueId = rem ? rem->_GetUniqueIdentifier() : nullptr;
    TfPyObjWrapper h = Tf_PyObjFromWeakPtr(
        uniqueId, rawPtr, mostDerivedPtr, staticTi, dynTi,
        +[](void const *p) {
            static_cast<Tf_Remnant const *>(p)->EnableNotification();
        },
        get_pointer(rem));
    PyObject *obj = h.ptr();
    if (refBase && obj && obj != Py_None &&
        !PyObject_HasAttrString(obj, "__owner")) {
        PyObject *capsule = makeOwnerCapsule(refPtrAddr);
        if (capsule) {
            if (PyObject_SetAttrString(obj, "__owner", capsule) == -1) {
                PyErr_Clear();
            } else {
                Tf_PyOwnershipPtrMap::Insert(refBase, uniqueId);
            }
            Py_DECREF(capsule);
        }
    }
    return h;
}

TfPyObjWrapper
Tf_PyObjFromCppByTypeInfo(
    std::type_info const &ti,
    std::type_info const *dynTi,
    void const *val,
    bool isPointer)
{
    auto it = _customToPythonConverters->find(std::type_index(ti));
    if (it != _customToPythonConverters->end() && val) {
        PyObject *res = it->second(val);
        if (!res) {
            Tf_PyThrowNoToPythonConverterError(ti);
            return TfPyObjWrapper();
        }
        return Tf_PyObjSteal(res);
    }
    nanobind::rv_policy policy = isPointer
        ? nanobind::rv_policy(nanobind::rv_policy::reference)
        : nanobind::rv_policy(nanobind::rv_policy::copy);
    nanobind::detail::cleanup_list cleanup(nullptr);
    PyObject *res = NB_CALL(nb_type_put)(
        NB_CTX_C(&cleanup), &ti, dynTi, const_cast<void *>(val),
        policy, &cleanup, nullptr);
    cleanup.release();
    if (!res) {
        Tf_PyThrowNoToPythonConverterError(ti);
        return TfPyObjWrapper();
    }
    return Tf_PyObjSteal(res);
}

TfPyObjWrapper
Tf_PyMakeTuple(TfPyObjWrapper const *items, size_t size)
{
    if (PyErr_Occurred()) {
        return TfPyObjWrapper();
    }
    PyObject *t = PyTuple_New(static_cast<Py_ssize_t>(size));
    if (!t) {
        return TfPyObjWrapper();
    }
    for (size_t i = 0; i < size; ++i) {
        PyObject *p = items[i].ptr();
        if (!p) {
            Py_DECREF(t);
            return TfPyObjWrapper();
        }
        Py_INCREF(p);
        PyTuple_SET_ITEM(t, static_cast<Py_ssize_t>(i), p);
    }
    return Tf_PyObjSteal(t);
}

TfPyObjWrapper
Tf_PyNewList(size_t size)
{
    return Tf_PyObjSteal(PyList_New(static_cast<Py_ssize_t>(size)));
}

void
Tf_PyListSetItem(
    TfPyObjWrapper &list, size_t index, TfPyObjWrapper const &item)
{
    PyObject *l = list.ptr();
    if (!l || l == Py_None) {
        return;
    }
    PyObject *p = item.ptr();
    if (!p || PyErr_Occurred()) {
        list = TfPyObjWrapper();
        return;
    }
    Py_INCREF(p);
    PyList_SET_ITEM(l, static_cast<Py_ssize_t>(index), p);
}

TfPyObjWrapper
Tf_PyNewSet()
{
    return Tf_PyObjSteal(PySet_New(nullptr));
}

void
Tf_PySetAdd(TfPyObjWrapper &set, TfPyObjWrapper const &item)
{
    PyObject *s = set.ptr();
    if (!s || s == Py_None) {
        return;
    }
    PyObject *p = item.ptr();
    if (!p || PyErr_Occurred() || PySet_Add(s, p) < 0) {
        set = TfPyObjWrapper();
    }
}

TfPyObjWrapper
Tf_PyNewDict()
{
    return Tf_PyObjSteal(PyDict_New());
}

void
Tf_PyDictSetItem(
    TfPyObjWrapper &dict,
    TfPyObjWrapper const &key,
    TfPyObjWrapper const &val)
{
    PyObject *d = dict.ptr();
    if (!d || d == Py_None) {
        return;
    }
    PyObject *k = key.ptr();
    PyObject *v = val.ptr();
    if (!k || !v || PyErr_Occurred() || PyDict_SetItem(d, k, v) < 0) {
        dict = TfPyObjWrapper();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_PYTHON_SUPPORT_ENABLED
