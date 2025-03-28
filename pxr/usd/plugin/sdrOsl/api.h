//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PLUGIN_SDR_OSL_API_H
#define PXR_USD_PLUGIN_SDR_OSL_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define SDROSL_API
#   define SDROSL_API_TEMPLATE_CLASS(...)
#   define SDROSL_API_TEMPLATE_STRUCT(...)
#   define SDROSL_LOCAL
#else
#   if defined(SDROSL_EXPORTS)
#       define SDROSL_API ARCH_EXPORT
#       define SDROSL_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define SDROSL_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define SDROSL_API ARCH_IMPORT
#       define SDROSL_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define SDROSL_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define SDROSL_LOCAL ARCH_HIDDEN
#endif

#endif
