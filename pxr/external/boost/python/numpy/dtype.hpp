//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Jim Bosch 2010-2012.
// Copyright Stefan Seefeld 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PXR_EXTERNAL_BOOST_PYTHON_NUMPY_DTYPE_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_NUMPY_DTYPE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/numpy/dtype.hpp>
#else

/**
 *  @file boost/python/numpy/dtype.hpp
 *  @brief Object manager for Python's numpy.dtype class.
 */

#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/numpy/config.hpp"
#include "pxr/external/boost/python/numpy/numpy_object_mgr_traits.hpp"
#include <boost/mpl/for_each.hpp>
#include "pxr/external/boost/python/detail/type_traits.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace numpy {

/**
 *  @brief A boost.python "object manager" (subclass of object) for numpy.dtype.
 *
 *  @todo This could have a lot more interesting accessors.
 */
class BOOST_NUMPY_DECL dtype : public object {
  static python::detail::new_reference convert(object::object_cref arg, bool align);
public:

  /// @brief Convert an arbitrary Python object to a data-type descriptor object.
  template <typename T>
  explicit dtype(T arg, bool align=false) : object(convert(arg, align)) {}

  /**
   *  @brief Get the built-in numpy dtype associated with the given scalar template type.
   *
   *  This is perhaps the most useful part of the numpy API: it returns the dtype object
   *  corresponding to a built-in C++ type.  This should work for any integer or floating point
   *  type supported by numpy, and will also work for std::complex if 
   *  sizeof(std::complex<T>) == 2*sizeof(T).
   *
   *  It can also be useful for users to add explicit specializations for POD structs
   *  that return field-based dtypes.
   */
  template <typename T> static dtype get_builtin();

  /// @brief Return the size of the data type in bytes.
  int get_itemsize() const;

  /**
   *  @brief Compare two dtypes for equivalence.
   *
   *  This is more permissive than equality tests.  For instance, if long and int are the same
   *  size, the dtypes corresponding to each will be equivalent, but not equal.
   */
  friend BOOST_NUMPY_DECL bool equivalent(dtype const & a, dtype const & b);

  /**
   *  @brief Register from-Python converters for NumPy's built-in array scalar types.
   *
   *  This is usually called automatically by initialize(), and shouldn't be called twice
   *  (doing so just adds unused converters to the Boost.Python registry).
   */
  static void register_scalar_converters();

  PXR_BOOST_PYTHON_FORWARD_OBJECT_CONSTRUCTORS(dtype, object);

};

BOOST_NUMPY_DECL bool equivalent(dtype const & a, dtype const & b);

namespace detail
{

template <int bits, bool isUnsigned> dtype get_int_dtype();

template <int bits> dtype get_float_dtype();

template <int bits> dtype get_complex_dtype();

template <typename T, bool isInt=boost::is_integral<T>::value>
struct builtin_dtype;

template <typename T>
struct builtin_dtype<T,true> {
  static dtype get() { return get_int_dtype< 8*sizeof(T), boost::is_unsigned<T>::value >(); }
};

template <>
struct BOOST_NUMPY_DECL builtin_dtype<bool,true> {
  static dtype get();
};

template <typename T>
struct builtin_dtype<T,false> {
  static dtype get() { return get_float_dtype< 8*sizeof(T) >(); }
};

template <typename T>
struct builtin_dtype< std::complex<T>, false > {
  static dtype get() { return get_complex_dtype< 16*sizeof(T) >(); }  
};

} // namespace detail

template <typename T>
inline dtype dtype::get_builtin() { return detail::builtin_dtype<T>::get(); }

} // namespace PXR_BOOST_NAMESPACE::python::numpy

namespace converter {
NUMPY_OBJECT_MANAGER_TRAITS(numpy::dtype);
}}} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif
