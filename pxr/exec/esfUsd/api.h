//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_USD_API_H
#define PXR_EXEC_ESF_USD_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define ESFUSD_API
#   define ESFUSD_API_TYPE
#   define ESFUSD_API_TEMPLATE_CLASS(...)
#   define ESFUSD_API_TEMPLATE_STRUCT(...)
#   define ESFUSD_LOCAL
#else
#   if defined(ESFUSD_EXPORTS)
#       define ESFUSD_API ARCH_EXPORT
#       define ESFUSD_API_TYPE ARCH_EXPORT_TYPE
#       define ESFUSD_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define ESFUSD_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define ESFUSD_API ARCH_IMPORT
#       define ESFUSD_API_TYPE
#       define ESFUSD_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define ESFUSD_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define ESFUSD_LOCAL ARCH_HIDDEN
#endif

#endif
