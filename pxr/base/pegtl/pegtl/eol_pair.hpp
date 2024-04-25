// Copyright (c) 2017-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_EOL_PAIR_HPP
#define PXR_PEGTL_EOL_PAIR_HPP

#include <cstddef>
#include <utility>

#include "config.hpp"

namespace PXR_PEGTL_NAMESPACE
{
   using eol_pair = std::pair< bool, std::size_t >;

}  // namespace PXR_PEGTL_NAMESPACE

#endif
