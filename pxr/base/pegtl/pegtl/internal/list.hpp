// Copyright (c) 2014-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_INTERNAL_LIST_HPP
#define PXR_PEGTL_INTERNAL_LIST_HPP

#include "../config.hpp"

#include "seq.hpp"
#include "star.hpp"

namespace PXR_PEGTL_NAMESPACE::internal
{
   template< typename Rule, typename Sep >
   using list = seq< Rule, star< Sep, Rule > >;

}  // namespace PXR_PEGTL_NAMESPACE::internal

#endif
