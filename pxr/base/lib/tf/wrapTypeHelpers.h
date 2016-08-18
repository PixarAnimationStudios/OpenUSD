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
#ifndef TF_WRAP_TYPE_HELPERS_H
#define TF_WRAP_TYPE_HELPERS_H

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/type.h"
#include <boost/python/class.hpp>
#include <boost/python/def_visitor.hpp>

// Private implementation namespace; public types are exposed below.
namespace TfType_WrapHelpers {

    using namespace boost::python;

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
TF_API TfType TfType_DefinePythonTypeAndBases( const boost::python::object & classObj );

#endif // TF_WRAP_TYPE_HELPERS_H
