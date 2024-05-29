//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_API_H
#define PXR_IMAGING_HDSI_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HDSI_API
#   define HDSI_API_TEMPLATE_CLASS(...)
#   define HDSI_API_TEMPLATE_STRUCT(...)
#   define HDSI_LOCAL
#else
#   if defined(HDSI_EXPORTS)
#       define HDSI_API ARCH_EXPORT
#       define HDSI_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDSI_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HDSI_API ARCH_IMPORT
#       define HDSI_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDSI_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HDSI_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_IMAGING_HDSI_API_H
