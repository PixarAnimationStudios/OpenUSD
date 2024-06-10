//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_API_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define HDPRMAN_API
#   define HDPRMAN_API_TEMPLATE_CLASS(...)
#   define HDPRMAN_API_TEMPLATE_STRUCT(...)
#   define HDPRMAN_LOCAL
#else
#   if defined(HDPRMAN_EXPORTS)
#       define HDPRMAN_API ARCH_EXPORT
#       define HDPRMAN_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDPRMAN_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define HDPRMAN_API ARCH_IMPORT
#       define HDPRMAN_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define HDPRMAN_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define HDPRMAN_LOCAL ARCH_HIDDEN
#endif

#endif
