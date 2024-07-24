//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_API_H
#define PXR_USD_PCP_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define PCP_API
#   define PCP_API_TEMPLATE_CLASS(...)
#   define PCP_API_TEMPLATE_STRUCT(...)
#   define PCP_LOCAL
#else
#   if defined(PCP_EXPORTS)
#       define PCP_API ARCH_EXPORT
#       define PCP_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define PCP_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define PCP_API ARCH_IMPORT
#       define PCP_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define PCP_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define PCP_LOCAL ARCH_HIDDEN
#endif

#endif
