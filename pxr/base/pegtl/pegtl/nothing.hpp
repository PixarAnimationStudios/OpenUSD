// Copyright (c) 2014-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_NOTHING_HPP
#define PXR_PEGTL_NOTHING_HPP

#include "config.hpp"

namespace PXR_PEGTL_NAMESPACE
{
   template< typename Rule >
   struct nothing
   {};

   using maybe_nothing = nothing< void >;

}  // namespace PXR_PEGTL_NAMESPACE

#endif
