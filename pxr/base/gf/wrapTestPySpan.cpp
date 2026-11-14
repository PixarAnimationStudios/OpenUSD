//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/range1d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/pySpan.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python/def.hpp"

#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

// Test functions that report where a span points and what it holds.
template <class T>
size_t
_test_SpanAddress(TfSpan<const T> span)
{
    return reinterpret_cast<size_t>(span.data());
}

template <class T>
list
_test_SpanElements(TfSpan<const T> span)
{
    return TfPyCopySequenceToList(span);
}

template <class T>
void
_test_FillSpan(TfSpan<T> span, T const &value)
{
    std::fill(span.begin(), span.end(), value);
}

} // anon

void wrapTestPySpan()
{
    def("_test_Vec3fSpanAddress", _test_SpanAddress<GfVec3f>);
    def("_test_Vec3fSpanElements", _test_SpanElements<GfVec3f>);
    def("_test_FillVec3fSpan", _test_FillSpan<GfVec3f>);
    def("_test_Matrix3fSpanAddress", _test_SpanAddress<GfMatrix3f>);
    def("_test_Matrix3fSpanElements", _test_SpanElements<GfMatrix3f>);
    def("_test_Range1dSpanElements", _test_SpanElements<GfRange1d>);
    def("_test_HalfSpanElements", _test_SpanElements<GfHalf>);
}
