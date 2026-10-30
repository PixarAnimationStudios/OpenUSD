//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#ifndef PXR_USD_SDF_PY_SPEC_H
#define PXR_USD_SDF_PY_SPEC_H

/// \file sdf/pySpec.h
///
/// SdfSpec Python wrapping utilities.
///
/// An SdfSpec subclass is not the representation of scene data.  An SdfSpec
/// simply provides an interface to data stored in some internal representation.
/// SdfSpec subclasses are value types and their lifetimes don't reflect the 
/// lifetime of the scene data.  However, clients still create scene data using 
/// the New methods on SdfSpec subclasses.
///
/// When wrapping to Python we need to wrap the New methods as the constructors. 
/// This used to look like this:
///
/// \code
///   class_<MyClass, MyClassHandle>("MyClass", no_init)
///       .def(TfPyRefAndWeakPtr())
///       .def(TfMakePyConstructor(&MyClass::New))
///       ...
/// \endcode
///
/// But we can't use TfMakePyConstructor() because an SdfSpec handle is
/// not a weak pointer.  Furthermore, we don't have the problem of needing
/// to store a ref pointer in the Python object.  But we do still need
/// conversion of spec types to yield the most-derived type in python.
///
/// This file introduces a few def visitors to make wrapping specs easy.
/// Spec wrapping should now look like:
///
/// \code
///   nb::class_<MyClass, SdfSpec>(m, "MyClass")
///       .def(SdfPySpec())  // or SdfPyAbstractSpec()
///       .def(SdfMakePySpecConstructor(&MyClass::New))
///       ...
/// \endcode
///
/// If you need a custom repr you can use SdfPySpecNoRepr() or
/// SdfPyAbstractSpecNoRepr() and def("__repr__", ...).

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/vt/valueFromPython.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/demangle.h"

#include <nanobind/nanobind.h>
#include <string>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

class SdfSpec;
class SdfPseudoRootSpec;

namespace Sdf_PySpecDetail {

template <typename CTOR>
struct NewVisitor : nanobind::def_visitor<NewVisitor<CTOR> > {
public:
    NewVisitor(const std::string &doc = std::string()) : _doc(doc) {}

    template <typename Class, typename... Extra>
    void execute(Class& c, const Extra&... extra) const
    {
        using SpecType = typename Class::Type;
        CTOR::template def_init<SpecType>(c, _doc.c_str(), extra...);
    }

private:
    const std::string _doc;

    template <typename T, typename... Ts> friend class nanobind::class_;
};

template <typename SIG>
struct CtorBase {
public:
    typedef SIG Sig;
    static Sig *_func;

    static void SetFunc(Sig *func)
    {
        if (! _func) {
            _func = func;
        }
        else {
            // CODE_COVERAGE_OFF
            TF_CODING_ERROR("Ctor with signature '%s' is already registered.  "
                            "Duplicate will be ignored.",
                            ArchGetDemangled(typeid(Sig)).c_str());
            // CODE_COVERAGE_ON
        }
    }
};

template <typename SIG> SIG *CtorBase<SIG>::_func = 0;

template <typename SIG> struct NewCtor;

} // namespace Sdf_PySpecDetail

template <typename T>
Sdf_PySpecDetail::NewVisitor<typename Sdf_PySpecDetail::NewCtor<T> >
SdfMakePySpecConstructor(T *func, const std::string &doc = std::string())
{
    // Instantiate to set static constructor pointer, then return the visitor.
    Sdf_PySpecDetail::NewCtor<T> Ctor(func);
    return Sdf_PySpecDetail::NewVisitor<Sdf_PySpecDetail::NewCtor<T> >(doc);
}

namespace Sdf_PySpecDetail {

// Create the repr for a spec using Sdf.Find().
SDF_API std::string _SpecRepr(nanobind::handle, const SdfSpec*);

// Registration for spec types to functions to create a holder with the spec
// corresponding to the spec type.
typedef PyObject* (*_HolderCreator)(const SdfSpec&);
SDF_API void _RegisterHolderCreator(const std::type_info&, _HolderCreator);
SDF_API PyObject* _CreateHolder(const std::type_info&, const SdfSpec&);

template <class _SpecType>
struct _NbHandleToPython {
    typedef _SpecType SpecType;
    typedef SdfHandle<SpecType> Handle;
    typedef _NbHandleToPython<SpecType> This;

    static void Register()
    {
        _RegisterHolderCreator(typeid(SpecType), &This::_Creator);
        Tf_PyRegisterCustomToPythonConverter(
            typeid(Handle),
            +[](void const *p) -> PyObject * {
                return _CreateHolder(
                    typeid(SpecType),
                    static_cast<Handle const *>(p)->GetSpec());
            });
        Tf_PyRegisterCustomToPythonConverter(
            typeid(SdfHandle<const SpecType>),
            +[](void const *p) -> PyObject * {
                return _CreateHolder(
                    typeid(SpecType),
                    static_cast<SdfHandle<const SpecType> const *>(p)->GetSpec());
            });
    }

    static PyObject* convert(const Handle& x)
    {
        return _CreateHolder(typeid(SpecType), x.GetSpec());
    }

private:
    static PyObject* _Creator(const SdfSpec& spec)
    {
        SpecType x(Sdf_CastAccess::CastSpec<SpecType, SdfSpec>(spec));
        return nanobind::cast(x, nanobind::rv_policy::move).release().ptr();
    }
};

// Visitor for def().
template <bool Abstract>
struct SpecVisitor : nanobind::def_visitor<SpecVisitor<Abstract> > {
public:
    SpecVisitor(bool addRepr = true) : _addRepr(addRepr) { }

    template <typename Class, typename... Extra>
    void execute(Class& c, const Extra&...) const
    {
        using SpecType = typename Class::Type;

        if constexpr (Abstract || std::is_same_v<SpecType, SdfPseudoRootSpec>) {
            c.def("__init__", [](nanobind::args, nanobind::kwargs) {
                TfPyThrowRuntimeError("This class cannot be instantiated from Python");
            });
        }

        c.def_prop_ro("expired", [](const SpecType& self) { return self.IsDormant(); });
        c.def("__bool__", [](const SpecType& self) { return !self.IsDormant(); });
        c.def("__hash__", [](const SpecType& self) { return hash_value(self); });
        c.def("__eq__", [](const SpecType& a, nanobind::handle b) -> nanobind::object {
            SdfSpec* b_ptr = nullptr;
            if (NB_CALL(nb_type_get)(NB_CTX, &typeid(SdfSpec), b.ptr(), 0, nullptr,
                                     reinterpret_cast<void**>(&b_ptr)) && b_ptr) {
                return nanobind::bool_(a == *b_ptr);
            }
            return nanobind::borrow<nanobind::object>(Py_NotImplemented);
        });
        c.def("__ne__", [](const SpecType& a, nanobind::handle b) -> nanobind::object {
            SdfSpec* b_ptr = nullptr;
            if (NB_CALL(nb_type_get)(NB_CTX, &typeid(SdfSpec), b.ptr(), 0, nullptr,
                                     reinterpret_cast<void**>(&b_ptr)) && b_ptr) {
                return nanobind::bool_(!(a == *b_ptr));
            }
            return nanobind::borrow<nanobind::object>(Py_NotImplemented);
        });
        c.def("__lt__", [](const SpecType& a, nanobind::handle b) -> nanobind::object {
            SdfSpec* b_ptr = nullptr;
            if (NB_CALL(nb_type_get)(NB_CTX, &typeid(SdfSpec), b.ptr(), 0, nullptr,
                                     reinterpret_cast<void**>(&b_ptr)) && b_ptr) {
                return nanobind::bool_(a < *b_ptr);
            }
            return nanobind::borrow<nanobind::object>(Py_NotImplemented);
        });
        c.def("__le__", [](const SpecType& a, nanobind::handle b) -> nanobind::object {
            SdfSpec* b_ptr = nullptr;
            if (NB_CALL(nb_type_get)(NB_CTX, &typeid(SdfSpec), b.ptr(), 0, nullptr,
                                     reinterpret_cast<void**>(&b_ptr)) && b_ptr) {
                return nanobind::bool_(!( *b_ptr < a ));
            }
            return nanobind::borrow<nanobind::object>(Py_NotImplemented);
        });
        c.def("__gt__", [](const SpecType& a, nanobind::handle b) -> nanobind::object {
            SdfSpec* b_ptr = nullptr;
            if (NB_CALL(nb_type_get)(NB_CTX, &typeid(SdfSpec), b.ptr(), 0, nullptr,
                                     reinterpret_cast<void**>(&b_ptr)) && b_ptr) {
                return nanobind::bool_(*b_ptr < a);
            }
            return nanobind::borrow<nanobind::object>(Py_NotImplemented);
        });
        c.def("__ge__", [](const SpecType& a, nanobind::handle b) -> nanobind::object {
            SdfSpec* b_ptr = nullptr;
            if (NB_CALL(nb_type_get)(NB_CTX, &typeid(SdfSpec), b.ptr(), 0, nullptr,
                                     reinterpret_cast<void**>(&b_ptr)) && b_ptr) {
                return nanobind::bool_(!(a < *b_ptr));
            }
            return nanobind::borrow<nanobind::object>(Py_NotImplemented);
        });

        _NbHandleToPython<SpecType>::Register();

        if (_addRepr) {
            c.def("__repr__", [](nanobind::handle self) {
                const SpecType* s = nanobind::inst_ptr<SpecType>(self);
                return _SpecRepr(self, s);
            });
        }
    }

private:
    bool _addRepr;
};

} // namespace Sdf_PySpecDetail

inline
Sdf_PySpecDetail::SpecVisitor<false>
SdfPySpec()
{
    return Sdf_PySpecDetail::SpecVisitor<false>();
}

inline
Sdf_PySpecDetail::SpecVisitor<true>
SdfPyAbstractSpec()
{
    return Sdf_PySpecDetail::SpecVisitor<true>();
}

inline
Sdf_PySpecDetail::SpecVisitor<false>
SdfPySpecNoRepr()
{
    return Sdf_PySpecDetail::SpecVisitor<false>(false);
}

inline
Sdf_PySpecDetail::SpecVisitor<true>
SdfPyAbstractSpecNoRepr()
{
    return Sdf_PySpecDetail::SpecVisitor<true>(false);
}

namespace Sdf_PySpecDetail
{

// This generates multi-argument specializations for NewCtor.

template <typename R, typename... Args>
struct NewCtor<R(Args...)> : CtorBase<R(Args...)> {
    typedef CtorBase<R(Args...)> Base;
    typedef typename Base::Sig Sig;
    NewCtor(Sig *func) { Base::SetFunc(func); }

    template <class SpecType, class Class, typename... Extra>
    static void def_init(Class &c, const char *doc, const Extra&... extra) {
        c.def("__init__", [](nanobind::pointer_and_handle<SpecType> v, Args... args) {
            TfErrorMark m;
            SdfHandle<SpecType> specHandle(Base::_func(args...));
            if (TfPyConvertTfErrorsToPythonException(m))
                throw nanobind::python_error();
            if (!specHandle)
                TfPyThrowRuntimeError("could not construct " +
                                      ArchGetDemangled(typeid(SdfHandle<SpecType>)));
            new (v.p) SpecType(specHandle.GetSpec());
        }, doc, extra...);
    }
};

} // namespace Sdf_PySpecDetail

PXR_NAMESPACE_CLOSE_SCOPE

namespace nanobind {
namespace detail {

template <typename T>
struct type_caster<PXR_NS::SdfHandle<T>> {
    NB_TYPE_CASTER(PXR_NS::SdfHandle<T>, const_name("Sdf.Spec"))

    bool from_python(handle src, uint8_t flags,
                     cleanup_list *cleanup) noexcept {
        if (src.is_none() &&
            !(flags & static_cast<uint8_t>(cast_flags::construct))) {
            value = PXR_NS::SdfHandle<T>();
            return true;
        }
        using NonConstT = std::remove_const_t<T>;
        NonConstT *ptr = nullptr;
        if (!NB_CALL(nb_type_get)(NB_CTX_C(cleanup), &typeid(NonConstT),
                                  src.ptr(), flags, cleanup,
                                  reinterpret_cast<void **>(&ptr)) ||
            !ptr) {
            return false;
        }
        value = PXR_NS::SdfHandle<T>(*ptr);
        return true;
    }

    static handle from_cpp(const PXR_NS::SdfHandle<T> &src, rv_policy,
                           cleanup_list *) noexcept {
        using NonConstT = std::remove_const_t<T>;
        return handle(PXR_NS::Sdf_PySpecDetail::_CreateHolder(
            typeid(NonConstT), src.GetSpec()));
    }
};

template <typename T>
struct has_arg_defaults<PXR_NS::SdfHandle<T>> : std::true_type {};

} // namespace detail
} // namespace nanobind

#endif // PXR_USD_SDF_PY_SPEC_H
