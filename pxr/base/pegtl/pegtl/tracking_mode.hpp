// Copyright (c) 2017-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_TRACKING_MODE_HPP
#define PXR_PEGTL_TRACKING_MODE_HPP

#include "config.hpp"

namespace PXR_PEGTL_NAMESPACE
{
   enum class tracking_mode : bool
   {
      eager,
      lazy
   };

}  // namespace PXR_PEGTL_NAMESPACE

#endif
