//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HF_PERF_LOG_H
#define PXR_IMAGING_HF_PERF_LOG_H

#include "pxr/pxr.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

PXR_NAMESPACE_OPEN_SCOPE


///
/// Creates an auto-mallocTag with the function, including template params.
///
#define HF_MALLOC_TAG_FUNCTION() \
    TfAutoMallocTag2 tagFunc(TF_PP_STRINGIZE(MFB_PACKAGE_NAME), \
                             __ARCH_PRETTY_FUNCTION__);

///
/// Creates an auto-mallocTag with the given named tag.
///
#define HF_MALLOC_TAG(x) \
    TfAutoMallocTag2 tag2(TF_PP_STRINGIZE(MFB_PACKAGE_NAME), x);

///
/// Overrides operator new/delete and injects malloc tags.
///
#define HF_MALLOC_TAG_NEW(x) \
    TF_MALLOC_TAG_NEW(TF_PP_STRINGIZE(MFB_PACKAGE_NAME), x);


#define HF_TRACE_FUNCTION_SCOPE(tag)                                  \
  TRACE_FUNCTION_SCOPE(tag)

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HF_PERF_LOG_H
