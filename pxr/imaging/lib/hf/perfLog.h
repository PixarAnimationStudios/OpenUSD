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
#ifndef HF_PERF_LOG_H
#define HF_PERF_LOG_H

#include "pxr/pxr.h"
#include "pxr/base/tf/mallocTag.h"
#include <boost/preprocessor/stringize.hpp>

PXR_NAMESPACE_OPEN_SCOPE


///
/// Creates an auto-mallocTag with the function, including template params.
///
#define HF_MALLOC_TAG_FUNCTION() \
    TfAutoMallocTag2 tagFunc(BOOST_PP_STRINGIZE(MFB_PACKAGE_NAME), \
                             __ARCH_PRETTY_FUNCTION__);

///
/// Creates an auto-mallocTag with the given named tag.
///
#define HF_MALLOC_TAG(x) \
    TfAutoMallocTag2 tag2(BOOST_PP_STRINGIZE(MFB_PACKAGE_NAME), x);

///
/// Overrides operator new/delete and injects malloc tags.
///
#define HF_MALLOC_TAG_NEW(x) \
    TF_MALLOC_TAG_NEW(BOOST_PP_STRINGIZE(MFB_PACKAGE_NAME), x);


#define HF_TRACE_FUNCTION_SCOPE(tag)                                  \
  TRACE_SCOPE(ArchGetPrettierFunctionName(__ARCH_FUNCTION__,          \
                                          __ARCH_PRETTY_FUNCTION__) + \
              std::string (" (" tag ")"))

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HF_PERF_LOG_H
