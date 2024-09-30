//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004.
// Copyright Stefan Seefeld 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_CLASS_METADATA_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_CLASS_METADATA_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/class_metadata.hpp>
#else

#include "pxr/external/boost/python/converter/shared_ptr_from_python.hpp"
#include "pxr/external/boost/python/object/inheritance.hpp"
#include "pxr/external/boost/python/object/class_wrapper.hpp"
#include "pxr/external/boost/python/object/make_instance.hpp"
#include "pxr/external/boost/python/object/value_holder.hpp"
#include "pxr/external/boost/python/object/pointer_holder.hpp"
#include "pxr/external/boost/python/object/make_ptr_instance.hpp"

#include "pxr/external/boost/python/detail/force_instantiate.hpp"
#include "pxr/external/boost/python/detail/not_specified.hpp"
#include "pxr/external/boost/python/detail/type_traits.hpp"

#include "pxr/external/boost/python/has_back_reference.hpp"
#include "pxr/external/boost/python/bases.hpp"
#include "pxr/external/boost/python/type_list.hpp"

#include "pxr/external/boost/python/detail/mpl2/at.hpp"
#include "pxr/external/boost/python/detail/mpl2/if.hpp"
#include "pxr/external/boost/python/detail/mpl2/eval_if.hpp"
#include "pxr/external/boost/python/detail/mpl2/bool.hpp"
#include "pxr/external/boost/python/detail/mpl2/or.hpp"
#include "pxr/external/boost/python/detail/mpl2/identity.hpp"

#include "pxr/external/boost/python/noncopyable.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace objects { 

PXR_BOOST_PYTHON_DECL
void copy_class_object(type_info const& src, type_info const& dst);

//
// Support for registering base/derived relationships
//
template <class Derived>
struct register_base_of
{
    template <class Base>
    inline void operator()(Base*) const
    {
        static_assert(!(PXR_BOOST_NAMESPACE::python::detail::is_same<Base,Derived>::value));
        
        // Register the Base class
        register_dynamic_id<Base>();

        // Register the up-cast
        register_conversion<Derived,Base>(false);

        // Register the down-cast, if appropriate.
        this->register_downcast((Base*)0, PXR_BOOST_NAMESPACE::python::detail::is_polymorphic<Base>());
    }

 private:
    static inline void register_downcast(void*, PXR_BOOST_NAMESPACE::python::detail::false_) {}
    
    template <class Base>
    static inline void register_downcast(Base*, PXR_BOOST_NAMESPACE::python::detail::true_)
    {
        register_conversion<Base, Derived>(true);
    }

};

//
// Preamble of register_class.  Also used for callback classes, which
// need some registration of their own.
//
template <class T, class Bases, size_t ...I>
inline void register_bases(std::index_sequence<I...>)
{
    ((register_base_of<T>()(
          typename python::detail::add_pointer<
              typename python::detail::mpl2::at_c<Bases, I>::type
           >::type(nullptr))
     ), ...);
}

template <class T, class Bases>
inline void register_shared_ptr_from_python_and_casts(T*, Bases)
{
  // Constructor performs registration
#ifdef PXR_BOOST_PYTHON_HAS_BOOST_SHARED_PTR
  python::detail::force_instantiate(converter::shared_ptr_from_python<T, boost::shared_ptr>());
#endif
  python::detail::force_instantiate(converter::shared_ptr_from_python<T, std::shared_ptr>());

  //
  // register all up/downcasts here.  We're using the alternate
  // interface to mpl::for_each to avoid an MSVC 6 bug.
  //
  register_dynamic_id<T>();
  register_bases<T, Bases>(
      std::make_index_sequence<python::detail::mpl2::size<Bases>::value>());
}

//
// Helper for choosing the unnamed held_type argument
//
template <class T, class Prev>
struct select_held_type
  : python::detail::mpl2::if_<
        python::detail::mpl2::or_<
            python::detail::specifies_bases<T>
          , PXR_BOOST_NAMESPACE::python::detail::is_same<T,python::noncopyable>
        >
      , Prev
      , T
    >
{
};

template <
    class T // class being wrapped
  , class X1 // = detail::not_specified
  , class X2 // = detail::not_specified
  , class X3 // = detail::not_specified
>
struct class_metadata
{
    //
    // Calculate the unnamed template arguments
    //
    
    // held_type_arg -- not_specified, [a class derived from] T or a
    // smart pointer to [a class derived from] T.  Preserving
    // not_specified allows us to give class_<T,T> a back-reference.
    typedef typename select_held_type<
        X1
      , typename select_held_type<
            X2
          , typename select_held_type<
                X3
              , python::detail::not_specified
            >::type
        >::type
    >::type held_type_arg;

    // bases
    typedef typename python::detail::select_bases<
        X1
      , typename python::detail::select_bases<
            X2
          , typename python::detail::select_bases<
                X3
              , python::bases<>
            >::type
        >::type
    >::type bases;

    typedef python::detail::mpl2::or_<
        PXR_BOOST_NAMESPACE::python::detail::is_same<X1,python::noncopyable>
      , PXR_BOOST_NAMESPACE::python::detail::is_same<X2,python::noncopyable>
      , PXR_BOOST_NAMESPACE::python::detail::is_same<X3,python::noncopyable>
    > is_noncopyable;
    
    //
    // Holder computation.
    //
    
    // Compute the actual type that will be held in the Holder.
    typedef typename python::detail::mpl2::if_<
        PXR_BOOST_NAMESPACE::python::detail::is_same<held_type_arg,python::detail::not_specified>, T, held_type_arg
    >::type held_type;

    // Determine if the object will be held by value
    typedef python::detail::mpl2::bool_<PXR_BOOST_NAMESPACE::python::detail::is_convertible<held_type*,T*>::value> use_value_holder;
    
    // Compute the "wrapped type", that is, if held_type is a smart
    // pointer, we're talking about the pointee.
    typedef typename python::detail::mpl2::eval_if<
        use_value_holder
      , python::detail::mpl2::identity<held_type>
      , pointee<held_type>
    >::type wrapped;

    // Determine whether to use a "back-reference holder"
    typedef python::detail::mpl2::bool_<
        python::detail::mpl2::or_<
            has_back_reference<T>
          , PXR_BOOST_NAMESPACE::python::detail::is_same<held_type_arg,T>
          , python::detail::is_base_and_derived<T,wrapped>
        >::value
    > use_back_reference;

    // Select the holder.
    typedef typename python::detail::mpl2::eval_if<
        use_back_reference
      , python::detail::mpl2::if_<
            use_value_holder
          , value_holder_back_reference<T, wrapped>
          , pointer_holder_back_reference<held_type,T>
        >
      , python::detail::mpl2::if_<
            use_value_holder
          , value_holder<T>
          , pointer_holder<held_type,wrapped>
        >
    >::type holder;
    
    inline static void register_() // Register the runtime metadata.
    {
        class_metadata::register_aux((T*)0);
    }

 private:
    template <class T2>
    inline static void register_aux(python::wrapper<T2>*) 
    {
        typedef typename python::detail::mpl2::not_<PXR_BOOST_NAMESPACE::python::detail::is_same<T2,wrapped> >::type use_callback;
        class_metadata::register_aux2((T2*)0, use_callback());
    }

    inline static void register_aux(void*) 
    {
        typedef typename python::detail::is_base_and_derived<T,wrapped>::type use_callback;
        class_metadata::register_aux2((T*)0, use_callback());
    }

    template <class T2, class Callback>
    inline static void register_aux2(T2*, Callback) 
    {
	objects::register_shared_ptr_from_python_and_casts((T2*)0, bases());
        class_metadata::maybe_register_callback_class((T2*)0, Callback());

        class_metadata::maybe_register_class_to_python((T2*)0, is_noncopyable());
        
        class_metadata::maybe_register_pointer_to_python(
            (T2*)0, (use_value_holder*)0, (use_back_reference*)0);
    }


    //
    // Support for converting smart pointers to python
    //
    inline static void maybe_register_pointer_to_python(...) {}

#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
    inline static void maybe_register_pointer_to_python(void*,void*,python::detail::mpl2::true_*)
    {
        objects::copy_class_object(python::type_id<T>(), python::type_id<back_reference<T const &> >());
        objects::copy_class_object(python::type_id<T>(), python::type_id<back_reference<T &> >());
    }
#endif

    template <class T2>
    inline static void maybe_register_pointer_to_python(T2*, python::detail::mpl2::false_*, python::detail::mpl2::false_*)
    {
        python::detail::force_instantiate(
            objects::class_value_wrapper<
                held_type
              , make_ptr_instance<T2, pointer_holder<held_type, T2> >
            >()
        );
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
        // explicit qualification of type_id makes msvc6 happy
        objects::copy_class_object(python::type_id<T2>(), python::type_id<held_type>());
#endif
    }
    //
    // Support for registering to-python converters
    //
    inline static void maybe_register_class_to_python(void*, python::detail::mpl2::true_) {}
    

    template <class T2>
    inline static void maybe_register_class_to_python(T2*, python::detail::mpl2::false_)
    {
        python::detail::force_instantiate(class_cref_wrapper<T2, make_instance<T2, holder> >());
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
        // explicit qualification of type_id makes msvc6 happy
        objects::copy_class_object(python::type_id<T2>(), python::type_id<held_type>());
#endif
    }

    //
    // Support for registering callback classes
    //
    inline static void maybe_register_callback_class(void*, python::detail::mpl2::false_) {}

    template <class T2>
    inline static void maybe_register_callback_class(T2*, python::detail::mpl2::true_)
    {
	objects::register_shared_ptr_from_python_and_casts(
            (wrapped*)0, python::type_list<T2>());
        // explicit qualification of type_id makes msvc6 happy
        objects::copy_class_object(python::type_id<T2>(), python::type_id<wrapped>());
    }
};

}}} // namespace PXR_BOOST_NAMESPACE::python::object

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif
