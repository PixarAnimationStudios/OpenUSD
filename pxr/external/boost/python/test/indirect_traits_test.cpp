//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//#include <stdio.h>
#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>
#include "pxr/external/boost/python/detail/indirect_traits.hpp"

//#define print(expr) printf("%s ==> %s\n", #expr, expr)

// not all the compilers can handle an incomplete class type here.
struct X {};

using namespace PXR_BOOST_NAMESPACE::python::indirect_traits;

typedef void (X::*pmf)();

static_assert((is_reference_to_function<int (&)()>::value));
static_assert(!(is_reference_to_function<int (*)()>::value));
static_assert(!(is_reference_to_function<int&>::value));
static_assert(!(is_reference_to_function<pmf>::value));
    
static_assert(!(is_pointer_to_function<int (&)()>::value));
static_assert((is_pointer_to_function<int (*)()>::value));
static_assert(!(is_pointer_to_function<int (*&)()>::value));
static_assert(!(is_pointer_to_function<int (*const&)()>::value));
static_assert(!(is_pointer_to_function<pmf>::value));
    
static_assert(!(is_reference_to_function_pointer<int (&)()>::value));
static_assert(!(is_reference_to_function_pointer<int (*)()>::value));
static_assert(!(is_reference_to_function_pointer<int&>::value));
static_assert((is_reference_to_function_pointer<int (*&)()>::value));
static_assert((is_reference_to_function_pointer<int (*const&)()>::value));
static_assert(!(is_reference_to_function_pointer<pmf>::value));

static_assert((is_reference_to_pointer<int*&>::value));
static_assert((is_reference_to_pointer<int* const&>::value));
static_assert((is_reference_to_pointer<int*volatile&>::value));
static_assert((is_reference_to_pointer<int*const volatile&>::value));
static_assert((is_reference_to_pointer<int const*&>::value));
static_assert((is_reference_to_pointer<int const* const&>::value));
static_assert((is_reference_to_pointer<int const*volatile&>::value));
static_assert((is_reference_to_pointer<int const*const volatile&>::value));
static_assert(!(is_reference_to_pointer<pmf>::value));

static_assert(!(is_reference_to_pointer<int const volatile>::value));
static_assert(!(is_reference_to_pointer<int>::value));
static_assert(!(is_reference_to_pointer<int*>::value));

static_assert(!(is_reference_to_const<int*&>::value));
static_assert((is_reference_to_const<int* const&>::value));
static_assert(!(is_reference_to_const<int*volatile&>::value));
static_assert((is_reference_to_const<int*const volatile&>::value));
    
static_assert(!(is_reference_to_const<int const volatile>::value));
static_assert(!(is_reference_to_const<int>::value));
static_assert(!(is_reference_to_const<int*>::value));

static_assert((is_reference_to_non_const<int*&>::value));
static_assert(!(is_reference_to_non_const<int* const&>::value));
static_assert((is_reference_to_non_const<int*volatile&>::value));
static_assert(!(is_reference_to_non_const<int*const volatile&>::value));
    
static_assert(!(is_reference_to_non_const<int const volatile>::value));
static_assert(!(is_reference_to_non_const<int>::value));
static_assert(!(is_reference_to_non_const<int*>::value));
    
static_assert(!(is_reference_to_volatile<int*&>::value));
static_assert(!(is_reference_to_volatile<int* const&>::value));
static_assert((is_reference_to_volatile<int*volatile&>::value));
static_assert((is_reference_to_volatile<int*const volatile&>::value));
    
static_assert(!(is_reference_to_volatile<int const volatile>::value));
static_assert(!(is_reference_to_volatile<int>::value));
static_assert(!(is_reference_to_volatile<int*>::value));

namespace tt = PXR_BOOST_NAMESPACE::python::indirect_traits;

static_assert(!(tt::is_reference_to_class<int>::value));
static_assert(!(tt::is_reference_to_class<int&>::value));
static_assert(!(tt::is_reference_to_class<int*>::value));
    

static_assert(!(tt::is_reference_to_class<pmf>::value));
static_assert(!(tt::is_reference_to_class<pmf const&>::value));
    
static_assert(!(tt::is_reference_to_class<X>::value));

static_assert((tt::is_reference_to_class<X&>::value));
static_assert((tt::is_reference_to_class<X const&>::value));
static_assert((tt::is_reference_to_class<X volatile&>::value));
static_assert((tt::is_reference_to_class<X const volatile&>::value));
    
static_assert(!(is_pointer_to_class<int>::value));
static_assert(!(is_pointer_to_class<int*>::value));
static_assert(!(is_pointer_to_class<int&>::value));
    
static_assert(!(is_pointer_to_class<X>::value));
static_assert(!(is_pointer_to_class<X&>::value));
static_assert(!(is_pointer_to_class<pmf>::value));
static_assert(!(is_pointer_to_class<pmf const>::value));
static_assert((is_pointer_to_class<X*>::value));
static_assert((is_pointer_to_class<X const*>::value));
static_assert((is_pointer_to_class<X volatile*>::value));
static_assert((is_pointer_to_class<X const volatile*>::value));

static_assert((tt::is_reference_to_member_function_pointer<pmf&>::value));
static_assert((tt::is_reference_to_member_function_pointer<pmf const&>::value));
static_assert((tt::is_reference_to_member_function_pointer<pmf volatile&>::value));
static_assert((tt::is_reference_to_member_function_pointer<pmf const volatile&>::value));
static_assert(!(tt::is_reference_to_member_function_pointer<pmf[2]>::value));
static_assert(!(tt::is_reference_to_member_function_pointer<pmf(&)[2]>::value));
static_assert(!(tt::is_reference_to_member_function_pointer<pmf>::value));
    
