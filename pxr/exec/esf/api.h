//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_API_H
#define PXR_EXEC_ESF_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define ESF_API
#   define ESF_API_TYPE
#   define ESF_API_TEMPLATE_CLASS(...)
#   define ESF_API_TEMPLATE_STRUCT(...)
#   define ESF_LOCAL
#else
#   if defined(ESF_EXPORTS)
#       define ESF_API ARCH_EXPORT
#       define ESF_API_TYPE ARCH_EXPORT_TYPE
#       define ESF_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define ESF_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define ESF_API ARCH_IMPORT
#       define ESF_API_TYPE
#       define ESF_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define ESF_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define ESF_LOCAL ARCH_HIDDEN
#endif

#endif
