//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IR_IMAGING_API_H
#define PXR_USD_IMAGING_USD_IR_IMAGING_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDIRIMAGING_API
#   define USDIRIMAGING_API_TYPE
#   define USDIRIMAGING_API_TEMPLATE_CLASS(...)
#   define USDIRIMAGING_API_TEMPLATE_STRUCT(...)
#   define USDIRIMAGING_LOCAL
#else
#   if defined(USDIRIMAGING_EXPORTS)
#       define USDIRIMAGING_API ARCH_EXPORT
#       define USDIRIMAGING_API_TYPE ARCH_EXPORT_TYPE
#       define USDIRIMAGING_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDIRIMAGING_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDIRIMAGING_API ARCH_IMPORT
#       define USDIRIMAGING_API_TYPE
#       define USDIRIMAGING_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDIRIMAGING_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDIRIMAGING_LOCAL ARCH_HIDDEN
#endif

#endif
