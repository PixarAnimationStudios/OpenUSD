// Copyright (c) 2017-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_CONFIG_HPP
#define PXR_PEGTL_CONFIG_HPP

// Define PXR_PEGTL_NAMESPACE based on internal namespace to isolate
// it from other versions of USD/PEGTL in client code.
#include "pxr/pxr.h"

#if PXR_USE_NAMESPACES
#define PXR_PEGTL_NAMESPACE PXR_INTERNAL_NS ## _pegtl
#else
#define PXR_PEGTL_NAMESPACE pxr_pegtl
#endif

#endif
