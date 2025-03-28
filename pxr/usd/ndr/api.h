//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_NDR_API_H
#define PXR_USD_NDR_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define NDR_API
#   define NDR_API_TEMPLATE_CLASS(...)
#   define NDR_API_TEMPLATE_STRUCT(...)
#   define NDR_LOCAL
#else
#   if defined(NDR_EXPORTS)
#       define NDR_API ARCH_EXPORT
#       define NDR_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define NDR_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define NDR_API ARCH_IMPORT
#       define NDR_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define NDR_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define NDR_LOCAL ARCH_HIDDEN
#endif

#endif
