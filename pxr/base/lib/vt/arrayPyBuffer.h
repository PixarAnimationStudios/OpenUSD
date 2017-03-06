//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef VT_ARRAY_PYBUFFER_H
#define VT_ARRAY_PYBUFFER_H

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/pyObjWrapper.h"

#include <boost/optional.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// Convert \p obj which should support the python buffer protocol (e.g. a
/// numpy array) to a VtArray if possible and return it.  Return empty
/// optional if \p obj does not support the buffer protocol or does not have
/// compatible type and dimensions.  If \p err is supplied, set it to an
/// explanatory message in case of conversion failure.  This function may be
/// invoked for VtArray<T> where T is one of VT_ARRAY_PYBUFFER_TYPES.
template <class T>
boost::optional<VtArray<T> >
VtArrayFromPyBuffer(TfPyObjWrapper const &obj, std::string *err=nullptr);

/// The set of types for which it's valid to call VtArrayFromPyBuffer().
#define VT_ARRAY_PYBUFFER_TYPES                 \
    VT_BUILTIN_NUMERIC_VALUE_TYPES              \
    VT_VEC_VALUE_TYPES                          \
    VT_MATRIX_VALUE_TYPES                       \
    VT_GFRANGE_VALUE_TYPES                      \
    ((GfRect2i, Rect2i))                        \
    ((GfQuath, Quath))                          \
    ((GfQuatf, Quatf))                          \
    ((GfQuatd, Quatd))


PXR_NAMESPACE_CLOSE_SCOPE

#endif // VT_ARRAY_PYBUFFER_H
