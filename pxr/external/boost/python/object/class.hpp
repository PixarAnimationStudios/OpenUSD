//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_CLASS_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_CLASS_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/class.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/object_core.hpp"
# include "pxr/external/boost/python/type_id.hpp"
# include <cstddef>

namespace PXR_BOOST_NAMESPACE { namespace python {

namespace objects { 

struct PXR_BOOST_PYTHON_DECL class_base : python::api::object
{
    // constructor
    class_base(
        char const* name              // The name of the class
        
        , std::size_t num_types         // A list of class_ids. The first is the type
        , type_info const*const types    // this is wrapping. The rest are the types of
                                        // any bases.
        
        , char const* doc = 0           // Docstring, if any.
        );


    // Implementation detail. Hiding this in the private section would
    // require use of template friend declarations.
    void enable_pickling_(bool getstate_manages_dict);

 protected:
    void add_property(
        char const* name, object const& fget, char const* docstr);
    void add_property(char const* name, 
        object const& fget, object const& fset, char const* docstr);

    void add_static_property(char const* name, object const& fget);
    void add_static_property(char const* name, object const& fget, object const& fset);
    
    // Retrieve the underlying object
    void setattr(char const* name, object const&);

    // Set a special attribute in the class which tells Boost.Python
    // to allocate extra bytes for embedded C++ objects in Python
    // instances.
    void set_instance_size(std::size_t bytes);

    // Set an __init__ function which throws an appropriate exception
    // for abstract classes.
    void def_no_init();

    // Effects:
    //  setattr(self, staticmethod(getattr(self, method_name)))
    void make_method_static(const char *method_name);
};

}}} // namespace PXR_BOOST_NAMESPACE::python::objects

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_CLASS_HPP
