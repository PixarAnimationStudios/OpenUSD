//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_ARGS_PARSER_API_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_ARGS_PARSER_API_H

#include "pxr/base/arch/export.h"

#if defined(PXR_STATIC)
#   define RMAN_ARGS_PARSER_API
#   define RMAN_ARGS_PARSER_API_TEMPLATE_CLASS(...)
#   define RMAN_ARGS_PARSER_API_TEMPLATE_STRUCT(...)
#   define RMAN_ARGS_PARSER_LOCAL
#else
#   if defined(RMAN_ARGS_PARSER_EXPORTS)
#       define RMAN_ARGS_PARSER_API ARCH_EXPORT
#       define RMAN_ARGS_PARSER_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
#       define RMAN_ARGS_PARSER_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
#   else
#       define RMAN_ARGS_PARSER_API ARCH_IMPORT
#       define RMAN_ARGS_PARSER_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
#       define RMAN_ARGS_PARSER_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
#   endif
#   define RMAN_ARGS_PARSER_LOCAL ARCH_HIDDEN
#endif

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_RMAN_ARGS_PARSER_API_H
