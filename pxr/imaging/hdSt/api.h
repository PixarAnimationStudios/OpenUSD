//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_API_H
#define PXR_IMAGING_HD_ST_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HDST_API
#   define HDST_API_TEMPLATE_CLASS(...)
#   define HDST_API_TEMPLATE_STRUCT(...)
#   define HDST_LOCAL
#else
#   if defined(HDST_EXPORTS)
#       define HDST_API ARCH_EXPORT
#       define HDST_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDST_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HDST_API ARCH_IMPORT
#       define HDST_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDST_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HDST_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_IMAGING_HD_ST_API_H
