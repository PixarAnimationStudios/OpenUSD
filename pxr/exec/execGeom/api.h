//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_GEOM_USD_API_H
#define PXR_EXEC_GEOM_USD_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define EXECGEOM_API
#   define EXECGEOM_API_TYPE
#   define EXECGEOM_API_TEMPLATE_CLASS(...)
#   define EXECGEOM_API_TEMPLATE_STRUCT(...)
#   define EXECGEOM_LOCAL
#else
#   if defined(EXECGEOM_EXPORTS)
#       define EXECGEOM_API ARCH_EXPORT
#       define EXECGEOM_API_TYPE ARCH_EXPORT_TYPE
#       define EXECGEOM_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define EXECGEOM_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define EXECGEOM_API ARCH_IMPORT
#       define EXECGEOM_API_TYPE
#       define EXECGEOM_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define EXECGEOM_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define EXECGEOM_LOCAL ARCH_HIDDEN
#endif

#endif
