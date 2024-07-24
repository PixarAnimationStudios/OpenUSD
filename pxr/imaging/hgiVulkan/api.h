//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_API_H
#define PXR_IMAGING_HGIVULKAN_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HGIVULKAN_API
#   define HGIVULKAN_API_TEMPLATE_CLASS(...)
#   define HGIVULKAN_API_TEMPLATE_STRUCT(...)
#   define HGIVULKAN_LOCAL
#else
#   if defined(HGIVULKAN_EXPORTS)
#       define HGIVULKAN_API ARCH_EXPORT
#       define HGIVULKAN_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIVULKAN_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HGIVULKAN_API ARCH_IMPORT
#       define HGIVULKAN_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HGIVULKAN_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HGIVULKAN_LOCAL ARCH_HIDDEN
#endif

#endif
