//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_READ_ITERATOR_RANGE_H
#define PXR_EXEC_VDF_READ_ITERATOR_RANGE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/iteratorRange.h"
#include "pxr/exec/vdf/readIterator.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfReadIteratorRange
///
/// This class allows for construction of iterable ranges of input values. The
/// underlying iterator used is VdfReadIterator. The
/// VdfReadIteratorRange::begin() and VdfReadIteatorRange::end() methods return
/// an instance of VdfReadIterator<T> denoting the iterable range. The class
/// satisfies the STL API requirements that make it suitable for use in range-
/// based for loops.
/// 
/// For example:
/// ```{.cpp}
/// VdfReadIteratorRange<double> range(context, _tokens->in);
/// for (const double x : range) {
///    ...
/// }
/// ```
///
/// This class can also be used to copy ranges of input values into STL
/// containers. It can be used in range constructors, for example.
/// 
/// ```{.cpp}
/// VdfReadIteratorRange<double> range(context, _tokens->in);
/// std::vector<double> values(range.begin(), range.end());
/// ```
/// 
/// \warning Note that std::distance has linear complexity on this range.
///
/// Determining the number of input values in a range is not trivial, and might
/// be costly. Invoking std::distance on VdfReadIterator is slightly more
/// expensive than calling VdfReadIteratorRange::ComputeSize() (i.e.
/// VdfReadIterator::ComputeSize().)
///
template<typename T>
using VdfReadIteratorRange = VdfIteratorRange<VdfReadIterator<T>>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
