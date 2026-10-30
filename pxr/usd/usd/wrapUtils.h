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
#include "pxr/usd/usd/pyConversions.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

PXR_NAMESPACE_OPEN_SCOPE


// This nanobind def_visitor is used to wrap UsdObject and its subclasses.
// It registers the downcast function for each UsdObject subclass so that
// a wrapped C++ function that returns a UsdObject or UsdProperty will produce
// a UsdPrim, UsdAttribute, or UsdRelationship in Python.
struct Usd_ObjectSubclass : nanobind::def_visitor<Usd_ObjectSubclass>
{
    template <typename T, typename... Ts> friend class nanobind::class_;

    // Function pointer type for downcasting UsdObject * to a more-derived type.
    typedef const void *(*DowncastFn)(const UsdObject *);

    template <typename Class, typename... Extra>
    void execute(Class &c, const Extra &...) const {
        typedef typename Class::Type Type;
        c.def("__init__", [](Type *self, nanobind::handle src) {
            if (PyObject_HasAttrString(src.ptr(), "GetAttr")) {
                nanobind::object attr = src.attr("GetAttr")();
                if (nanobind::isinstance<Type>(attr)) {
                    new (self) Type(nanobind::cast<const Type &>(attr));
                    return;
                }
            }
            throw nanobind::type_error("Cannot convert to UsdObject subclass");
        }, nanobind::is_implicit());
        _ReplaceConverter(typeid(Type),
                          _Detail::GetObjType<Type>::Value,
                          _Downcast<Type>);
    }

private:
    // Downcast UsdObject to T.
    template <class T>
    static const void *_Downcast(const UsdObject *in) {
        return static_cast<const T *>(in);
    }

    USD_API
    static void _ReplaceConverter(
        std::type_info const &ti,
        UsdObjType objType,
        DowncastFn downcast);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_WRAP_UTILS_H
