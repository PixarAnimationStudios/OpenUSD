//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_GARCH_API_H
#define PXR_IMAGING_GARCH_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define GARCH_API
#   define GARCH_API_TEMPLATE_CLASS(...)
#   define GARCH_API_TEMPLATE_STRUCT(...)
#   define GARCH_LOCAL
#else
#   if defined(GARCH_EXPORTS)
#       define GARCH_API ARCH_EXPORT
#       define GARCH_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define GARCH_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define GARCH_API ARCH_IMPORT
#       define GARCH_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define GARCH_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define GARCH_LOCAL ARCH_HIDDEN
#endif

#endif
