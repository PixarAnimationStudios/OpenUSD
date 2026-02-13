//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LOADER_API_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_LOADER_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HDPRMANLOADER_API
#   define HDPRMANLOADER_API_TEMPLATE_CLASS(...)
#   define HDPRMANLOADER_API_TEMPLATE_STRUCT(...)
#   define HDPRMANLOADER_LOCAL
#else
#   if defined(HDPRMANLOADER_EXPORTS)
#       define HDPRMANLOADER_API ARCH_EXPORT
#       define HDPRMANLOADER_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDPRMANLOADER_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HDPRMANLOADER_API ARCH_IMPORT
#       define HDPRMANLOADER_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDPRMANLOADER_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HDPRMANLOADER_LOCAL ARCH_HIDDEN
#endif

#endif
