// Copyright (c) 2014-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_APPLY_MODE_HPP
#define PXR_PEGTL_APPLY_MODE_HPP

#include "config.hpp"

namespace PXR_PEGTL_NAMESPACE
{
   enum class apply_mode : bool
   {
      action = true,
      nothing = false
   };

}  // namespace PXR_PEGTL_NAMESPACE

#endif
