//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDPROC_API_H
#define USDPROC_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDPROC_API
#   define USDPROC_API_TEMPLATE_CLASS(...)
#   define USDPROC_API_TEMPLATE_STRUCT(...)
#   define USDPROC_LOCAL
#else
#   if defined(USDPROC_EXPORTS)
#       define USDPROC_API ARCH_EXPORT
#       define USDPROC_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPROC_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDPROC_API ARCH_IMPORT
#       define USDPROC_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDPROC_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDPROC_LOCAL ARCH_HIDDEN
#endif

#endif
