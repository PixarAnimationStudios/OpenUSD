//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USDVIEWQ_API_H
#define PXR_USD_IMAGING_USDVIEWQ_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDVIEWQ_API
#   define USDVIEWQ_API_TEMPLATE_CLASS(...)
#   define USDVIEWQ_API_TEMPLATE_STRUCT(...)
#   define USDVIEWQ_LOCAL
#else
#   if defined(USDVIEWQ_EXPORTS)
#       define USDVIEWQ_API ARCH_EXPORT
#       define USDVIEWQ_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDVIEWQ_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDVIEWQ_API ARCH_IMPORT
#       define USDVIEWQ_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDVIEWQ_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDVIEWQ_LOCAL ARCH_HIDDEN
#endif

#endif
