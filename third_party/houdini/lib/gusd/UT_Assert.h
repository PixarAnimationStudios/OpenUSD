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
#ifndef _GUSD_UT_ASSERT_H_
#define _GUSD_UT_ASSERT_H_

#include <UT/UT_Assert.h>

#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

/** Helper for adding inline assertions to validate that a pointer is non-null.

    Example:
    
    @code
    OP_Node* cwd = GusdUTverify_ptr(OPgetDirector())->getCwd();
    @endcode */
template <class T>
T*  GusdUTverify_ptr(T* ptr)
{
    UT_ASSERT_P(ptr != NULL);
    return ptr;
}


/** Helper for inline assertions of the validity of some non-pointer type.
    
    Example:
    @code
    UsdStage stage = GusdUTverify_val(GetStage(...));
    @endcode */
template <class T>
T&  GusdUTverify_val(T&& val)
{
    UT_ASSERT_P((bool)val);
    return val;
}

PXR_NAMESPACE_CLOSE_SCOPE


#endif /*_GUSD_UT_ASSERT_H_*/
