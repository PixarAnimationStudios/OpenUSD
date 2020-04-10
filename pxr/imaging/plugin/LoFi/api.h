//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_API_H
#define PXR_IMAGING_PLUGIN_LOFI_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define LOFI_API
#   define LOFI_API_TEMPLATE_CLASS(...)
#   define LOFI_API_TEMPLATE_STRUCT(...)
#   define HD_LOCAL
#else
#   if defined(HD_EXPORTS)
#       define LOFI_API ARCH_EXPORT
#       define LOFI_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define LOFI_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define LOFI_API ARCH_IMPORT
#       define LOFI_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define LOFI_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define LOFI_LOCAL ARCH_HIDDEN
#endif

#endif // PXR_IMAGING_PLUGIN_LOFI_API_H
