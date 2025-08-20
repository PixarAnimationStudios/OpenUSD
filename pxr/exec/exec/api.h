//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_API_H
#define PXR_EXEC_EXEC_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define EXEC_API
#   define EXEC_API_TYPE
#   define EXEC_API_TEMPLATE_CLASS(...)
#   define EXEC_API_TEMPLATE_STRUCT(...)
#   define EXEC_LOCAL
#else
#   if defined(EXEC_EXPORTS)
#       define EXEC_API ARCH_EXPORT
#       define EXEC_API_TYPE ARCH_EXPORT_TYPE
#       define EXEC_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define EXEC_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define EXEC_API ARCH_IMPORT
#       define EXEC_API_TYPE
#       define EXEC_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define EXEC_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define EXEC_LOCAL ARCH_HIDDEN
#endif

#endif
