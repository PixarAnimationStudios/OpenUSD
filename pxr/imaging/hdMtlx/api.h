//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_MTLX_API_H
#define PXR_IMAGING_HD_MTLX_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HDMTLX_API
#   define HDMTLX_API_TEMPLATE_CLASS(...)
#   define HDMTLX_API_TEMPLATE_STRUCT(...)
#   define HDMTLX_LOCAL
#else
#   if defined(HDMTLX_EXPORTS)
#       define HDMTLX_API ARCH_EXPORT
#       define HDMTLX_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDMTLX_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HDMTLX_API ARCH_IMPORT
#       define HDMTLX_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDMTLX_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HDMTLX_LOCAL ARCH_HIDDEN
#endif

#endif
