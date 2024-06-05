//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_API_H
#define PXR_USD_SDF_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define SDF_API
#   define SDF_API_TEMPLATE_CLASS(...)
#   define SDF_API_TEMPLATE_STRUCT(...)
#   define SDF_LOCAL
#else
#   if defined(SDF_EXPORTS)
#       define SDF_API ARCH_EXPORT
#       define SDF_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define SDF_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define SDF_API ARCH_IMPORT
#       define SDF_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define SDF_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define SDF_LOCAL ARCH_HIDDEN
#endif

#endif
