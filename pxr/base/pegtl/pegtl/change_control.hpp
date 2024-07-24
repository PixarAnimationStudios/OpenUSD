// Copyright (c) 2019-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_CHANGE_CONTROL_HPP
#define PXR_PEGTL_CHANGE_CONTROL_HPP

#include "apply_mode.hpp"
#include "config.hpp"
#include "match.hpp"
#include "nothing.hpp"
#include "rewind_mode.hpp"

namespace PXR_PEGTL_NAMESPACE
{
   template< template< typename... > class NewControl >
   struct change_control
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
         return PXR_PEGTL_NAMESPACE::match< Rule, A, M, Action, NewControl >( in, st... );
      }
   };

}  // namespace PXR_PEGTL_NAMESPACE

#endif
