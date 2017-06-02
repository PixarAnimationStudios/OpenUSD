//
// Copyright 2017 Pixar
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
#include "gusd/UT_TypeTraits.h"

#include <UT/UT_Matrix2.h>
#include <UT/UT_Matrix3.h>
#include <UT/UT_Matrix4.h>
#include <UT/UT_Quaternion.h>
#include <UT/UT_Vector2.h>
#include <UT/UT_Vector3.h>
#include <UT/UT_Vector4.h>

PXR_NAMESPACE_OPEN_SCOPE

/* Make sure that the HDK doesn't change out the UT types on us,
   since we have hard-coded expectations of theses types.
   
   We don't do this in UT_TypeTraits.h because this level of validation
   requires the full type definitions, whereas we only want forward
   declarations in the header.*/
#define _VERIFY_TYPE(TYPE)                                                  \
    static_assert(std::is_same<GusdUT_TypeTraits::PODTuple<TYPE>::ValueType,\
                  TYPE::value_type>::value &&                               \
                  TYPE::tuple_size ==                                       \
                  GusdUT_TypeTraits::PODTuple<TYPE>::tupleSize,             \
                  "HDK type matches type/size registered in GusdUT_TypeTraits");

_VERIFY_TYPE(UT_Matrix2F);
_VERIFY_TYPE(UT_Matrix3F);
_VERIFY_TYPE(UT_Matrix4F);

_VERIFY_TYPE(UT_Matrix2D);
_VERIFY_TYPE(UT_Matrix3D);
_VERIFY_TYPE(UT_Matrix4D);

_VERIFY_TYPE(UT_QuaternionF);
_VERIFY_TYPE(UT_QuaternionD);

_VERIFY_TYPE(UT_Vector2F);
_VERIFY_TYPE(UT_Vector3F);
_VERIFY_TYPE(UT_Vector4F);

_VERIFY_TYPE(UT_Vector2D);
_VERIFY_TYPE(UT_Vector3D);
_VERIFY_TYPE(UT_Vector4D);

_VERIFY_TYPE(UT_Vector2i);
_VERIFY_TYPE(UT_Vector3i);
_VERIFY_TYPE(UT_Vector4i);

PXR_NAMESPACE_CLOSE_SCOPE

