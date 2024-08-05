//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_API_H
#define PXR_BASE_TS_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define TS_API
#   define TS_API_TEMPLATE_CLASS(...)
#   define TS_API_TEMPLATE_STRUCT(...)
#   define TS_LOCAL
#else
#   if defined(TS_EXPORTS)
#       define TS_API ARCH_EXPORT
#       define TS_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define TS_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define TS_API ARCH_IMPORT
#       define TS_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define TS_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define TS_LOCAL ARCH_HIDDEN
#endif

#endif
