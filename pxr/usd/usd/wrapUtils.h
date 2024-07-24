//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_WRAP_UTILS_H
#define PXR_USD_USD_WRAP_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/object.h"

#include <boost/python/def_visitor.hpp>
#include <boost/python/type_id.hpp>
#include <boost/python/converter/to_python_function_type.hpp>

PXR_NAMESPACE_OPEN_SCOPE


// This boost.python def_visitor is used to wrap UsdObject and its subclasses.
// It replaces boost.python's to_python converter with one that downcasts to the
// most derived UsdObject subclass.  This way, a wrapped C++ function that
// returns a UsdObject, for instance, will produce a UsdPrim or a UsdAttribute
// or a UsdRelationship in python, instead of a UsdObject.
struct Usd_ObjectSubclass : boost::python::def_visitor<Usd_ObjectSubclass>
{
    friend class boost::python::def_visitor_access;

    // Function pointer type for downcasting UsdObject * to a more-derived type.
    typedef const void *(*DowncastFn)(const UsdObject *);

    // We replace boost.python's to_python converter with one that downcasts
    // UsdObject types to their most derived type.  For example, when converting
    // a UsdProperty to python, we downcast it to either UsdAttribute or
    // UsdRelationship, as appropriate.
    template <typename CLS>
    void visit(CLS &c) const {
        typedef typename CLS::wrapped_type Type;
        _ReplaceConverter(boost::python::type_id<Type>(),
                          _Detail::GetObjType<Type>::Value,
                          _Convert<Type>, _Downcast<Type>);
    }

private:
    // Converter implementation for UsdObject subclass T.
    template <class T>
    static PyObject *_Convert(const void *in) {
        return _ConvertHelper(static_cast<const T *>(in));
    }

    // Downcast UsdObject to T.
    template <class T>
    static const void *_Downcast(const UsdObject *in) {
        return static_cast<const T *>(in);
    }

    // Internal method that replaces the boost.python to_python converter for
    // the type \p pti.
    static void _ReplaceConverter(
        boost::python::type_info pti,
        UsdObjType objType,
        boost::python::converter::to_python_function_t convert,
        DowncastFn downcast);

    // Non-template helper function for _Convert.
    static PyObject *_ConvertHelper(const UsdObject *obj);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_WRAP_UTILS_H
