//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_USD_API_H
#define PXR_EXEC_EXEC_USD_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define EXECUSD_API
#   define EXECUSD_API_TYPE
#   define EXECUSD_API_TEMPLATE_CLASS(...)
#   define EXECUSD_API_TEMPLATE_STRUCT(...)
#   define EXECUSD_LOCAL
#else
#   if defined(EXECUSD_EXPORTS)
#       define EXECUSD_API ARCH_EXPORT
#       define EXECUSD_API_TYPE ARCH_EXPORT_TYPE
#       define EXECUSD_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define EXECUSD_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define EXECUSD_API ARCH_IMPORT
#       define EXECUSD_API_TYPE
#       define EXECUSD_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define EXECUSD_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define EXECUSD_LOCAL ARCH_HIDDEN
#endif

#endif
