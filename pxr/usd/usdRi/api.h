//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef USDRI_API_H
#define USDRI_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDRI_API
#   define USDRI_API_TEMPLATE_CLASS(...)
#   define USDRI_API_TEMPLATE_STRUCT(...)
#   define USDRI_LOCAL
#else
#   if defined(USDRI_EXPORTS)
#       define USDRI_API ARCH_EXPORT
#       define USDRI_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDRI_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDRI_API ARCH_IMPORT
#       define USDRI_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDRI_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDRI_LOCAL ARCH_HIDDEN
#endif

#endif
