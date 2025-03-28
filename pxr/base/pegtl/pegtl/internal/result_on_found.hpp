// Copyright (c) 2014-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_INTERNAL_RESULT_ON_FOUND_HPP
#define PXR_PEGTL_INTERNAL_RESULT_ON_FOUND_HPP

#include "../config.hpp"

namespace PXR_PEGTL_NAMESPACE::internal
{
   enum class result_on_found : bool
   {
      success = true,
      failure = false
   };

}  // namespace PXR_PEGTL_NAMESPACE::internal

#endif
