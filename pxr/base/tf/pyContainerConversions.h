//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_CONTAINER_CONVERSIONS_H
#define PXR_BASE_TF_PY_CONTAINER_CONVERSIONS_H

/// \file tf/pyContainerConversions.h
/// Utilities for providing C++ <-> Python container support.

/*
 * Adapted (modified) from original at http://cctbx.sourceforge.net
 * Original file:
 * cctbx/scitbx/include/scitbx/boost_python/container_conversions.h
 * License:
 * http://cvs.sourceforge.net/viewcvs.py/cctbx/cctbx/
 *                                      LICENSE.txt?rev=1.2&view=markup
 */

#include "pxr/pxr.h"

#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pyUtils.h"

#include <nanobind/nanobind.h>

#include <deque>
#include <list>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Converts any iterable C++ container to a Python list.
template <typename ContainerType>
struct TfPySequenceToPython
{
    static PyObject* convert(ContainerType const &c)
    {
        return TfPyCopySequenceToList(c).release().ptr();
    }
};

// Converts any iterable C++ container to a Python set.
template <typename ContainerType>
struct TfPySequenceToPythonSet
{
    static PyObject* convert(ContainerType const &c)
    {
        return TfPyCopySequenceToSet(c).release().ptr();
    }
};

template <typename ContainerType>
struct TfPyMapToPythonDict
{
    static PyObject* convert(ContainerType const &c)
    {
        return TfPyCopyMapToDictionary(c).release().ptr();
    }    
};

namespace TfPyContainerConversions {

  template <typename ContainerType>
  struct to_tuple
  {
    static PyObject* convert(ContainerType const& a)
    {
      return TfPyCopySequenceToTuple(a).release().ptr();
    }
  };

  template <typename First, typename Second>
  struct to_tuple<std::pair<First, Second> > {
    static PyObject* convert(std::pair<First, Second> const& a)
    {
      return nanobind::make_tuple(a.first, a.second).release().ptr();
    }
  };

  template <typename... T>
  struct to_tuple<std::tuple<T...>> {
    static PyObject* convert(std::tuple<T...> const& a)
    {
      return std::apply(
        [](T const&... v) {
          return nanobind::make_tuple(v...).release().ptr();
        }, a);
    }
  };

  struct default_policy {};
  struct fixed_size_policy {};
  struct variable_capacity_policy : default_policy {};
  struct variable_capacity_all_items_convertible_policy : variable_capacity_policy {};
  struct fixed_capacity_policy : variable_capacity_policy {};
  struct linked_list_policy : default_policy {};
  struct set_policy : default_policy {};

  template <typename ContainerType, typename ConversionPolicy>
  struct from_python_sequence
  {
    from_python_sequence() = default;
  };

  template <typename PairType>
  struct from_python_tuple_pair
  {
    from_python_tuple_pair() = default;
  };

  template <typename TupleType>
  struct from_python_tuple
  {
    from_python_tuple() = default;
  };

  template <typename ContainerType>
  struct to_tuple_mapping
  {
    to_tuple_mapping() = default;
  };

  template <typename ContainerType, typename ConversionPolicy>
  struct tuple_mapping : to_tuple_mapping<ContainerType>
  {
    tuple_mapping() = default;
  };

  template <typename ContainerType>
  struct tuple_mapping_fixed_size
  {
    tuple_mapping_fixed_size() = default;
  };

  template <typename ContainerType>
  struct tuple_mapping_fixed_capacity
  {
    tuple_mapping_fixed_capacity() = default;
  };

  template <typename ContainerType>
  struct tuple_mapping_variable_capacity
  {
    tuple_mapping_variable_capacity() = default;
  };

  template <typename ContainerType>
  struct tuple_mapping_set
  {
    tuple_mapping_set() = default;
  };

  template <typename ContainerType>
  struct tuple_mapping_pair
  {
    tuple_mapping_pair() = default;
  };

} // namespace TfPyContainerConversions

template <class T>
void TfPyRegisterStlSequencesFromPython()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_CONTAINER_CONVERSIONS_H
