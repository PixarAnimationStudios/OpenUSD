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
#ifndef TF_PYPTRHELPERS_H
#define TF_PYPTRHELPERS_H

///
/// \file pyPtrHelpers.h
/// \brief Enables wrapping of Weak or Ref & Weak held types to python.
///

#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyIdentity.h"
#include "pxr/base/tf/pyObjectFinder.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/anyWeakPtr.h"

#include <boost/python/class.hpp>
#include <boost/python/converter/from_python.hpp>
#include <boost/python/converter/registered.hpp>
#include <boost/python/converter/registrations.hpp>
#include <boost/python/converter/registry.hpp>
#include <boost/python/converter/rvalue_from_python_data.hpp>
#include <boost/python/converter/to_python_function_type.hpp>
#include <boost/python/def_visitor.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/type_traits/is_abstract.hpp>

//
// Boost.Python def visitors for wrapping objects held by weak pointers.  This
// will create a read-only property on your class called 'expired' which is
// true if the object has expired.  This also adds an implementation for __eq__
// and __ne__ which compare the pointers for equality and non-equality, 
// respectively.
//
// Example usage:
//
// class_<MyClass, MyClassPtr>("MyClass", no_init)
//     .def(TfPyWeakPtr())
//     .def(...)
//     ...
//

// Helper class to return or create a PyObject holder for a Ptr.  This
// can be specialized for custom behavior.
template <typename Ptr>
struct TfMakePyPtr {
    typedef typename Ptr::DataType Pointee;
    typedef boost::python::objects::pointer_holder<Ptr, Pointee> Holder;
    typedef std::pair<PyObject*, bool> Result;

    // Return an existing PyObject for the pointer paired with false or
    // create and return a new PyObject paired with true.  The PyObject
    // ref count must have been incremented.
    static Result Execute(Ptr const& p)
    {
        // null pointers -> python None.
        if (not p.GetUniqueIdentifier())
            return Result(boost::python::detail::none(), false);

        // Force instantiation.  We must do this before checking if we
        // have a python identity, otherwise the identity might be set
        // during instantiation and our caller will attempt to set it
        // again, which isn't allowed.
        get_pointer(p);

        if (PyObject *id = Tf_PyGetPythonIdentity(p))
            return Result(id, false);

        // Just make a new python object holding this pointer.
        /// XXX FIXME : use existing to-python conversion?
        PyObject *res = boost::python::objects::make_ptr_instance
                            <Pointee, Holder>::execute(p);
        // If we got back Py_None, no new object was made, so make sure
        // to pass back false in result.
        return Result(res, res != Py_None);
    }
};

namespace Tf_PyDefHelpers {

namespace mpl = boost::mpl;

using namespace boost::python;

using boost::disable_if;
using boost::enable_if;
using boost::is_abstract;
using boost::is_same;


template <typename Ptr>
struct _PtrInterface {
    typedef typename Ptr::DataType Pointee;
    typedef typename boost::add_const<Pointee>::type ConstPointee;
    typedef typename boost::remove_const<Pointee>::type NonConstPointee;
    
    template <typename U>
    struct Rebind {
        typedef typename Ptr::template Rebind<U>::Type Type;
    };

    typedef typename Rebind<ConstPointee>::Type ConstPtr;
    typedef typename Rebind<NonConstPointee>::Type NonConstPtr;

};

template <typename PtrType>
bool _IsPtrExpired(object const &self) {
    try {
        PtrType p = extract<PtrType>(self);
        return not p;
    } catch (boost::python::error_already_set const &) {
        PyErr_Clear();
        return true;
    }
}

template <typename PtrType>
bool _IsPtrValid(object const &self) {
    return not _IsPtrExpired<PtrType>(self);
}

template <typename PtrType>
bool _ArePtrsEqual(PtrType const &self,
                   PtrType const &other) { return self == other; }
template <typename PtrType>
bool _ArePtrsNotEqual(PtrType const &self,
                      PtrType const &other) { return self != other; }

// Default ownership policy does nothing.
template <class PtrType>
struct _PtrFromPythonConversionPolicy {
    static void Apply(PtrType const &, PyObject *) { }
};

// Ownership policy for ref ptrs when going from python to c++ is to
// transfer ownership (remove ownership from python if it has it).
template <typename T>
struct _PtrFromPythonConversionPolicy<TfRefPtr<T> > {
    static void Apply(TfRefPtr<T> const &p, PyObject *obj) {
        Tf_PyRemovePythonOwnership(p, obj);
    }
};

template <class Ptr>
struct _PtrFromPython {
    typedef typename _PtrInterface<Ptr>::Pointee Pointee;
    _PtrFromPython() {
        converter::registry::insert(&convertible, &construct,
                                    type_id<Ptr>());
    }
  private:
    static void *convertible(PyObject *p) {
        if (p == Py_None)
            return p;
        void *result = converter::get_lvalue_from_python
            (p, converter::registered<Pointee>::converters);
        return result;
    }

    static void construct(PyObject* source, converter::
                          rvalue_from_python_stage1_data* data) {
        void* const storage = ((converter::rvalue_from_python_storage<Ptr>*)
                               data)->storage.bytes;
        // Deal with the "None" case.
        if (data->convertible == source)
            new (storage) Ptr();
        else {
            Ptr ptr(static_cast<Pointee*>(data->convertible));
            new (storage) Ptr(ptr);
            _PtrFromPythonConversionPolicy<Ptr>::Apply(ptr, source);
            // Set ptr's python object to source if the pointer is valid.
            if (ptr)
                Tf_PySetPythonIdentity(ptr, source);
        }
        data->convertible = storage;
    }
};


// Converter from python to AnyWeakPtr.  We use this converter to wrap 
// the weak-pointable object into an AnyWeakPtr when we don't know what
// specific C++ type it has--for example, see wrapNotice.cpp.
template <typename PtrType>
struct _AnyWeakPtrFromPython {

    _AnyWeakPtrFromPython() {
        converter::registry::insert(&convertible, &construct,
                                    type_id<TfAnyWeakPtr>());
    }

    static void *convertible(PyObject *p) {
        if (p == Py_None)
            return p;
        void *result = converter::get_lvalue_from_python
            (p, converter::registered
             <typename _PtrInterface<PtrType>::Pointee>::converters);
        return result;
    }
    
    static void construct(PyObject* source, converter::
                          rvalue_from_python_stage1_data* data) {
        void* const storage = ((converter::rvalue_from_python_storage
                                <TfAnyWeakPtr>*)data)->storage.bytes;
        // Deal with the "None" case.
        if (data->convertible == source)
            new (storage) TfAnyWeakPtr();
        else {
            typedef typename _PtrInterface<PtrType>::Pointee T;
            T *ptr = static_cast<T*>(data->convertible);
            PtrType smartPtr(ptr);
            new (storage) TfAnyWeakPtr(smartPtr);

        }
        data->convertible = storage;
    }
};


template <typename Source, typename Target>
typename disable_if<mpl::or_<is_abstract<Source>, is_abstract<Target> > >::type
_RegisterImplicitConversion() {
    implicitly_convertible<Source, Target>();
}

template <typename Source, typename Target>
typename enable_if<mpl::or_<is_abstract<Source>, is_abstract<Target> > >::type
_RegisterImplicitConversion() {
}

template <typename Ptr>
struct _ConstPtrToPython {
    typedef typename _PtrInterface<Ptr>::ConstPtr ConstPtr;
    typedef typename _PtrInterface<Ptr>::NonConstPtr NonConstPtr;
    _ConstPtrToPython() {
        to_python_converter<ConstPtr, _ConstPtrToPython<Ptr> >();
    }
    static PyObject *convert(ConstPtr const &p) {
        return incref(object(TfConst_cast<NonConstPtr>(p)).ptr());
    }
};



template <typename Ptr>
struct _PtrToPython {
    _PtrToPython() {
        to_python_converter<Ptr, _PtrToPython<Ptr> >();
    }
    static PyObject *convert(Ptr const &p) {
        std::pair<PyObject*, bool> ret = TfMakePyPtr<Ptr>::Execute(p);
        if (ret.second) {
            Tf_PySetPythonIdentity(p, ret.first);
        }
        return ret.first;
    }
};


template <typename SrcPtr, typename DstPtr>
struct _ConvertPtrToPython {
    _ConvertPtrToPython() {
        to_python_converter<SrcPtr, _ConvertPtrToPython<SrcPtr, DstPtr> >();
    }
    static PyObject *convert(SrcPtr const &p) {
        DstPtr dst = p;
        return incref(object(dst).ptr());
    }
};


template <typename Ptr>
struct _PtrToPythonWrapper {

    // We store the original to-python converter for our use.  It's fine to be
    // static, as there's only one to-python converter for a type T, and there's
    // one instantiation of this template for each T.
    static converter::to_python_function_t _originalConverter;

    // This signature has to match to_python_function_t
    static PyObject *Convert(void const *x) {
        // See boost/python/converter/as_to_python_function.hpp
        Ptr const &p = *static_cast<Ptr const *>(x);

        std::pair<PyObject*, bool> ret = TfMakePyPtr<Ptr>::Execute(p);
        if (ret.first == Py_None) {
            // Fallback to the original converter.
            Py_DECREF(ret.first);
            ret.first = _originalConverter(x);
        }
        if (ret.second) {
            Tf_PySetPythonIdentity(p, ret.first);
        }
        return ret.first;
    }
};
template <typename T>
converter::to_python_function_t
_PtrToPythonWrapper<T>::_originalConverter = 0;


struct WeakPtr : def_visitor<WeakPtr> {
    friend class def_visitor_access;

    template <typename WrapperPtrType, typename Wrapper, typename T>
    static void _RegisterConversions(Wrapper *, T *) {
        _RegisterConversionsHelper<WrapperPtrType, Wrapper, T>();
    }

    template <typename WrapperPtrType, typename Wrapper, typename T>
    static void _RegisterConversionsHelper() {

        // Pointee should be same as Wrapper.
        BOOST_STATIC_ASSERT((boost::is_same \
                             <typename _PtrInterface<WrapperPtrType>::Pointee, \
                             Wrapper>::value));

        typedef typename
            _PtrInterface<WrapperPtrType>::template Rebind<T>::Type PtrType;
        
        // Register the from-python conversion.
        _PtrFromPython<PtrType>();

        // Register AnyWeakPtr from python conversion.
        _AnyWeakPtrFromPython<PtrType>();

        // From python, can always make a const pointer from a non-const one.
        _RegisterImplicitConversion<PtrType,
            typename _PtrInterface<PtrType>::ConstPtr >();
        
        // Register a conversion that casts away constness when going to python.
        _ConstPtrToPython<PtrType>();

        // Replace the existing to_python conversion for weakptr<wrapper> to do
        // object id first.  It would be great if we could get better support in
        // boost python for doing this sort of thing.
        //
        // We do this for wrapper because this is the "wrapped type" -- the type
        // for which boost python has already registered a to-python
        // conversion.  The unwrapped type is handled separately -- we don't
        // have to replace an existing converter, we can just register our own.
        converter::registration *r = const_cast<converter::registration *>
            (converter::registry::query(type_id<WrapperPtrType>()));
        if (r) {
            _PtrToPythonWrapper<WrapperPtrType>::
                _originalConverter = r->m_to_python;
            r->m_to_python = _PtrToPythonWrapper<WrapperPtrType>::Convert;
        } else {
            // CODE_COVERAGE_OFF Can only happen if there's a bug.
            TF_CODING_ERROR("No python registration for '%s'!",
                            ArchGetDemangled(typeid(WrapperPtrType)).c_str());
            // CODE_COVERAGE_ON
        }
        
        if (not is_same<Wrapper, T>::value)
            _PtrToPython<PtrType>();

    }



    template <typename PtrType, typename CLS, typename Wrapper, typename T>
    static void _AddAPI(CLS &c, Wrapper *, T *) {
        typedef typename
            _PtrInterface<PtrType>::template Rebind<T>::Type UnwrappedPtrType;
        // Add 'expired' property and (in)equality testing.
        c.add_property("expired", _IsPtrExpired<UnwrappedPtrType>,
                       (const char *)
                       "True if this object has expired, False otherwise.");
        c.def("__nonzero__", _IsPtrValid<UnwrappedPtrType>,
              (char const *)
              "True if this object has not expired.  False otherwise.");
        c.def("__eq__", _ArePtrsEqual<UnwrappedPtrType>,
              "Equality operator:  x == y");
        c.def("__ne__", _ArePtrsNotEqual<UnwrappedPtrType>,
              "Non-equality  operator: x != y");
        c.def( TfTypePythonClass() );
    } 

    template <typename CLS>
    void visit(CLS &c) const {
        typedef typename CLS::wrapped_type Type;
        typedef typename CLS::metadata::held_type_arg PtrType;
        // Must support weak ptr.
        BOOST_STATIC_ASSERT(TF_SUPPORTS_WEAKPTR(Type));
        // Register conversions
        _RegisterConversions<PtrType>
            ((Type *)0, detail::unwrap_wrapper((Type *)0));

        // Register a PyObjectFinder.
        Tf_RegisterPythonObjectFinder<Type, PtrType>();
        
        // Add weak ptr api.
        _AddAPI<PtrType>(c, (Type *)0, detail::unwrap_wrapper((Type *)0));
    }
};

struct RefAndWeakPtr : def_visitor<RefAndWeakPtr> {
    friend class def_visitor_access;

    template <typename CLS, typename Wrapper, typename T>
    static void _AddAPI(Wrapper *, T *) {
        _PtrFromPython<TfRefPtr<T> >();
        typedef typename
            _PtrInterface<typename CLS::metadata::held_type>::template
            Rebind<T>::Type PtrType;
        _ConvertPtrToPython<TfRefPtr<T>, PtrType>();
    }
    
    template <typename CLS>    
    void visit(CLS &c) const {
        typedef typename CLS::wrapped_type Type;
        // Must support ref ptr.
        BOOST_STATIC_ASSERT((TF_SUPPORTS_REFPTR(Type)));
        // Same as weak ptr plus ref conversions.
        WeakPtr().visit(c);
        _AddAPI<CLS>((Type *)0, detail::unwrap_wrapper((Type *)0));
    }
};

};

struct TfPyWeakPtr : Tf_PyDefHelpers::WeakPtr {};
struct TfPyRefAndWeakPtr : Tf_PyDefHelpers::RefAndWeakPtr {};

#endif // TF_PYPTRHELPERS_H



