//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_WRAP_TYPE_HELPERS_H
#define PXR_BASE_TF_WRAP_TYPE_HELPERS_H

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/type.h"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/def_visitor.hpp"

PXR_NAMESPACE_OPEN_SCOPE

// Private implementation namespace; public types are exposed below.
namespace TfType_WrapHelpers {

    using namespace pxr_boost::python;

    struct _PythonClass : def_visitor<_PythonClass>
    {
        friend class def_visitor_access;
            
    private:
        template <class CLS, class T>
        void _Visit(CLS &c, T *) const {
            if (TfType t = TfType::Find<T>())
                t.DefinePythonClass(c);
        }

    public:
        template <class CLS>
        void visit(CLS &c) const {
            // Use function template resolution to wrap the type
            // appropriately depending on whether it is a polymorphic
            // wrapper<> type.
            typedef typename CLS::wrapped_type Type;
            _Visit(c, detail::unwrap_wrapper((Type*)0));
        }
    };

} // namespace TfType_WrapHelpers


/// \struct TfTypePythonClass
/// A boost.python visitor that associates the Python class object created by
/// the wrapping with the TfType of the C++ type being wrapped.
///
/// Example use:
/// \code
/// class_<Foo, ...>("Foo", ...)
///     .def( TfTypePythonClass() )
/// \endcode
///
struct TfTypePythonClass : public TfType_WrapHelpers::_PythonClass {};

/// A helper for wrapping C++ types.
/// This method defines a TfType for the given python class object, and also
/// recursively defines TfTypes for all the Python bases if necessary.
TF_API TfType TfType_DefinePythonTypeAndBases( const pxr_boost::python::object & classObj );

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_WRAP_TYPE_HELPERS_H
