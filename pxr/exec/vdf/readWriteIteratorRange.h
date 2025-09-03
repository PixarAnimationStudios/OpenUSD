//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_READ_WRITE_ITERATOR_RANGE_H
#define PXR_EXEC_VDF_READ_WRITE_ITERATOR_RANGE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/iteratorRange.h"
#include "pxr/exec/vdf/readWriteIterator.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfReadWriteIteratorRange
///
/// This class allows for construction of iterable ranges of input/output
/// values. The underlying iterator used is VdfReadWriteIterator. The
/// VdfReadWriteIteratorRange::begin() and VdfReadIteatorRange::end() methods
/// return an instance of VdfReadWriteIterator<T> denoting the iterable range.
/// The class satisfies the STL API requirements that make it suitable for use
/// in range-based for loops.
/// 
/// For example:
/// ```{.cpp}
/// VdfReadWriteIteratorRange<double> range(context);
/// for (const double x : range) {
///    ...
/// }
/// ```
///
/// This class can also be used to copy ranges of input/output values into STL
/// containers. It can be used in range constructors, for example.
/// 
/// ```{.cpp}
/// VdfReadWriteIteratorRange<double> range(context);
/// std::vector<double> values(range.begin(), range.end());
/// ```
/// 
/// It can also be used in STL algorithms that modify the iterated values:
/// 
/// ```{.cpp}
/// VdfReadWriteIteratorRange<double> range(context);
/// std::copy(values.begin(), values.end(), range.begin());
/// ```
/// 
template<typename T>
using VdfReadWriteIteratorRange = VdfIteratorRange<VdfReadWriteIterator<T>>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
