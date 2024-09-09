//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TF_MAKE_PY_CONSTRUCTOR_H
#define PXR_BASE_TF_MAKE_PY_CONSTRUCTOR_H

/// \file tf/makePyConstructor.h
/// An injected constructor mechanism that works with polymorphic wrapped
/// classes.

#ifndef TF_MAX_ARITY
#  define TF_MAX_ARITY 7
#endif // TF_MAX_ARITY


#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/functionTraits.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyIdentity.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/demangle.h"

#include "pxr/external/boost/python/def_visitor.hpp"
#include "pxr/external/boost/python/dict.hpp"
#include "pxr/external/boost/python/errors.hpp"
#include "pxr/external/boost/python/list.hpp"
#include "pxr/external/boost/python/object/iterator.hpp"
#include "pxr/external/boost/python/raw_function.hpp"
#include "pxr/external/boost/python/tuple.hpp"
#include "pxr/external/boost/python/type_id.hpp"

#include <array>
#include <string>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// Helper for wrapping objects that are held by weak pointers, but may also be
// constructed from script.  This lets one construct an object from script and
// stores a ref pointer to the C++ object inside the python object.  This way,
// objects created from script are owned by script, but objects obtained from
// the C++ API cannot be owned by script.  When the owning python object is
// collected, its ref pointer will go away and the C++ object will be
// deallocated.
//
// Example usage:
//
// class_<MyClass, MyClassPtr>("MyClass", no_init)
//    .def(TfPyRefAndWeakPtr())
//    .def(TfMakePyConstructor(MyClass::New))
//    .def(...)
//    ...
//
// TfMakePyConstructorWithVarArgs may be used to wrap an object so that it
// may be constructed with a variable number of positional and keyword
// arguments. The last two arguments of the function being wrapped must
// be a pxr_boost::python::tuple and dict. These will contain the remaining
// positional and keyword args after required arguments are parsed.
//
// Example usage:
//
// static MyObjectRefPtr MyObjectFactory(
//     int formalArg1, const std::string& formalArg2,
//     const pxr_boost::python::tuple& args, const pxr_boost::python::dict& kwargs);
//
// class_<MyClass, MyClassPtr>("MyClass", no_init)
//    .def(TfPyRefAndWeakPtr())
//    .def(TfMakePyConstructorWithVarArgs(MyObjectFactory))
//    .def(...)
//
// NOTE: The current implementation does not handle pxr_boost::python::arg for
//       specifying keywords for required arguments.

namespace Tf_MakePyConstructor {

namespace bp = pxr_boost::python;

template <typename CTOR>
struct InitVisitor : bp::def_visitor<InitVisitor<CTOR> > {
    friend class bp::def_visitor_access;
    const std::string _doc;
    InitVisitor(const std::string &doc = std::string()) : _doc(doc) {}

    template <typename CLS>
    void visit(CLS &c) const {
        c.def("__init__", CTOR::template init_callable<CLS>(), _doc.c_str());
    }

    template <class CLS, class Options>
    void visit(CLS &c, char const* name, Options& options) const {
        // Note: we ignore options.doc() in favor of _doc
        c.def(name, CTOR::template init_callable<CLS>(options), _doc.c_str());
    }

};

TF_API
bp::object _DummyInit(bp::tuple const & /* args */,
                      bp::dict const & /* kw */);

template <typename CTOR>
struct NewVisitor : bp::def_visitor<NewVisitor<CTOR> > {
    friend class bp::def_visitor_access;
    const std::string _doc;
    NewVisitor(const std::string &doc = std::string()) : _doc(doc) {}

    template <typename CLS>
    void visit(CLS &c) const {
        // If there's already a __new__ method, look through the staticmethod to
        // get the underlying function, replace __new__ with that, then add the
        // overload, and recreate the staticmethod.  This is required because
        // boost python needs to have all overloads exported before you say
        // .staticmethod.

        // Note that it looks like this should do nothing, but it actually does
        // something!  Here's what it does: looking up __new__ on c doesn't
        // actually produce the staticmethod object -- it does a "descriptor
        // __get__" which produces the underlying function.  Replacing __new__
        // with that underlying thing has the effect of unwrapping the
        // staticmethod, which is exactly what we want.
        if (PyObject_HasAttrString(c.ptr(), "__new__"))
            c.attr("__new__") = c.attr("__new__");
        c.def("__new__", CTOR::template __new__<CLS>, _doc.c_str());
        c.staticmethod("__new__");

        //c.def("__init__", CTOR::template __init__<CLS>, _doc.c_str());
        c.def("__init__", bp::raw_function(_DummyInit));
    }

    template <class CLS, class Options>
    void visit(CLS &c, char const* name, Options& options) const {
        // If there's already a __new__ method, look through the staticmethod to
        // get the underlying function, replace __new__ with that, then add the
        // overload, and recreate the staticmethod.  This is required because
        // boost python needs to have all overloads exported before you say
        // .staticmethod.

        // Note that it looks like this should do nothing, but it actually does
        // something!  Here's what it does: looking up __new__ on c doesn't
        // actually produce the staticmethod object -- it does a "descriptor
        // __get__" which produces the underlying function.  Replacing __new__
        // with that underlying thing has the effect of unwrapping the
        // staticmethod, which is exactly what we want.
        if (PyObject_HasAttrString(c.ptr(), "__new__"))
            c.attr("__new__") = c.attr("__new__");
        c.def("__new__", CTOR::template __new__<CLS>,
              // Note: we ignore options.doc() in favor of _doc
              _doc.c_str(),
              options.keywords(),
              options.policies()
             );
        c.staticmethod("__new__");

        //c.def("__init__", CTOR::template __init__<CLS>, _doc.c_str());
        c.def("__init__", bp::raw_function(_DummyInit));
    }

};


typedef bp::object object;

template <typename T>
struct InstallPolicy {
    static void PostInstall(object const &self, T const &t,
                            const void *) {}
};

// Specialize install policy for refptrs.
template <typename T>
struct InstallPolicy<TfRefPtr<T> > {
    static_assert(Tf_SupportsUniqueChanged<T>::Value,
                  "Type T must support refcount unique changed notification.");
    static void PostInstall(object const &self, TfRefPtr<T> const &ptr,
                            const void *uniqueId) {
        // Stash a self-reference ref ptr into the python object that will
        // keep the object alive.  Need to get a ref ptr to the held type,
        // since that's what's wrapped.
        Tf_PyAddPythonOwnership(ptr, uniqueId, self.ptr());
    }
};

template <typename CLS, typename T>
void Install(object const &self, T const &t, TfErrorMark const &m) {
    // Stick the weakptr into the python object self to complete
    // construction.
    typedef typename CLS::metadata::holder Holder;
    typedef typename bp::objects::instance<Holder> instance_t;
    typedef InstallPolicy<T> Policy;
    typedef typename CLS::metadata::held_type HeldType;

    // CODE_COVERAGE_OFF
    void *memory = Holder::
        // CODE_COVERAGE_ON
        allocate(self.ptr(), offsetof(instance_t, storage), sizeof(Holder));
    try {
        HeldType held(t);
        Holder *holder = (new (memory) Holder(held));
        // If there was a TfError, raise that back to python.
        if (TfPyConvertTfErrorsToPythonException(m))
            bp::throw_error_already_set();
        // If no TfError, but object construction failed, raise a generic error
        // back to python.
        if (!held)
            TfPyThrowRuntimeError("could not construct " +
                                  ArchGetDemangled(typeid(HeldType)));
        bp::detail::initialize_wrapper(self.ptr(), &(*(held.operator->())));
        holder->install(self.ptr());

        // Set object identity
        Tf_PySetPythonIdentity(held, self.ptr());

        Policy::PostInstall(self, t, held.GetUniqueIdentifier());

    } catch(...) {
        Holder::deallocate(self.ptr(), memory); throw;
    }

}


template <typename WeakPtr, typename P>
struct _RefPtrFactoryConverter {
    typedef std::remove_reference_t<P> Ptr;
    bool convertible() const {
        // FIXME should actually check here...  It's not really horrible because
        // if the conversion will fail, we'll just get a runtime error down
        // below when we try to create the resulting object.  That's basically
        // what we want anyway.
        return true;
    }
    PyObject *operator()(Ptr const &p) const {
        typedef InstallPolicy<Ptr> Policy;
        WeakPtr ptr(static_cast<typename WeakPtr::DataType *>
                    (get_pointer(p)));

        // If resulting pointer is null, return None.
        if (!ptr)
            return bp::incref(Py_None);

        // The to-python converter will set identity here.
        object result(ptr);

        Policy::PostInstall(result, p, ptr.GetUniqueIdentifier());
        return bp::incref(result.ptr());
    }
    // Required for boost.python signature generator, in play when
    // BOOST_PYTHON_NO_PY_SIGNATURES is undefined.
    PyTypeObject const *get_pytype() const {
        return pxr_boost::python::objects::registered_class_object(
            pxr_boost::python::type_id<typename WeakPtr::DataType>()).get();
    }
};

template <typename WeakPtr = void>
struct RefPtrFactory {
    template <typename FactoryResultPtr>
    struct apply {
        using WeakPtrType = std::conditional_t<
            std::is_same<WeakPtr, void>::value,
            TfWeakPtr<typename FactoryResultPtr::DataType>,
            WeakPtr>;
        typedef _RefPtrFactoryConverter<WeakPtrType, FactoryResultPtr> type;
    };
};

// EXTRA_ARITY is added for InitCtorWithVarArgs backwards compatability.
// The previous BOOST_PP implementation didn't count the tuple and dict
// against the arity limit while the new version does. A future change
// should remove EXTRA_ARITY and increase TF_MAX_ARITY now that the
// implementations are templated and no longer generated by BOOST_PP
template <typename SIG, size_t EXTRA_ARITY = 0>
struct CtorBase {
    typedef SIG Sig;
    using Traits = TfFunctionTraits<SIG*>;
    static_assert(Traits::Arity <= (TF_MAX_ARITY + EXTRA_ARITY));

    static Sig *_func;
    static void SetFunc(Sig *func) {
        if (!_func)
            _func = func;
        else {
            // CODE_COVERAGE_OFF
            TF_CODING_ERROR("Ctor with signature '%s' is already registered.  "
                            "Duplicate will be ignored.",
                            ArchGetDemangled(typeid(Sig)).c_str());
            // CODE_COVERAGE_ON
        }
    }
};

template <typename SIG, size_t EXTRA_ARITY>
SIG *CtorBase<SIG, EXTRA_ARITY>::_func = nullptr;

template <typename SIG> struct InitCtor;
template <typename SIG> struct InitCtorWithBackReference;
template <typename SIG> struct InitCtorWithVarArgs;
template <typename SIG> struct NewCtor;
template <typename SIG> struct NewCtorWithClassReference;

}


template <typename T>
Tf_MakePyConstructor::InitVisitor
<typename Tf_MakePyConstructor::InitCtor<T> >
TfMakePyConstructor(T *func, const std::string &doc = std::string()) {
    // Instantiate to set static constructor pointer, then return the visitor.
    Tf_MakePyConstructor::InitCtor<T> Ctor(func);
    return Tf_MakePyConstructor::InitVisitor
        <Tf_MakePyConstructor::InitCtor<T> >(doc);
}

template <typename T>
Tf_MakePyConstructor::InitVisitor
<typename Tf_MakePyConstructor::InitCtorWithBackReference<T> >
TfMakePyConstructorWithBackReference(T *func, const std::string &doc = std::string()) {
    // Instantiate to set static constructor pointer, then return the visitor.
    Tf_MakePyConstructor::InitCtorWithBackReference<T> Ctor(func);
    return Tf_MakePyConstructor::InitVisitor
        <Tf_MakePyConstructor::InitCtorWithBackReference<T> >(doc);
}

template <typename T>
Tf_MakePyConstructor::InitVisitor
<typename Tf_MakePyConstructor::InitCtorWithVarArgs<T> >
TfMakePyConstructorWithVarArgs(T *func, const std::string &doc = std::string()) {
    // Instantiate to set static constructor pointer, then return the visitor.
    Tf_MakePyConstructor::InitCtorWithVarArgs<T> Ctor(func);
    return Tf_MakePyConstructor::InitVisitor
        <Tf_MakePyConstructor::InitCtorWithVarArgs<T> >(doc);
}

template <typename T>
Tf_MakePyConstructor::NewVisitor
<typename Tf_MakePyConstructor::NewCtor<T> >
TfMakePyNew(T *func, const std::string &doc = std::string()) {
    // Instantiate to set static constructor pointer, then return the visitor.
    Tf_MakePyConstructor::NewCtor<T> Ctor(func);
    return Tf_MakePyConstructor::NewVisitor
        <Tf_MakePyConstructor::NewCtor<T> >(doc);
}

template <typename T>
Tf_MakePyConstructor::NewVisitor
<typename Tf_MakePyConstructor::NewCtorWithClassReference<T> >
TfMakePyNewWithClassReference(T *func, const std::string &doc = std::string()) {
    // Instantiate to set static constructor pointer, then return the visitor.
    Tf_MakePyConstructor::NewCtorWithClassReference<T> Ctor(func);
    return Tf_MakePyConstructor::NewVisitor
        <Tf_MakePyConstructor::NewCtorWithClassReference<T> >(doc);
}


template <typename T = void>
struct TfPyRefPtrFactory : public Tf_MakePyConstructor::RefPtrFactory<T> {};

template <typename T> struct Tf_PySequenceToListConverterRefPtrFactory;

/// A \c pxr_boost::python result converter generator which converts standard
/// library sequences to lists of python owned objects.
struct TfPySequenceToListRefPtrFactory {
    template <typename T>
    struct apply {
        typedef Tf_PySequenceToListConverterRefPtrFactory<T> type;
    };
};

// XXX: would be nicer to be able to compose converters with factory
template <typename T>
struct Tf_PySequenceToListConverterRefPtrFactory {
    typedef std::remove_reference_t<T> SeqType;
    bool convertible() const {
        return true;
    }
    PyObject *operator()(T seq) const {
        using namespace pxr_boost::python;

        typedef typename Tf_MakePyConstructor::RefPtrFactory<>::
            apply<typename SeqType::value_type>::type RefPtrFactory;

        pxr_boost::python::list l;
        for (typename SeqType::const_iterator i = seq.begin();
             i != seq.end(); ++i) {
            l.append(object(handle<>(RefPtrFactory()(*i))));
        }
        return pxr_boost::python::incref(l.ptr());
    }
    // Required for boost.python signature generator, in play when
    // BOOST_PYTHON_NO_PY_SIGNATURES is undefined.
    PyTypeObject const *get_pytype() const {
        return &PyList_Type;
    }
};

namespace Tf_MakePyConstructor {

template <typename R, typename... Args>
struct InitCtor<R(Args...)> : CtorBase<R(Args...)> {
    typedef CtorBase<R(Args...)> Base;
    typedef typename Base::Sig Sig;

    InitCtor(Sig* func) { Base::SetFunc(func); }

    template <typename CLS>
    static bp::object init_callable() {
        return bp::make_function(__init__<CLS>);
    }

    template <typename CLS, typename Options>
    static bp::object init_callable(Options& o) {
        return bp::make_function(__init__<CLS>, o.policies(), o.keywords()) ;
    }

    template <typename CLS>
    static void __init__(object &self, Args... args) {
        TfErrorMark m;
        Install<CLS>(self, Base::_func(args...), m);
    }
};

template <typename R, typename... Args>
struct NewCtor<R(Args...)> : CtorBase<R(Args...)> {
    typedef CtorBase<R(Args...)> Base;
    typedef typename Base::Sig Sig;
    NewCtor(Sig *func) { Base::SetFunc(func); }

    template <class CLS>
    static bp::object __new__(object &cls, Args... args) {
        typedef typename CLS::metadata::held_type HeldType;
        TfErrorMark m;
        R r((Base::_func(args...)));
        HeldType h((r));
        if (TfPyConvertTfErrorsToPythonException(m))
            bp::throw_error_already_set();
        bp::object ret = TfPyObject(h);
        if (TfPyIsNone(ret))
            TfPyThrowRuntimeError("could not construct " +
                                  ArchGetDemangled(typeid(HeldType)));

        bp::detail::initialize_wrapper(ret.ptr(), get_pointer(h));
        // make the object have the right class.
        bp::setattr(ret, "__class__", cls);

        InstallPolicy<R>::PostInstall(ret, r, h.GetUniqueIdentifier());
        return ret;
    }
};

template <typename R, typename... Args>
struct InitCtorWithVarArgs<R(Args...)> :
    // Pad the arity for backwards compatability
    CtorBase<R(Args...), /*EXTRA_ARITY*/ 2> {
    typedef CtorBase<R(Args...), /*EXTRA_ARITY*/ 2> Base;
    typedef typename Base::Sig Sig;

    // Ideally, Arity would be pulled from Base::Traits, but
    // compilers have inconsistently allowed this. Redefine
    // Arity as a workaround for now.
    using Arity = TfMetaLength<Args...>;

    InitCtorWithVarArgs(Sig *func) { Base::SetFunc(func); }

    static_assert((Arity::value >= 2) &&
                  (std::is_same_v<
                    const bp::tuple&,
                    typename Base::Traits::template NthArg<(Arity::value-2)>>) &&
                  (std::is_same_v<
                    const bp::dict&,
                    typename Base::Traits::template NthArg<(Arity::value-1)>>),
                  "InitCtorWithVarArgs requires a function of form "
                  "(..., const tuple&, const dict&)");

    template <typename CLS>
    static bp::object init_callable() {
        // Specify min_args as 1 to account for just the 'self' argument.
        // min_args really should be N + 1. However, we want to do this check
        // ourselves later so we can emit a better error message.
        return bp::raw_function(__init__<CLS>, /* min_args = */ 1);
    }

    template <typename CLS, typename Options>
    static bp::object init_callable(Options& options) {
        // XXX: Note ignoring options.keywords(), current implementation can't
        //      handle that correctly.
        return bp::raw_function(
            bp::make_function(__init__<CLS>, options.policies()),
            /* min_args = */ 1);
    }

    template <typename CLS, size_t... I>
    static bp::object __init__impl(const bp::tuple& args,
                                   const bp::dict& kwargs,
                                   std::index_sequence<I...>) {
        TfErrorMark m;

        // We know that there are at least two args because the specialization only
        // matches against (..., *args, **kwargs)
        const unsigned int expectedNamedArgs = Arity::value - 2;
        // self is included in the tuple, so it should always be at least 1
        const unsigned int positionalArgs = bp::len(args) - 1;
        if (positionalArgs < expectedNamedArgs) {
            std::array<std::string, Arity::value - 2>
                positionalArgTypes = {{
                 (bp::type_id<typename Base::Traits::template NthArg<I>>().name())...
            }};
            std::string joinedTypes = TfStringJoin(
                std::begin(positionalArgTypes),
                std::end(positionalArgTypes), ", "
            );
            if (!joinedTypes.empty()) {
                joinedTypes += ", ";
            }
            // User didn't provide enough positional arguments for the factory
            // function. Complain.
            TfPyThrowTypeError(
                TfStringPrintf(
                    "Arguments to __init__ did not match C++ signature:\n"
                    "\t__init__(%s...)", joinedTypes.c_str()
                )
            );
            return bp::object();
        }

        Install<CLS>(
            // self object for new instance is the first arg to __init__
            args[0],
            Base::_func(
                bp::extract<
                    std::remove_reference_t<
                        typename Base::Traits::template NthArg<I>>>(args[I + 1])...,
                bp::tuple(args.slice(expectedNamedArgs + 1, bp::len(args))), kwargs),
            m);

        return bp::object();
    }

    template <typename CLS>
    static bp::object __init__(const bp::tuple& args,
                               const bp::dict& kwargs) {
        return __init__impl<CLS>(
            args, kwargs, std::make_index_sequence<Arity::value - 2>());

    }
};

// This is a variant of Ctor which includes a back reference to self
// (the Python object being initialized) in the args passed to the
// constructor.  This is used to expose the factory methods for classes
// which we expect to subclass in Python.  When the constructor is called,
// it can examine self and initialize itself appropriately.
template <typename R, typename SelfRef, typename... Args>
struct InitCtorWithBackReference<R(SelfRef, Args...)> :
    CtorBase<R(SelfRef, Args...)> {
    typedef CtorBase<R(SelfRef, Args...)> Base;
    typedef typename Base::Sig Sig;
    InitCtorWithBackReference(Sig *func) { Base::SetFunc(func); }

    template <typename CLS>
    static bp::object init_callable() {
        return bp::make_function(__init__<CLS>);
    }

    template <typename CLS, typename Options>
    static bp::object init_callable(Options& o) {
        return bp::make_function(__init__<CLS>, o.policies(), o.keywords());
    }

    template <typename CLS>
    static void __init__(SelfRef self, Args... args) {
        TfErrorMark m;
        Install<CLS>(self, Base::_func(self, args...), m);
    }
};

template <typename R, typename ClsRef, typename... Args>
struct NewCtorWithClassReference<R(ClsRef, Args...)> :
    CtorBase<R(ClsRef, Args...)> {
    typedef CtorBase<R(ClsRef, Args...)> Base;
    typedef typename Base::Sig Sig;
    NewCtorWithClassReference(Sig *func) { Base::SetFunc(func); }

    template <class CLS>
    static bp::object __new__(ClsRef cls, Args... args) {
        typedef typename CLS::metadata::held_type HeldType;
        TfErrorMark m;
        R r(Base::_func(cls, args...));
        HeldType h(r);
        if (TfPyConvertTfErrorsToPythonException(m))
            bp::throw_error_already_set();
        bp::object ret = TfPyObject(h);
        if (TfPyIsNone(ret))
            TfPyThrowRuntimeError("could not construct " +
                                  ArchGetDemangled(typeid(HeldType)));

        bp::detail::initialize_wrapper(ret.ptr(), get_pointer(h));
        // make the object have the right class.
        bp::setattr(ret, "__class__", cls);

        InstallPolicy<R>::PostInstall(ret, r, h.GetUniqueIdentifier());
        return ret;
    }
};
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_MAKE_PY_CONSTRUCTOR_H
