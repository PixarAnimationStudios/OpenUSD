//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_API_H
#define PXR_EXEC_EF_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define EF_API
#   define EF_API_TYPE
#   define EF_API_TEMPLATE_CLASS(...)
#   define EF_API_TEMPLATE_STRUCT(...)
#   define EF_LOCAL
#else
#   if defined(EF_EXPORTS)
#       define EF_API ARCH_EXPORT
#       define EF_API_TYPE ARCH_EXPORT_TYPE
#       define EF_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define EF_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define EF_API ARCH_IMPORT
#       define EF_API_TYPE
#       define EF_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define EF_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define EF_LOCAL ARCH_HIDDEN
#endif

#endif
