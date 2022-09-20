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
#ifndef PXR_BASE_VT_OPERATORS_H
#define PXR_BASE_VT_OPERATORS_H

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/arch/pragmas.h"

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>

//
// Operators.h
//
// Define macros to make it easier to create and wrap operators that work
// with multiple types.  In general, we want most operators to allow
// array OP array
// array OP scalar
// scalar OP array
// array OP tuple
// tuple OP array
// array OP list
// list OP array
//
// where the operators work element-by-element.  For arrays, this means they
// need to be the same size and shape.   For tuples and lists, this means
// the array shape is ignored, and the array and tuple/list need to contain the
// same number of elements.  For scalars, this means the same scalar value is
// used for every array element.  Naturally we require that the types held in
// the array, tuple, list, and scalar are all the same (i.e no multiplying
// a VtDoubleArray by a GfVec3d).
//
// However, we do have special cases for
// array OP double
// double OP array
// for some OPs, for cases where you may want to do element-by-element
// operations with a double when the array type is *not* double (e.g. Vec3d).
// This is done in Core/Anim, for example, for spline calculations on the
// underlying data type.
//
// We define macros because this is so repetitive, and because they need to be
// defined in different places (the array operators on the class, the array and
// scalar operators as free functions, and the tuple/list operators in the
// Python wrapping of course).  Having them here allows us to put them all
// in one place for better editability.

// -------------------------------------------------------------------------
// C++ operator definitions
// -------------------------------------------------------------------------
// These will be callable from C++, and define operations between arrays
// and between arrays and scalars.

PXR_NAMESPACE_OPEN_SCOPE

struct Vt_ShapeData;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_OPERATORS_H
