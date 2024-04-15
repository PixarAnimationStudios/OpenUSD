// Copyright (c) 2020-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_CONTRIB_INSTANTIATE_HPP
#define PXR_PEGTL_CONTRIB_INSTANTIATE_HPP

#include "../config.hpp"

#include "../apply_mode.hpp"
#include "../match.hpp"
#include "../nothing.hpp"
#include "../rewind_mode.hpp"

namespace PXR_PEGTL_NAMESPACE
{
   template< typename T >
   struct instantiate
      : maybe_nothing
   {
      template< typename Rule,
                apply_mode A,
                rewind_mode M,
                template< typename... >
                class Action,
                template< typename... >
                class Control,
                typename ParseInput,
                typename... States >
      [[nodiscard]] static bool match( ParseInput& in, States&&... st )
      {
         const T t( static_cast< const ParseInput& >( in ), st... );
         return PXR_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, st... );
      }
   };

}  // namespace PXR_PEGTL_NAMESPACE

#endif
