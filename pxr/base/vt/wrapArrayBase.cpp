//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/wrapArray.h"
#include "pxr/base/gf/vec3f.h"

#include <algorithm>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Test functions for passing VtArrays to wrapped functions taking TfSpan.
template <class T>
size_t
_test_SpanAddress(TfSpan<const T> span)
{
    return reinterpret_cast<size_t>(span.data());
}

template <class T>
size_t
_test_ArrayAddress(VtArray<T> const &array)
{
    return reinterpret_cast<size_t>(array.cdata());
}

template <class T>
VtArray<T>
_test_SpanToArray(TfSpan<const T> span)
{
    return VtArray<T>(span.begin(), span.end());
}

template <class T>
void
_test_FillSpan(TfSpan<T> span, T const &value)
{
    std::fill(span.begin(), span.end(), value);
}

} // anon

void wrapArray()
{
    // The actual wrapping of particular template instantiations is done in the
    // specific wrapArrayXXX.cpp files to avoid quadratic compiler behavior.

    using namespace pxr_boost::python;

    def("_test_IntSpanAddress", _test_SpanAddress<int>);
    def("_test_IntArrayAddress", _test_ArrayAddress<int>);
    def("_test_IntSpanToArray", _test_SpanToArray<int>);
    def("_test_FillIntSpan", _test_FillSpan<int>);

    def("_test_StringSpanAddress", _test_SpanAddress<std::string>);
    def("_test_StringArrayAddress", _test_ArrayAddress<std::string>);
    def("_test_StringSpanToArray", _test_SpanToArray<std::string>);

    def("_test_Vec3fSpanAddress", _test_SpanAddress<GfVec3f>);
    def("_test_Vec3fArrayAddress", _test_ArrayAddress<GfVec3f>);
    def("_test_FillVec3fSpan", _test_FillSpan<GfVec3f>);
}
