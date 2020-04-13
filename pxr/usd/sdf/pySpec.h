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
/// This file introduces a few boost::python::class_ def visitors to make
/// wrapping specs easy.  Spec wrapping should now look like:
///
/// \code
///   class_<MyClass, SdfHandle<MyClass>, bases<SdfSpec>, boost::noncopyable>
///       ("MyClass", no_init)
///       .def(SdfPySpec())  // or SdfPyAbstractSpec()
///       .def(SdfMakePySpecConstructor(&MyClass::New))
///       ...
/// \endcode
///
/// If you need a custom repr you can use SdfPySpecNoRepr() or
/// SdfPyAbstractSpecNoRepr() and def("__repr__", ...).

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include <boost/python/def_visitor.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/raw_function.hpp>
#include <boost/python/pointee.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/tuple.hpp>

#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/demangle.h"

#include <string>
#include <type_traits>

namespace boost{
namespace python {

template <typename T>
struct pointee<PXR_NS::SdfHandle<T> > {
    typedef T type;
};
}
}

PXR_NAMESPACE_OPEN_SCOPE

class SdfSpec;

namespace Sdf_PySpecDetail {

namespace bp = boost::python;

SDF_API bp::object _DummyInit(bp::tuple const & /* args */, bp::dict const & /* kw */);

template <typename CTOR>
struct NewVisitor : bp::def_visitor<NewVisitor<CTOR> > {
public:
    NewVisitor(const std::string &doc = std::string()) : _doc(doc) {}

    template <typename CLS>
    void visit(CLS& c) const
    {
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

        c.def("__init__", bp::raw_function(_DummyInit));
    }

    template <class CLS, class Options>
    void visit(CLS& c, char const* name, Options& options) const
    {
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

        c.def("__init__", bp::raw_function(_DummyInit));
    }

private:
    const std::string _doc;

    friend class bp::def_visitor_access;
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
SDF_API std::string _SpecRepr(const bp::object&, const SdfSpec*);

// Registration for spec types to functions to create a holder with the spec
// corresponding to the spec type.
typedef PyObject* (*_HolderCreator)(const SdfSpec&);
SDF_API void _RegisterHolderCreator(const std::type_info&, _HolderCreator);
SDF_API PyObject* _CreateHolder(const std::type_info&, const SdfSpec&);

template <class _SpecType>
struct _ConstHandleToPython {
    typedef _SpecType SpecType;
    typedef SdfHandle<SpecType> Handle;
    typedef SdfHandle<const SpecType> ConstHandle;
    _ConstHandleToPython() {
        bp::to_python_converter<ConstHandle, _ConstHandleToPython<SpecType> >();
    }
    static PyObject *convert(ConstHandle const &p) {
        return bp::incref(bp::object(TfConst_cast<Handle>(p)).ptr());
    }
};

// Register and perform python conversions of SdfHandles to holders.
template <class _SpecType, class _Held, class _Holder>
struct _HandleToPython {
public:
    typedef _SpecType SpecType;
    typedef _Holder Holder;
    typedef _Held Handle;
    typedef _HandleToPython<SpecType, Handle, Holder> This;

    static void Register()
    {
        _originalConverter = _RegisterConverter<Handle>(&This::_Convert);
        _RegisterHolderCreator(typeid(SpecType), &This::_Creator);
    }

    static PyObject* convert(const Handle& x)
    {
        return _CreateHolder(typeid(SpecType), x.GetSpec());
    }

private:
    static PyObject* _Creator(const SdfSpec& spec)
    {
        Handle x(Sdf_CastAccess::CastSpec<SpecType,SdfSpec>(spec));
        return bp::objects::make_ptr_instance<SpecType, Holder>::execute(x);
    }

    template <class T>
    static
    bp::converter::to_python_function_t
    _RegisterConverter(bp::converter::to_python_function_t f)
    {
        // Replace the old converter, installed automatically when we
        // registered the class.  WBN if boost python let us do this
        // without playing games.
        bp::converter::registration* r =
            const_cast<bp::converter::registration*>(
                bp::converter::registry::query(bp::type_id<T>()));
        if (r) {
            bp::converter::to_python_function_t old = r->m_to_python;
            r->m_to_python = f;
            return old;
        }
        else {
            // CODE_COVERAGE_OFF Can only happen if there's a bug.
            TF_CODING_ERROR("No python registration for '%s'!",
                            ArchGetDemangled(typeid(Handle)).c_str());
            return 0;
            // CODE_COVERAGE_ON
        }
    }

    static PyObject* _Convert(const void* p)
    {
        const Handle& x = *static_cast<const Handle*>(p);
        return _CreateHolder(typeid(SpecType), x.GetSpec());
    }

private:
    static bp::converter::to_python_function_t _originalConverter;
};
template <class SpecType, class Held, class Holder>
bp::converter::to_python_function_t
_HandleToPython<SpecType, Held, Holder>::_originalConverter = 0;

template <class _SpecType>
struct _HandleFromPython {
    typedef _SpecType SpecType;
    typedef SdfHandle<SpecType> Handle;

    _HandleFromPython()
    {
        bp::converter::registry::insert(&convertible, &construct,
                                        bp::type_id<Handle>());
    }

  private:
    static void *convertible(PyObject *p)
    {
        if (p == Py_None)
            return p;
        void *result =
            bp::converter::get_lvalue_from_python(p,
                bp::converter::registered<SpecType>::converters);
        return result;
    }

    static void construct(PyObject* source,
                          bp::converter::rvalue_from_python_stage1_data* data)
    {
        void* const storage =
            ((bp::converter::rvalue_from_python_storage<Handle>*)
                               data)->storage.bytes;
        // Deal with the "None" case.
        if (data->convertible == source)
            new (storage) Handle();
        else {
            new (storage) Handle(*static_cast<SpecType*>(data->convertible));
        }
        data->convertible = storage;
    }
};

// Visitor for def().
template <bool Abstract>
struct SpecVisitor : bp::def_visitor<SpecVisitor<Abstract> > {

    template<typename CLS>
    struct _Helper {
        typedef typename CLS::wrapped_type SpecType;
        typedef typename CLS::metadata::held_type HeldType;
        typedef typename CLS::metadata::held_type_arg HeldArgType;
        typedef typename CLS::metadata::holder HolderType;

    public:
        static std::string Repr(const bp::object& self)
        {
            const HeldType& held = bp::extract<const HeldType&>(self);
            return _SpecRepr(self, get_pointer(held));
        }

        static bool IsExpired(const HeldType& self)
        {
            return !self;
        }

        static bool NonZero(const HeldType& self)
        {
            return self;
        }

        static size_t __hash__(const HeldType& self)
        {
            return hash_value(self);
        }

        static bool __eq__(const HeldType& a, const HeldType& b)
        {
            return a == b;
        }

        static bool __ne__(const HeldType& a, const HeldType& b)
        {
            return a != b;
        }

        static bool __lt__(const HeldType& a, const HeldType& b)
        {
            return a < b;
        }

        static bool __le__(const HeldType& a, const HeldType& b)
        {
            return a <= b;
        }

        static bool __gt__(const HeldType& a, const HeldType& b)
        {
            return a > b;
        }

        static bool __ge__(const HeldType& a, const HeldType& b)
        {
            return a >= b;
        }
    };

public:
    SpecVisitor(bool addRepr = true) : _addRepr(addRepr) { }

    template <typename CLS>
    void visit(CLS& c) const
    {
        typedef typename CLS::wrapped_type SpecType;
        typedef typename CLS::metadata::held_type HeldType;
        typedef typename CLS::metadata::held_type_arg HeldArgType;
        typedef typename CLS::metadata::holder HolderType;

        static_assert(std::is_same<HeldType, SdfHandle<SpecType> >::value,
                      "HeldType must be SdfHandle<SpecType>.");

        // Add methods.
        c.add_property("expired", &_Helper<CLS>::IsExpired);
        c.def(TfPyBoolBuiltinFuncName, &_Helper<CLS>::NonZero);
        c.def("__hash__", &_Helper<CLS>::__hash__);
        c.def("__eq__", &_Helper<CLS>::__eq__);
        c.def("__ne__", &_Helper<CLS>::__ne__);
        c.def("__lt__", &_Helper<CLS>::__lt__);
        c.def("__le__", &_Helper<CLS>::__le__);
        c.def("__gt__", &_Helper<CLS>::__gt__);
        c.def("__ge__", &_Helper<CLS>::__ge__);

        // Add python conversion to cast away constness.
        _ConstHandleToPython<SpecType>();

        // Add python conversion for SdfHandle<SpecType>.
        _HandleFromPython<SpecType>();
        _HandleFromPython<const SpecType>();
        _HandleToPython<SpecType, HeldArgType, HolderType>::Register();

        // Add __repr__.
        if (_addRepr) {
            c.def("__repr__", &_Helper<CLS>::Repr);
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

    template <class CLS>
    static bp::object __new__(bp::object &cls, Args... args) {
        typedef typename CLS::metadata::held_type HeldType;
        TfErrorMark m;
        HeldType specHandle(Base::_func(args...));
        if (TfPyConvertTfErrorsToPythonException(m))
            bp::throw_error_already_set();
        bp::object result = TfPyObject(specHandle);
        if (TfPyIsNone(result))
            TfPyThrowRuntimeError("could not construct " +
                                  ArchGetDemangled(typeid(HeldType)));

        bp::detail::initialize_wrapper(result.ptr(), get_pointer(specHandle));
        // make the object have the right class.
        bp::setattr(result, "__class__", cls);

        return result;
    }
};

} // namespace Sdf_PySpecDetail

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PY_SPEC_H
