//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_IR_API_H
#define PXR_EXEC_EXEC_IR_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define EXECIR_API
#   define EXECIR_API_TYPE
#   define EXECIR_API_TEMPLATE_CLASS(...)
#   define EXECIR_API_TEMPLATE_STRUCT(...)
#   define EXECIR_LOCAL
#else
#   if defined(EXECIR_EXPORTS)
#       define EXECIR_API ARCH_EXPORT
#       define EXECIR_API_TYPE ARCH_EXPORT_TYPE
#       define EXECIR_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define EXECIR_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define EXECIR_API ARCH_IMPORT
#       define EXECIR_API_TYPE
#       define EXECIR_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define EXECIR_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define EXECIR_LOCAL ARCH_HIDDEN
#endif

#endif
