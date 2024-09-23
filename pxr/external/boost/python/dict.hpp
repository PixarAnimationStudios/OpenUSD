//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DICT_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_DICT_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/dict.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/list.hpp"
#include "pxr/external/boost/python/tuple.hpp"
#include "pxr/external/boost/python/converter/pytype_object_mgr_traits.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {

class dict;

namespace detail
{
  struct PXR_BOOST_PYTHON_DECL dict_base : object
  {
      // D.clear() -> None.  Remove all items from D.
      void clear();

      // D.copy() -> a shallow copy of D
      dict copy();

      // D.get(k[,d]) -> D[k] if D.has_key(k), else d.  d defaults to None.
      object get(object_cref k) const;
    
      object get(object_cref k, object_cref d) const;

      // D.has_key(k) -> 1 if D has a key k, else 0
      bool has_key(object_cref k) const;

      // D.items() -> list of D's (key, value) pairs, as 2-tuples
      list items() const;
 
      // D.iteritems() -> an iterator over the (key, value) items of D
      object iteritems() const;

      // D.iterkeys() -> an iterator over the keys of D
      object iterkeys() const;

      // D.itervalues() -> an iterator over the values of D
      object itervalues() const;
 
      // D.keys() -> list of D's keys
      list keys() const;
 
      // D.popitem() -> (k, v), remove and return some (key, value) pair as a
      // 2-tuple; but raise KeyError if D is empty
      tuple popitem();

      // D.setdefault(k[,d]) -> D.get(k,d), also set D[k]=d if not D.has_key(k)
      object setdefault(object_cref k);

      object setdefault(object_cref k, object_cref d);

      // D.update(E) -> None.  Update D from E: for k in E.keys(): D[k] = E[k]
      void update(object_cref E);

      // D.values() -> list of D's values
      list values() const;

   protected:
      // dict() -> new empty dictionary.
      // dict(mapping) -> new dictionary initialized from a mapping object's
      //     (key, value) pairs.
      // dict(seq) -> new dictionary initialized as if via:
      dict_base();   // new dict
      explicit dict_base(object_cref data);

      PXR_BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(dict_base, object)
   private:
      static detail::new_reference call(object const&);
  };
}

class dict : public detail::dict_base
{
    typedef detail::dict_base base;
 public:
    // dict() -> new empty dictionary.
    // dict(mapping) -> new dictionary initialized from a mapping object's
    //     (key, value) pairs.
    // dict(seq) -> new dictionary initialized as if via:
    dict() {}   // new dict

    template <class T>
    explicit dict(T const& data)
        : base(object(data))
    {
    }

    template<class T>
    object get(T const& k) const 
    {
        return base::get(object(k));
    }
    
    template<class T1, class T2>
    object get(T1 const& k, T2 const& d) const 
    {
        return base::get(object(k),object(d));
    }
    
    template<class T>
    bool has_key(T const& k) const
    {
        return base::has_key(object(k));
    }
    
    template<class T>
    object setdefault(T const& k)
    {
        return base::setdefault(object(k));
    }
    
    template<class T1, class T2>
    object setdefault(T1 const& k, T2 const& d)
    {
        return base::setdefault(object(k),object(d));
    }

    template<class T>
    void update(T const& E)
    {
        base::update(object(E));
    }

 public: // implementation detail -- for internal use only
    PXR_BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(dict, base)
};

//
// Converter Specializations
//
namespace converter
{
  template <>
  struct object_manager_traits<dict>
      : pytype_object_manager_traits<&PyDict_Type,dict>
  {
  };
}

}}   // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif

