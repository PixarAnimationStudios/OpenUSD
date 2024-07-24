//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDR_API_H
#define PXR_USD_SDR_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define SDR_API
#   define SDR_API_TEMPLATE_CLASS(...)
#   define SDR_API_TEMPLATE_STRUCT(...)
#   define SDR_LOCAL
#else
#   if defined(SDR_EXPORTS)
#       define SDR_API ARCH_EXPORT
#       define SDR_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define SDR_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define SDR_API ARCH_IMPORT
#       define SDR_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define SDR_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define SDR_LOCAL ARCH_HIDDEN
#endif

#endif
