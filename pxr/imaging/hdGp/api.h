//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDGP_API_H
#define PXR_IMAGING_HDGP_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HDGP_API
#   define HDGP_API_TEMPLATE_CLASS(...)
#   define HDGP_API_TEMPLATE_STRUCT(...)
#   define HDGP_LOCAL
#else
#   if defined(HDGP_EXPORTS)
#       define HDGP_API ARCH_EXPORT
#       define HDGP_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDGP_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HDGP_API ARCH_IMPORT
#       define HDGP_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDGP_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HDGP_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_IMAGING_HDGP_API_H
