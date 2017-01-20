//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#if !BOOST_PP_IS_ITERATING

#ifndef TF_MAKE_CONSTRUCTOR_H
#define TF_MAKE_CONSTRUCTOR_H

/// \file tf/makePyConstructor.h
/// An injected constructor mechanism that works with polymorphic wrapped
/// classes.

#ifndef TF_MAX_ARITY
#  define TF_MAX_ARITY 7
#endif // TF_MAX_ARITY


#include "pxr/pxr.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyIdentity.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/demangle.h"

#include <boost/mpl/assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/preprocessor.hpp>
#include <boost/python/def_visitor.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/list.hpp>
#include <boost/python/object/iterator.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/type_id.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_reference.hpp>

#include <string>

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
// be a boost::python::tuple and dict. These will contain the remaining
// positional and keyword args after required arguments are parsed.
//
// Example usage:
//
// static MyObjectRefPtr MyObjectFactory(
//     int formalArg1, const std::string& formalArg2,
//     const boost::python::tuple& args, const boost::python::dict& kwargs);
//
// class_<MyClass, MyClassPtr>("MyClass", no_init)
//    .def(TfPyRefAndWeakPtr())
//    .def(TfMakePyConstructorWithVarArgs(MyObjectFactory))
//    .def(...)
//
// NOTE: The current implementation does not handle boost::python::arg for
//       specifying keywords for required arguments.

namespace Tf_MakePyConstructor {

namespace bp = boost::python;

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
    BOOST_MPL_ASSERT_MSG(Tf_SupportsUniqueChanged<T>::Value,
                         Type_must_support_refcount_unique_changed_notification,
                         (types<T>));
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
    typedef typename boost::remove_reference<P>::type Ptr;
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
        return boost::python::objects::registered_class_object(
            boost::python::type_id<typename WeakPtr::DataType>()).get();
    }
};

template <typename WeakPtr = void>
struct RefPtrFactory {
    template <typename FactoryResultPtr>
    struct apply {
        typedef typename boost::mpl::if_<boost::is_same<WeakPtr, void>,
            TfWeakPtr<typename FactoryResultPtr::DataType>,
            WeakPtr>::type WeakPtrType;
        typedef _RefPtrFactoryConverter<WeakPtrType, FactoryResultPtr> type;
    };
};

template <typename SIG>
struct CtorBase {
    typedef SIG Sig;
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

template <typename SIG> SIG *CtorBase<SIG>::_func = 0;

// The following preprocessor code repeatedly includes this file to generate
// specializations of Ctor taking 0 through TF_MAX_ARITY parameters.
template <typename SIG> struct InitCtor;
template <typename SIG> struct InitCtorWithBackReference;
template <typename SIG> struct InitCtorWithVarArgs;
template <typename SIG> struct NewCtor;
template <typename SIG> struct NewCtorWithClassReference;
#define BOOST_PP_ITERATION_LIMITS (0, TF_MAX_ARITY)
#define BOOST_PP_FILENAME_1 "pxr/base/tf/makePyConstructor.h"
#include BOOST_PP_ITERATE()
/* comment needed for scons dependency scanner
#include "pxr/base/tf/makePyConstructor.h"
*/

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

/// A \c boost::python result converter generator which converts standard
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
    typedef typename boost::remove_reference<T>::type SeqType;
    bool convertible() const {
        return true;
    }
    PyObject *operator()(T seq) const {
        using namespace boost::python;

        typedef typename Tf_MakePyConstructor::RefPtrFactory<>::
            apply<typename SeqType::value_type>::type RefPtrFactory;

        boost::python::list l;
        for (typename SeqType::const_iterator i = seq.begin();
             i != seq.end(); ++i) {
            l.append(object(handle<>(RefPtrFactory()(*i))));
        }
        return boost::python::incref(l.ptr());
    }
    // Required for boost.python signature generator, in play when
    // BOOST_PYTHON_NO_PY_SIGNATURES is undefined.
    PyTypeObject const *get_pytype() const {
        return &PyList_Type;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_MAKE_CONSTRUCTOR_H

#else // BOOST_PP_IS_ITERATING

#define N BOOST_PP_ITERATION()

#define SIGNATURE R (BOOST_PP_ENUM_PARAMS(N, A))
#define PARAMLIST BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, A, a)
#define ARGLIST BOOST_PP_ENUM_PARAMS(N, a)

// This generates multi-argument specializations for Tf_MakePyConstructor::Ctor.
// One nice thing about this style of PP repetition is that the debugger will
// actually step you over these lines for any instantiation of Ctor.

template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
struct InitCtor<SIGNATURE> : CtorBase<SIGNATURE> {
    typedef CtorBase<SIGNATURE> Base;
    typedef typename Base::Sig Sig;
    InitCtor(Sig *func) { Base::SetFunc(func); }

    template <typename CLS>
    static bp::object init_callable() {
        return bp::make_function(__init__<CLS>);
    }

    template <typename CLS, typename Options>
    static bp::object init_callable(Options& o) {
        return bp::make_function(__init__<CLS>, o.policies(), o.keywords()) ;
    }

    template <typename CLS>
    static void __init__(object &self PARAMLIST) {
        TfErrorMark m;
        Install<CLS>(self, Base::_func(ARGLIST), m);
    }
};

template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
struct NewCtor<SIGNATURE> : CtorBase<SIGNATURE> {
    typedef CtorBase<SIGNATURE> Base;
    typedef typename Base::Sig Sig;
    NewCtor(Sig *func) { Base::SetFunc(func); }

    template <class CLS>
    static bp::object __new__(object &cls PARAMLIST) {
        typedef typename CLS::metadata::held_type HeldType;
        TfErrorMark m;
        R r((Base::_func(ARGLIST)));
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

#define VAR_SIGNATURE                                   \
    R (BOOST_PP_ENUM_PARAMS(N, A) BOOST_PP_COMMA_IF(N)  \
       const bp::tuple&, const bp::dict&)

#define FORMAT_STR(z, n, data) "%s, "
#define ARG_TYPE_STR_A(z, n, data) bp::type_id<A##n>().name()

#define EXTRACT_REQ_ARG_A(z, n, data)                                     \
    /* The n'th required arg is stored at n + 1 in the positional args */ \
    /* tuple as the 0'th element is always the self object */             \
    bp::extract<typename boost::remove_reference<A##n>::type>(data[n + 1])

template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
struct InitCtorWithVarArgs<VAR_SIGNATURE> : CtorBase<VAR_SIGNATURE> {
    typedef CtorBase<VAR_SIGNATURE> Base;
    typedef typename Base::Sig Sig;
    InitCtorWithVarArgs(Sig *func) { Base::SetFunc(func); }

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

    template <typename CLS>
    static bp::object __init__(const bp::tuple& args, const bp::dict& kwargs) {
        TfErrorMark m;

        const unsigned int numArgs = bp::len(args);
        if (numArgs - 1 < N) {
            // User didn't provide enough positional arguments for the factory
            // function. Complain.
            TfPyThrowTypeError(
                TfStringPrintf(
                    "Arguments to __init__ did not match C++ signature:\n"
                    "\t__init__(" BOOST_PP_REPEAT(N, FORMAT_STR, 0) "...)"
                    BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM(N, ARG_TYPE_STR_A, 0)
                )
            );
            return bp::object();
        }

        Install<CLS>(
            // self object for new instance is the first arg to __init__
            args[0],

            // Slice the first N arguments from positional arguments as
            // those are the required arguments for the factory function.
            Base::_func(
                BOOST_PP_ENUM(N, EXTRACT_REQ_ARG_A, args) BOOST_PP_COMMA_IF(N)
                bp::tuple(args.slice(N + 1, numArgs)), kwargs),
            m);

        return bp::object();
    }

};

#if N > 0
#undef PARAMLIST
#define PARAMLIST BOOST_PP_ENUM_BINARY_PARAMS(N, A, a)

// This is a variant of Ctor which includes a back reference to self
// (the Python object being initialized) in the args passed to the
// constructor.  This is used to expose the factory methods for classes
// which we expect to subclass in Python.  When the constructor is called,
// it can examine self and initialize itself appropriately.

template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
struct InitCtorWithBackReference<SIGNATURE> : CtorBase<SIGNATURE> {
    typedef CtorBase<SIGNATURE> Base;
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
    static void __init__(PARAMLIST) {
        TfErrorMark m;
        Install<CLS>(a0, Base::_func(ARGLIST), m);
    }
};

template <typename R BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
struct NewCtorWithClassReference<SIGNATURE> : CtorBase<SIGNATURE> {
    typedef CtorBase<SIGNATURE> Base;
    typedef typename Base::Sig Sig;
    NewCtorWithClassReference(Sig *func) { Base::SetFunc(func); }

    template <class CLS>
    static bp::object __new__(PARAMLIST) {
        typedef typename CLS::metadata::held_type HeldType;
        TfErrorMark m;
        R r(Base::_func(ARGLIST));
        HeldType h(r);
        if (TfPyConvertTfErrorsToPythonException(m))
            bp::throw_error_already_set();
        bp::object ret = TfPyObject(h);
        if (TfPyIsNone(ret))
            TfPyThrowRuntimeError("could not construct " +
                                  ArchGetDemangled(typeid(HeldType)));

        bp::detail::initialize_wrapper(ret.ptr(), get_pointer(h));
        // make the object have the right class.
        bp::setattr(ret, "__class__", a0);

        InstallPolicy<R>::PostInstall(ret, r, h.GetUniqueIdentifier());
        return ret;
    }
};

#endif

#undef N
#undef SIGNATURE
#undef PARAMLIST
#undef ARGLIST
#undef VAR_SIGNATURE
#undef FORMAT_STR
#undef ARG_TYPE_STR_A
#undef EXTRACT_REQ_ARG_A

#endif // BOOST_PP_IS_ITERATING
