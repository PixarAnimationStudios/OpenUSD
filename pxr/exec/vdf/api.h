//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_API_H
#define PXR_EXEC_VDF_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define VDF_API
#   define VDF_API_TYPE
#   define VDF_API_TEMPLATE_CLASS(...)
#   define VDF_API_TEMPLATE_STRUCT(...)
#   define VDF_LOCAL
#else
#   if defined(VDF_EXPORTS)
#       define VDF_API ARCH_EXPORT
#       define VDF_API_TYPE ARCH_EXPORT_TYPE
#       define VDF_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define VDF_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define VDF_API ARCH_IMPORT
#       define VDF_API_TYPE
#       define VDF_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define VDF_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define VDF_LOCAL ARCH_HIDDEN
#endif

#endif
