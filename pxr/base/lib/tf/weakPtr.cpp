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
#include "pxr/base/tf/weakPtr.h"

/* 
 * Compile-time testing of the Tf_SupportsWeakPtr mechanism.  Change the 0
 * to 1 to enable.
 */
#if 0

#include <boost/static_assert.hpp>

struct _Tf_TestHasGetWeakBase {
    TfWeakBase const &__GetTfWeakBase__() const;
};

struct _Tf_TestHasGetWeakBaseDerived : public _Tf_TestHasGetWeakBase
{
};

struct _Tf_TestHasGetWeakBaseNot
{
};

struct _Tf_TestIsWeakBase : public TfWeakBase
{
};

BOOST_STATIC_ASSERT(TF_SUPPORTS_WEAKPTR(_Tf_TestHasGetWeakBase));
BOOST_STATIC_ASSERT(TF_SUPPORTS_WEAKPTR(_Tf_TestHasGetWeakBaseDerived));
BOOST_STATIC_ASSERT(not TF_SUPPORTS_WEAKPTR(_Tf_TestHasGetWeakBaseNot));
BOOST_STATIC_ASSERT(not TF_SUPPORTS_WEAKPTR(TfWeakPtr<_Tf_TestIsWeakBase>));
#endif // testing Tf_SupportsWeakPtr.
