// Copyright (c) 2020-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_INTERNAL_MINUS_HPP
#define PXR_PEGTL_INTERNAL_MINUS_HPP

#include "../config.hpp"

#include "eof.hpp"
#include "not_at.hpp"
#include "rematch.hpp"
#include "seq.hpp"

namespace PXR_PEGTL_NAMESPACE::internal
{
   template< typename M, typename S >
   using minus = rematch< M, not_at< S, eof > >;

}  // namespace PXR_PEGTL_NAMESPACE::internal

#endif
