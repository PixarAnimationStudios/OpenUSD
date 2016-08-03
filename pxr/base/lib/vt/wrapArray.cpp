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
#include "pxr/base/vt/wrapArray.h"


namespace Vt_WrapArray {

// The following bit of preprocessor code produces specializations of
// GetVtArrayName (declared above) for each VtArray type.  The function bodies
// simply return the "common name" for the VtArray.  For instance,
// GetVtArrayName<VtArray<int> >() -> "VtIntArray".
#define MAKE_NAME_FUNC(r, unused, elem) \
template <> \
VT_API string GetVtArrayName< VT_TYPE(elem) >() { \
    return BOOST_PP_STRINGIZE(VT_TYPE_NAME(elem)); \
}
BOOST_PP_SEQ_FOR_EACH(MAKE_NAME_FUNC, ~, VT_ARRAY_VALUE_TYPES)
#undef MAKE_NAME_FUNC

}
