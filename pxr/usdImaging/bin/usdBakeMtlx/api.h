//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_BAKE_MTLX_API_H
#define PXR_USD_IMAGING_USD_BAKE_MTLX_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define USDBAKEMTLX_API
#   define USDBAKEMTLX_API_TEMPLATE_CLASS(...)
#   define USDBAKEMTLX_API_TEMPLATE_STRUCT(...)
#   define USDBAKEMTLX_LOCAL
#else
#   if defined(USDBAKEMTLX_EXPORTS)
#       define USDBAKEMTLX_API ARCH_EXPORT
#       define USDBAKEMTLX_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDBAKEMTLX_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define USDBAKEMTLX_API ARCH_IMPORT
#       define USDBAKEMTLX_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define USDBAKEMTLX_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define USDBAKEMTLX_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_USD_IMAGING_USD_BAKE_MTLX_API_H
