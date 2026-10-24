//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_PTR_HELPERS_H
#define PXR_BASE_TF_PY_PTR_HELPERS_H

/// \file tf/pyPtrHelpers.h
/// Enables wrapping of Weak or Ref & Weak held types to python.

#include "pxr/pxr.h"

#include "pxr/base/tf/pyIdentity.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyObjectFinder.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/anyWeakPtr.h"

#include <memory>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

// Helper class to return or create a PyObject holder for a Ptr.  This
// can be specialized for custom behavior.
template <typename Ptr>
struct TfMakePyPtr {
    typedef typename Ptr::DataType Pointee;
    typedef std::pair<PyObject*, bool> Result;

    // Return an existing PyObject for the pointer paired with false or
    // create and return a new PyObject paired with true.  The PyObject
    // ref count must have been incremented.
    static Result Execute(Ptr const& p)
    {
        // null pointers -> python None.
        if (!p.GetUniqueIdentifier()) {
            Py_INCREF(Py_None);
            return Result(Py_None, false);
        }

        // Force instantiation.  We must do this before checking if we
        // have a python identity, otherwise the identity might be set
        // during instantiation and our caller will attempt to set it
        // again, which isn't allowed.
        get_pointer(p);

        if (PyObject *id = Tf_PyGetPythonIdentity(p))
            return Result(id, false);

        using NonConstPointee = std::remove_const_t<Pointee>;
        if (nanobind::type<NonConstPointee>().is_valid()) {
            nanobind::handle pyType = nanobind::type<NonConstPointee>();
            Pointee *rawPtr = get_pointer(p);
            if (!rawPtr) {
                nanobind::object nbObj = nanobind::inst_alloc_zero(pyType);
                if (nbObj.is_valid()) {
                    return Result(nbObj.release().ptr(), false);
                }
                Py_INCREF(Py_None);
                return Result(Py_None, false);
            }
            void *instPtr = const_cast<NonConstPointee *>(rawPtr);
            if constexpr (std::is_polymorphic_v<NonConstPointee>) {
                if (PyObject *dynType =
                        NB_CALL(nb_type_lookup)(NB_CTX, &typeid(*rawPtr))) {
                    pyType = dynType;
                    instPtr = const_cast<void *>(TfCastToMostDerivedType(rawPtr));
                }
            }
            nanobind::object nbObj = nanobind::inst_reference(pyType, instPtr);
            if (nbObj.is_valid()) {
                return Result(nbObj.release().ptr(), true);
            }
        }

        Py_INCREF(Py_None);
        return Result(Py_None, false);
    }
};

template <typename T, typename PtrType = TfWeakPtr<T>, typename... Extra>
void TfNbWeakPtr(nanobind::class_<T, Extra...> &cls) {
    static_assert(TF_SUPPORTS_WEAKPTR(T), "Type must support TfWeakPtr.");
    TF_VERIFY(reinterpret_cast<PyTypeObject *>(cls.ptr())->tp_weaklistoffset != 0,
              "nanobind::class_<%s> using TfNbWeakPtr must pass "
              "nanobind::is_weak_referenceable()",
              ArchGetDemangled(typeid(T)).c_str());
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;
    Tf_PyRegisterAnyWeakPtrExtractor(
        typeid(T), +[](PyObject *obj) -> TfAnyWeakPtr {
            if (!nanobind::inst_ready(obj)) {
                return TfAnyWeakPtr();
            }
            T *ptr = nullptr;
            if (NB_CALL(nb_type_get)(NB_CTX, &typeid(T), obj, 0, nullptr,
                                     reinterpret_cast<void **>(&ptr)) &&
                ptr) {
                return TfAnyWeakPtr(PtrType(ptr));
            }
            return TfAnyWeakPtr();
        });
    Tf_RegisterPythonObjectFinder<T, PtrType>();

    cls.def_prop_ro(
        "expired",
        [](nanobind::handle self) { return !nanobind::inst_ready(self); },
        "True if this object has expired, False otherwise.");
    cls.def(
        "__bool__",
        [](nanobind::handle self) { return nanobind::inst_ready(self); },
        "True if this object has not expired.  False otherwise.");
    cls.def("__eq__", [](nanobind::handle self, nanobind::handle other) {
        return self.is(other);
    });
    cls.def("__ne__", [](nanobind::handle self, nanobind::handle other) {
        return !self.is(other);
    });
    cls.def("__lt__", [](nanobind::handle self, nanobind::handle other) {
        return self.ptr() < other.ptr();
    });
    cls.def(TfTypePythonClass());
}

template <typename T, typename PtrType = TfWeakPtr<T>, typename... Extra>
void TfNbRefAndWeakPtr(nanobind::class_<T, Extra...> &cls) {
    static_assert(TF_SUPPORTS_REFPTR(T), "Type must support TfRefPtr.");
    TfNbWeakPtr<T, PtrType>(cls);
}

struct Tf_NbWeakPtrVisitor : nanobind::def_visitor<Tf_NbWeakPtrVisitor> {
    template <typename Class, typename... Extra>
    static void execute(Class &cls, const Extra &...) {
        TfNbWeakPtr(cls);
    }
};

inline Tf_NbWeakPtrVisitor TfNbWeakPtr() { return {}; }

struct Tf_NbRefAndWeakPtrVisitor
    : nanobind::def_visitor<Tf_NbRefAndWeakPtrVisitor> {
    template <typename Class, typename... Extra>
    static void execute(Class &cls, const Extra &...) {
        TfNbRefAndWeakPtr(cls);
    }
};

inline Tf_NbRefAndWeakPtrVisitor TfNbRefAndWeakPtr() { return {}; }

template <typename Func,
          typename Sig = nanobind::detail::function_signature_t<Func>>
struct Tf_NbMakePyConstructorVisitor;

template <typename Func, typename Return, typename... Args>
struct Tf_NbMakePyConstructorVisitor<Func, Return(Args...)>
    : nanobind::def_visitor<
          Tf_NbMakePyConstructorVisitor<Func, Return(Args...)>> {
    std::remove_reference_t<Func> func;

    Tf_NbMakePyConstructorVisitor(Func &&f)
        : func((nanobind::detail::forward_t<Func>)f) {}

    template <typename Class, typename... Extra>
    void execute(Class &cl, const Extra &...extra) {
        using Pointee = typename Return::DataType;
        using NonConstPointee = std::remove_const_t<Pointee>;
        using WeakPtrType = TfWeakPtr<NonConstPointee>;

        auto wrapper = [func_ = (nanobind::detail::forward_t<Func>)func](
                           nanobind::handle cls,
                           Args... args) -> nanobind::object {
            TfErrorMark m;
            Return ptr(func_((nanobind::detail::forward_t<Args>)args...));
            WeakPtrType weakPtr(
                const_cast<NonConstPointee *>(get_pointer(ptr)));
            if (TfPyConvertTfErrorsToPythonException(m)) {
                throw nanobind::python_error();
            }
            if (!weakPtr) {
                TfPyThrowRuntimeError("could not construct " +
                                      ArchGetDemangled(typeid(WeakPtrType)));
            }
            if (PyObject *existing = Tf_PyGetPythonIdentity(weakPtr)) {
                if constexpr (Tf_PyIsRefPtr<Return>::value) {
                    if (!PyObject_HasAttrString(existing, "__owner")) {
                        Tf_PyAddPythonOwnership(
                            ptr, weakPtr.GetUniqueIdentifier(), existing);
                    }
                }
                return nanobind::steal(existing);
            }
            nanobind::object self = nanobind::inst_reference(
                cls, static_cast<typename Class::Type *>(
                         const_cast<NonConstPointee *>(get_pointer(ptr))));
            Tf_PySetPythonIdentity(weakPtr, self.ptr());
            if constexpr (Tf_PyIsRefPtr<Return>::value) {
                Tf_PyAddPythonOwnership(
                    ptr, weakPtr.GetUniqueIdentifier(), self.ptr());
            }
            return self;
        };

        if constexpr ((nanobind::detail::arg_traits<Extra>::is_arg || ...)) {
            cl.def_static("__new__", std::move(wrapper), nanobind::arg("cls"),
                          extra...);
        } else {
            cl.def_static("__new__", std::move(wrapper), extra...);
        }
        cl.def("__init__", [](nanobind::handle, Args...) {}, extra...);
    }
};

template <typename Func>
auto TfNbMakePyConstructor(Func &&func) {
    return Tf_NbMakePyConstructorVisitor<std::decay_t<Func>>(
        std::forward<Func>(func));
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_PTR_HELPERS_H
