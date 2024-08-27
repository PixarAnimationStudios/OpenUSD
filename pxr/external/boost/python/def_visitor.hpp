//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DEF_VISITOR_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DEF_VISITOR_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/def_visitor.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
# include <boost/detail/workaround.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python { 

template <class DerivedVisitor> class def_visitor;
template <class T, class X1, class X2, class X3> class class_;

class def_visitor_access
{
# if defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)                  \
    || BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x551))
    // Tasteless as this may seem, making all members public allows member templates
    // to work in the absence of member template friends.
 public:
# else      
    template <class Derived> friend class def_visitor;
# endif
    
    // unnamed visit, c.f. init<...>, container suites
    template <class V, class classT>
    static void visit(V const& v, classT& c)
    {
        v.derived_visitor().visit(c);
    }

    // named visit, c.f. object, pure_virtual
    template <class V, class classT, class OptionalArgs>
    static void visit(
        V const& v
      , classT& c
      , char const* name
      , OptionalArgs const& options
    ) 
    {
        v.derived_visitor().visit(c, name, options);
    }
    
};


template <class DerivedVisitor>
class def_visitor
{
    friend class def_visitor_access;
    
# if defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)                  \
    || BOOST_WORKAROUND(BOOST_BORLANDC, BOOST_TESTED_AT(0x551))
    // Tasteless as this may seem, making all members public allows member templates
    // to work in the absence of member template friends.
 public:
# else      
    template <class T, class X1, class X2, class X3> friend class class_;
# endif
    
    // unnamed visit, c.f. init<...>, container suites
    template <class classT>
    void visit(classT& c) const
    {
        def_visitor_access::visit(*this, c);
    }

    // named visit, c.f. object, pure_virtual
    template <class classT, class OptionalArgs>
    void visit(classT& c, char const* name, OptionalArgs const& options) const
    {
        def_visitor_access::visit(*this, c, name, options);
    }
    
 protected:
    DerivedVisitor const& derived_visitor() const
    {
        return static_cast<DerivedVisitor const&>(*this);
    }
};

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DEF_VISITOR_HPP
