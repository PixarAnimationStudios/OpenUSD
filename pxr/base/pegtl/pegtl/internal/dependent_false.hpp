// Copyright (c) 2018-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_INTERNAL_DEPENDENT_FALSE_HPP
#define PXR_PEGTL_INTERNAL_DEPENDENT_FALSE_HPP

#include "../config.hpp"

namespace PXR_PEGTL_NAMESPACE::internal
{
   template< typename... >
   inline constexpr bool dependent_false = false;

}  // namespace PXR_PEGTL_NAMESPACE::internal

#endif
