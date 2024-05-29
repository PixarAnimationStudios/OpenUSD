//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HF_API_H
#define PXR_IMAGING_HF_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HF_API
#   define HF_API_TEMPLATE_CLASS(...)
#   define HF_API_TEMPLATE_STRUCT(...)
#   define HF_LOCAL
#else
#   if defined(HF_EXPORTS)
#       define HF_API ARCH_EXPORT
#       define HF_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HF_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HF_API ARCH_IMPORT
#       define HF_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HF_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HF_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_IMAGING_HF_API_H
