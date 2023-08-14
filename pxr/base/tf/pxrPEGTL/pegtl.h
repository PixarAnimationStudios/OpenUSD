/*

Welcome to the Parsing Expression Grammar Template Library (PEGTL).
See https://github.com/taocpp/PEGTL/ for more information, documentation, etc.

The library is licensed as follows:

The MIT License (MIT)

Copyright (c) 2007-2020 Dr. Colin Hirsch and Daniel Frey

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// PIXAR:
// This header is not meant to be included in a .h file, to guard against
// conflicts if a program includes their own pegtl header and then transitively
// includes this header.
#ifdef PXR_PEGTL_H
#error This file should only be included once in any given source (.cpp) file.
#endif
#define PXR_PEGTL_H

// PIXAR: 
// Define TAO_PEGTL_NAMESPACE based on internal namespace to isolate
// it from other versions of USD/PEGTL in client code.
//
// This would typically be done by wrapping the contents of this header in
// PXR_NAMESPACE_OPEN_SCOPE and PXR_NAMESPACE_CLOSE_SCOPE, but that would
// require more invasive changes to this header since there are STL header
// includes scattered throughout.
#include "pxr/pxr.h"

#if PXR_USE_NAMESPACES
#define TAO_PEGTL_NAMESPACE PXR_INTERNAL_NS ## _pegtl
#else
#define TAO_PEGTL_NAMESPACE pxr_pegtl
#endif

#line 1 "amalgamated.hpp"
#line 1 "<built-in>"
#line 1 "<command-line>"
#line 1 "amalgamated.hpp"
#line 1 "tao/pegtl.hpp"
       
#line 1 "tao/pegtl.hpp"



#ifndef TAO_PEGTL_HPP
#define TAO_PEGTL_HPP

#line 1 "tao/pegtl/config.hpp"
       
#line 1 "tao/pegtl/config.hpp"



#ifndef TAO_PEGTL_CONFIG_HPP
#define TAO_PEGTL_CONFIG_HPP

// Compatibility, remove with 3.0.0
#ifdef TAOCPP_PEGTL_NAMESPACE
#define TAO_PEGTL_NAMESPACE TAOCPP_PEGTL_NAMESPACE
#endif

#ifndef TAO_PEGTL_NAMESPACE
#define TAO_PEGTL_NAMESPACE pegtl
#endif

// Enable some improvements to the readability of
// demangled type names under some circumstances.
// #define TAO_PEGTL_PRETTY_DEMANGLE

#endif
#line 8 "tao/pegtl.hpp"
#line 1 "tao/pegtl/version.hpp"
       
#line 1 "tao/pegtl/version.hpp"



#ifndef TAO_PEGTL_VERSION_HPP
#define TAO_PEGTL_VERSION_HPP

#define TAO_PEGTL_VERSION "2.8.3"

#define TAO_PEGTL_VERSION_MAJOR 2
#define TAO_PEGTL_VERSION_MINOR 8
#define TAO_PEGTL_VERSION_PATCH 3

// Compatibility, remove with 3.0.0
#define TAOCPP_PEGTL_VERSION TAO_PEGTL_VERSION
#define TAOCPP_PEGTL_VERSION_MAJOR TAO_PEGTL_VERSION_MAJOR
#define TAOCPP_PEGTL_VERSION_MINOR TAO_PEGTL_VERSION_MINOR
#define TAOCPP_PEGTL_VERSION_PATCH TAO_PEGTL_VERSION_PATCH

#endif
#line 9 "tao/pegtl.hpp"

#line 1 "tao/pegtl/parse.hpp"
       
#line 1 "tao/pegtl/parse.hpp"



#ifndef TAO_PEGTL_PARSE_HPP
#define TAO_PEGTL_PARSE_HPP

#include <cassert>

#line 1 "tao/pegtl/apply_mode.hpp"
       
#line 1 "tao/pegtl/apply_mode.hpp"



#ifndef TAO_PEGTL_APPLY_MODE_HPP
#define TAO_PEGTL_APPLY_MODE_HPP



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      enum class apply_mode : bool
      {
         action = true,
         nothing = false,

         // Compatibility, remove with 3.0.0
         ACTION = action,
         NOTHING = nothing
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/parse.hpp"

#line 1 "tao/pegtl/normal.hpp"
       
#line 1 "tao/pegtl/normal.hpp"



#ifndef TAO_PEGTL_NORMAL_HPP
#define TAO_PEGTL_NORMAL_HPP

#include <type_traits>
#include <utility>



#line 1 "tao/pegtl/match.hpp"
       
#line 1 "tao/pegtl/match.hpp"



#ifndef TAO_PEGTL_MATCH_HPP
#define TAO_PEGTL_MATCH_HPP

#include <type_traits>



#line 1 "tao/pegtl/nothing.hpp"
       
#line 1 "tao/pegtl/nothing.hpp"



#ifndef TAO_PEGTL_NOTHING_HPP
#define TAO_PEGTL_NOTHING_HPP



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename Rule >
      struct nothing
      {
      };

      using maybe_nothing = nothing< void >;

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/match.hpp"
#line 1 "tao/pegtl/require_apply.hpp"
       
#line 1 "tao/pegtl/require_apply.hpp"



#ifndef TAO_PEGTL_REQUIRE_APPLY_HPP
#define TAO_PEGTL_REQUIRE_APPLY_HPP



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct require_apply
      {};

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 13 "tao/pegtl/match.hpp"
#line 1 "tao/pegtl/require_apply0.hpp"
       
#line 1 "tao/pegtl/require_apply0.hpp"



#ifndef TAO_PEGTL_REQUIRE_APPLY0_HPP
#define TAO_PEGTL_REQUIRE_APPLY0_HPP



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct require_apply0
      {};

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 14 "tao/pegtl/match.hpp"
#line 1 "tao/pegtl/rewind_mode.hpp"
       
#line 1 "tao/pegtl/rewind_mode.hpp"



#ifndef TAO_PEGTL_REWIND_MODE_HPP
#define TAO_PEGTL_REWIND_MODE_HPP



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      enum class rewind_mode : char
      {
         active,
         required,
         dontcare,

         // Compatibility, remove with 3.0.0
         ACTIVE = active,
         REQUIRED = required,
         DONTCARE = dontcare
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 15 "tao/pegtl/match.hpp"

#line 1 "tao/pegtl/internal/dusel_mode.hpp"
       
#line 1 "tao/pegtl/internal/dusel_mode.hpp"



#ifndef TAO_PEGTL_INTERNAL_DUSEL_MODE_HPP
#define TAO_PEGTL_INTERNAL_DUSEL_MODE_HPP



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         enum class dusel_mode : char
         {
            nothing = 0,
            control = 1,
            control_and_apply_void = 2,
            control_and_apply_bool = 3,
            control_and_apply0_void = 4,
            control_and_apply0_bool = 5,
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 17 "tao/pegtl/match.hpp"
#line 1 "tao/pegtl/internal/duseltronik.hpp"
       
#line 1 "tao/pegtl/internal/duseltronik.hpp"



#ifndef TAO_PEGTL_INTERNAL_DUSELTRONIK_HPP
#define TAO_PEGTL_INTERNAL_DUSELTRONIK_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   dusel_mode = dusel_mode::nothing >
         struct duseltronik;

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control >
         struct duseltronik< Rule, A, M, Action, Control, dusel_mode::nothing >
         {
            template< typename Input, typename... States >
            static auto match( Input& in, States&&... st )
               -> decltype( Rule::template match< A, M, Action, Control >( in, st... ), true )
            {
               return Rule::template match< A, M, Action, Control >( in, st... );
            }

            // NOTE: The additional "int = 0" is a work-around for missing expression SFINAE in VS2015.

            template< typename Input, typename... States, int = 0 >
            static auto match( Input& in, States&&... /*unused*/ )
               -> decltype( Rule::match( in ), true )
            {
               return Rule::match( in );
            }
         };

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control >
         struct duseltronik< Rule, A, M, Action, Control, dusel_mode::control >
         {
            template< typename Input, typename... States >
            static bool match( Input& in, States&&... st )
            {
               Control< Rule >::start( static_cast< const Input& >( in ), st... );

               if( duseltronik< Rule, A, M, Action, Control, dusel_mode::nothing >::match( in, st... ) ) {
                  Control< Rule >::success( static_cast< const Input& >( in ), st... );
                  return true;
               }
               Control< Rule >::failure( static_cast< const Input& >( in ), st... );
               return false;
            }
         };

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control >
         struct duseltronik< Rule, A, M, Action, Control, dusel_mode::control_and_apply_void >
         {
            template< typename Input, typename... States >
            static bool match( Input& in, States&&... st )
            {
               auto m = in.template mark< rewind_mode::required >();

               Control< Rule >::start( static_cast< const Input& >( in ), st... );

               if( duseltronik< Rule, A, rewind_mode::active, Action, Control, dusel_mode::nothing >::match( in, st... ) ) {
                  Control< Rule >::template apply< Action >( m.iterator(), static_cast< const Input& >( in ), st... );
                  Control< Rule >::success( static_cast< const Input& >( in ), st... );
                  return m( true );
               }
               Control< Rule >::failure( static_cast< const Input& >( in ), st... );
               return false;
            }
         };

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control >
         struct duseltronik< Rule, A, M, Action, Control, dusel_mode::control_and_apply_bool >
         {
            template< typename Input, typename... States >
            static bool match( Input& in, States&&... st )
            {
               auto m = in.template mark< rewind_mode::required >();

               Control< Rule >::start( static_cast< const Input& >( in ), st... );

               if( duseltronik< Rule, A, rewind_mode::active, Action, Control, dusel_mode::nothing >::match( in, st... ) ) {
                  if( Control< Rule >::template apply< Action >( m.iterator(), static_cast< const Input& >( in ), st... ) ) {
                     Control< Rule >::success( static_cast< const Input& >( in ), st... );
                     return m( true );
                  }
               }
               Control< Rule >::failure( static_cast< const Input& >( in ), st... );
               return false;
            }
         };

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control >
         struct duseltronik< Rule, A, M, Action, Control, dusel_mode::control_and_apply0_void >
         {
            template< typename Input, typename... States >
            static bool match( Input& in, States&&... st )
            {
               Control< Rule >::start( static_cast< const Input& >( in ), st... );

               if( duseltronik< Rule, A, M, Action, Control, dusel_mode::nothing >::match( in, st... ) ) {
                  Control< Rule >::template apply0< Action >( static_cast< const Input& >( in ), st... );
                  Control< Rule >::success( static_cast< const Input& >( in ), st... );
                  return true;
               }
               Control< Rule >::failure( static_cast< const Input& >( in ), st... );
               return false;
            }
         };

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control >
         struct duseltronik< Rule, A, M, Action, Control, dusel_mode::control_and_apply0_bool >
         {
            template< typename Input, typename... States >
            static bool match( Input& in, States&&... st )
            {
               auto m = in.template mark< rewind_mode::required >();

               Control< Rule >::start( static_cast< const Input& >( in ), st... );

               if( duseltronik< Rule, A, rewind_mode::active, Action, Control, dusel_mode::nothing >::match( in, st... ) ) {
                  if( Control< Rule >::template apply0< Action >( static_cast< const Input& >( in ), st... ) ) {
                     Control< Rule >::success( static_cast< const Input& >( in ), st... );
                     return m( true );
                  }
               }
               Control< Rule >::failure( static_cast< const Input& >( in ), st... );
               return false;
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 18 "tao/pegtl/match.hpp"
#line 1 "tao/pegtl/internal/has_apply.hpp"
       
#line 1 "tao/pegtl/internal/has_apply.hpp"



#ifndef TAO_PEGTL_INTERNAL_HAS_APPLY_HPP
#define TAO_PEGTL_INTERNAL_HAS_APPLY_HPP

#include <type_traits>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename, typename, template< typename... > class, typename... >
         struct has_apply
            : std::false_type
         {};

         template< typename C, template< typename... > class Action, typename... S >
         struct has_apply< C, decltype( C::template apply< Action >( std::declval< S >()... ) ), Action, S... >
            : std::true_type
         {};

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 19 "tao/pegtl/match.hpp"
#line 1 "tao/pegtl/internal/has_apply0.hpp"
       
#line 1 "tao/pegtl/internal/has_apply0.hpp"



#ifndef TAO_PEGTL_INTERNAL_HAS_APPLY0_HPP
#define TAO_PEGTL_INTERNAL_HAS_APPLY0_HPP

#include <type_traits>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename, typename, template< typename... > class, typename... >
         struct has_apply0
            : std::false_type
         {};

         template< typename C, template< typename... > class Action, typename... S >
         struct has_apply0< C, decltype( C::template apply0< Action >( std::declval< S >()... ) ), Action, S... >
            : std::true_type
         {};

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 20 "tao/pegtl/match.hpp"
#line 1 "tao/pegtl/internal/if_missing.hpp"
       
#line 1 "tao/pegtl/internal/if_missing.hpp"



#ifndef TAO_PEGTL_INTERNAL_IF_MISSING_HPP
#define TAO_PEGTL_INTERNAL_IF_MISSING_HPP




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< bool >
         struct if_missing;

         template<>
         struct if_missing< true >
         {
            template< typename Control,
                      template< typename... >
                      class Action,
                      typename Input,
                      typename... States >
            static void apply( Input& in, States&&... st )
            {
               auto m = in.template mark< rewind_mode::required >();
               Control::template apply< Action >( m.iterator(), in, st... );
            }

            template< typename Control,
                      template< typename... >
                      class Action,
                      typename Input,
                      typename... States >
            static void apply0( Input& in, States&&... st )
            {
               Control::template apply0< Action >( in, st... );
            }
         };

         template<>
         struct if_missing< false >
         {
            template< typename Control,
                      template< typename... >
                      class Action,
                      typename Input,
                      typename... States >
            static void apply( Input& /*unused*/, States&&... /*unused*/ )
            {
            }

            template< typename Control,
                      template< typename... >
                      class Action,
                      typename Input,
                      typename... States >
            static void apply0( Input& /*unused*/, States&&... /*unused*/ )
            {
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 21 "tao/pegtl/match.hpp"
#line 1 "tao/pegtl/internal/skip_control.hpp"
       
#line 1 "tao/pegtl/internal/skip_control.hpp"



#ifndef TAO_PEGTL_INTERNAL_SKIP_CONTROL_HPP
#define TAO_PEGTL_INTERNAL_SKIP_CONTROL_HPP

#include <type_traits>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         // This class is a simple tagging mechanism.
         // By default, skip_control< Rule >::value
         // is 'false'. Each internal (!) rule that should
         // be hidden from the control and action class'
         // callbacks simply specializes skip_control<>
         // to return 'true' for the above expression.

         template< typename Rule >
         struct skip_control : std::false_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 22 "tao/pegtl/match.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename Rule,
                apply_mode A,
                rewind_mode M,
                template< typename... >
                class Action,
                template< typename... >
                class Control,
                typename Input,
                typename... States >
      bool match( Input& in, States&&... st )
      {
         constexpr bool enable_control = !internal::skip_control< Rule >::value;
         constexpr bool enable_action = enable_control && ( A == apply_mode::action );

         using iterator_t = typename Input::iterator_t;
         constexpr bool has_apply_void = enable_action && internal::has_apply< Control< Rule >, void, Action, const iterator_t&, const Input&, States... >::value;
         constexpr bool has_apply_bool = enable_action && internal::has_apply< Control< Rule >, bool, Action, const iterator_t&, const Input&, States... >::value;
         constexpr bool has_apply = has_apply_void || has_apply_bool;

         constexpr bool has_apply0_void = enable_action && internal::has_apply0< Control< Rule >, void, Action, const Input&, States... >::value;
         constexpr bool has_apply0_bool = enable_action && internal::has_apply0< Control< Rule >, bool, Action, const Input&, States... >::value;
         constexpr bool has_apply0 = has_apply0_void || has_apply0_bool;

         static_assert( !( has_apply && has_apply0 ), "both apply() and apply0() defined" );

         constexpr bool is_nothing = std::is_base_of< nothing< Rule >, Action< Rule > >::value;
         static_assert( !( has_apply && is_nothing ), "unexpected apply() defined" );
         static_assert( !( has_apply0 && is_nothing ), "unexpected apply0() defined" );

         internal::if_missing< !has_apply && std::is_base_of< require_apply, Action< Rule > >::value >::template apply< Control< Rule >, Action >( in, st... );
         internal::if_missing< !has_apply0 && std::is_base_of< require_apply0, Action< Rule > >::value >::template apply0< Control< Rule >, Action >( in, st... );

         constexpr bool validate_nothing = std::is_base_of< maybe_nothing, Action< void > >::value;
         constexpr bool is_maybe_nothing = std::is_base_of< maybe_nothing, Action< Rule > >::value;
         static_assert( !enable_action || !validate_nothing || is_nothing || is_maybe_nothing || has_apply || has_apply0, "either apply() or apply0() must be defined" );

         constexpr auto mode = static_cast< internal::dusel_mode >( enable_control + has_apply_void + 2 * has_apply_bool + 3 * has_apply0_void + 4 * has_apply0_bool );
         return internal::duseltronik< Rule, A, M, Action, Control, mode >::match( in, st... );
      }

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 13 "tao/pegtl/normal.hpp"
#line 1 "tao/pegtl/parse_error.hpp"
       
#line 1 "tao/pegtl/parse_error.hpp"



#ifndef TAO_PEGTL_PARSE_ERROR_HPP
#define TAO_PEGTL_PARSE_ERROR_HPP

#include <stdexcept>
#include <vector>


#line 1 "tao/pegtl/position.hpp"
       
#line 1 "tao/pegtl/position.hpp"



#ifndef TAO_PEGTL_POSITION_HPP
#define TAO_PEGTL_POSITION_HPP

#include <cstdlib>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>



#line 1 "tao/pegtl/internal/iterator.hpp"
       
#line 1 "tao/pegtl/internal/iterator.hpp"



#ifndef TAO_PEGTL_INTERNAL_ITERATOR_HPP
#define TAO_PEGTL_INTERNAL_ITERATOR_HPP

#include <cstdlib>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct iterator
         {
            iterator() noexcept = default;

            explicit iterator( const char* in_data ) noexcept
               : data( in_data )
            {
            }

            iterator( const char* in_data, const std::size_t in_byte, const std::size_t in_line, const std::size_t in_byte_in_line ) noexcept
               : data( in_data ),
                 byte( in_byte ),
                 line( in_line ),
                 byte_in_line( in_byte_in_line )
            {
            }

            iterator( const iterator& ) = default;
            iterator( iterator&& ) = default;

            ~iterator() = default;

            iterator& operator=( const iterator& ) = default;
            iterator& operator=( iterator&& ) = default;

            void reset() noexcept
            {
               *this = iterator();
            }

            const char* data = nullptr;

            std::size_t byte = 0;
            std::size_t line = 1;
            std::size_t byte_in_line = 0;
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 16 "tao/pegtl/position.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct position
      {
         template< typename T >
         position( const internal::iterator& in_iter, T&& in_source )
            : byte( in_iter.byte ),
              line( in_iter.line ),
              byte_in_line( in_iter.byte_in_line ),
              source( std::forward< T >( in_source ) )
         {
         }

         std::size_t byte;
         std::size_t line;
         std::size_t byte_in_line;
         std::string source;
      };

      inline std::ostream& operator<<( std::ostream& o, const position& p )
      {
         return o << p.source << ':' << p.line << ':' << p.byte_in_line << '(' << p.byte << ')';
      }

      inline std::string to_string( const position& p )
      {
         std::ostringstream o;
         o << p;
         return o.str();
      }

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/parse_error.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct parse_error
         : public std::runtime_error
      {
         parse_error( const std::string& msg, std::vector< position >&& in_positions )
            : std::runtime_error( msg ),
              positions( std::move( in_positions ) )
         {
         }

         template< typename Input >
         parse_error( const std::string& msg, const Input& in )
            : parse_error( msg, in.position() )
         {
         }

         parse_error( const std::string& msg, const position& pos )
            : std::runtime_error( to_string( pos ) + ": " + msg ),
              positions( 1, pos )
         {
         }

         parse_error( const std::string& msg, position&& pos )
            : std::runtime_error( to_string( pos ) + ": " + msg )
         {
            positions.emplace_back( std::move( pos ) );
         }

         std::vector< position > positions;
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 14 "tao/pegtl/normal.hpp"


#line 1 "tao/pegtl/internal/demangle.hpp"
       
#line 1 "tao/pegtl/internal/demangle.hpp"



#ifndef TAO_PEGTL_INTERNAL_DEMANGLE_HPP
#define TAO_PEGTL_INTERNAL_DEMANGLE_HPP

#include <string>
#include <typeinfo>



#if defined( __clang__ )
#if __has_feature( cxx_rtti )
#define TAO_PEGTL_RTTI_ENABLED
#endif
#elif defined( __GNUC__ )
#if defined( __GXX_RTTI )
#define TAO_PEGTL_RTTI_ENABLED
#endif
#elif defined( _MSC_VER )
#if defined( _CPPRTTI )
#define TAO_PEGTL_RTTI_ENABLED
#endif
#else
#define TAO_PEGTL_RTTI_ENABLED
#endif

#if !defined( TAO_PEGTL_RTTI_ENABLED )
#include <cassert>
#include <cstring>
#endif

#if defined( TAO_PEGTL_RTTI_ENABLED )
#if defined( __GLIBCXX__ )
#define TAO_PEGTL_USE_CXXABI_DEMANGLE
#elif defined( __has_include )
#if __has_include( <cxxabi.h> )
#define TAO_PEGTL_USE_CXXABI_DEMANGLE
#endif
#endif
#endif

#if defined( TAO_PEGTL_USE_CXXABI_DEMANGLE )
#line 1 "tao/pegtl/internal/demangle_cxxabi.hpp"
       
#line 1 "tao/pegtl/internal/demangle_cxxabi.hpp"



#ifndef TAO_PEGTL_INTERNAL_DEMANGLE_CXXABI_HPP
#define TAO_PEGTL_INTERNAL_DEMANGLE_CXXABI_HPP

#include <cstdlib>
#include <cxxabi.h>
#include <memory>
#include <string>



#line 1 "tao/pegtl/internal/demangle_sanitise.hpp"
       
#line 1 "tao/pegtl/internal/demangle_sanitise.hpp"



#ifndef TAO_PEGTL_INTERNAL_DEMANGLE_SANITISE_HPP
#define TAO_PEGTL_INTERNAL_DEMANGLE_SANITISE_HPP

#include <string>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         inline void demangle_sanitise_chars( std::string& s )
         {
            std::string::size_type p;
            while( ( p = s.find( "(char)" ) ) != std::string::npos ) {
               int c = 0;
               std::string::size_type q;
               for( q = p + 6; ( q < s.size() ) && ( s[ q ] >= '0' ) && ( s[ q ] <= '9' ); ++q ) {
                  c *= 10;
                  c += s[ q ] - '0';
               }
               if( c == '\'' ) {
                  s.replace( p, q - p, "'\\''" );
               }
               else if( c == '\\' ) {
                  s.replace( p, q - p, "'\\\\'" );
               }
               else if( ( c < 32 ) || ( c > 126 ) ) {
                  s.replace( p, 6, std::string() );
               }
               else {
                  s.replace( p, q - p, std::string( 1, '\'' ) + char( c ) + '\'' );
               }
            }
         }

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 15 "tao/pegtl/internal/demangle_cxxabi.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         inline std::string demangle( const char* symbol )
         {
            const std::unique_ptr< char, decltype( &std::free ) > demangled( abi::__cxa_demangle( symbol, nullptr, nullptr, nullptr ), &std::free );
            if( !demangled ) {
               return symbol;
            }
            std::string result( demangled.get() );
#ifdef TAO_PEGTL_PRETTY_DEMANGLE
            demangle_sanitise_chars( result ); // LCOV_EXCL_LINE
#endif
            return result;
         }

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 45 "tao/pegtl/internal/demangle.hpp"
#undef TAO_PEGTL_USE_CXXABI_DEMANGLE
#else
#line 1 "tao/pegtl/internal/demangle_nop.hpp"
       
#line 1 "tao/pegtl/internal/demangle_nop.hpp"



#ifndef TAO_PEGTL_INTERNAL_DEMANGLE_NOP_HPP
#define TAO_PEGTL_INTERNAL_DEMANGLE_NOP_HPP

#include <string>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         inline std::string demangle( const char* symbol )
         {
            return symbol;
         }

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 48 "tao/pegtl/internal/demangle.hpp"
#endif

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename T >
         std::string demangle()
         {
#if defined( TAO_PEGTL_RTTI_ENABLED )
            return demangle( typeid( T ).name() );
#else
            const char* start = nullptr;
            const char* stop = nullptr;
#if defined( __clang__ ) || defined( __GNUC__ )
            start = std::strchr( __PRETTY_FUNCTION__, '=' ) + 2;
            stop = std::strrchr( start, ';' );
#elif defined( _MSC_VER )
            start = std::strstr( __FUNCSIG__, "demangle<" ) + ( sizeof( "demangle<" ) - 1 );
            stop = std::strrchr( start, '>' );
#else
            static_assert( false, "expected to use rtti with this compiler" );
#endif
            assert( start != nullptr );
            assert( stop != nullptr );
            return { start, std::size_t( stop - start ) };
#endif
         }

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 17 "tao/pegtl/normal.hpp"
#line 1 "tao/pegtl/internal/has_match.hpp"
       
#line 1 "tao/pegtl/internal/has_match.hpp"



#ifndef TAO_PEGTL_INTERNAL_HAS_MATCH_HPP
#define TAO_PEGTL_INTERNAL_HAS_MATCH_HPP

#include <type_traits>
#include <utility>





namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename,
                   typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         struct has_match
            : std::false_type
         {};

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         struct has_match< decltype( Action< Rule >::template match< Rule, A, M, Action, Control >( std::declval< Input& >(), std::declval< States&& >()... ), void() ), Rule, A, M, Action, Control, Input, States... >
            : std::true_type
         {};

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 18 "tao/pegtl/normal.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename Rule >
      struct normal
      {
         template< typename Input, typename... States >
         static void start( const Input& /*unused*/, States&&... /*unused*/ ) noexcept
         {
         }

         template< typename Input, typename... States >
         static void success( const Input& /*unused*/, States&&... /*unused*/ ) noexcept
         {
         }

         template< typename Input, typename... States >
         static void failure( const Input& /*unused*/, States&&... /*unused*/ ) noexcept
         {
         }

         template< typename Input, typename... States >
         static void raise( const Input& in, States&&... /*unused*/ )
         {
            throw parse_error( "parse error matching " + internal::demangle< Rule >(), in );
         }

         template< template< typename... > class Action, typename Input, typename... States >
         static auto apply0( const Input& /*unused*/, States&&... st )
            -> decltype( Action< Rule >::apply0( st... ) )
         {
            return Action< Rule >::apply0( st... );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename... States >
         static auto apply( const Iterator& begin, const Input& in, States&&... st )
            -> decltype( Action< Rule >::apply( std::declval< typename Input::action_t >(), st... ) )
         {
            const typename Input::action_t action_input( begin, in );
            return Action< Rule >::apply( action_input, st... );
         }

         template< apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static auto match( Input& in, States&&... st )
            -> typename std::enable_if< internal::has_match< void, Rule, A, M, Action, Control, Input, States... >::value, bool >::type
         {
            return Action< Rule >::template match< Rule, A, M, Action, Control >( in, st... );
         }

         template< apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States,
                   int = 1 >
         static auto match( Input& in, States&&... st )
            -> typename std::enable_if< !internal::has_match< void, Rule, A, M, Action, Control, Input, States... >::value, bool >::type
         {
            return TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, st... );
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/parse.hpp"




#line 1 "tao/pegtl/internal/action_input.hpp"
       
#line 1 "tao/pegtl/internal/action_input.hpp"



#ifndef TAO_PEGTL_INTERNAL_ACTION_INPUT_HPP
#define TAO_PEGTL_INTERNAL_ACTION_INPUT_HPP

#include <cstddef>
#include <cstdint>
#include <string>






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         inline const char* begin_c_ptr( const char* p ) noexcept
         {
            return p;
         }

         inline const char* begin_c_ptr( const iterator& it ) noexcept
         {
            return it.data;
         }

         template< typename Input >
         class action_input
         {
         public:
            using input_t = Input;
            using iterator_t = typename Input::iterator_t;

            action_input( const iterator_t& in_begin, const Input& in_input ) noexcept
               : m_begin( in_begin ),
                 m_input( in_input )
            {
            }

            action_input( const action_input& ) = delete;
            action_input( action_input&& ) = delete;

            ~action_input() = default;

            action_input& operator=( const action_input& ) = delete;
            action_input& operator=( action_input&& ) = delete;

            const iterator_t& iterator() const noexcept
            {
               return m_begin;
            }

            const Input& input() const noexcept
            {
               return m_input;
            }

            const char* begin() const noexcept
            {
               return begin_c_ptr( iterator() );
            }

            const char* end() const noexcept
            {
               return input().current();
            }

            bool empty() const noexcept
            {
               return begin() == end();
            }

            std::size_t size() const noexcept
            {
               return std::size_t( end() - begin() );
            }

            std::string string() const
            {
               return std::string( begin(), end() );
            }

            char peek_char( const std::size_t offset = 0 ) const noexcept
            {
               return begin()[ offset ];
            }

            std::uint8_t peek_uint8( const std::size_t offset = 0 ) const noexcept
            {
               return static_cast< std::uint8_t >( peek_char( offset ) );
            }

            // Compatibility, remove with 3.0.0
            std::uint8_t peek_byte( const std::size_t offset = 0 ) const noexcept
            {
               return static_cast< std::uint8_t >( peek_char( offset ) );
            }

            TAO_PEGTL_NAMESPACE::position position() const
            {
               return input().position( iterator() ); // NOTE: Not efficient with lazy inputs.
            }

         protected:
            const iterator_t m_begin;
            const Input& m_input;
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 17 "tao/pegtl/parse.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename Rule,
                template< typename... > class Action = nothing,
                template< typename... > class Control = normal,
                apply_mode A = apply_mode::action,
                rewind_mode M = rewind_mode::required,
                typename Input,
                typename... States >
      bool parse( Input&& in, States&&... st )
      {
         return Control< Rule >::template match< A, M, Action, Control >( in, st... );
      }

      template< typename Rule,
                template< typename... > class Action = nothing,
                template< typename... > class Control = normal,
                apply_mode A = apply_mode::action,
                rewind_mode M = rewind_mode::required,
                typename Outer,
                typename Input,
                typename... States >
      bool parse_nested( const Outer& oi, Input&& in, States&&... st )
      {
         try {
            return parse< Rule, Action, Control, A, M >( in, st... );
         }
         catch( parse_error& e ) {
            e.positions.push_back( oi.position() );
            throw;
         }
      }

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 11 "tao/pegtl.hpp"

#line 1 "tao/pegtl/ascii.hpp"
       
#line 1 "tao/pegtl/ascii.hpp"



#ifndef TAO_PEGTL_ASCII_HPP
#define TAO_PEGTL_ASCII_HPP


#line 1 "tao/pegtl/eol.hpp"
       
#line 1 "tao/pegtl/eol.hpp"



#ifndef TAO_PEGTL_EOL_HPP
#define TAO_PEGTL_EOL_HPP



#line 1 "tao/pegtl/internal/eol.hpp"
       
#line 1 "tao/pegtl/internal/eol.hpp"



#ifndef TAO_PEGTL_INTERNAL_EOL_HPP
#define TAO_PEGTL_INTERNAL_EOL_HPP





#line 1 "tao/pegtl/internal/../analysis/generic.hpp"
       
#line 1 "tao/pegtl/internal/../analysis/generic.hpp"



#ifndef TAO_PEGTL_ANALYSIS_GENERIC_HPP
#define TAO_PEGTL_ANALYSIS_GENERIC_HPP



#line 1 "tao/pegtl/internal/../analysis/grammar_info.hpp"
       
#line 1 "tao/pegtl/internal/../analysis/grammar_info.hpp"



#ifndef TAO_PEGTL_ANALYSIS_GRAMMAR_INFO_HPP
#define TAO_PEGTL_ANALYSIS_GRAMMAR_INFO_HPP

#include <map>
#include <string>
#include <utility>




#line 1 "tao/pegtl/internal/../analysis/rule_info.hpp"
       
#line 1 "tao/pegtl/internal/../analysis/rule_info.hpp"



#ifndef TAO_PEGTL_ANALYSIS_RULE_INFO_HPP
#define TAO_PEGTL_ANALYSIS_RULE_INFO_HPP

#include <string>
#include <vector>



#line 1 "tao/pegtl/internal/../analysis/rule_type.hpp"
       
#line 1 "tao/pegtl/internal/../analysis/rule_type.hpp"



#ifndef TAO_PEGTL_ANALYSIS_RULE_TYPE_HPP
#define TAO_PEGTL_ANALYSIS_RULE_TYPE_HPP



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         enum class rule_type : char
         {
            any, // Consumption-on-success is always true; assumes bounded repetition of conjunction of sub-rules.
            opt, // Consumption-on-success not necessarily true; assumes bounded repetition of conjunction of sub-rules.
            seq, // Consumption-on-success depends on consumption of (non-zero bounded repetition of) conjunction of sub-rules.
            sor, // Consumption-on-success depends on consumption of (non-zero bounded repetition of) disjunction of sub-rules.

            // Compatibility, remove with 3.0.0
            ANY = any,
            OPT = opt,
            SEQ = seq,
            SOR = sor
         };

      } // namespace analysis

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 13 "tao/pegtl/internal/../analysis/rule_info.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         struct rule_info
         {
            explicit rule_info( const rule_type in_type ) noexcept
               : type( in_type )
            {
            }

            rule_type type;
            std::vector< std::string > rules;
         };

      } // namespace analysis

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 15 "tao/pegtl/internal/../analysis/grammar_info.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         struct grammar_info
         {
            using map_t = std::map< std::string, rule_info >;
            map_t map;

            template< typename Name >
            std::pair< map_t::iterator, bool > insert( const rule_type type )
            {
               return map.insert( map_t::value_type( internal::demangle< Name >(), rule_info( type ) ) );
            }
         };

      } // namespace analysis

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/../analysis/generic.hpp"


namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         template< rule_type Type, typename... Rules >
         struct generic
         {
            template< typename Name >
            static std::string insert( grammar_info& g )
            {
               const auto r = g.insert< Name >( Type );
               if( r.second ) {
#ifdef __cpp_fold_expressions
                  ( r.first->second.rules.push_back( Rules::analyze_t::template insert< Rules >( g ) ), ... );
#else
                  using swallow = bool[];
                  (void)swallow{ ( r.first->second.rules.push_back( Rules::analyze_t::template insert< Rules >( g ) ), true )..., true };
#endif
               }
               return r.first->first;
            }
         };

      } // namespace analysis

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/internal/eol.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct eol
         {
            using analyze_t = analysis::generic< analysis::rule_type::any >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( Input::eol_t::match( in ) ) )
            {
               return Input::eol_t::match( in ).first;
            }
         };

         template<>
         struct skip_control< eol > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/eol.hpp"

#line 1 "tao/pegtl/internal/cr_crlf_eol.hpp"
       
#line 1 "tao/pegtl/internal/cr_crlf_eol.hpp"



#ifndef TAO_PEGTL_INTERNAL_CR_CRLF_EOL_HPP
#define TAO_PEGTL_INTERNAL_CR_CRLF_EOL_HPP


#line 1 "tao/pegtl/internal/../eol_pair.hpp"
       
#line 1 "tao/pegtl/internal/../eol_pair.hpp"



#ifndef TAO_PEGTL_EOL_PAIR_HPP
#define TAO_PEGTL_EOL_PAIR_HPP

#include <cstddef>
#include <utility>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      using eol_pair = std::pair< bool, std::size_t >;

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 9 "tao/pegtl/internal/cr_crlf_eol.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct cr_crlf_eol
         {
            static constexpr int ch = '\r';

            template< typename Input >
            static eol_pair match( Input& in ) noexcept( noexcept( in.size( 2 ) ) )
            {
               eol_pair p = { false, in.size( 2 ) };
               if( p.second ) {
                  if( in.peek_char() == '\r' ) {
                     in.bump_to_next_line( 1 + ( ( p.second > 1 ) && ( in.peek_char( 1 ) == '\n' ) ) );
                     p.first = true;
                  }
               }
               return p;
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/eol.hpp"
#line 1 "tao/pegtl/internal/cr_eol.hpp"
       
#line 1 "tao/pegtl/internal/cr_eol.hpp"



#ifndef TAO_PEGTL_INTERNAL_CR_EOL_HPP
#define TAO_PEGTL_INTERNAL_CR_EOL_HPP




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct cr_eol
         {
            static constexpr int ch = '\r';

            template< typename Input >
            static eol_pair match( Input& in ) noexcept( noexcept( in.size( 1 ) ) )
            {
               eol_pair p = { false, in.size( 1 ) };
               if( p.second ) {
                  if( in.peek_char() == '\r' ) {
                     in.bump_to_next_line();
                     p.first = true;
                  }
               }
               return p;
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 13 "tao/pegtl/eol.hpp"
#line 1 "tao/pegtl/internal/crlf_eol.hpp"
       
#line 1 "tao/pegtl/internal/crlf_eol.hpp"



#ifndef TAO_PEGTL_INTERNAL_CRLF_EOL_HPP
#define TAO_PEGTL_INTERNAL_CRLF_EOL_HPP




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct crlf_eol
         {
            static constexpr int ch = '\n';

            template< typename Input >
            static eol_pair match( Input& in ) noexcept( noexcept( in.size( 2 ) ) )
            {
               eol_pair p = { false, in.size( 2 ) };
               if( p.second > 1 ) {
                  if( ( in.peek_char() == '\r' ) && ( in.peek_char( 1 ) == '\n' ) ) {
                     in.bump_to_next_line( 2 );
                     p.first = true;
                  }
               }
               return p;
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 14 "tao/pegtl/eol.hpp"
#line 1 "tao/pegtl/internal/lf_crlf_eol.hpp"
       
#line 1 "tao/pegtl/internal/lf_crlf_eol.hpp"



#ifndef TAO_PEGTL_INTERNAL_LF_CRLF_EOL_HPP
#define TAO_PEGTL_INTERNAL_LF_CRLF_EOL_HPP




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct lf_crlf_eol
         {
            static constexpr int ch = '\n';

            template< typename Input >
            static eol_pair match( Input& in ) noexcept( noexcept( in.size( 2 ) ) )
            {
               eol_pair p = { false, in.size( 2 ) };
               if( p.second ) {
                  const auto a = in.peek_char();
                  if( a == '\n' ) {
                     in.bump_to_next_line();
                     p.first = true;
                  }
                  else if( ( a == '\r' ) && ( p.second > 1 ) && ( in.peek_char( 1 ) == '\n' ) ) {
                     in.bump_to_next_line( 2 );
                     p.first = true;
                  }
               }
               return p;
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 15 "tao/pegtl/eol.hpp"
#line 1 "tao/pegtl/internal/lf_eol.hpp"
       
#line 1 "tao/pegtl/internal/lf_eol.hpp"



#ifndef TAO_PEGTL_INTERNAL_LF_EOL_HPP
#define TAO_PEGTL_INTERNAL_LF_EOL_HPP




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct lf_eol
         {
            static constexpr int ch = '\n';

            template< typename Input >
            static eol_pair match( Input& in ) noexcept( noexcept( in.size( 1 ) ) )
            {
               eol_pair p = { false, in.size( 1 ) };
               if( p.second ) {
                  if( in.peek_char() == '\n' ) {
                     in.bump_to_next_line();
                     p.first = true;
                  }
               }
               return p;
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 16 "tao/pegtl/eol.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      inline namespace ascii
      {
         // this is both a rule and a pseudo-namespace for eol::cr, ...
         struct eol : internal::eol
         {
            // clang-format off
            struct cr : internal::cr_eol {};
            struct cr_crlf : internal::cr_crlf_eol {};
            struct crlf : internal::crlf_eol {};
            struct lf : internal::lf_eol {};
            struct lf_crlf : internal::lf_crlf_eol {};
            // clang-format on
         };

      } // namespace ascii

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 9 "tao/pegtl/ascii.hpp"

#line 1 "tao/pegtl/internal/always_false.hpp"
       
#line 1 "tao/pegtl/internal/always_false.hpp"



#ifndef TAO_PEGTL_INTERNAL_ALWAYS_FALSE_HPP
#define TAO_PEGTL_INTERNAL_ALWAYS_FALSE_HPP



#include <type_traits>

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename... >
         struct always_false
            : std::false_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 11 "tao/pegtl/ascii.hpp"
#line 1 "tao/pegtl/internal/result_on_found.hpp"
       
#line 1 "tao/pegtl/internal/result_on_found.hpp"



#ifndef TAO_PEGTL_INTERNAL_RESULT_ON_FOUND_HPP
#define TAO_PEGTL_INTERNAL_RESULT_ON_FOUND_HPP



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         enum class result_on_found : bool
         {
            success = true,
            failure = false,
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/ascii.hpp"
#line 1 "tao/pegtl/internal/rules.hpp"
       
#line 1 "tao/pegtl/internal/rules.hpp"



#ifndef TAO_PEGTL_INTERNAL_RULES_HPP
#define TAO_PEGTL_INTERNAL_RULES_HPP

#line 1 "tao/pegtl/internal/action.hpp"
       
#line 1 "tao/pegtl/internal/action.hpp"



#ifndef TAO_PEGTL_INTERNAL_ACTION_HPP
#define TAO_PEGTL_INTERNAL_ACTION_HPP



#line 1 "tao/pegtl/internal/seq.hpp"
       
#line 1 "tao/pegtl/internal/seq.hpp"



#ifndef TAO_PEGTL_INTERNAL_SEQ_HPP
#define TAO_PEGTL_INTERNAL_SEQ_HPP




#line 1 "tao/pegtl/internal/trivial.hpp"
       
#line 1 "tao/pegtl/internal/trivial.hpp"



#ifndef TAO_PEGTL_INTERNAL_TRIVIAL_HPP
#define TAO_PEGTL_INTERNAL_TRIVIAL_HPP





#line 1 "tao/pegtl/internal/../analysis/counted.hpp"
       
#line 1 "tao/pegtl/internal/../analysis/counted.hpp"



#ifndef TAO_PEGTL_ANALYSIS_COUNTED_HPP
#define TAO_PEGTL_ANALYSIS_COUNTED_HPP



#include <cstddef>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         template< rule_type Type, std::size_t Count, typename... Rules >
         struct counted
            : generic< ( Count != 0 ) ? Type : rule_type::opt, Rules... >
         {
         };

      } // namespace analysis

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/internal/trivial.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< bool Result >
         struct trivial
         {
            using analyze_t = analysis::counted< analysis::rule_type::any, unsigned( !Result ) >;

            template< typename Input >
            static bool match( Input& /*unused*/ ) noexcept
            {
               return Result;
            }
         };

         template< bool Result >
         struct skip_control< trivial< Result > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 11 "tao/pegtl/internal/seq.hpp"






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename... Rules >
         struct seq;

         template<>
         struct seq<>
            : trivial< true >
         {
         };

         template< typename Rule >
         struct seq< Rule >
         {
            using analyze_t = typename Rule::analyze_t;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return Control< Rule >::template match< A, M, Action, Control >( in, st... );
            }
         };

         template< typename... Rules >
         struct seq
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, Rules... >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               auto m = in.template mark< M >();
               using m_t = decltype( m );
#ifdef __cpp_fold_expressions
               return m( ( Control< Rules >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) && ... ) );
#else
               bool result = true;
               using swallow = bool[];
               (void)swallow{ result = result && Control< Rules >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... )... };
               return m( result );
#endif
            }

         }; // namespace internal

         template< typename... Rules >
         struct skip_control< seq< Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/action.hpp"







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< template< typename... > class Action, typename... Rules >
         struct action
            : action< Action, seq< Rules... > >
         {
         };

         template< template< typename... > class Action, typename Rule >
         struct action< Action, Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, Rule >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return Control< Rule >::template match< A, M, Action, Control >( in, st... );
            }
         };

         template< template< typename... > class Action, typename... Rules >
         struct skip_control< action< Action, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 8 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/alnum.hpp"
       
#line 1 "tao/pegtl/internal/alnum.hpp"



#ifndef TAO_PEGTL_INTERNAL_ALNUM_HPP
#define TAO_PEGTL_INTERNAL_ALNUM_HPP



#line 1 "tao/pegtl/internal/peek_char.hpp"
       
#line 1 "tao/pegtl/internal/peek_char.hpp"



#ifndef TAO_PEGTL_INTERNAL_PEEK_CHAR_HPP
#define TAO_PEGTL_INTERNAL_PEEK_CHAR_HPP

#include <cstddef>



#line 1 "tao/pegtl/internal/input_pair.hpp"
       
#line 1 "tao/pegtl/internal/input_pair.hpp"



#ifndef TAO_PEGTL_INTERNAL_INPUT_PAIR_HPP
#define TAO_PEGTL_INTERNAL_INPUT_PAIR_HPP

#include <cstdint>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Data >
         struct input_pair
         {
            Data data;
            std::uint8_t size;

            using data_t = Data;

            explicit operator bool() const noexcept
            {
               return size > 0;
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/internal/peek_char.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct peek_char
         {
            using data_t = char;
            using pair_t = input_pair< char >;

            template< typename Input >
            static pair_t peek( Input& in ) noexcept( noexcept( in.empty() ) )
            {
               if( in.empty() ) {
                  return { 0, 0 };
               }
               return { in.peek_char(), 1 };
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/alnum.hpp"
#line 1 "tao/pegtl/internal/ranges.hpp"
       
#line 1 "tao/pegtl/internal/ranges.hpp"



#ifndef TAO_PEGTL_INTERNAL_RANGES_HPP
#define TAO_PEGTL_INTERNAL_RANGES_HPP



#line 1 "tao/pegtl/internal/bump_help.hpp"
       
#line 1 "tao/pegtl/internal/bump_help.hpp"



#ifndef TAO_PEGTL_INTERNAL_BUMP_HELP_HPP
#define TAO_PEGTL_INTERNAL_BUMP_HELP_HPP

#include <cstddef>
#include <type_traits>





namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< bool >
         struct bump_impl;

         template<>
         struct bump_impl< true >
         {
            template< typename Input >
            static void bump( Input& in, const std::size_t count ) noexcept
            {
               in.bump( count );
            }
         };

         template<>
         struct bump_impl< false >
         {
            template< typename Input >
            static void bump( Input& in, const std::size_t count ) noexcept
            {
               in.bump_in_this_line( count );
            }
         };

         template< bool... >
         struct bool_list
         {
         };

         template< bool... Bs >
         using bool_and = std::is_same< bool_list< Bs..., true >, bool_list< true, Bs... > >;

         template< result_on_found R, typename Input, typename Char, Char... Cs >
         void bump_help( Input& in, const std::size_t count ) noexcept
         {
            bump_impl< bool_and< ( Cs != Input::eol_t::ch )... >::value != bool( R ) >::bump( in, count );
         }

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/ranges.hpp"
#line 1 "tao/pegtl/internal/range.hpp"
       
#line 1 "tao/pegtl/internal/range.hpp"



#ifndef TAO_PEGTL_INTERNAL_RANGE_HPP
#define TAO_PEGTL_INTERNAL_RANGE_HPP
#line 15 "tao/pegtl/internal/range.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< result_on_found R, typename Peek, typename Peek::data_t Lo, typename Peek::data_t Hi >
         struct range
         {
            static_assert( Lo <= Hi, "invalid range detected" );

            using analyze_t = analysis::generic< analysis::rule_type::any >;

            template< int Eol >
            struct can_match_eol
            {
               static constexpr bool value = ( ( ( Lo <= Eol ) && ( Eol <= Hi ) ) == bool( R ) );
            };

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( Peek::peek( in ) ) )
            {
               if( const auto t = Peek::peek( in ) ) {
                  if( ( ( Lo <= t.data ) && ( t.data <= Hi ) ) == bool( R ) ) {
                     bump_impl< can_match_eol< Input::eol_t::ch >::value >::bump( in, t.size );
                     return true;
                  }
               }
               return false;
            }
         };

         template< result_on_found R, typename Peek, typename Peek::data_t Lo, typename Peek::data_t Hi >
         struct skip_control< range< R, Peek, Lo, Hi > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 11 "tao/pegtl/internal/ranges.hpp"




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< int Eol, typename Char, Char... Cs >
         struct ranges_impl;

         template< int Eol, typename Char >
         struct ranges_impl< Eol, Char >
         {
            static constexpr bool can_match_eol = false;

            static bool match( const Char /*unused*/ ) noexcept
            {
               return false;
            }
         };

         template< int Eol, typename Char, Char Eq >
         struct ranges_impl< Eol, Char, Eq >
         {
            static constexpr bool can_match_eol = ( Eq == Eol );

            static bool match( const Char c ) noexcept
            {
               return c == Eq;
            }
         };

         template< int Eol, typename Char, Char Lo, Char Hi, Char... Cs >
         struct ranges_impl< Eol, Char, Lo, Hi, Cs... >
         {
            static_assert( Lo <= Hi, "invalid range detected" );

            static constexpr bool can_match_eol = ( ( ( Lo <= Eol ) && ( Eol <= Hi ) ) || ranges_impl< Eol, Char, Cs... >::can_match_eol );

            static bool match( const Char c ) noexcept
            {
               return ( ( Lo <= c ) && ( c <= Hi ) ) || ranges_impl< Eol, Char, Cs... >::match( c );
            }
         };

         template< typename Peek, typename Peek::data_t... Cs >
         struct ranges
         {
            using analyze_t = analysis::generic< analysis::rule_type::any >;

            template< int Eol >
            struct can_match_eol
            {
               static constexpr bool value = ranges_impl< Eol, typename Peek::data_t, Cs... >::can_match_eol;
            };

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( Peek::peek( in ) ) )
            {
               if( const auto t = Peek::peek( in ) ) {
                  if( ranges_impl< Input::eol_t::ch, typename Peek::data_t, Cs... >::match( t.data ) ) {
                     bump_impl< can_match_eol< Input::eol_t::ch >::value >::bump( in, t.size );
                     return true;
                  }
               }
               return false;
            }
         };

         template< typename Peek, typename Peek::data_t Lo, typename Peek::data_t Hi >
         struct ranges< Peek, Lo, Hi >
            : range< result_on_found::success, Peek, Lo, Hi >
         {
         };

         template< typename Peek, typename Peek::data_t... Cs >
         struct skip_control< ranges< Peek, Cs... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 11 "tao/pegtl/internal/alnum.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         using alnum = ranges< peek_char, 'a', 'z', 'A', 'Z', '0', '9' >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 9 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/alpha.hpp"
       
#line 1 "tao/pegtl/internal/alpha.hpp"



#ifndef TAO_PEGTL_INTERNAL_ALPHA_HPP
#define TAO_PEGTL_INTERNAL_ALPHA_HPP






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         using alpha = ranges< peek_char, 'a', 'z', 'A', 'Z' >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/any.hpp"
       
#line 1 "tao/pegtl/internal/any.hpp"



#ifndef TAO_PEGTL_INTERNAL_ANY_HPP
#define TAO_PEGTL_INTERNAL_ANY_HPP
#line 14 "tao/pegtl/internal/any.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Peek >
         struct any;

         template<>
         struct any< peek_char >
         {
            using analyze_t = analysis::generic< analysis::rule_type::any >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( in.empty() ) )
            {
               if( !in.empty() ) {
                  in.bump();
                  return true;
               }
               return false;
            }
         };

         template< typename Peek >
         struct any
         {
            using analyze_t = analysis::generic< analysis::rule_type::any >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( Peek::peek( in ) ) )
            {
               if( const auto t = Peek::peek( in ) ) {
                  in.bump( t.size );
                  return true;
               }
               return false;
            }
         };

         template< typename Peek >
         struct skip_control< any< Peek > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 11 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/apply.hpp"
       
#line 1 "tao/pegtl/internal/apply.hpp"



#ifndef TAO_PEGTL_INTERNAL_APPLY_HPP
#define TAO_PEGTL_INTERNAL_APPLY_HPP



#line 1 "tao/pegtl/internal/apply_single.hpp"
       
#line 1 "tao/pegtl/internal/apply_single.hpp"



#ifndef TAO_PEGTL_INTERNAL_APPLY_SINGLE_HPP
#define TAO_PEGTL_INTERNAL_APPLY_SINGLE_HPP



#include <type_traits>

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Action >
         struct apply_single
         {
            template< typename Input, typename... States >
            static auto match( const Input& i2, States&&... st )
               -> typename std::enable_if< std::is_same< decltype( Action::apply( i2, st... ) ), void >::value, bool >::type
            {
               Action::apply( i2, st... );
               return true;
            }

            template< typename Input, typename... States >
            static auto match( const Input& i2, States&&... st )
               -> typename std::enable_if< std::is_same< decltype( Action::apply( i2, st... ) ), bool >::value, bool >::type
            {
               return Action::apply( i2, st... );
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/apply.hpp"






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< apply_mode A, typename... Actions >
         struct apply_impl;

         template<>
         struct apply_impl< apply_mode::action >
         {
            template< typename Input, typename... States >
            static bool match( Input& /*unused*/, States&&... /*unused*/ )
            {
               return true;
            }
         };

         template< typename... Actions >
         struct apply_impl< apply_mode::action, Actions... >
         {
            template< typename Input, typename... States >
            static bool match( Input& in, States&&... st )
            {
               using action_t = typename Input::action_t;
               const action_t i2( in.iterator(), in ); // No data -- range is from begin to begin.
#ifdef __cpp_fold_expressions
               return ( apply_single< Actions >::match( i2, st... ) && ... );
#else
               bool result = true;
               using swallow = bool[];
               (void)swallow{ result = result && apply_single< Actions >::match( i2, st... )... };
               return result;
#endif
            }
         };

         template< typename... Actions >
         struct apply_impl< apply_mode::nothing, Actions... >
         {
            template< typename Input, typename... States >
            static bool match( Input& /*unused*/, States&&... /*unused*/ )
            {
               return true;
            }
         };

         template< typename... Actions >
         struct apply
         {
            using analyze_t = analysis::counted< analysis::rule_type::any, 0 >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return apply_impl< A, Actions... >::match( in, st... );
            }
         };

         template< typename... Actions >
         struct skip_control< apply< Actions... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/apply0.hpp"
       
#line 1 "tao/pegtl/internal/apply0.hpp"



#ifndef TAO_PEGTL_INTERNAL_APPLY0_HPP
#define TAO_PEGTL_INTERNAL_APPLY0_HPP



#line 1 "tao/pegtl/internal/apply0_single.hpp"
       
#line 1 "tao/pegtl/internal/apply0_single.hpp"



#ifndef TAO_PEGTL_INTERNAL_APPLY0_SINGLE_HPP
#define TAO_PEGTL_INTERNAL_APPLY0_SINGLE_HPP



#include <type_traits>

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Action >
         struct apply0_single
         {
            template< typename... States >
            static auto match( States&&... st )
               -> typename std::enable_if< std::is_same< decltype( Action::apply0( st... ) ), void >::value, bool >::type
            {
               Action::apply0( st... );
               return true;
            }

            template< typename... States >
            static auto match( States&&... st )
               -> typename std::enable_if< std::is_same< decltype( Action::apply0( st... ) ), bool >::value, bool >::type
            {
               return Action::apply0( st... );
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/apply0.hpp"






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< apply_mode A, typename... Actions >
         struct apply0_impl;

         template<>
         struct apply0_impl< apply_mode::action >
         {
            template< typename... States >
            static bool match( States&&... /*unused*/ ) noexcept
            {
               return true;
            }
         };

         template< typename... Actions >
         struct apply0_impl< apply_mode::action, Actions... >
         {
            template< typename... States >
            static bool match( States&&... st )
            {
#ifdef __cpp_fold_expressions
               return ( apply0_single< Actions >::match( st... ) && ... );
#else
               bool result = true;
               using swallow = bool[];
               (void)swallow{ result = result && apply0_single< Actions >::match( st... )... };
               return result;
#endif
            }
         };

         template< typename... Actions >
         struct apply0_impl< apply_mode::nothing, Actions... >
         {
            template< typename... States >
            static bool match( States&&... /*unused*/ ) noexcept
            {
               return true;
            }
         };

         template< typename... Actions >
         struct apply0
         {
            using analyze_t = analysis::counted< analysis::rule_type::any, 0 >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& /*unused*/, States&&... st )
            {
               return apply0_impl< A, Actions... >::match( st... );
            }
         };

         template< typename... Actions >
         struct skip_control< apply0< Actions... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 13 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/at.hpp"
       
#line 1 "tao/pegtl/internal/at.hpp"



#ifndef TAO_PEGTL_INTERNAL_AT_HPP
#define TAO_PEGTL_INTERNAL_AT_HPP
#line 18 "tao/pegtl/internal/at.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename... Rules >
         struct at
            : at< seq< Rules... > >
         {
         };

         template<>
         struct at<>
            : trivial< true >
         {
         };

         template< typename Rule >
         struct at< Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt, Rule >;

            template< apply_mode,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               const auto m = in.template mark< rewind_mode::required >();
               return Control< Rule >::template match< apply_mode::nothing, rewind_mode::active, Action, Control >( in, st... );
            }
         };

         template< typename... Rules >
         struct skip_control< at< Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 14 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/bof.hpp"
       
#line 1 "tao/pegtl/internal/bof.hpp"



#ifndef TAO_PEGTL_INTERNAL_BOF_HPP
#define TAO_PEGTL_INTERNAL_BOF_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct bof
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt >;

            template< typename Input >
            static bool match( Input& in ) noexcept
            {
               return in.byte() == 0;
            }
         };

         template<>
         struct skip_control< bof > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 15 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/bol.hpp"
       
#line 1 "tao/pegtl/internal/bol.hpp"



#ifndef TAO_PEGTL_INTERNAL_BOL_HPP
#define TAO_PEGTL_INTERNAL_BOL_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct bol
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt >;

            template< typename Input >
            static bool match( Input& in ) noexcept
            {
               return in.byte_in_line() == 0;
            }
         };

         template<>
         struct skip_control< bol > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 16 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/bytes.hpp"
       
#line 1 "tao/pegtl/internal/bytes.hpp"



#ifndef TAO_PEGTL_INTERNAL_BYTES_HPP
#define TAO_PEGTL_INTERNAL_BYTES_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< unsigned Num >
         struct bytes
         {
            using analyze_t = analysis::counted< analysis::rule_type::any, Num >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( in.size( 0 ) ) )
            {
               if( in.size( Num ) >= Num ) {
                  in.bump( Num );
                  return true;
               }
               return false;
            }
         };

         template< unsigned Num >
         struct skip_control< bytes< Num > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 17 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/control.hpp"
       
#line 1 "tao/pegtl/internal/control.hpp"



#ifndef TAO_PEGTL_INTERNAL_CONTROL_HPP
#define TAO_PEGTL_INTERNAL_CONTROL_HPP
#line 17 "tao/pegtl/internal/control.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< template< typename... > class Control, typename... Rules >
         struct control
            : control< Control, seq< Rules... > >
         {
         };

         template< template< typename... > class Control, typename Rule >
         struct control< Control, Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, Rule >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return Control< Rule >::template match< A, M, Action, Control >( in, st... );
            }
         };

         template< template< typename... > class Control, typename... Rules >
         struct skip_control< control< Control, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 18 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/disable.hpp"
       
#line 1 "tao/pegtl/internal/disable.hpp"



#ifndef TAO_PEGTL_INTERNAL_DISABLE_HPP
#define TAO_PEGTL_INTERNAL_DISABLE_HPP
#line 17 "tao/pegtl/internal/disable.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename... Rules >
         struct disable
            : disable< seq< Rules... > >
         {
         };

         template< typename Rule >
         struct disable< Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, Rule >;

            template< apply_mode,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return Control< Rule >::template match< apply_mode::nothing, M, Action, Control >( in, st... );
            }
         };

         template< typename... Rules >
         struct skip_control< disable< Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 19 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/discard.hpp"
       
#line 1 "tao/pegtl/internal/discard.hpp"



#ifndef TAO_PEGTL_INTERNAL_DISCARD_HPP
#define TAO_PEGTL_INTERNAL_DISCARD_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct discard
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt >;

            template< typename Input >
            static bool match( Input& in ) noexcept
            {
               static_assert( noexcept( in.discard() ), "an input's discard()-method must be noexcept" );
               in.discard();
               return true;
            }
         };

         template<>
         struct skip_control< discard > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 20 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/enable.hpp"
       
#line 1 "tao/pegtl/internal/enable.hpp"



#ifndef TAO_PEGTL_INTERNAL_ENABLE_HPP
#define TAO_PEGTL_INTERNAL_ENABLE_HPP
#line 17 "tao/pegtl/internal/enable.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename... Rules >
         struct enable
            : enable< seq< Rules... > >
         {
         };

         template< typename Rule >
         struct enable< Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, Rule >;

            template< apply_mode,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return Control< Rule >::template match< apply_mode::action, M, Action, Control >( in, st... );
            }
         };

         template< typename... Rules >
         struct skip_control< enable< Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 21 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/eof.hpp"
       
#line 1 "tao/pegtl/internal/eof.hpp"



#ifndef TAO_PEGTL_INTERNAL_EOF_HPP
#define TAO_PEGTL_INTERNAL_EOF_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct eof
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( in.empty() ) )
            {
               return in.empty();
            }
         };

         template<>
         struct skip_control< eof > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 22 "tao/pegtl/internal/rules.hpp"

#line 1 "tao/pegtl/internal/eolf.hpp"
       
#line 1 "tao/pegtl/internal/eolf.hpp"



#ifndef TAO_PEGTL_INTERNAL_EOLF_HPP
#define TAO_PEGTL_INTERNAL_EOLF_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct eolf
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( Input::eol_t::match( in ) ) )
            {
               const auto p = Input::eol_t::match( in );
               return p.first || ( !p.second );
            }
         };

         template<>
         struct skip_control< eolf > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 24 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/identifier.hpp"
       
#line 1 "tao/pegtl/internal/identifier.hpp"



#ifndef TAO_PEGTL_INTERNAL_IDENTIFIER_HPP
#define TAO_PEGTL_INTERNAL_IDENTIFIER_HPP






#line 1 "tao/pegtl/internal/star.hpp"
       
#line 1 "tao/pegtl/internal/star.hpp"



#ifndef TAO_PEGTL_INTERNAL_STAR_HPP
#define TAO_PEGTL_INTERNAL_STAR_HPP

#include <type_traits>
#line 19 "tao/pegtl/internal/star.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Rule, typename... Rules >
         struct star
            : star< seq< Rule, Rules... > >
         {};

         template< typename Rule >
         struct star< Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt, Rule, star >;

            template< apply_mode A,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               while( Control< Rule >::template match< A, rewind_mode::required, Action, Control >( in, st... ) ) {
               }
               return true;
            }
         };

         template< typename Rule, typename... Rules >
         struct skip_control< star< Rule, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 13 "tao/pegtl/internal/identifier.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         using identifier_first = ranges< peek_char, 'a', 'z', 'A', 'Z', '_' >;
         using identifier_other = ranges< peek_char, 'a', 'z', 'A', 'Z', '0', '9', '_' >;
         using identifier = seq< identifier_first, star< identifier_other > >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 25 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/if_apply.hpp"
       
#line 1 "tao/pegtl/internal/if_apply.hpp"



#ifndef TAO_PEGTL_INTERNAL_IF_APPLY_HPP
#define TAO_PEGTL_INTERNAL_IF_APPLY_HPP
#line 16 "tao/pegtl/internal/if_apply.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< apply_mode A, typename Rule, typename... Actions >
         struct if_apply_impl;

         template< typename Rule >
         struct if_apply_impl< apply_mode::action, Rule >
         {
            template< rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return Control< Rule >::template match< apply_mode::action, M, Action, Control >( in, st... );
            }
         };

         template< typename Rule, typename... Actions >
         struct if_apply_impl< apply_mode::action, Rule, Actions... >
         {
            template< rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               using action_t = typename Input::action_t;

               auto m = in.template mark< rewind_mode::required >();

               if( Control< Rule >::template match< apply_mode::action, rewind_mode::active, Action, Control >( in, st... ) ) {
                  const action_t i2( m.iterator(), in );
#ifdef __cpp_fold_expressions
                  return m( ( apply_single< Actions >::match( i2, st... ) && ... ) );
#else
                  bool result = true;
                  using swallow = bool[];
                  (void)swallow{ result = result && apply_single< Actions >::match( i2, st... )... };
                  return m( result );
#endif
               }
               return false;
            }
         };

         template< typename Rule, typename... Actions >
         struct if_apply_impl< apply_mode::nothing, Rule, Actions... >
         {
            template< rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return Control< Rule >::template match< apply_mode::nothing, M, Action, Control >( in, st... );
            }
         };

         template< typename Rule, typename... Actions >
         struct if_apply
         {
            using analyze_t = typename Rule::analyze_t;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return if_apply_impl< A, Rule, Actions... >::template match< M, Action, Control >( in, st... );
            }
         };

         template< typename Rule, typename... Actions >
         struct skip_control< if_apply< Rule, Actions... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 26 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/if_must.hpp"
       
#line 1 "tao/pegtl/internal/if_must.hpp"



#ifndef TAO_PEGTL_INTERNAL_IF_MUST_HPP
#define TAO_PEGTL_INTERNAL_IF_MUST_HPP



#line 1 "tao/pegtl/internal/must.hpp"
       
#line 1 "tao/pegtl/internal/must.hpp"



#ifndef TAO_PEGTL_INTERNAL_MUST_HPP
#define TAO_PEGTL_INTERNAL_MUST_HPP



#line 1 "tao/pegtl/internal/raise.hpp"
       
#line 1 "tao/pegtl/internal/raise.hpp"



#ifndef TAO_PEGTL_INTERNAL_RAISE_HPP
#define TAO_PEGTL_INTERNAL_RAISE_HPP

#include <cstdlib>
#include <stdexcept>
#include <type_traits>
#line 19 "tao/pegtl/internal/raise.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename T >
         struct raise
         {
            using analyze_t = analysis::generic< analysis::rule_type::any >;

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4702 )
#endif
            template< apply_mode,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               Control< T >::raise( static_cast< const Input& >( in ), st... );
               throw std::logic_error( "code should be unreachable: Control< T >::raise() did not throw an exception" ); // NOLINT, LCOV_EXCL_LINE
#ifdef _MSC_VER
#pragma warning( pop )
#endif
            }
         };

         template< typename T >
         struct skip_control< raise< T > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/must.hpp"
#line 18 "tao/pegtl/internal/must.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         // The general case applies must<> to each of the
         // rules in the 'Rules' parameter pack individually.

         template< typename... Rules >
         struct must
            : seq< must< Rules >... >
         {
         };

         // While in theory the implementation for a single rule could
         // be simplified to must< Rule > = sor< Rule, raise< Rule > >, this
         // would result in some unnecessary run-time overhead.

         template< typename Rule >
         struct must< Rule >
         {
            using analyze_t = typename Rule::analyze_t;

            template< apply_mode A,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               if( !Control< Rule >::template match< A, rewind_mode::dontcare, Action, Control >( in, st... ) ) {
                  raise< Rule >::template match< A, rewind_mode::dontcare, Action, Control >( in, st... );
               }
               return true;
            }
         };

         template< typename... Rules >
         struct skip_control< must< Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/if_must.hpp"
#line 18 "tao/pegtl/internal/if_must.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< bool Default, typename Cond, typename... Rules >
         struct if_must
         {
            using analyze_t = analysis::counted< analysis::rule_type::seq, Default ? 0 : 1, Cond, must< Rules... > >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               if( Control< Cond >::template match< A, M, Action, Control >( in, st... ) ) {
                  Control< must< Rules... > >::template match< A, M, Action, Control >( in, st... );
                  return true;
               }
               return Default;
            }
         };

         template< bool Default, typename Cond, typename... Rules >
         struct skip_control< if_must< Default, Cond, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 27 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/if_must_else.hpp"
       
#line 1 "tao/pegtl/internal/if_must_else.hpp"



#ifndef TAO_PEGTL_INTERNAL_IF_MUST_ELSE_HPP
#define TAO_PEGTL_INTERNAL_IF_MUST_ELSE_HPP



#line 1 "tao/pegtl/internal/if_then_else.hpp"
       
#line 1 "tao/pegtl/internal/if_then_else.hpp"



#ifndef TAO_PEGTL_INTERNAL_IF_THEN_ELSE_HPP
#define TAO_PEGTL_INTERNAL_IF_THEN_ELSE_HPP



#line 1 "tao/pegtl/internal/not_at.hpp"
       
#line 1 "tao/pegtl/internal/not_at.hpp"



#ifndef TAO_PEGTL_INTERNAL_NOT_AT_HPP
#define TAO_PEGTL_INTERNAL_NOT_AT_HPP
#line 18 "tao/pegtl/internal/not_at.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename... Rules >
         struct not_at
            : not_at< seq< Rules... > >
         {
         };

         template<>
         struct not_at<>
            : trivial< false >
         {
         };

         template< typename Rule >
         struct not_at< Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt, Rule >;

            template< apply_mode,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               const auto m = in.template mark< rewind_mode::required >();
               return !Control< Rule >::template match< apply_mode::nothing, rewind_mode::active, Action, Control >( in, st... );
            }
         };

         template< typename... Rules >
         struct skip_control< not_at< Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/if_then_else.hpp"


#line 1 "tao/pegtl/internal/sor.hpp"
       
#line 1 "tao/pegtl/internal/sor.hpp"



#ifndef TAO_PEGTL_INTERNAL_SOR_HPP
#define TAO_PEGTL_INTERNAL_SOR_HPP



#line 1 "tao/pegtl/internal/integer_sequence.hpp"
       
#line 1 "tao/pegtl/internal/integer_sequence.hpp"



#ifndef TAO_PEGTL_INTERNAL_INTEGER_SEQUENCE_HPP
#define TAO_PEGTL_INTERNAL_INTEGER_SEQUENCE_HPP

#include <cstddef>
#include <type_traits>
#include <utility>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename T, T... Ns >
         struct integer_sequence
         {
            using value_type = T;

            static constexpr std::size_t size() noexcept
            {
               return sizeof...( Ns );
            }
         };

         template< std::size_t... Ns >
         using index_sequence = integer_sequence< std::size_t, Ns... >;

         template< bool V, bool E >
         struct generate_sequence;

         template<>
         struct generate_sequence< false, true >
         {
            template< typename T, T M, T N, std::size_t S, T... Ns >
            using f = integer_sequence< T, Ns... >;
         };

         template<>
         struct generate_sequence< true, true >
         {
            template< typename T, T M, T N, std::size_t S, T... Ns >
            using f = integer_sequence< T, Ns..., S >;
         };

         template<>
         struct generate_sequence< false, false >
         {
            template< typename T, T M, T N, std::size_t S, T... Ns >
            using f = typename generate_sequence< ( N & ( M / 2 ) ) != 0, ( M / 2 ) == 0 >::template f< T, M / 2, N, 2 * S, Ns..., ( Ns + S )... >;
         };

         template<>
         struct generate_sequence< true, false >
         {
            template< typename T, T M, T N, std::size_t S, T... Ns >
            using f = typename generate_sequence< ( N & ( M / 2 ) ) != 0, ( M / 2 ) == 0 >::template f< T, M / 2, N, 2 * S + 1, Ns..., ( Ns + S )..., 2 * S >;
         };

         template< typename T, T N >
         struct memoize_sequence
         {
            static_assert( N < T( 1 << 20 ), "N too large" );
            using type = typename generate_sequence< false, false >::template f< T, ( N < T( 1 << 1 ) ) ? T( 1 << 1 ) : ( N < T( 1 << 2 ) ) ? T( 1 << 2 ) : ( N < T( 1 << 3 ) ) ? T( 1 << 3 ) : ( N < T( 1 << 4 ) ) ? T( 1 << 4 ) : ( N < T( 1 << 5 ) ) ? T( 1 << 5 ) : ( N < T( 1 << 6 ) ) ? T( 1 << 6 ) : ( N < T( 1 << 7 ) ) ? T( 1 << 7 ) : ( N < T( 1 << 8 ) ) ? T( 1 << 8 ) : ( N < T( 1 << 9 ) ) ? T( 1 << 9 ) : ( N < T( 1 << 10 ) ) ? T( 1 << 10 ) : ( N < T( 1 << 11 ) ) ? T( 1 << 11 ) : ( N < T( 1 << 12 ) ) ? T( 1 << 12 ) : ( N < T( 1 << 13 ) ) ? T( 1 << 13 ) : ( N < T( 1 << 14 ) ) ? T( 1 << 14 ) : ( N < T( 1 << 15 ) ) ? T( 1 << 15 ) : ( N < T( 1 << 16 ) ) ? T( 1 << 16 ) : ( N < T( 1 << 17 ) ) ? T( 1 << 17 ) : ( N < T( 1 << 18 ) ) ? T( 1 << 18 ) : ( N < T( 1 << 19 ) ) ? T( 1 << 19 ) : T( 1 << 20 ), N, 0 >;
         };

         template< typename T, T N >
         using make_integer_sequence = typename memoize_sequence< T, N >::type;

         template< std::size_t N >
         using make_index_sequence = make_integer_sequence< std::size_t, N >;

         template< typename... Ts >
         using index_sequence_for = make_index_sequence< sizeof...( Ts ) >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/sor.hpp"
#line 18 "tao/pegtl/internal/sor.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename... Rules >
         struct sor;

         template<>
         struct sor<>
            : trivial< false >
         {
         };

         template< typename... Rules >
         struct sor
            : sor< index_sequence_for< Rules... >, Rules... >
         {
         };

         template< std::size_t... Indices, typename... Rules >
         struct sor< index_sequence< Indices... >, Rules... >
         {
            using analyze_t = analysis::generic< analysis::rule_type::sor, Rules... >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
#ifdef __cpp_fold_expressions
               return ( Control< Rules >::template match < A, ( Indices == ( sizeof...( Rules ) - 1 ) ) ? M : rewind_mode::required, Action, Control > ( in, st... ) || ... );
#else
               bool result = false;
               using swallow = bool[];
               (void)swallow{ result = result || Control< Rules >::template match < A, ( Indices == ( sizeof...( Rules ) - 1 ) ) ? M : rewind_mode::required, Action, Control > ( in, st... )... };
               return result;
#endif
            }
         };

         template< typename... Rules >
         struct skip_control< sor< Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 13 "tao/pegtl/internal/if_then_else.hpp"






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Cond, typename Then, typename Else >
         struct if_then_else
         {
            using analyze_t = analysis::generic< analysis::rule_type::sor, seq< Cond, Then >, seq< not_at< Cond >, Else > >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               auto m = in.template mark< M >();
               using m_t = decltype( m );

               if( Control< Cond >::template match< A, rewind_mode::required, Action, Control >( in, st... ) ) {
                  return m( Control< Then >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) );
               }
               return m( Control< Else >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) );
            }
         };

         template< typename Cond, typename Then, typename Else >
         struct skip_control< if_then_else< Cond, Then, Else > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/internal/if_must_else.hpp"


namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Cond, typename Then, typename Else >
         using if_must_else = if_then_else< Cond, must< Then >, must< Else > >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 28 "tao/pegtl/internal/rules.hpp"

#line 1 "tao/pegtl/internal/istring.hpp"
       
#line 1 "tao/pegtl/internal/istring.hpp"



#ifndef TAO_PEGTL_INTERNAL_ISTRING_HPP
#define TAO_PEGTL_INTERNAL_ISTRING_HPP

#include <type_traits>
#line 18 "tao/pegtl/internal/istring.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< char C >
         using is_alpha = std::integral_constant< bool, ( ( 'a' <= C ) && ( C <= 'z' ) ) || ( ( 'A' <= C ) && ( C <= 'Z' ) ) >;

         template< char C, bool A = is_alpha< C >::value >
         struct ichar_equal;

         template< char C >
         struct ichar_equal< C, true >
         {
            static bool match( const char c ) noexcept
            {
               return ( C | 0x20 ) == ( c | 0x20 );
            }
         };

         template< char C >
         struct ichar_equal< C, false >
         {
            static bool match( const char c ) noexcept
            {
               return c == C;
            }
         };

         template< char... Cs >
         struct istring_equal;

         template<>
         struct istring_equal<>
         {
            static bool match( const char* /*unused*/ ) noexcept
            {
               return true;
            }
         };

         template< char C, char... Cs >
         struct istring_equal< C, Cs... >
         {
            static bool match( const char* r ) noexcept
            {
               return ichar_equal< C >::match( *r ) && istring_equal< Cs... >::match( r + 1 );
            }
         };

         template< char... Cs >
         struct istring;

         template<>
         struct istring<>
            : trivial< true >
         {
         };

         template< char... Cs >
         struct istring
         {
            using analyze_t = analysis::counted< analysis::rule_type::any, sizeof...( Cs ) >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( in.size( 0 ) ) )
            {
               if( in.size( sizeof...( Cs ) ) >= sizeof...( Cs ) ) {
                  if( istring_equal< Cs... >::match( in.current() ) ) {
                     bump_help< result_on_found::success, Input, char, Cs... >( in, sizeof...( Cs ) );
                     return true;
                  }
               }
               return false;
            }
         };

         template< char... Cs >
         struct skip_control< istring< Cs... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 30 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/list.hpp"
       
#line 1 "tao/pegtl/internal/list.hpp"



#ifndef TAO_PEGTL_INTERNAL_LIST_HPP
#define TAO_PEGTL_INTERNAL_LIST_HPP






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Rule, typename Sep >
         using list = seq< Rule, star< Sep, Rule > >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 31 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/list_must.hpp"
       
#line 1 "tao/pegtl/internal/list_must.hpp"



#ifndef TAO_PEGTL_INTERNAL_LIST_MUST_HPP
#define TAO_PEGTL_INTERNAL_LIST_MUST_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Rule, typename Sep >
         using list_must = seq< Rule, star< Sep, must< Rule > > >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 32 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/list_tail.hpp"
       
#line 1 "tao/pegtl/internal/list_tail.hpp"



#ifndef TAO_PEGTL_INTERNAL_LIST_TAIL_HPP
#define TAO_PEGTL_INTERNAL_LIST_TAIL_HPP




#line 1 "tao/pegtl/internal/opt.hpp"
       
#line 1 "tao/pegtl/internal/opt.hpp"



#ifndef TAO_PEGTL_INTERNAL_OPT_HPP
#define TAO_PEGTL_INTERNAL_OPT_HPP

#include <type_traits>
#line 20 "tao/pegtl/internal/opt.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename... Rules >
         struct opt
            : opt< seq< Rules... > >
         {
         };

         template<>
         struct opt<>
            : trivial< true >
         {
         };

         template< typename Rule >
         struct opt< Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt, Rule >;

            template< apply_mode A,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               Control< Rule >::template match< A, rewind_mode::required, Action, Control >( in, st... );
               return true;
            }
         };

         template< typename... Rules >
         struct skip_control< opt< Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 11 "tao/pegtl/internal/list_tail.hpp"


namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Rule, typename Sep >
         using list_tail = seq< list< Rule, Sep >, opt< Sep > >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 33 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/list_tail_pad.hpp"
       
#line 1 "tao/pegtl/internal/list_tail_pad.hpp"



#ifndef TAO_PEGTL_INTERNAL_LIST_TAIL_PAD_HPP
#define TAO_PEGTL_INTERNAL_LIST_TAIL_PAD_HPP





#line 1 "tao/pegtl/internal/pad.hpp"
       
#line 1 "tao/pegtl/internal/pad.hpp"



#ifndef TAO_PEGTL_INTERNAL_PAD_HPP
#define TAO_PEGTL_INTERNAL_PAD_HPP






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Rule, typename Pad1, typename Pad2 = Pad1 >
         using pad = seq< star< Pad1 >, Rule, star< Pad2 > >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/internal/list_tail_pad.hpp"



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Rule, typename Sep, typename Pad >
         using list_tail_pad = seq< list< Rule, pad< Sep, Pad > >, opt< star< Pad >, Sep > >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 34 "tao/pegtl/internal/rules.hpp"


#line 1 "tao/pegtl/internal/one.hpp"
       
#line 1 "tao/pegtl/internal/one.hpp"



#ifndef TAO_PEGTL_INTERNAL_ONE_HPP
#define TAO_PEGTL_INTERNAL_ONE_HPP

#include <algorithm>
#include <utility>
#line 18 "tao/pegtl/internal/one.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Char >
         bool contains( const Char c, const std::initializer_list< Char >& l ) noexcept
         {
            return std::find( l.begin(), l.end(), c ) != l.end();
         }

         template< result_on_found R, typename Peek, typename Peek::data_t... Cs >
         struct one
         {
            using analyze_t = analysis::generic< analysis::rule_type::any >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( Peek::peek( in ) ) )
            {
               if( const auto t = Peek::peek( in ) ) {
                  if( contains( t.data, { Cs... } ) == bool( R ) ) {
                     bump_help< R, Input, typename Peek::data_t, Cs... >( in, t.size );
                     return true;
                  }
               }
               return false;
            }
         };

         template< result_on_found R, typename Peek, typename Peek::data_t C >
         struct one< R, Peek, C >
         {
            using analyze_t = analysis::generic< analysis::rule_type::any >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( Peek::peek( in ) ) )
            {
               if( const auto t = Peek::peek( in ) ) {
                  if( ( t.data == C ) == bool( R ) ) {
                     bump_help< R, Input, typename Peek::data_t, C >( in, t.size );
                     return true;
                  }
               }
               return false;
            }
         };

         template< result_on_found R, typename Peek, typename Peek::data_t... Cs >
         struct skip_control< one< R, Peek, Cs... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 37 "tao/pegtl/internal/rules.hpp"


#line 1 "tao/pegtl/internal/pad_opt.hpp"
       
#line 1 "tao/pegtl/internal/pad_opt.hpp"



#ifndef TAO_PEGTL_INTERNAL_PAD_OPT_HPP
#define TAO_PEGTL_INTERNAL_PAD_OPT_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Rule, typename Pad >
         using pad_opt = seq< star< Pad >, opt< Rule, star< Pad > > >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 40 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/plus.hpp"
       
#line 1 "tao/pegtl/internal/plus.hpp"



#ifndef TAO_PEGTL_INTERNAL_PLUS_HPP
#define TAO_PEGTL_INTERNAL_PLUS_HPP

#include <type_traits>
#line 21 "tao/pegtl/internal/plus.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         // While plus<> could easily be implemented with
         // seq< Rule, Rules ..., star< Rule, Rules ... > > we
         // provide an explicit implementation to optimise away
         // the otherwise created input mark.

         template< typename Rule, typename... Rules >
         struct plus
            : plus< seq< Rule, Rules... > >
         {
         };

         template< typename Rule >
         struct plus< Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, Rule, opt< plus > >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return Control< Rule >::template match< A, M, Action, Control >( in, st... ) && Control< star< Rule > >::template match< A, M, Action, Control >( in, st... );
            }
         };

         template< typename Rule, typename... Rules >
         struct skip_control< plus< Rule, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 41 "tao/pegtl/internal/rules.hpp"



#line 1 "tao/pegtl/internal/rematch.hpp"
       
#line 1 "tao/pegtl/internal/rematch.hpp"



#ifndef TAO_PEGTL_INTERNAL_REMATCH_HPP
#define TAO_PEGTL_INTERNAL_REMATCH_HPP






#line 1 "tao/pegtl/internal/../memory_input.hpp"
       
#line 1 "tao/pegtl/internal/../memory_input.hpp"



#ifndef TAO_PEGTL_MEMORY_INPUT_HPP
#define TAO_PEGTL_MEMORY_INPUT_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>






#line 1 "tao/pegtl/internal/../tracking_mode.hpp"
       
#line 1 "tao/pegtl/internal/../tracking_mode.hpp"



#ifndef TAO_PEGTL_TRACKING_MODE_HPP
#define TAO_PEGTL_TRACKING_MODE_HPP



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      enum class tracking_mode : bool
      {
         eager,
         lazy,

         // Compatibility, remove with 3.0.0
         IMMEDIATE = eager,
         LAZY = lazy
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 20 "tao/pegtl/internal/../memory_input.hpp"



#line 1 "tao/pegtl/internal/../internal/bump.hpp"
       
#line 1 "tao/pegtl/internal/../internal/bump.hpp"



#ifndef TAO_PEGTL_INTERNAL_BUMP_HPP
#define TAO_PEGTL_INTERNAL_BUMP_HPP





namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         inline void bump( iterator& iter, const std::size_t count, const int ch ) noexcept
         {
            for( std::size_t i = 0; i < count; ++i ) {
               if( iter.data[ i ] == ch ) {
                  ++iter.line;
                  iter.byte_in_line = 0;
               }
               else {
                  ++iter.byte_in_line;
               }
            }
            iter.byte += count;
            iter.data += count;
         }

         inline void bump_in_this_line( iterator& iter, const std::size_t count ) noexcept
         {
            iter.data += count;
            iter.byte += count;
            iter.byte_in_line += count;
         }

         inline void bump_to_next_line( iterator& iter, const std::size_t count ) noexcept
         {
            ++iter.line;
            iter.byte += count;
            iter.byte_in_line = 0;
            iter.data += count;
         }

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 24 "tao/pegtl/internal/../memory_input.hpp"


#line 1 "tao/pegtl/internal/../internal/marker.hpp"
       
#line 1 "tao/pegtl/internal/../internal/marker.hpp"



#ifndef TAO_PEGTL_INTERNAL_MARKER_HPP
#define TAO_PEGTL_INTERNAL_MARKER_HPP




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Iterator, rewind_mode M >
         class marker
         {
         public:
            static constexpr rewind_mode next_rewind_mode = M;

            explicit marker( const Iterator& /*unused*/ ) noexcept
            {
            }

            marker( const marker& ) = delete;

            marker( marker&& /*unused*/ ) noexcept
            {
            }

            ~marker() = default;

            void operator=( const marker& ) = delete;
            void operator=( marker&& ) = delete;

            bool operator()( const bool result ) const noexcept
            {
               return result;
            }
         };

         template< typename Iterator >
         class marker< Iterator, rewind_mode::required >
         {
         public:
            static constexpr rewind_mode next_rewind_mode = rewind_mode::active;

            explicit marker( Iterator& i ) noexcept
               : m_saved( i ),
                 m_input( &i )
            {
            }

            marker( const marker& ) = delete;

            marker( marker&& i ) noexcept
               : m_saved( i.m_saved ),
                 m_input( i.m_input )
            {
               i.m_input = nullptr;
            }

            ~marker() noexcept
            {
               if( m_input != nullptr ) {
                  ( *m_input ) = m_saved;
               }
            }

            void operator=( const marker& ) = delete;
            void operator=( marker&& ) = delete;

            bool operator()( const bool result ) noexcept
            {
               if( result ) {
                  m_input = nullptr;
                  return true;
               }
               return false;
            }

            const Iterator& iterator() const noexcept
            {
               return m_saved;
            }

         private:
            const Iterator m_saved;
            Iterator* m_input;
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 27 "tao/pegtl/internal/../memory_input.hpp"
#line 1 "tao/pegtl/internal/../internal/until.hpp"
       
#line 1 "tao/pegtl/internal/../internal/until.hpp"



#ifndef TAO_PEGTL_INTERNAL_UNTIL_HPP
#define TAO_PEGTL_INTERNAL_UNTIL_HPP
#line 21 "tao/pegtl/internal/../internal/until.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Cond, typename... Rules >
         struct until
            : until< Cond, seq< Rules... > >
         {
         };

         template< typename Cond >
         struct until< Cond >
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, star< not_at< Cond >, not_at< eof >, bytes< 1 > >, Cond >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               auto m = in.template mark< M >();

               while( !Control< Cond >::template match< A, rewind_mode::required, Action, Control >( in, st... ) ) {
                  if( in.empty() ) {
                     return false;
                  }
                  in.bump();
               }
               return m( true );
            }
         };

         template< typename Cond, typename Rule >
         struct until< Cond, Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, star< not_at< Cond >, not_at< eof >, Rule >, Cond >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               auto m = in.template mark< M >();
               using m_t = decltype( m );

               while( !Control< Cond >::template match< A, rewind_mode::required, Action, Control >( in, st... ) ) {
                  if( !Control< Rule >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) ) {
                     return false;
                  }
               }
               return m( true );
            }
         };

         template< typename Cond, typename... Rules >
         struct skip_control< until< Cond, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 28 "tao/pegtl/internal/../memory_input.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< tracking_mode, typename Eol, typename Source >
         class memory_input_base;

         template< typename Eol, typename Source >
         class memory_input_base< tracking_mode::eager, Eol, Source >
         {
         public:
            using iterator_t = internal::iterator;

            template< typename T >
            memory_input_base( const iterator_t& in_begin, const char* in_end, T&& in_source ) noexcept( std::is_nothrow_constructible< Source, T&& >::value )
               : m_begin( in_begin.data ),
                 m_current( in_begin ),
                 m_end( in_end ),
                 m_source( std::forward< T >( in_source ) )
            {
            }

            template< typename T >
            memory_input_base( const char* in_begin, const char* in_end, T&& in_source ) noexcept( std::is_nothrow_constructible< Source, T&& >::value )
               : m_begin( in_begin ),
                 m_current( in_begin ),
                 m_end( in_end ),
                 m_source( std::forward< T >( in_source ) )
            {
            }

            memory_input_base( const memory_input_base& ) = delete;
            memory_input_base( memory_input_base&& ) = delete;

            ~memory_input_base() = default;

            memory_input_base operator=( const memory_input_base& ) = delete;
            memory_input_base operator=( memory_input_base&& ) = delete;

            const char* current() const noexcept
            {
               return m_current.data;
            }

            const char* begin() const noexcept
            {
               return m_begin;
            }

            const char* end( const std::size_t /*unused*/ = 0 ) const noexcept
            {
               return m_end;
            }

            std::size_t byte() const noexcept
            {
               return m_current.byte;
            }

            std::size_t line() const noexcept
            {
               return m_current.line;
            }

            std::size_t byte_in_line() const noexcept
            {
               return m_current.byte_in_line;
            }

            void bump( const std::size_t in_count = 1 ) noexcept
            {
               internal::bump( m_current, in_count, Eol::ch );
            }

            void bump_in_this_line( const std::size_t in_count = 1 ) noexcept
            {
               internal::bump_in_this_line( m_current, in_count );
            }

            void bump_to_next_line( const std::size_t in_count = 1 ) noexcept
            {
               internal::bump_to_next_line( m_current, in_count );
            }

            TAO_PEGTL_NAMESPACE::position position( const iterator_t& it ) const
            {
               return TAO_PEGTL_NAMESPACE::position( it, m_source );
            }

            void restart( const std::size_t in_byte = 0, const std::size_t in_line = 1, const std::size_t in_byte_in_line = 0 )
            {
               m_current.data = m_begin;
               m_current.byte = in_byte;
               m_current.line = in_line;
               m_current.byte_in_line = in_byte_in_line;
            }

         protected:
            const char* const m_begin;
            iterator_t m_current;
            const char* const m_end;
            const Source m_source;
         };

         template< typename Eol, typename Source >
         class memory_input_base< tracking_mode::lazy, Eol, Source >
         {
         public:
            using iterator_t = const char*;

            template< typename T >
            memory_input_base( const internal::iterator& in_begin, const char* in_end, T&& in_source ) noexcept( std::is_nothrow_constructible< Source, T&& >::value )
               : m_begin( in_begin ),
                 m_current( in_begin.data ),
                 m_end( in_end ),
                 m_source( std::forward< T >( in_source ) )
            {
            }

            template< typename T >
            memory_input_base( const char* in_begin, const char* in_end, T&& in_source ) noexcept( std::is_nothrow_constructible< Source, T&& >::value )
               : m_begin( in_begin ),
                 m_current( in_begin ),
                 m_end( in_end ),
                 m_source( std::forward< T >( in_source ) )
            {
            }

            memory_input_base( const memory_input_base& ) = delete;
            memory_input_base( memory_input_base&& ) = delete;

            ~memory_input_base() = default;

            memory_input_base operator=( const memory_input_base& ) = delete;
            memory_input_base operator=( memory_input_base&& ) = delete;

            const char* current() const noexcept
            {
               return m_current;
            }

            const char* begin() const noexcept
            {
               return m_begin.data;
            }

            const char* end( const std::size_t /*unused*/ = 0 ) const noexcept
            {
               return m_end;
            }

            std::size_t byte() const noexcept
            {
               return std::size_t( current() - m_begin.data );
            }

            void bump( const std::size_t in_count = 1 ) noexcept
            {
               m_current += in_count;
            }

            void bump_in_this_line( const std::size_t in_count = 1 ) noexcept
            {
               m_current += in_count;
            }

            void bump_to_next_line( const std::size_t in_count = 1 ) noexcept
            {
               m_current += in_count;
            }

            TAO_PEGTL_NAMESPACE::position position( const iterator_t it ) const
            {
               internal::iterator c( m_begin );
               internal::bump( c, std::size_t( it - m_begin.data ), Eol::ch );
               return TAO_PEGTL_NAMESPACE::position( c, m_source );
            }

            void restart()
            {
               m_current = m_begin.data;
            }

         protected:
            const internal::iterator m_begin;
            iterator_t m_current;
            const char* const m_end;
            const Source m_source;
         };

      } // namespace internal

      template< tracking_mode P = tracking_mode::eager, typename Eol = eol::lf_crlf, typename Source = std::string >
      class memory_input
         : public internal::memory_input_base< P, Eol, Source >
      {
      public:
         static constexpr tracking_mode tracking_mode_v = P;

         using eol_t = Eol;
         using source_t = Source;

         using typename internal::memory_input_base< P, Eol, Source >::iterator_t;

         using action_t = internal::action_input< memory_input >;

         using internal::memory_input_base< P, Eol, Source >::memory_input_base;

         template< typename T >
         memory_input( const char* in_begin, const std::size_t in_size, T&& in_source ) noexcept( std::is_nothrow_constructible< Source, T&& >::value )
            : memory_input( in_begin, in_begin + in_size, std::forward< T >( in_source ) )
         {
         }

         template< typename T >
         memory_input( const std::string& in_string, T&& in_source ) noexcept( std::is_nothrow_constructible< Source, T&& >::value )
            : memory_input( in_string.data(), in_string.size(), std::forward< T >( in_source ) )
         {
         }

         template< typename T >
         memory_input( std::string&&, T&& ) = delete;

         template< typename T >
         memory_input( const char* in_begin, T&& in_source ) noexcept( std::is_nothrow_constructible< Source, T&& >::value )
            : memory_input( in_begin, std::strlen( in_begin ), std::forward< T >( in_source ) )
         {
         }

         template< typename T >
         memory_input( const char* in_begin, const char* in_end, T&& in_source, const std::size_t in_byte, const std::size_t in_line, const std::size_t in_byte_in_line ) noexcept( std::is_nothrow_constructible< Source, T&& >::value )
            : memory_input( { in_begin, in_byte, in_line, in_byte_in_line }, in_end, std::forward< T >( in_source ) )
         {
         }

         memory_input( const memory_input& ) = delete;
         memory_input( memory_input&& ) = delete;

         ~memory_input() = default;

         memory_input operator=( const memory_input& ) = delete;
         memory_input operator=( memory_input&& ) = delete;

         const Source& source() const noexcept
         {
            return this->m_source;
         }

         bool empty() const noexcept
         {
            return this->current() == this->end();
         }

         std::size_t size( const std::size_t /*unused*/ = 0 ) const noexcept
         {
            return std::size_t( this->end() - this->current() );
         }

         char peek_char( const std::size_t offset = 0 ) const noexcept
         {
            return this->current()[ offset ];
         }

         std::uint8_t peek_uint8( const std::size_t offset = 0 ) const noexcept
         {
            return static_cast< std::uint8_t >( peek_char( offset ) );
         }

         // Compatibility, remove with 3.0.0
         std::uint8_t peek_byte( const std::size_t offset = 0 ) const noexcept
         {
            return static_cast< std::uint8_t >( peek_char( offset ) );
         }

         iterator_t& iterator() noexcept
         {
            return this->m_current;
         }

         const iterator_t& iterator() const noexcept
         {
            return this->m_current;
         }

         using internal::memory_input_base< P, Eol, Source >::restart;

         template< rewind_mode M >
         void restart( const internal::marker< iterator_t, M >& m )
         {
            iterator() = m.iterator();
         }

         using internal::memory_input_base< P, Eol, Source >::position;

         TAO_PEGTL_NAMESPACE::position position() const
         {
            return position( iterator() );
         }

         void discard() const noexcept
         {
         }

         void require( const std::size_t /*unused*/ ) const noexcept
         {
         }

         template< rewind_mode M >
         internal::marker< iterator_t, M > mark() noexcept
         {
            return internal::marker< iterator_t, M >( iterator() );
         }

         const char* at( const TAO_PEGTL_NAMESPACE::position& p ) const noexcept
         {
            return this->begin() + p.byte;
         }

         const char* begin_of_line( const TAO_PEGTL_NAMESPACE::position& p ) const noexcept
         {
            return at( p ) - p.byte_in_line;
         }

         const char* end_of_line( const TAO_PEGTL_NAMESPACE::position& p ) const noexcept
         {
            using input_t = memory_input< tracking_mode::lazy, Eol, const char* >;
            input_t in( at( p ), this->end(), "" );
            using grammar = internal::until< internal::at< internal::eolf > >;
            normal< grammar >::match< apply_mode::nothing, rewind_mode::dontcare, nothing, normal >( in );
            return in.current();
         }

         std::string line_at( const TAO_PEGTL_NAMESPACE::position& p ) const
         {
            return std::string( begin_of_line( p ), end_of_line( p ) );
         }
      };

#ifdef __cpp_deduction_guides
      template< typename... Ts >
      memory_input( Ts&&... )->memory_input<>;
#endif

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 13 "tao/pegtl/internal/rematch.hpp"


namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Head, typename... Rules >
         struct rematch;

         template< typename Head >
         struct rematch< Head >
         {
            using analyze_t = typename Head::analyze_t;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               return Control< Head >::template match< A, M, Action, Control >( in, st... );
            }
         };

         template< typename Head, typename Rule, typename... Rules >
         struct rematch< Head, Rule, Rules... >
         {
            using analyze_t = typename Head::analyze_t; // NOTE: Rule and Rules are ignored for analyze().

            template< apply_mode A,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               auto m = in.template mark< rewind_mode::required >();

               if( Control< Head >::template match< A, rewind_mode::active, Action, Control >( in, st... ) ) {
                  memory_input< Input::tracking_mode_v, typename Input::eol_t, typename Input::source_t > i2( m.iterator(), in.current(), in.source() );
#ifdef __cpp_fold_expressions
                  return m( ( Control< Rule >::template match< A, rewind_mode::active, Action, Control >( i2, st... ) && ... && ( i2.restart( m ), Control< Rules >::template match< A, rewind_mode::active, Action, Control >( i2, st... ) ) ) );
#else
                  bool result = Control< Rule >::template match< A, rewind_mode::active, Action, Control >( i2, st... );
                  using swallow = bool[];
                  (void)swallow{ result = result && ( i2.restart( m ), Control< Rules >::template match< A, rewind_mode::active, Action, Control >( i2, st... ) )..., true };
                  return m( result );
#endif
               }
               return false;
            }
         };

         template< typename Head, typename... Rules >
         struct skip_control< rematch< Head, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 45 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/rep.hpp"
       
#line 1 "tao/pegtl/internal/rep.hpp"



#ifndef TAO_PEGTL_INTERNAL_REP_HPP
#define TAO_PEGTL_INTERNAL_REP_HPP
#line 18 "tao/pegtl/internal/rep.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< unsigned Num, typename... Rules >
         struct rep
            : rep< Num, seq< Rules... > >
         {
         };

         template< unsigned Num >
         struct rep< Num >
            : trivial< true >
         {
         };

         template< typename Rule >
         struct rep< 0, Rule >
            : trivial< true >
         {
         };

         template< unsigned Num, typename Rule >
         struct rep< Num, Rule >
         {
            using analyze_t = analysis::counted< analysis::rule_type::seq, Num, Rule >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               auto m = in.template mark< M >();
               using m_t = decltype( m );

               for( unsigned i = 0; i != Num; ++i ) {
                  if( !Control< Rule >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) ) {
                     return false;
                  }
               }
               return m( true );
            }
         };

         template< unsigned Num, typename... Rules >
         struct skip_control< rep< Num, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 46 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/rep_min.hpp"
       
#line 1 "tao/pegtl/internal/rep_min.hpp"



#ifndef TAO_PEGTL_INTERNAL_REP_MIN_HPP
#define TAO_PEGTL_INTERNAL_REP_MIN_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< unsigned Min, typename Rule, typename... Rules >
         using rep_min = seq< rep< Min, Rule, Rules... >, star< Rule, Rules... > >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 47 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/rep_min_max.hpp"
       
#line 1 "tao/pegtl/internal/rep_min_max.hpp"



#ifndef TAO_PEGTL_INTERNAL_REP_MIN_MAX_HPP
#define TAO_PEGTL_INTERNAL_REP_MIN_MAX_HPP

#include <type_traits>
#line 21 "tao/pegtl/internal/rep_min_max.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< unsigned Min, unsigned Max, typename... Rules >
         struct rep_min_max
            : rep_min_max< Min, Max, seq< Rules... > >
         {
         };

         template< unsigned Min, unsigned Max >
         struct rep_min_max< Min, Max >
            : trivial< false >
         {
            static_assert( Min <= Max, "invalid rep_min_max rule (maximum number of repetitions smaller than minimum)" );
         };

         template< typename Rule >
         struct rep_min_max< 0, 0, Rule >
            : not_at< Rule >
         {
         };

         template< unsigned Min, unsigned Max, typename Rule >
         struct rep_min_max< Min, Max, Rule >
         {
            using analyze_t = analysis::counted< analysis::rule_type::seq, Min, Rule >;

            static_assert( Min <= Max, "invalid rep_min_max rule (maximum number of repetitions smaller than minimum)" );

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               auto m = in.template mark< M >();
               using m_t = decltype( m );

               for( unsigned i = 0; i != Min; ++i ) {
                  if( !Control< Rule >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) ) {
                     return false;
                  }
               }
               for( unsigned i = Min; i != Max; ++i ) {
                  if( !Control< Rule >::template match< A, rewind_mode::required, Action, Control >( in, st... ) ) {
                     return m( true );
                  }
               }
               return m( Control< not_at< Rule > >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) ); // NOTE that not_at<> will always rewind.
            }
         };

         template< unsigned Min, unsigned Max, typename... Rules >
         struct skip_control< rep_min_max< Min, Max, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 48 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/rep_opt.hpp"
       
#line 1 "tao/pegtl/internal/rep_opt.hpp"



#ifndef TAO_PEGTL_INTERNAL_REP_OPT_HPP
#define TAO_PEGTL_INTERNAL_REP_OPT_HPP
#line 17 "tao/pegtl/internal/rep_opt.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< unsigned Max, typename... Rules >
         struct rep_opt
            : rep_opt< Max, seq< Rules... > >
         {
         };

         template< unsigned Max, typename Rule >
         struct rep_opt< Max, Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt, Rule >;

            template< apply_mode A,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               for( unsigned i = 0; ( i != Max ) && Control< Rule >::template match< A, rewind_mode::required, Action, Control >( in, st... ); ++i ) {
               }
               return true;
            }
         };

         template< unsigned Max, typename... Rules >
         struct skip_control< rep_opt< Max, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 49 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/require.hpp"
       
#line 1 "tao/pegtl/internal/require.hpp"



#ifndef TAO_PEGTL_INTERNAL_REQUIRE_HPP
#define TAO_PEGTL_INTERNAL_REQUIRE_HPP
#line 14 "tao/pegtl/internal/require.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< unsigned Amount >
         struct require;

         template<>
         struct require< 0 >
            : trivial< true >
         {
         };

         template< unsigned Amount >
         struct require
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( in.size( 0 ) ) )
            {
               return in.size( Amount ) >= Amount;
            }
         };

         template< unsigned Amount >
         struct skip_control< require< Amount > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 50 "tao/pegtl/internal/rules.hpp"




#line 1 "tao/pegtl/internal/star_must.hpp"
       
#line 1 "tao/pegtl/internal/star_must.hpp"



#ifndef TAO_PEGTL_INTERNAL_STAR_MUST_HPP
#define TAO_PEGTL_INTERNAL_STAR_MUST_HPP






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Cond, typename... Rules >
         using star_must = star< if_must< false, Cond, Rules... > >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 55 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/state.hpp"
       
#line 1 "tao/pegtl/internal/state.hpp"



#ifndef TAO_PEGTL_INTERNAL_STATE_HPP
#define TAO_PEGTL_INTERNAL_STATE_HPP
#line 17 "tao/pegtl/internal/state.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename State, typename... Rules >
         struct state
            : state< State, seq< Rules... > >
         {
         };

         template< typename State, typename Rule >
         struct state< State, Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, Rule >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static auto success( State& s, const Input& in, States&&... st )
               -> decltype( s.template success< A, M, Action, Control >( in, st... ), void() )
            {
               s.template success< A, M, Action, Control >( in, st... );
            }

            // NOTE: The additional "int = 0" is a work-around for missing expression SFINAE in VS2015.

            template< apply_mode,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States,
                      int = 0 >
            static auto success( State& s, const Input& in, States&&... st )
               -> decltype( s.success( in, st... ), void() )
            {
               s.success( in, st... );
            }

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               State s( static_cast< const Input& >( in ), st... );

               if( Control< Rule >::template match< A, M, Action, Control >( in, s ) ) {
                  success< A, M, Action, Control >( s, in, st... );
                  return true;
               }
               return false;
            }
         };

         template< typename State, typename... Rules >
         struct skip_control< state< State, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 56 "tao/pegtl/internal/rules.hpp"
#line 1 "tao/pegtl/internal/string.hpp"
       
#line 1 "tao/pegtl/internal/string.hpp"



#ifndef TAO_PEGTL_INTERNAL_STRING_HPP
#define TAO_PEGTL_INTERNAL_STRING_HPP

#include <cstring>
#include <utility>
#line 19 "tao/pegtl/internal/string.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         inline bool unsafe_equals( const char* s, const std::initializer_list< char >& l ) noexcept
         {
            return std::memcmp( s, &*l.begin(), l.size() ) == 0;
         }

         template< char... Cs >
         struct string;

         template<>
         struct string<>
            : trivial< true >
         {
         };

         template< char... Cs >
         struct string
         {
            using analyze_t = analysis::counted< analysis::rule_type::any, sizeof...( Cs ) >;

            template< typename Input >
            static bool match( Input& in ) noexcept( noexcept( in.size( 0 ) ) )
            {
               if( in.size( sizeof...( Cs ) ) >= sizeof...( Cs ) ) {
                  if( unsafe_equals( in.current(), { Cs... } ) ) {
                     bump_help< result_on_found::success, Input, char, Cs... >( in, sizeof...( Cs ) );
                     return true;
                  }
               }
               return false;
            }
         };

         template< char... Cs >
         struct skip_control< string< Cs... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 57 "tao/pegtl/internal/rules.hpp"

#line 1 "tao/pegtl/internal/try_catch_type.hpp"
       
#line 1 "tao/pegtl/internal/try_catch_type.hpp"



#ifndef TAO_PEGTL_INTERNAL_TRY_CATCH_TYPE_HPP
#define TAO_PEGTL_INTERNAL_TRY_CATCH_TYPE_HPP

#include <type_traits>
#line 20 "tao/pegtl/internal/try_catch_type.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Exception, typename... Rules >
         struct try_catch_type
            : try_catch_type< Exception, seq< Rules... > >
         {
         };

         template< typename Exception >
         struct try_catch_type< Exception >
            : trivial< true >
         {
         };

         template< typename Exception, typename Rule >
         struct try_catch_type< Exception, Rule >
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, Rule >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               auto m = in.template mark< M >();
               using m_t = decltype( m );

               try {
                  return m( Control< Rule >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) );
               }
               catch( const Exception& ) {
                  return false;
               }
            }
         };

         template< typename Exception, typename... Rules >
         struct skip_control< try_catch_type< Exception, Rules... > > : std::true_type
         {
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 59 "tao/pegtl/internal/rules.hpp"


#endif
#line 13 "tao/pegtl/ascii.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      inline namespace ascii
      {
         // clang-format off
         struct alnum : internal::alnum {};
         struct alpha : internal::alpha {};
         struct any : internal::any< internal::peek_char > {};
         struct blank : internal::one< internal::result_on_found::success, internal::peek_char, ' ', '\t' > {};
         struct digit : internal::range< internal::result_on_found::success, internal::peek_char, '0', '9' > {};
         struct ellipsis : internal::string< '.', '.', '.' > {};
         struct eolf : internal::eolf {};
         template< char... Cs > struct forty_two : internal::rep< 42, internal::one< internal::result_on_found::success, internal::peek_char, Cs... > > {};
         struct identifier_first : internal::identifier_first {};
         struct identifier_other : internal::identifier_other {};
         struct identifier : internal::identifier {};
         template< char... Cs > struct istring : internal::istring< Cs... > {};
         template< char... Cs > struct keyword : internal::seq< internal::string< Cs... >, internal::not_at< internal::identifier_other > > {};
         struct lower : internal::range< internal::result_on_found::success, internal::peek_char, 'a', 'z' > {};
         template< char... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_char, Cs... > {};
         template< char Lo, char Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_char, Lo, Hi > {};
         struct nul : internal::one< internal::result_on_found::success, internal::peek_char, char( 0 ) > {};
         template< char... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_char, Cs... > {};
         struct print : internal::range< internal::result_on_found::success, internal::peek_char, char( 32 ), char( 126 ) > {};
         template< char Lo, char Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_char, Lo, Hi > {};
         template< char... Cs > struct ranges : internal::ranges< internal::peek_char, Cs... > {};
         struct seven : internal::range< internal::result_on_found::success, internal::peek_char, char( 0 ), char( 127 ) > {};
         struct shebang : internal::if_must< false, internal::string< '#', '!' >, internal::until< internal::eolf > > {};
         struct space : internal::one< internal::result_on_found::success, internal::peek_char, ' ', '\n', '\r', '\t', '\v', '\f' > {};
         template< char... Cs > struct string : internal::string< Cs... > {};
         template< char C > struct three : internal::string< C, C, C > {};
         template< char C > struct two : internal::string< C, C > {};
         struct upper : internal::range< internal::result_on_found::success, internal::peek_char, 'A', 'Z' > {};
         struct xdigit : internal::ranges< internal::peek_char, '0', '9', 'a', 'f', 'A', 'F' > {};
         // clang-format on

         template<>
         struct keyword<>
         {
            template< typename Input >
            static bool match( Input& /*unused*/ ) noexcept
            {
               static_assert( internal::always_false< Input >::value, "empty keywords not allowed" );
               return false;
            }
         };

      } // namespace ascii

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#line 1 "tao/pegtl/internal/pegtl_string.hpp"
       
#line 1 "tao/pegtl/internal/pegtl_string.hpp"



#ifndef TAO_PEGTL_INTERNAL_PEGTL_STRING_HPP
#define TAO_PEGTL_INTERNAL_PEGTL_STRING_HPP

#include <cstddef>
#include <type_traits>




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      // Inspired by https://github.com/irrequietus/typestring
      // Rewritten and reduced to what is needed for the PEGTL
      // and to work with Visual Studio 2015.

      namespace internal
      {
         template< typename, typename, typename, typename, typename, typename, typename, typename >
         struct string_join;

         template< template< char... > class S, char... C0s, char... C1s, char... C2s, char... C3s, char... C4s, char... C5s, char... C6s, char... C7s >
         struct string_join< S< C0s... >, S< C1s... >, S< C2s... >, S< C3s... >, S< C4s... >, S< C5s... >, S< C6s... >, S< C7s... > >
         {
            using type = S< C0s..., C1s..., C2s..., C3s..., C4s..., C5s..., C6s..., C7s... >;
         };

         template< template< char... > class S, char, bool >
         struct string_at
         {
            using type = S<>;
         };

         template< template< char... > class S, char C >
         struct string_at< S, C, true >
         {
            using type = S< C >;
         };

         template< typename T, std::size_t S >
         struct string_max_length
         {
            static_assert( S <= 512, "String longer than 512 (excluding terminating \\0)!" );
            using type = T;
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#define TAO_PEGTL_INTERNAL_EMPTY()
#define TAO_PEGTL_INTERNAL_DEFER( X ) X TAO_PEGTL_INTERNAL_EMPTY()
#define TAO_PEGTL_INTERNAL_EXPAND( ... ) __VA_ARGS__

#define TAO_PEGTL_INTERNAL_STRING_AT( S, x, n )    tao::TAO_PEGTL_NAMESPACE::internal::string_at< S, ( 0##n < ( sizeof( x ) / sizeof( char ) ) ) ? ( x )[ 0##n ] : 0, ( 0##n < ( sizeof( x ) / sizeof( char ) ) - 1 ) >::type


#define TAO_PEGTL_INTERNAL_JOIN_8( M, S, x, n )                                                     tao::TAO_PEGTL_NAMESPACE::internal::string_join< TAO_PEGTL_INTERNAL_DEFER( M )( S, x, n##0 ),                                                     TAO_PEGTL_INTERNAL_DEFER( M )( S, x, n##1 ),                                                     TAO_PEGTL_INTERNAL_DEFER( M )( S, x, n##2 ),                                                     TAO_PEGTL_INTERNAL_DEFER( M )( S, x, n##3 ),                                                     TAO_PEGTL_INTERNAL_DEFER( M )( S, x, n##4 ),                                                     TAO_PEGTL_INTERNAL_DEFER( M )( S, x, n##5 ),                                                     TAO_PEGTL_INTERNAL_DEFER( M )( S, x, n##6 ),                                                     TAO_PEGTL_INTERNAL_DEFER( M )( S, x, n##7 ) >::type
#line 74 "tao/pegtl/internal/pegtl_string.hpp"
#define TAO_PEGTL_INTERNAL_STRING_8( S, x, n )    TAO_PEGTL_INTERNAL_JOIN_8( TAO_PEGTL_INTERNAL_STRING_AT, S, x, n )


#define TAO_PEGTL_INTERNAL_STRING_64( S, x, n )    TAO_PEGTL_INTERNAL_JOIN_8( TAO_PEGTL_INTERNAL_STRING_8, S, x, n )


#define TAO_PEGTL_INTERNAL_STRING_512( S, x, n )    TAO_PEGTL_INTERNAL_JOIN_8( TAO_PEGTL_INTERNAL_STRING_64, S, x, n )


#define TAO_PEGTL_INTERNAL_STRING( S, x )    TAO_PEGTL_INTERNAL_EXPAND(                   TAO_PEGTL_INTERNAL_EXPAND(                   TAO_PEGTL_INTERNAL_EXPAND(                   tao::TAO_PEGTL_NAMESPACE::internal::string_max_length< TAO_PEGTL_INTERNAL_STRING_512( S, x, ), sizeof( x ) - 1 >::type ) ) )





#define TAO_PEGTL_STRING( x )    TAO_PEGTL_INTERNAL_STRING( tao::TAO_PEGTL_NAMESPACE::ascii::string, x )


#define TAO_PEGTL_ISTRING( x )    TAO_PEGTL_INTERNAL_STRING( tao::TAO_PEGTL_NAMESPACE::ascii::istring, x )


#define TAO_PEGTL_KEYWORD( x )    TAO_PEGTL_INTERNAL_STRING( tao::TAO_PEGTL_NAMESPACE::ascii::keyword, x )


// Compatibility, remove with 3.0.0
#define TAOCPP_PEGTL_STRING( x ) TAO_PEGTL_STRING( x )
#define TAOCPP_PEGTL_ISTRING( x ) TAO_PEGTL_ISTRING( x )
#define TAOCPP_PEGTL_KEYWORD( x ) TAO_PEGTL_KEYWORD( x )

#endif
#line 70 "tao/pegtl/ascii.hpp"

#endif
#line 13 "tao/pegtl.hpp"
#line 1 "tao/pegtl/rules.hpp"
       
#line 1 "tao/pegtl/rules.hpp"



#ifndef TAO_PEGTL_RULES_HPP
#define TAO_PEGTL_RULES_HPP






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      // clang-format off
      template< typename... Actions > struct apply : internal::apply< Actions... > {};
      template< typename... Actions > struct apply0 : internal::apply0< Actions... > {};
      template< template< typename... > class Action, typename... Rules > struct action : internal::action< Action, Rules... > {};
      template< typename... Rules > struct at : internal::at< Rules... > {};
      struct bof : internal::bof {};
      struct bol : internal::bol {};
      template< unsigned Num > struct bytes : internal::bytes< Num > {};
      template< template< typename... > class Control, typename... Rules > struct control : internal::control< Control, Rules... > {};
      template< typename... Rules > struct disable : internal::disable< Rules... > {};
      struct discard : internal::discard {};
      template< typename... Rules > struct enable : internal::enable< Rules... > {};
      struct eof : internal::eof {};
      struct failure : internal::trivial< false > {};
      template< typename Rule, typename... Actions > struct if_apply : internal::if_apply< Rule, Actions... > {};
      template< typename Cond, typename... Thens > struct if_must : internal::if_must< false, Cond, Thens... > {};
      template< typename Cond, typename Then, typename Else > struct if_must_else : internal::if_must_else< Cond, Then, Else > {};
      template< typename Cond, typename Then, typename Else > struct if_then_else : internal::if_then_else< Cond, Then, Else > {};
      template< typename Rule, typename Sep, typename Pad = void > struct list : internal::list< Rule, internal::pad< Sep, Pad > > {};
      template< typename Rule, typename Sep > struct list< Rule, Sep, void > : internal::list< Rule, Sep > {};
      template< typename Rule, typename Sep, typename Pad = void > struct list_must : internal::list_must< Rule, internal::pad< Sep, Pad > > {};
      template< typename Rule, typename Sep > struct list_must< Rule, Sep, void > : internal::list_must< Rule, Sep > {};
      template< typename Rule, typename Sep, typename Pad = void > struct list_tail : internal::list_tail_pad< Rule, Sep, Pad > {};
      template< typename Rule, typename Sep > struct list_tail< Rule, Sep, void > : internal::list_tail< Rule, Sep > {};
      template< typename M, typename S > struct minus : internal::rematch< M, internal::not_at< S, internal::eof > > {};
      template< typename... Rules > struct must : internal::must< Rules... > {};
      template< typename... Rules > struct not_at : internal::not_at< Rules... > {};
      template< typename... Rules > struct opt : internal::opt< Rules... > {};
      template< typename Cond, typename... Rules > struct opt_must : internal::if_must< true, Cond, Rules... > {};
      template< typename Rule, typename Pad1, typename Pad2 = Pad1 > struct pad : internal::pad< Rule, Pad1, Pad2 > {};
      template< typename Rule, typename Pad > struct pad_opt : internal::pad_opt< Rule, Pad > {};
      template< typename Rule, typename... Rules > struct plus : internal::plus< Rule, Rules... > {};
      template< typename Exception > struct raise : internal::raise< Exception > {};
      template< typename Head, typename... Rules > struct rematch : internal::rematch< Head, Rules... > {};
      template< unsigned Num, typename... Rules > struct rep : internal::rep< Num, Rules... > {};
      template< unsigned Max, typename... Rules > struct rep_max : internal::rep_min_max< 0, Max, Rules... > {};
      template< unsigned Min, typename Rule, typename... Rules > struct rep_min : internal::rep_min< Min, Rule, Rules... > {};
      template< unsigned Min, unsigned Max, typename... Rules > struct rep_min_max : internal::rep_min_max< Min, Max, Rules... > {};
      template< unsigned Max, typename... Rules > struct rep_opt : internal::rep_opt< Max, Rules... > {};
      template< unsigned Amount > struct require : internal::require< Amount > {};
      template< typename... Rules > struct seq : internal::seq< Rules... > {};
      template< typename... Rules > struct sor : internal::sor< Rules... > {};
      template< typename Rule, typename... Rules > struct star : internal::star< Rule, Rules... > {};
      template< typename Cond, typename... Rules > struct star_must : internal::star_must< Cond, Rules... > {};
      template< typename State, typename... Rules > struct state : internal::state< State, Rules... > {};
      struct success : internal::trivial< true > {};
      template< typename... Rules > struct try_catch : internal::seq< internal::try_catch_type< parse_error, Rules... > > {};
      template< typename Exception, typename... Rules > struct try_catch_type : internal::seq< internal::try_catch_type< Exception, Rules... > > {};
      template< typename Cond, typename... Rules > struct until : internal::until< Cond, Rules... > {};
      // clang-format on

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 14 "tao/pegtl.hpp"
#line 1 "tao/pegtl/uint16.hpp"
       
#line 1 "tao/pegtl/uint16.hpp"



#ifndef TAO_PEGTL_UINT16_HPP
#define TAO_PEGTL_UINT16_HPP



#line 1 "tao/pegtl/internal/peek_mask_uint.hpp"
       
#line 1 "tao/pegtl/internal/peek_mask_uint.hpp"



#ifndef TAO_PEGTL_INTERNAL_PEEK_MASK_UINT_HPP
#define TAO_PEGTL_INTERNAL_PEEK_MASK_UINT_HPP

#include <cstddef>
#include <cstdint>




#line 1 "tao/pegtl/internal/read_uint.hpp"
       
#line 1 "tao/pegtl/internal/read_uint.hpp"



#ifndef TAO_PEGTL_INTERNAL_READ_UINT_HPP
#define TAO_PEGTL_INTERNAL_READ_UINT_HPP

#include <cstdint>



#line 1 "tao/pegtl/internal/endian.hpp"
       
#line 1 "tao/pegtl/internal/endian.hpp"



#ifndef TAO_PEGTL_INTERNAL_ENDIAN_HPP
#define TAO_PEGTL_INTERNAL_ENDIAN_HPP

#include <cstdint>
#include <cstring>



#if defined( _WIN32 ) && !defined( __MINGW32__ )
#line 1 "tao/pegtl/internal/endian_win.hpp"
       
#line 1 "tao/pegtl/internal/endian_win.hpp"



#ifndef TAO_PEGTL_INTERNAL_ENDIAN_WIN_HPP
#define TAO_PEGTL_INTERNAL_ENDIAN_WIN_HPP

#include <cstdint>
#include <cstring>

#include <stdlib.h>

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< std::size_t S >
         struct to_and_from_le
         {
            template< typename T >
            static T convert( const T t ) noexcept
            {
               return t;
            }
         };

         template< std::size_t S >
         struct to_and_from_be;

         template<>
         struct to_and_from_be< 1 >
         {
            static std::int8_t convert( const std::int8_t n ) noexcept
            {
               return n;
            }

            static std::uint8_t convert( const std::uint8_t n ) noexcept
            {
               return n;
            }
         };

         template<>
         struct to_and_from_be< 2 >
         {
            static std::int16_t convert( const std::int16_t n ) noexcept
            {
               return std::int16_t( _byteswap_ushort( std::uint16_t( n ) ) );
            }

            static std::uint16_t convert( const std::uint16_t n ) noexcept
            {
               return _byteswap_ushort( n );
            }
         };

         template<>
         struct to_and_from_be< 4 >
         {
            static float convert( float n ) noexcept
            {
               std::uint32_t u;
               std::memcpy( &u, &n, 4 );
               u = convert( u );
               std::memcpy( &n, &u, 4 );
               return n;
            }

            static std::int32_t convert( const std::int32_t n ) noexcept
            {
               return std::int32_t( _byteswap_ulong( std::uint32_t( n ) ) );
            }

            static std::uint32_t convert( const std::uint32_t n ) noexcept
            {
               return _byteswap_ulong( n );
            }
         };

         template<>
         struct to_and_from_be< 8 >
         {
            static double convert( double n ) noexcept
            {
               std::uint64_t u;
               std::memcpy( &u, &n, 8 );
               u = convert( u );
               std::memcpy( &n, &u, 8 );
               return n;
            }

            static std::int64_t convert( const std::int64_t n ) noexcept
            {
               return std::int64_t( _byteswap_uint64( std::uint64_t( n ) ) );
            }

            static std::uint64_t convert( const std::uint64_t n ) noexcept
            {
               return _byteswap_uint64( n );
            }
         };

#define TAO_PEGTL_NATIVE_ORDER le
#define TAO_PEGTL_NATIVE_UTF16 utf16_le
#define TAO_PEGTL_NATIVE_UTF32 utf32_le

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 14 "tao/pegtl/internal/endian.hpp"
#else
#line 1 "tao/pegtl/internal/endian_gcc.hpp"
       
#line 1 "tao/pegtl/internal/endian_gcc.hpp"



#ifndef TAO_PEGTL_INTERNAL_ENDIAN_GCC_HPP
#define TAO_PEGTL_INTERNAL_ENDIAN_GCC_HPP

#include <cstdint>
#include <cstring>

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
#if !defined( __BYTE_ORDER__ )
#error No byte order defined!
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

         template< std::size_t S >
         struct to_and_from_be
         {
            template< typename T >
            static T convert( const T n ) noexcept
            {
               return n;
            }
         };

         template< std::size_t S >
         struct to_and_from_le;

         template<>
         struct to_and_from_le< 1 >
         {
            static std::uint8_t convert( const std::uint8_t n ) noexcept
            {
               return n;
            }

            static std::int8_t convert( const std::int8_t n ) noexcept
            {
               return n;
            }
         };

         template<>
         struct to_and_from_le< 2 >
         {
            static std::int16_t convert( const std::int16_t n ) noexcept
            {
               return static_cast< std::int16_t >( __builtin_bswap16( static_cast< std::uint16_t >( n ) ) );
            }

            static std::uint16_t convert( const std::uint16_t n ) noexcept
            {
               return __builtin_bswap16( n );
            }
         };

         template<>
         struct to_and_from_le< 4 >
         {
            static float convert( float n ) noexcept
            {
               std::uint32_t u;
               std::memcpy( &u, &n, 4 );
               u = convert( u );
               std::memcpy( &n, &u, 4 );
               return n;
            }

            static std::int32_t convert( const std::int32_t n ) noexcept
            {
               return static_cast< std::int32_t >( __builtin_bswap32( static_cast< std::uint32_t >( n ) ) );
            }

            static std::uint32_t convert( const std::uint32_t n ) noexcept
            {
               return __builtin_bswap32( n );
            }
         };

         template<>
         struct to_and_from_le< 8 >
         {
            static double convert( double n ) noexcept
            {
               std::uint64_t u;
               std::memcpy( &u, &n, 8 );
               u = convert( u );
               std::memcpy( &n, &u, 8 );
               return n;
            }

            static std::int64_t convert( const std::int64_t n ) noexcept
            {
               return static_cast< std::int64_t >( __builtin_bswap64( static_cast< std::uint64_t >( n ) ) );
            }

            static std::uint64_t convert( const std::uint64_t n ) noexcept
            {
               return __builtin_bswap64( n );
            }
         };

#define TAO_PEGTL_NATIVE_ORDER be
#define TAO_PEGTL_NATIVE_UTF16 utf16_be
#define TAO_PEGTL_NATIVE_UTF32 utf32_be

#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

         template< std::size_t S >
         struct to_and_from_le
         {
            template< typename T >
            static T convert( const T n ) noexcept
            {
               return n;
            }
         };

         template< std::size_t S >
         struct to_and_from_be;

         template<>
         struct to_and_from_be< 1 >
         {
            static std::int8_t convert( const std::int8_t n ) noexcept
            {
               return n;
            }

            static std::uint8_t convert( const std::uint8_t n ) noexcept
            {
               return n;
            }
         };

         template<>
         struct to_and_from_be< 2 >
         {
            static std::int16_t convert( const std::int16_t n ) noexcept
            {
               return static_cast< std::int16_t >( __builtin_bswap16( static_cast< std::uint16_t >( n ) ) );
            }

            static std::uint16_t convert( const std::uint16_t n ) noexcept
            {
               return __builtin_bswap16( n );
            }
         };

         template<>
         struct to_and_from_be< 4 >
         {
            static float convert( float n ) noexcept
            {
               std::uint32_t u;
               std::memcpy( &u, &n, 4 );
               u = convert( u );
               std::memcpy( &n, &u, 4 );
               return n;
            }

            static std::int32_t convert( const std::int32_t n ) noexcept
            {
               return static_cast< std::int32_t >( __builtin_bswap32( static_cast< std::uint32_t >( n ) ) );
            }

            static std::uint32_t convert( const std::uint32_t n ) noexcept
            {
               return __builtin_bswap32( n );
            }
         };

         template<>
         struct to_and_from_be< 8 >
         {
            static double convert( double n ) noexcept
            {
               std::uint64_t u;
               std::memcpy( &u, &n, 8 );
               u = convert( u );
               std::memcpy( &n, &u, 8 );
               return n;
            }

            static std::int64_t convert( const std::int64_t n ) noexcept
            {
               return static_cast< std::int64_t >( __builtin_bswap64( static_cast< std::uint64_t >( n ) ) );
            }

            static std::uint64_t convert( const std::uint64_t n ) noexcept
            {
               return __builtin_bswap64( n );
            }
         };

#define TAO_PEGTL_NATIVE_ORDER le
#define TAO_PEGTL_NATIVE_UTF16 utf16_le
#define TAO_PEGTL_NATIVE_UTF32 utf32_le

#else
#error Unknown host byte order!
#endif

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 16 "tao/pegtl/internal/endian.hpp"
#endif

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename N >
         N h_to_be( const N n ) noexcept
         {
            return N( to_and_from_be< sizeof( N ) >::convert( n ) );
         }

         template< typename N >
         N be_to_h( const N n ) noexcept
         {
            return h_to_be( n );
         }

         template< typename N >
         N be_to_h( const void* p ) noexcept
         {
            N n;
            std::memcpy( &n, p, sizeof( n ) );
            return internal::be_to_h( n );
         }

         template< typename N >
         N h_to_le( const N n ) noexcept
         {
            return N( to_and_from_le< sizeof( N ) >::convert( n ) );
         }

         template< typename N >
         N le_to_h( const N n ) noexcept
         {
            return h_to_le( n );
         }

         template< typename N >
         N le_to_h( const void* p ) noexcept
         {
            N n;
            std::memcpy( &n, p, sizeof( n ) );
            return internal::le_to_h( n );
         }

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/internal/read_uint.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct read_uint16_be
         {
            using type = std::uint16_t;

            static std::uint16_t read( const void* d ) noexcept
            {
               return be_to_h< std::uint16_t >( d );
            }
         };

         struct read_uint16_le
         {
            using type = std::uint16_t;

            static std::uint16_t read( const void* d ) noexcept
            {
               return le_to_h< std::uint16_t >( d );
            }
         };

         struct read_uint32_be
         {
            using type = std::uint32_t;

            static std::uint32_t read( const void* d ) noexcept
            {
               return be_to_h< std::uint32_t >( d );
            }
         };

         struct read_uint32_le
         {
            using type = std::uint32_t;

            static std::uint32_t read( const void* d ) noexcept
            {
               return le_to_h< std::uint32_t >( d );
            }
         };

         struct read_uint64_be
         {
            using type = std::uint64_t;

            static std::uint64_t read( const void* d ) noexcept
            {
               return be_to_h< std::uint64_t >( d );
            }
         };

         struct read_uint64_le
         {
            using type = std::uint64_t;

            static std::uint64_t read( const void* d ) noexcept
            {
               return le_to_h< std::uint64_t >( d );
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 14 "tao/pegtl/internal/peek_mask_uint.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename R, typename R::type M >
         struct peek_mask_uint_impl
         {
            using data_t = typename R::type;
            using pair_t = input_pair< data_t >;

            template< typename Input >
            static pair_t peek( Input& in ) noexcept( noexcept( in.size( sizeof( data_t ) ) ) )
            {
               if( in.size( sizeof( data_t ) ) < sizeof( data_t ) ) {
                  return { 0, 0 };
               }
               const data_t data = R::read( in.current() ) & M;
               return { data, sizeof( data_t ) };
            }
         };

         template< std::uint16_t M >
         using peek_mask_uint16_be = peek_mask_uint_impl< read_uint16_be, M >;

         template< std::uint16_t M >
         using peek_mask_uint16_le = peek_mask_uint_impl< read_uint16_le, M >;

         template< std::uint32_t M >
         using peek_mask_uint32_be = peek_mask_uint_impl< read_uint32_be, M >;

         template< std::uint32_t M >
         using peek_mask_uint32_le = peek_mask_uint_impl< read_uint32_le, M >;

         template< std::uint64_t M >
         using peek_mask_uint64_be = peek_mask_uint_impl< read_uint64_be, M >;

         template< std::uint64_t M >
         using peek_mask_uint64_le = peek_mask_uint_impl< read_uint64_le, M >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/uint16.hpp"
#line 1 "tao/pegtl/internal/peek_uint.hpp"
       
#line 1 "tao/pegtl/internal/peek_uint.hpp"



#ifndef TAO_PEGTL_INTERNAL_PEEK_UINT_HPP
#define TAO_PEGTL_INTERNAL_PEEK_UINT_HPP

#include <cstddef>
#include <cstdint>






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename R >
         struct peek_uint_impl
         {
            using data_t = typename R::type;
            using pair_t = input_pair< data_t >;

            template< typename Input >
            static pair_t peek( Input& in ) noexcept( noexcept( in.size( sizeof( data_t ) ) ) )
            {
               if( in.size( sizeof( data_t ) ) < sizeof( data_t ) ) {
                  return { 0, 0 };
               }
               const data_t data = R::read( in.current() );
               return { data, sizeof( data_t ) };
            }
         };

         using peek_uint16_be = peek_uint_impl< read_uint16_be >;
         using peek_uint16_le = peek_uint_impl< read_uint16_le >;

         using peek_uint32_be = peek_uint_impl< read_uint32_be >;
         using peek_uint32_le = peek_uint_impl< read_uint32_le >;

         using peek_uint64_be = peek_uint_impl< read_uint64_be >;
         using peek_uint64_le = peek_uint_impl< read_uint64_le >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 11 "tao/pegtl/uint16.hpp"



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace uint16_be
      {
         // clang-format off
         struct any : internal::any< internal::peek_uint16_be > {};

         template< std::uint16_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_uint16_be, Cs... > {};
         template< std::uint16_t Lo, std::uint16_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_uint16_be, Lo, Hi > {};
         template< std::uint16_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_uint16_be, Cs... > {};
         template< std::uint16_t Lo, std::uint16_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_uint16_be, Lo, Hi > {};
         template< std::uint16_t... Cs > struct ranges : internal::ranges< internal::peek_uint16_be, Cs... > {};
         template< std::uint16_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_uint16_be, Cs >... > {};

         template< std::uint16_t M, std::uint16_t... Cs > struct mask_not_one : internal::one< internal::result_on_found::failure, internal::peek_mask_uint16_be< M >, Cs... > {};
         template< std::uint16_t M, std::uint16_t Lo, std::uint16_t Hi > struct mask_not_range : internal::range< internal::result_on_found::failure, internal::peek_mask_uint16_be< M >, Lo, Hi > {};
         template< std::uint16_t M, std::uint16_t... Cs > struct mask_one : internal::one< internal::result_on_found::success, internal::peek_mask_uint16_be< M >, Cs... > {};
         template< std::uint16_t M, std::uint16_t Lo, std::uint16_t Hi > struct mask_range : internal::range< internal::result_on_found::success, internal::peek_mask_uint16_be< M >, Lo, Hi > {};
         template< std::uint16_t M, std::uint16_t... Cs > struct mask_ranges : internal::ranges< internal::peek_mask_uint16_be< M >, Cs... > {};
         template< std::uint16_t M, std::uint16_t... Cs > struct mask_string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_mask_uint16_be< M >, Cs >... > {};
         // clang-format on

      } // namespace uint16_be

      namespace uint16_le
      {
         // clang-format off
         struct any : internal::any< internal::peek_uint16_le > {};

         template< std::uint16_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_uint16_le, Cs... > {};
         template< std::uint16_t Lo, std::uint16_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_uint16_le, Lo, Hi > {};
         template< std::uint16_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_uint16_le, Cs... > {};
         template< std::uint16_t Lo, std::uint16_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_uint16_le, Lo, Hi > {};
         template< std::uint16_t... Cs > struct ranges : internal::ranges< internal::peek_uint16_le, Cs... > {};
         template< std::uint16_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_uint16_le, Cs >... > {};

         template< std::uint16_t M, std::uint16_t... Cs > struct mask_not_one : internal::one< internal::result_on_found::failure, internal::peek_mask_uint16_le< M >, Cs... > {};
         template< std::uint16_t M, std::uint16_t Lo, std::uint16_t Hi > struct mask_not_range : internal::range< internal::result_on_found::failure, internal::peek_mask_uint16_le< M >, Lo, Hi > {};
         template< std::uint16_t M, std::uint16_t... Cs > struct mask_one : internal::one< internal::result_on_found::success, internal::peek_mask_uint16_le< M >, Cs... > {};
         template< std::uint16_t M, std::uint16_t Lo, std::uint16_t Hi > struct mask_range : internal::range< internal::result_on_found::success, internal::peek_mask_uint16_le< M >, Lo, Hi > {};
         template< std::uint16_t M, std::uint16_t... Cs > struct mask_ranges : internal::ranges< internal::peek_mask_uint16_le< M >, Cs... > {};
         template< std::uint16_t M, std::uint16_t... Cs > struct mask_string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_mask_uint16_le< M >, Cs >... > {};
         // clang-format on

      } // namespace uint16_le

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 15 "tao/pegtl.hpp"
#line 1 "tao/pegtl/uint32.hpp"
       
#line 1 "tao/pegtl/uint32.hpp"



#ifndef TAO_PEGTL_UINT32_HPP
#define TAO_PEGTL_UINT32_HPP
#line 14 "tao/pegtl/uint32.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace uint32_be
      {
         // clang-format off
         struct any : internal::any< internal::peek_uint32_be > {};

         template< std::uint32_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_uint32_be, Cs... > {};
         template< std::uint32_t Lo, std::uint32_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_uint32_be, Lo, Hi > {};
         template< std::uint32_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_uint32_be, Cs... > {};
         template< std::uint32_t Lo, std::uint32_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_uint32_be, Lo, Hi > {};
         template< std::uint32_t... Cs > struct ranges : internal::ranges< internal::peek_uint32_be, Cs... > {};
         template< std::uint32_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_uint32_be, Cs >... > {};

         template< std::uint32_t M, std::uint32_t... Cs > struct mask_not_one : internal::one< internal::result_on_found::failure, internal::peek_mask_uint32_be< M >, Cs... > {};
         template< std::uint32_t M, std::uint32_t Lo, std::uint32_t Hi > struct mask_not_range : internal::range< internal::result_on_found::failure, internal::peek_mask_uint32_be< M >, Lo, Hi > {};
         template< std::uint32_t M, std::uint32_t... Cs > struct mask_one : internal::one< internal::result_on_found::success, internal::peek_mask_uint32_be< M >, Cs... > {};
         template< std::uint32_t M, std::uint32_t Lo, std::uint32_t Hi > struct mask_range : internal::range< internal::result_on_found::success, internal::peek_mask_uint32_be< M >, Lo, Hi > {};
         template< std::uint32_t M, std::uint32_t... Cs > struct mask_ranges : internal::ranges< internal::peek_mask_uint32_be< M >, Cs... > {};
         template< std::uint32_t M, std::uint32_t... Cs > struct mask_string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_mask_uint32_be< M >, Cs >... > {};
         // clang-format on

      } // namespace uint32_be

      namespace uint32_le
      {
         // clang-format off
         struct any : internal::any< internal::peek_uint32_le > {};

         template< std::uint32_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_uint32_le, Cs... > {};
         template< std::uint32_t Lo, std::uint32_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_uint32_le, Lo, Hi > {};
         template< std::uint32_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_uint32_le, Cs... > {};
         template< std::uint32_t Lo, std::uint32_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_uint32_le, Lo, Hi > {};
         template< std::uint32_t... Cs > struct ranges : internal::ranges< internal::peek_uint32_le, Cs... > {};
         template< std::uint32_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_uint32_le, Cs >... > {};

         template< std::uint32_t M, std::uint32_t... Cs > struct mask_not_one : internal::one< internal::result_on_found::failure, internal::peek_mask_uint32_le< M >, Cs... > {};
         template< std::uint32_t M, std::uint32_t Lo, std::uint32_t Hi > struct mask_not_range : internal::range< internal::result_on_found::failure, internal::peek_mask_uint32_le< M >, Lo, Hi > {};
         template< std::uint32_t M, std::uint32_t... Cs > struct mask_one : internal::one< internal::result_on_found::success, internal::peek_mask_uint32_le< M >, Cs... > {};
         template< std::uint32_t M, std::uint32_t Lo, std::uint32_t Hi > struct mask_range : internal::range< internal::result_on_found::success, internal::peek_mask_uint32_le< M >, Lo, Hi > {};
         template< std::uint32_t M, std::uint32_t... Cs > struct mask_ranges : internal::ranges< internal::peek_mask_uint32_le< M >, Cs... > {};
         template< std::uint32_t M, std::uint32_t... Cs > struct mask_string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_mask_uint32_le< M >, Cs >... > {};
         // clang-format on

      } // namespace uint32_le

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 16 "tao/pegtl.hpp"
#line 1 "tao/pegtl/uint64.hpp"
       
#line 1 "tao/pegtl/uint64.hpp"



#ifndef TAO_PEGTL_UINT64_HPP
#define TAO_PEGTL_UINT64_HPP
#line 14 "tao/pegtl/uint64.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace uint64_be
      {
         // clang-format off
         struct any : internal::any< internal::peek_uint64_be > {};

         template< std::uint64_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_uint64_be, Cs... > {};
         template< std::uint64_t Lo, std::uint64_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_uint64_be, Lo, Hi > {};
         template< std::uint64_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_uint64_be, Cs... > {};
         template< std::uint64_t Lo, std::uint64_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_uint64_be, Lo, Hi > {};
         template< std::uint64_t... Cs > struct ranges : internal::ranges< internal::peek_uint64_be, Cs... > {};
         template< std::uint64_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_uint64_be, Cs >... > {};


         template< std::uint64_t M, std::uint64_t... Cs > struct mask_not_one : internal::one< internal::result_on_found::failure, internal::peek_mask_uint64_be< M >, Cs... > {};
         template< std::uint64_t M, std::uint64_t Lo, std::uint64_t Hi > struct mask_not_range : internal::range< internal::result_on_found::failure, internal::peek_mask_uint64_be< M >, Lo, Hi > {};
         template< std::uint64_t M, std::uint64_t... Cs > struct mask_one : internal::one< internal::result_on_found::success, internal::peek_mask_uint64_be< M >, Cs... > {};
         template< std::uint64_t M, std::uint64_t Lo, std::uint64_t Hi > struct mask_range : internal::range< internal::result_on_found::success, internal::peek_mask_uint64_be< M >, Lo, Hi > {};
         template< std::uint64_t M, std::uint64_t... Cs > struct mask_ranges : internal::ranges< internal::peek_mask_uint64_be< M >, Cs... > {};
         template< std::uint64_t M, std::uint64_t... Cs > struct mask_string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_mask_uint64_be< M >, Cs >... > {};
         // clang-format on

      } // namespace uint64_be

      namespace uint64_le
      {
         // clang-format off
         struct any : internal::any< internal::peek_uint64_le > {};

         template< std::uint64_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_uint64_le, Cs... > {};
         template< std::uint64_t Lo, std::uint64_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_uint64_le, Lo, Hi > {};
         template< std::uint64_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_uint64_le, Cs... > {};
         template< std::uint64_t Lo, std::uint64_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_uint64_le, Lo, Hi > {};
         template< std::uint64_t... Cs > struct ranges : internal::ranges< internal::peek_uint64_le, Cs... > {};
         template< std::uint64_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_uint64_le, Cs >... > {};

         template< std::uint64_t M, std::uint64_t... Cs > struct mask_not_one : internal::one< internal::result_on_found::failure, internal::peek_mask_uint64_le< M >, Cs... > {};
         template< std::uint64_t M, std::uint64_t Lo, std::uint64_t Hi > struct mask_not_range : internal::range< internal::result_on_found::failure, internal::peek_mask_uint64_le< M >, Lo, Hi > {};
         template< std::uint64_t M, std::uint64_t... Cs > struct mask_one : internal::one< internal::result_on_found::success, internal::peek_mask_uint64_le< M >, Cs... > {};
         template< std::uint64_t M, std::uint64_t Lo, std::uint64_t Hi > struct mask_range : internal::range< internal::result_on_found::success, internal::peek_mask_uint64_le< M >, Lo, Hi > {};
         template< std::uint64_t M, std::uint64_t... Cs > struct mask_ranges : internal::ranges< internal::peek_mask_uint64_le< M >, Cs... > {};
         template< std::uint64_t M, std::uint64_t... Cs > struct mask_string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_mask_uint64_le< M >, Cs >... > {};
         // clang-format on

      } // namespace uint64_le

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 17 "tao/pegtl.hpp"
#line 1 "tao/pegtl/uint8.hpp"
       
#line 1 "tao/pegtl/uint8.hpp"



#ifndef TAO_PEGTL_UINT8_HPP
#define TAO_PEGTL_UINT8_HPP



#line 1 "tao/pegtl/internal/peek_mask_uint8.hpp"
       
#line 1 "tao/pegtl/internal/peek_mask_uint8.hpp"



#ifndef TAO_PEGTL_INTERNAL_PEEK_MASK_UINT8_HPP
#define TAO_PEGTL_INTERNAL_PEEK_MASK_UINT8_HPP

#include <cstddef>
#include <cstdint>





namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< std::uint8_t M >
         struct peek_mask_uint8
         {
            using data_t = std::uint8_t;
            using pair_t = input_pair< std::uint8_t >;

            template< typename Input >
            static pair_t peek( Input& in ) noexcept( noexcept( in.empty() ) )
            {
               if( in.empty() ) {
                  return { 0, 0 };
               }
               return { std::uint8_t( in.peek_uint8() & M ), 1 };
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/uint8.hpp"
#line 1 "tao/pegtl/internal/peek_uint8.hpp"
       
#line 1 "tao/pegtl/internal/peek_uint8.hpp"



#ifndef TAO_PEGTL_INTERNAL_PEEK_UINT8_HPP
#define TAO_PEGTL_INTERNAL_PEEK_UINT8_HPP

#include <cstddef>
#include <cstdint>





namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct peek_uint8
         {
            using data_t = std::uint8_t;
            using pair_t = input_pair< std::uint8_t >;

            template< typename Input >
            static pair_t peek( Input& in ) noexcept( noexcept( in.empty() ) )
            {
               if( in.empty() ) {
                  return { 0, 0 };
               }
               return { in.peek_uint8(), 1 };
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 11 "tao/pegtl/uint8.hpp"



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace uint8
      {
         // clang-format off
         struct any : internal::any< internal::peek_uint8 > {};

         template< std::uint8_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_uint8, Cs... > {};
         template< std::uint8_t Lo, std::uint8_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_uint8, Lo, Hi > {};
         template< std::uint8_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_uint8, Cs... > {};
         template< std::uint8_t Lo, std::uint8_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_uint8, Lo, Hi > {};
         template< std::uint8_t... Cs > struct ranges : internal::ranges< internal::peek_uint8, Cs... > {};
         template< std::uint8_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_uint8, Cs >... > {};

         template< std::uint8_t M, std::uint8_t... Cs > struct mask_not_one : internal::one< internal::result_on_found::failure, internal::peek_mask_uint8< M >, Cs... > {};
         template< std::uint8_t M, std::uint8_t Lo, std::uint8_t Hi > struct mask_not_range : internal::range< internal::result_on_found::failure, internal::peek_mask_uint8< M >, Lo, Hi > {};
         template< std::uint8_t M, std::uint8_t... Cs > struct mask_one : internal::one< internal::result_on_found::success, internal::peek_mask_uint8< M >, Cs... > {};
         template< std::uint8_t M, std::uint8_t Lo, std::uint8_t Hi > struct mask_range : internal::range< internal::result_on_found::success, internal::peek_mask_uint8< M >, Lo, Hi > {};
         template< std::uint8_t M, std::uint8_t... Cs > struct mask_ranges : internal::ranges< internal::peek_mask_uint8< M >, Cs... > {};
         template< std::uint8_t M, std::uint8_t... Cs > struct mask_string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_mask_uint8< M >, Cs >... > {};
         // clang-format on

      } // namespace uint8

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 18 "tao/pegtl.hpp"
#line 1 "tao/pegtl/utf16.hpp"
       
#line 1 "tao/pegtl/utf16.hpp"



#ifndef TAO_PEGTL_UTF16_HPP
#define TAO_PEGTL_UTF16_HPP



#line 1 "tao/pegtl/internal/peek_utf16.hpp"
       
#line 1 "tao/pegtl/internal/peek_utf16.hpp"



#ifndef TAO_PEGTL_INTERNAL_PEEK_UTF16_HPP
#define TAO_PEGTL_INTERNAL_PEEK_UTF16_HPP

#include <type_traits>






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename R >
         struct peek_utf16_impl
         {
            using data_t = char32_t;
            using pair_t = input_pair< char32_t >;

            using short_t = std::make_unsigned< char16_t >::type;

            static_assert( sizeof( short_t ) == 2, "expected size 2 for 16bit value" );
            static_assert( sizeof( char16_t ) == 2, "expected size 2 for 16bit value" );

            template< typename Input >
            static pair_t peek( Input& in ) noexcept( noexcept( in.size( 4 ) ) )
            {
               if( in.size( 2 ) < 2 ) {
                  return { 0, 0 };
               }
               const char32_t t = R::read( in.current() );
               if( ( t < 0xd800 ) || ( t > 0xdfff ) ) {
                  return { t, 2 };
               }
               if( ( t >= 0xdc00 ) || ( in.size( 4 ) < 4 ) ) {
                  return { 0, 0 };
               }
               const char32_t u = R::read( in.current() + 2 );
               if( ( u >= 0xdc00 ) && ( u <= 0xdfff ) ) {
                  const auto cp = ( ( ( t & 0x03ff ) << 10 ) | ( u & 0x03ff ) ) + 0x10000;
                  return { cp, 4 };
               }
               return { 0, 0 };
            }
         };

         using peek_utf16_be = peek_utf16_impl< read_uint16_be >;
         using peek_utf16_le = peek_utf16_impl< read_uint16_le >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/utf16.hpp"



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace utf16_be
      {
         // clang-format off
         struct any : internal::any< internal::peek_utf16_be > {};
         struct bom : internal::one< internal::result_on_found::success, internal::peek_utf16_be, 0xfeff > {};
         template< char32_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_utf16_be, Cs... > {};
         template< char32_t Lo, char32_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_utf16_be, Lo, Hi > {};
         template< char32_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_utf16_be, Cs... > {};
         template< char32_t Lo, char32_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_utf16_be, Lo, Hi > {};
         template< char32_t... Cs > struct ranges : internal::ranges< internal::peek_utf16_be, Cs... > {};
         template< char32_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_utf16_be, Cs >... > {};
         // clang-format on

      } // namespace utf16_be

      namespace utf16_le
      {
         // clang-format off
         struct any : internal::any< internal::peek_utf16_le > {};
         struct bom : internal::one< internal::result_on_found::success, internal::peek_utf16_le, 0xfeff > {};
         template< char32_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_utf16_le, Cs... > {};
         template< char32_t Lo, char32_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_utf16_le, Lo, Hi > {};
         template< char32_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_utf16_le, Cs... > {};
         template< char32_t Lo, char32_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_utf16_le, Lo, Hi > {};
         template< char32_t... Cs > struct ranges : internal::ranges< internal::peek_utf16_le, Cs... > {};
         template< char32_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_utf16_le, Cs >... > {};
         // clang-format on

      } // namespace utf16_le

      namespace utf16 = TAO_PEGTL_NATIVE_UTF16;

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 19 "tao/pegtl.hpp"
#line 1 "tao/pegtl/utf32.hpp"
       
#line 1 "tao/pegtl/utf32.hpp"



#ifndef TAO_PEGTL_UTF32_HPP
#define TAO_PEGTL_UTF32_HPP



#line 1 "tao/pegtl/internal/peek_utf32.hpp"
       
#line 1 "tao/pegtl/internal/peek_utf32.hpp"



#ifndef TAO_PEGTL_INTERNAL_PEEK_UTF32_HPP
#define TAO_PEGTL_INTERNAL_PEEK_UTF32_HPP

#include <cstddef>






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename R >
         struct peek_utf32_impl
         {
            using data_t = char32_t;
            using pair_t = input_pair< char32_t >;

            static_assert( sizeof( char32_t ) == 4, "expected size 4 for 32bit value" );

            template< typename Input >
            static pair_t peek( Input& in ) noexcept( noexcept( in.size( 4 ) ) )
            {
               if( in.size( 4 ) < 4 ) {
                  return { 0, 0 };
               }
               const char32_t t = R::read( in.current() );
               if( ( t <= 0x10ffff ) && !( t >= 0xd800 && t <= 0xdfff ) ) {
                  return { t, 4 };
               }
               return { 0, 0 };
            }
         };

         using peek_utf32_be = peek_utf32_impl< read_uint32_be >;
         using peek_utf32_le = peek_utf32_impl< read_uint32_le >;

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/utf32.hpp"



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace utf32_be
      {
         // clang-format off
         struct any : internal::any< internal::peek_utf32_be > {};
         struct bom : internal::one< internal::result_on_found::success, internal::peek_utf32_be, 0xfeff > {};
         template< char32_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_utf32_be, Cs... > {};
         template< char32_t Lo, char32_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_utf32_be, Lo, Hi > {};
         template< char32_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_utf32_be, Cs... > {};
         template< char32_t Lo, char32_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_utf32_be, Lo, Hi > {};
         template< char32_t... Cs > struct ranges : internal::ranges< internal::peek_utf32_be, Cs... > {};
         template< char32_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_utf32_be, Cs >... > {};
         // clang-format on

      } // namespace utf32_be

      namespace utf32_le
      {
         // clang-format off
         struct any : internal::any< internal::peek_utf32_le > {};
         struct bom : internal::one< internal::result_on_found::success, internal::peek_utf32_le, 0xfeff > {};
         template< char32_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_utf32_le, Cs... > {};
         template< char32_t Lo, char32_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_utf32_le, Lo, Hi > {};
         template< char32_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_utf32_le, Cs... > {};
         template< char32_t Lo, char32_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_utf32_le, Lo, Hi > {};
         template< char32_t... Cs > struct ranges : internal::ranges< internal::peek_utf32_le, Cs... > {};
         template< char32_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_utf32_le, Cs >... > {};
         // clang-format on

      } // namespace utf32_le

      namespace utf32 = TAO_PEGTL_NATIVE_UTF32;

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 20 "tao/pegtl.hpp"
#line 1 "tao/pegtl/utf8.hpp"
       
#line 1 "tao/pegtl/utf8.hpp"



#ifndef TAO_PEGTL_UTF8_HPP
#define TAO_PEGTL_UTF8_HPP



#line 1 "tao/pegtl/internal/peek_utf8.hpp"
       
#line 1 "tao/pegtl/internal/peek_utf8.hpp"



#ifndef TAO_PEGTL_INTERNAL_PEEK_UTF8_HPP
#define TAO_PEGTL_INTERNAL_PEEK_UTF8_HPP





namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct peek_utf8
         {
            using data_t = char32_t;
            using pair_t = input_pair< char32_t >;

            template< typename Input >
            static pair_t peek( Input& in ) noexcept( noexcept( in.empty() ) )
            {
               if( in.empty() ) {
                  return { 0, 0 };
               }
               char32_t c0 = in.peek_uint8();
               if( ( c0 & 0x80 ) == 0 ) {
                  return { c0, 1 };
               }
               return peek_impl( in, c0 );
            }

         private:
            template< typename Input >
            static pair_t peek_impl( Input& in, char32_t c0 ) noexcept( noexcept( in.size( 4 ) ) )
            {
               if( ( c0 & 0xE0 ) == 0xC0 ) {
                  if( in.size( 2 ) >= 2 ) {
                     const char32_t c1 = in.peek_uint8( 1 );
                     if( ( c1 & 0xC0 ) == 0x80 ) {
                        c0 &= 0x1F;
                        c0 <<= 6;
                        c0 |= ( c1 & 0x3F );
                        if( c0 >= 0x80 ) {
                           return { c0, 2 };
                        }
                     }
                  }
               }
               else if( ( c0 & 0xF0 ) == 0xE0 ) {
                  if( in.size( 3 ) >= 3 ) {
                     const char32_t c1 = in.peek_uint8( 1 );
                     const char32_t c2 = in.peek_uint8( 2 );
                     if( ( ( c1 & 0xC0 ) == 0x80 ) && ( ( c2 & 0xC0 ) == 0x80 ) ) {
                        c0 &= 0x0F;
                        c0 <<= 6;
                        c0 |= ( c1 & 0x3F );
                        c0 <<= 6;
                        c0 |= ( c2 & 0x3F );
                        if( c0 >= 0x800 && !( c0 >= 0xD800 && c0 <= 0xDFFF ) ) {
                           return { c0, 3 };
                        }
                     }
                  }
               }
               else if( ( c0 & 0xF8 ) == 0xF0 ) {
                  if( in.size( 4 ) >= 4 ) {
                     const char32_t c1 = in.peek_uint8( 1 );
                     const char32_t c2 = in.peek_uint8( 2 );
                     const char32_t c3 = in.peek_uint8( 3 );
                     if( ( ( c1 & 0xC0 ) == 0x80 ) && ( ( c2 & 0xC0 ) == 0x80 ) && ( ( c3 & 0xC0 ) == 0x80 ) ) {
                        c0 &= 0x07;
                        c0 <<= 6;
                        c0 |= ( c1 & 0x3F );
                        c0 <<= 6;
                        c0 |= ( c2 & 0x3F );
                        c0 <<= 6;
                        c0 |= ( c3 & 0x3F );
                        if( c0 >= 0x10000 && c0 <= 0x10FFFF ) {
                           return { c0, 4 };
                        }
                     }
                  }
               }
               return { 0, 0 };
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/utf8.hpp"



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace utf8
      {
         // clang-format off
         struct any : internal::any< internal::peek_utf8 > {};
         struct bom : internal::one< internal::result_on_found::success, internal::peek_utf8, 0xfeff > {};
         template< char32_t... Cs > struct not_one : internal::one< internal::result_on_found::failure, internal::peek_utf8, Cs... > {};
         template< char32_t Lo, char32_t Hi > struct not_range : internal::range< internal::result_on_found::failure, internal::peek_utf8, Lo, Hi > {};
         template< char32_t... Cs > struct one : internal::one< internal::result_on_found::success, internal::peek_utf8, Cs... > {};
         template< char32_t Lo, char32_t Hi > struct range : internal::range< internal::result_on_found::success, internal::peek_utf8, Lo, Hi > {};
         template< char32_t... Cs > struct ranges : internal::ranges< internal::peek_utf8, Cs... > {};
         template< char32_t... Cs > struct string : internal::seq< internal::one< internal::result_on_found::success, internal::peek_utf8, Cs >... > {};
         // clang-format on

      } // namespace utf8

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 21 "tao/pegtl.hpp"

#line 1 "tao/pegtl/argv_input.hpp"
       
#line 1 "tao/pegtl/argv_input.hpp"



#ifndef TAO_PEGTL_ARGV_INPUT_HPP
#define TAO_PEGTL_ARGV_INPUT_HPP

#include <cstddef>
#include <sstream>
#include <string>
#include <utility>






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         inline std::string make_argv_source( const std::size_t argn )
         {
            std::ostringstream os;
            os << "argv[" << argn << ']';
            return os.str();
         }

      } // namespace internal

      template< tracking_mode P = tracking_mode::eager, typename Eol = eol::lf_crlf >
      struct argv_input
         : public memory_input< P, Eol >
      {
         template< typename T >
         argv_input( char** argv, const std::size_t argn, T&& in_source )
            : memory_input< P, Eol >( static_cast< const char* >( argv[ argn ] ), std::forward< T >( in_source ) )
         {
         }

         argv_input( char** argv, const std::size_t argn )
            : argv_input( argv, argn, internal::make_argv_source( argn ) )
         {
         }
      };

#ifdef __cpp_deduction_guides
      template< typename... Ts >
      argv_input( Ts&&... )->argv_input<>;
#endif

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 23 "tao/pegtl.hpp"
#line 1 "tao/pegtl/buffer_input.hpp"
       
#line 1 "tao/pegtl/buffer_input.hpp"



#ifndef TAO_PEGTL_BUFFER_INPUT_HPP
#define TAO_PEGTL_BUFFER_INPUT_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#line 27 "tao/pegtl/buffer_input.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename Reader, typename Eol = eol::lf_crlf, typename Source = std::string, std::size_t Chunk = 64 >
      class buffer_input
      {
      public:
         using reader_t = Reader;

         using eol_t = Eol;
         using source_t = Source;

         using iterator_t = internal::iterator;

         using action_t = internal::action_input< buffer_input >;

         static constexpr std::size_t chunk_size = Chunk;
         static constexpr tracking_mode tracking_mode_v = tracking_mode::eager;

         template< typename T, typename... As >
         buffer_input( T&& in_source, const std::size_t maximum, As&&... as )
            : m_reader( std::forward< As >( as )... ),
              m_maximum( maximum + Chunk ),
              m_buffer( new char[ maximum + Chunk ] ),
              m_current( m_buffer.get() ),
              m_end( m_buffer.get() ),
              m_source( std::forward< T >( in_source ) )
         {
            static_assert( Chunk != 0, "zero chunk size not implemented" );
            assert( m_maximum > maximum ); // Catches overflow; change to >= when zero chunk size is implemented.
         }

         buffer_input( const buffer_input& ) = delete;
         buffer_input( buffer_input&& ) = delete;

         ~buffer_input() = default;

         void operator=( const buffer_input& ) = delete;
         void operator=( buffer_input&& ) = delete;

         bool empty()
         {
            require( 1 );
            return m_current.data == m_end;
         }

         std::size_t size( const std::size_t amount )
         {
            require( amount );
            return buffer_occupied();
         }

         const char* current() const noexcept
         {
            return m_current.data;
         }

         const char* end( const std::size_t amount )
         {
            require( amount );
            return m_end;
         }

         std::size_t byte() const noexcept
         {
            return m_current.byte;
         }

         std::size_t line() const noexcept
         {
            return m_current.line;
         }

         std::size_t byte_in_line() const noexcept
         {
            return m_current.byte_in_line;
         }

         const Source& source() const noexcept
         {
            return m_source;
         }

         char peek_char( const std::size_t offset = 0 ) const noexcept
         {
            return m_current.data[ offset ];
         }

         std::uint8_t peek_uint8( const std::size_t offset = 0 ) const noexcept
         {
            return static_cast< std::uint8_t >( peek_char( offset ) );
         }

         // Compatibility, remove with 3.0.0
         std::uint8_t peek_byte( const std::size_t offset = 0 ) const noexcept
         {
            return static_cast< std::uint8_t >( peek_char( offset ) );
         }

         void bump( const std::size_t in_count = 1 ) noexcept
         {
            internal::bump( m_current, in_count, Eol::ch );
         }

         void bump_in_this_line( const std::size_t in_count = 1 ) noexcept
         {
            internal::bump_in_this_line( m_current, in_count );
         }

         void bump_to_next_line( const std::size_t in_count = 1 ) noexcept
         {
            internal::bump_to_next_line( m_current, in_count );
         }

         void discard() noexcept
         {
            if( m_current.data > m_buffer.get() + Chunk ) {
               const auto s = m_end - m_current.data;
               std::memmove( m_buffer.get(), m_current.data, s );
               m_current.data = m_buffer.get();
               m_end = m_buffer.get() + s;
            }
         }

         void require( const std::size_t amount )
         {
            if( m_current.data + amount <= m_end ) {
               return;
            }
            if( m_current.data + amount > m_buffer.get() + m_maximum ) {
               throw std::overflow_error( "require beyond end of buffer" );
            }
            if( const auto r = m_reader( m_end, ( std::min )( buffer_free_after_end(), ( std::max )( amount - buffer_occupied(), Chunk ) ) ) ) {
               m_end += r;
            }
         }

         template< rewind_mode M >
         internal::marker< iterator_t, M > mark() noexcept
         {
            return internal::marker< iterator_t, M >( m_current );
         }

         TAO_PEGTL_NAMESPACE::position position( const iterator_t& it ) const
         {
            return TAO_PEGTL_NAMESPACE::position( it, m_source );
         }

         TAO_PEGTL_NAMESPACE::position position() const
         {
            return position( m_current );
         }

         const iterator_t& iterator() const noexcept
         {
            return m_current;
         }

         std::size_t buffer_capacity() const noexcept
         {
            return m_maximum;
         }

         std::size_t buffer_occupied() const noexcept
         {
            assert( m_end >= m_current.data );
            return std::size_t( m_end - m_current.data );
         }

         std::size_t buffer_free_before_current() const noexcept
         {
            assert( m_current.data >= m_buffer.get() );
            return std::size_t( m_current.data - m_buffer.get() );
         }

         std::size_t buffer_free_after_end() const noexcept
         {
            assert( m_buffer.get() + m_maximum >= m_end );
            return std::size_t( m_buffer.get() + m_maximum - m_end );
         }

      private:
         Reader m_reader;
         std::size_t m_maximum;
         std::unique_ptr< char[] > m_buffer; // NOLINT
         iterator_t m_current;
         char* m_end;
         const Source m_source;
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 24 "tao/pegtl.hpp"
#line 1 "tao/pegtl/cstream_input.hpp"
       
#line 1 "tao/pegtl/cstream_input.hpp"



#ifndef TAO_PEGTL_CSTREAM_INPUT_HPP
#define TAO_PEGTL_CSTREAM_INPUT_HPP

#include <cstdio>





#line 1 "tao/pegtl/internal/cstream_reader.hpp"
       
#line 1 "tao/pegtl/internal/cstream_reader.hpp"



#ifndef TAO_PEGTL_INTERNAL_CSTREAM_READER_HPP
#define TAO_PEGTL_INTERNAL_CSTREAM_READER_HPP

#include <cassert>
#include <cstddef>
#include <cstdio>


#line 1 "tao/pegtl/internal/../input_error.hpp"
       
#line 1 "tao/pegtl/internal/../input_error.hpp"



#ifndef TAO_PEGTL_INPUT_ERROR_HPP
#define TAO_PEGTL_INPUT_ERROR_HPP

#include <cerrno>
#include <sstream>
#include <stdexcept>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct input_error
         : std::runtime_error
      {
         input_error( const std::string& message, const int in_errorno )
            : std::runtime_error( message ),
              errorno( in_errorno )
         {
         }

         int errorno;
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#define TAO_PEGTL_INTERNAL_UNWRAP( ... ) __VA_ARGS__

#define TAO_PEGTL_THROW_INPUT_ERROR( MESSAGE )                                             do {                                                                                       const int errorno = errno;                                                              std::ostringstream oss;                                                                 oss << "pegtl: " << TAO_PEGTL_INTERNAL_UNWRAP( MESSAGE ) << " errno " << errorno;       throw tao::TAO_PEGTL_NAMESPACE::input_error( oss.str(), errorno );                   } while( false )







#define TAO_PEGTL_THROW_INPUT_WIN32_ERROR( MESSAGE )                                                do {                                                                                                const int errorno = GetLastError();                                                              std::ostringstream oss;                                                                          oss << "pegtl: " << TAO_PEGTL_INTERNAL_UNWRAP( MESSAGE ) << " GetLastError() " << errorno;       throw tao::TAO_PEGTL_NAMESPACE::input_error( oss.str(), errorno );                            } while( false )







#endif
#line 13 "tao/pegtl/internal/cstream_reader.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct cstream_reader
         {
            explicit cstream_reader( std::FILE* s ) noexcept
               : m_cstream( s )
            {
               assert( m_cstream != nullptr );
            }

            std::size_t operator()( char* buffer, const std::size_t length ) const
            {
               if( const auto r = std::fread( buffer, 1, length, m_cstream ) ) {
                  return r;
               }
               if( std::feof( m_cstream ) != 0 ) {
                  return 0;
               }
               // Please contact us if you know how to provoke the following exception.
               // The example on cppreference.com doesn't work, at least not on macOS.
               TAO_PEGTL_THROW_INPUT_ERROR( "error in fread() from cstream" ); // LCOV_EXCL_LINE
            }

            std::FILE* m_cstream;
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 14 "tao/pegtl/cstream_input.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename Eol = eol::lf_crlf, std::size_t Chunk = 64 >
      struct cstream_input
         : buffer_input< internal::cstream_reader, Eol, std::string, Chunk >
      {
         template< typename T >
         cstream_input( std::FILE* in_stream, const std::size_t in_maximum, T&& in_source )
            : buffer_input< internal::cstream_reader, Eol, std::string, Chunk >( std::forward< T >( in_source ), in_maximum, in_stream )
         {
         }
      };

#ifdef __cpp_deduction_guides
      template< typename... Ts >
      cstream_input( Ts&&... )->cstream_input<>;
#endif

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 25 "tao/pegtl.hpp"
#line 1 "tao/pegtl/istream_input.hpp"
       
#line 1 "tao/pegtl/istream_input.hpp"



#ifndef TAO_PEGTL_ISTREAM_INPUT_HPP
#define TAO_PEGTL_ISTREAM_INPUT_HPP

#include <istream>





#line 1 "tao/pegtl/internal/istream_reader.hpp"
       
#line 1 "tao/pegtl/internal/istream_reader.hpp"



#ifndef TAO_PEGTL_INTERNAL_ISTREAM_READER_HPP
#define TAO_PEGTL_INTERNAL_ISTREAM_READER_HPP

#include <istream>




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct istream_reader
         {
            explicit istream_reader( std::istream& s ) noexcept
               : m_istream( s )
            {
            }

            std::size_t operator()( char* buffer, const std::size_t length )
            {
               m_istream.read( buffer, std::streamsize( length ) );

               if( const auto r = m_istream.gcount() ) {
                  return std::size_t( r );
               }
               if( m_istream.eof() ) {
                  return 0;
               }
               TAO_PEGTL_THROW_INPUT_ERROR( "error in istream.read()" );
            }

            std::istream& m_istream;
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 14 "tao/pegtl/istream_input.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename Eol = eol::lf_crlf, std::size_t Chunk = 64 >
      struct istream_input
         : buffer_input< internal::istream_reader, Eol, std::string, Chunk >
      {
         template< typename T >
         istream_input( std::istream& in_stream, const std::size_t in_maximum, T&& in_source )
            : buffer_input< internal::istream_reader, Eol, std::string, Chunk >( std::forward< T >( in_source ), in_maximum, in_stream )
         {
         }
      };

#ifdef __cpp_deduction_guides
      template< typename... Ts >
      istream_input( Ts&&... )->istream_input<>;
#endif

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 26 "tao/pegtl.hpp"

#line 1 "tao/pegtl/read_input.hpp"
       
#line 1 "tao/pegtl/read_input.hpp"



#ifndef TAO_PEGTL_READ_INPUT_HPP
#define TAO_PEGTL_READ_INPUT_HPP

#include <string>



#line 1 "tao/pegtl/string_input.hpp"
       
#line 1 "tao/pegtl/string_input.hpp"



#ifndef TAO_PEGTL_STRING_INPUT_HPP
#define TAO_PEGTL_STRING_INPUT_HPP

#include <string>
#include <utility>






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct string_holder
         {
            const std::string data;

            template< typename T >
            explicit string_holder( T&& in_data )
               : data( std::forward< T >( in_data ) )
            {
            }

            string_holder( const string_holder& ) = delete;
            string_holder( string_holder&& ) = delete;

            ~string_holder() = default;

            void operator=( const string_holder& ) = delete;
            void operator=( string_holder&& ) = delete;
         };

      } // namespace internal

      template< tracking_mode P = tracking_mode::eager, typename Eol = eol::lf_crlf, typename Source = std::string >
      struct string_input
         : private internal::string_holder,
           public memory_input< P, Eol, Source >
      {
         template< typename V, typename T, typename... Ts >
         explicit string_input( V&& in_data, T&& in_source, Ts&&... ts )
            : internal::string_holder( std::forward< V >( in_data ) ),
              memory_input< P, Eol, Source >( data.data(), data.size(), std::forward< T >( in_source ), std::forward< Ts >( ts )... )
         {
         }

         string_input( const string_input& ) = delete;
         string_input( string_input&& ) = delete;

         ~string_input() = default;

         void operator=( const string_input& ) = delete;
         void operator=( string_input&& ) = delete;
      };

#ifdef __cpp_deduction_guides
      template< typename... Ts >
      explicit string_input( Ts&&... )->string_input<>;
#endif

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "tao/pegtl/read_input.hpp"


#line 1 "tao/pegtl/internal/file_reader.hpp"
       
#line 1 "tao/pegtl/internal/file_reader.hpp"



#ifndef TAO_PEGTL_INTERNAL_FILE_READER_HPP
#define TAO_PEGTL_INTERNAL_FILE_READER_HPP

#include <cstdio>
#include <memory>
#include <string>
#include <utility>




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         inline std::FILE* file_open( const char* filename )
         {
            errno = 0;
#if defined( _MSC_VER )
            std::FILE* file;
            if( ::fopen_s( &file, filename, "rb" ) == 0 )
#elif defined( __MINGW32__ )
            if( auto* file = std::fopen( filename, "rb" ) ) // NOLINT
#else
            if( auto* file = std::fopen( filename, "rbe" ) ) // NOLINT
#endif
            {
               return file;
            }
            TAO_PEGTL_THROW_INPUT_ERROR( "unable to fopen() file " << filename << " for reading" );
         }

         struct file_close
         {
            void operator()( FILE* f ) const noexcept
            {
               std::fclose( f ); // NOLINT
            }
         };

         class file_reader
         {
         public:
            explicit file_reader( const char* filename )
               : m_source( filename ),
                 m_file( file_open( m_source ) )
            {
            }

            file_reader( FILE* file, const char* filename ) noexcept
               : m_source( filename ),
                 m_file( file )
            {
            }

            file_reader( const file_reader& ) = delete;
            file_reader( file_reader&& ) = delete;

            ~file_reader() = default;

            void operator=( const file_reader& ) = delete;
            void operator=( file_reader&& ) = delete;

            std::size_t size() const
            {
               errno = 0;
               if( std::fseek( m_file.get(), 0, SEEK_END ) != 0 ) {
                  TAO_PEGTL_THROW_INPUT_ERROR( "unable to fseek() to end of file " << m_source ); // LCOV_EXCL_LINE
               }
               errno = 0;
               const auto s = std::ftell( m_file.get() );
               if( s < 0 ) {
                  TAO_PEGTL_THROW_INPUT_ERROR( "unable to ftell() file size of file " << m_source ); // LCOV_EXCL_LINE
               }
               errno = 0;
               if( std::fseek( m_file.get(), 0, SEEK_SET ) != 0 ) {
                  TAO_PEGTL_THROW_INPUT_ERROR( "unable to fseek() to beginning of file " << m_source ); // LCOV_EXCL_LINE
               }
               return std::size_t( s );
            }

            std::string read() const
            {
               std::string nrv;
               nrv.resize( size() );
               errno = 0;
               if( !nrv.empty() && ( std::fread( &nrv[ 0 ], nrv.size(), 1, m_file.get() ) != 1 ) ) {
                  TAO_PEGTL_THROW_INPUT_ERROR( "unable to fread() file " << m_source << " size " << nrv.size() ); // LCOV_EXCL_LINE
               }
               return nrv;
            }

         private:
            const char* const m_source;
            const std::unique_ptr< std::FILE, file_close > m_file;
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 15 "tao/pegtl/read_input.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct filename_holder
         {
            const std::string filename;

            template< typename T >
            explicit filename_holder( T&& in_filename )
               : filename( std::forward< T >( in_filename ) )
            {
            }

            filename_holder( const filename_holder& ) = delete;
            filename_holder( filename_holder&& ) = delete;

            ~filename_holder() = default;

            void operator=( const filename_holder& ) = delete;
            void operator=( filename_holder&& ) = delete;
         };

      } // namespace internal

      template< tracking_mode P = tracking_mode::eager, typename Eol = eol::lf_crlf >
      struct read_input
         : private internal::filename_holder,
           public string_input< P, Eol, const char* >
      {
         template< typename T >
         explicit read_input( T&& in_filename )
            : internal::filename_holder( std::forward< T >( in_filename ) ),
              string_input< P, Eol, const char* >( internal::file_reader( filename.c_str() ).read(), filename.c_str() )
         {
         }

         template< typename T >
         read_input( FILE* in_file, T&& in_filename )
            : internal::filename_holder( std::forward< T >( in_filename ) ),
              string_input< P, Eol, const char* >( internal::file_reader( in_file, filename.c_str() ).read(), filename.c_str() )
         {
         }

         read_input( const read_input& ) = delete;
         read_input( read_input&& ) = delete;

         ~read_input() = default;

         void operator=( const read_input& ) = delete;
         void operator=( read_input&& ) = delete;
      };

#ifdef __cpp_deduction_guides
      template< typename... Ts >
      explicit read_input( Ts&&... )->read_input<>;
#endif

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 28 "tao/pegtl.hpp"


// this has to be included *after* the above inputs,
// otherwise the amalgamated header will not work!
#line 1 "tao/pegtl/file_input.hpp"
       
#line 1 "tao/pegtl/file_input.hpp"



#ifndef TAO_PEGTL_FILE_INPUT_HPP
#define TAO_PEGTL_FILE_INPUT_HPP





#if defined( __unix__ ) || ( defined( __APPLE__ ) && defined( __MACH__ ) )
#include <unistd.h>  // Required for _POSIX_MAPPED_FILES
#endif

#if defined( _POSIX_MAPPED_FILES ) || defined( _WIN32 )
#line 1 "tao/pegtl/mmap_input.hpp"
       
#line 1 "tao/pegtl/mmap_input.hpp"



#ifndef TAO_PEGTL_MMAP_INPUT_HPP
#define TAO_PEGTL_MMAP_INPUT_HPP

#include <string>
#include <utility>






#if defined( __unix__ ) || ( defined( __APPLE__ ) && defined( __MACH__ ) )
#include <unistd.h>  // Required for _POSIX_MAPPED_FILES
#endif

#if defined( _POSIX_MAPPED_FILES )
#line 1 "tao/pegtl/internal/file_mapper_posix.hpp"
       
#line 1 "tao/pegtl/internal/file_mapper_posix.hpp"



#ifndef TAO_PEGTL_INTERNAL_FILE_MAPPER_POSIX_HPP
#define TAO_PEGTL_INTERNAL_FILE_MAPPER_POSIX_HPP

#include <sys/mman.h>
#include <unistd.h>



#line 1 "tao/pegtl/internal/file_opener.hpp"
       
#line 1 "tao/pegtl/internal/file_opener.hpp"



#ifndef TAO_PEGTL_INTERNAL_FILE_OPENER_HPP
#define TAO_PEGTL_INTERNAL_FILE_OPENER_HPP

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct file_opener
         {
            explicit file_opener( const char* filename )
               : m_source( filename ),
                 m_fd( open() )
            {
            }

            file_opener( const file_opener& ) = delete;
            file_opener( file_opener&& ) = delete;

            ~file_opener() noexcept
            {
               ::close( m_fd );
            }

            void operator=( const file_opener& ) = delete;
            void operator=( file_opener&& ) = delete;

            std::size_t size() const
            {
               struct stat st; // NOLINT
               errno = 0;
               if( ::fstat( m_fd, &st ) < 0 ) {
                  TAO_PEGTL_THROW_INPUT_ERROR( "unable to fstat() file " << m_source << " descriptor " << m_fd );
               }
               return std::size_t( st.st_size );
            }

            const char* const m_source;
            const int m_fd;

         private:
            int open() const
            {
               errno = 0;
               const int fd = ::open( m_source, // NOLINT
                                      O_RDONLY
#ifdef O_CLOEXEC
                                         | O_CLOEXEC
#endif
               );
               if( fd >= 0 ) {
                  return fd;
               }
               TAO_PEGTL_THROW_INPUT_ERROR( "unable to open() file " << m_source << " for reading" );
            }
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 13 "tao/pegtl/internal/file_mapper_posix.hpp"



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         class file_mapper
         {
         public:
            explicit file_mapper( const char* filename )
               : file_mapper( file_opener( filename ) )
            {
            }

            explicit file_mapper( const file_opener& reader )
               : m_size( reader.size() ),
                 m_data( static_cast< const char* >( ::mmap( nullptr, m_size, PROT_READ, MAP_PRIVATE, reader.m_fd, 0 ) ) )
            {
               if( ( m_size != 0 ) && ( intptr_t( m_data ) == -1 ) ) {
                  TAO_PEGTL_THROW_INPUT_ERROR( "unable to mmap() file " << reader.m_source << " descriptor " << reader.m_fd );
               }
            }

            file_mapper( const file_mapper& ) = delete;
            file_mapper( file_mapper&& ) = delete;

            ~file_mapper() noexcept
            {
               // Legacy C interface requires pointer-to-mutable but does not write through the pointer.
               ::munmap( const_cast< char* >( m_data ), m_size ); // NOLINT
            }

            void operator=( const file_mapper& ) = delete;
            void operator=( file_mapper&& ) = delete;

            bool empty() const noexcept
            {
               return m_size == 0;
            }

            std::size_t size() const noexcept
            {
               return m_size;
            }

            using iterator = const char*;
            using const_iterator = const char*;

            iterator data() const noexcept
            {
               return m_data;
            }

            iterator begin() const noexcept
            {
               return m_data;
            }

            iterator end() const noexcept
            {
               return m_data + m_size;
            }

            std::string string() const
            {
               return std::string( m_data, m_size );
            }

         private:
            const std::size_t m_size;
            const char* const m_data;
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 21 "tao/pegtl/mmap_input.hpp"
#elif defined( _WIN32 )
#line 1 "tao/pegtl/internal/file_mapper_win32.hpp"
       
#line 1 "tao/pegtl/internal/file_mapper_win32.hpp"



#ifndef TAO_PEGTL_INTERNAL_FILE_MAPPER_WIN32_HPP
#define TAO_PEGTL_INTERNAL_FILE_MAPPER_WIN32_HPP

#if !defined( NOMINMAX )
#define NOMINMAX
#define TAO_PEGTL_NOMINMAX_WAS_DEFINED
#endif

#if !defined( WIN32_LEAN_AND_MEAN )
#define WIN32_LEAN_AND_MEAN
#define TAO_PEGTL_WIN32_LEAN_AND_MEAN_WAS_DEFINED
#endif

#include <windows.h>

#if defined( TAO_PEGTL_NOMINMAX_WAS_DEFINED )
#undef NOMINMAX
#undef TAO_PEGTL_NOMINMAX_WAS_DEFINED
#endif

#if defined( TAO_PEGTL_WIN32_LEAN_AND_MEAN_WAS_DEFINED )
#undef WIN32_LEAN_AND_MEAN
#undef TAO_PEGTL_WIN32_LEAN_AND_MEAN_WAS_DEFINED
#endif




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct win32_file_opener
         {
            explicit win32_file_opener( const char* filename )
               : m_source( filename ),
                 m_handle( open() )
            {
            }

            win32_file_opener( const win32_file_opener& ) = delete;
            win32_file_opener( win32_file_opener&& ) = delete;

            ~win32_file_opener() noexcept
            {
               ::CloseHandle( m_handle );
            }

            void operator=( const win32_file_opener& ) = delete;
            void operator=( win32_file_opener&& ) = delete;

            std::size_t size() const
            {
               LARGE_INTEGER size;
               if( !::GetFileSizeEx( m_handle, &size ) ) {
                  TAO_PEGTL_THROW_INPUT_WIN32_ERROR( "unable to GetFileSizeEx() file " << m_source << " handle " << m_handle );
               }
               return std::size_t( size.QuadPart );
            }

            const char* const m_source;
            const HANDLE m_handle;

         private:
            HANDLE open() const
            {
               SetLastError( 0 );
               std::wstring ws( m_source, m_source + strlen( m_source ) );

#if( _WIN32_WINNT >= 0x0602 )
               const HANDLE handle = ::CreateFile2( ws.c_str(),
                                                    GENERIC_READ,
                                                    FILE_SHARE_READ,
                                                    OPEN_EXISTING,
                                                    nullptr );
               if( handle != INVALID_HANDLE_VALUE ) {
                  return handle;
               }
               TAO_PEGTL_THROW_INPUT_WIN32_ERROR( "CreateFile2() failed opening file " << m_source << " for reading" );
#else
               const HANDLE handle = ::CreateFileW( ws.c_str(),
                                                    GENERIC_READ,
                                                    FILE_SHARE_READ,
                                                    nullptr,
                                                    OPEN_EXISTING,
                                                    FILE_ATTRIBUTE_NORMAL,
                                                    nullptr );
               if( handle != INVALID_HANDLE_VALUE ) {
                  return handle;
               }
               TAO_PEGTL_THROW_INPUT_WIN32_ERROR( "CreateFileW() failed opening file " << m_source << " for reading" );
#endif
            }
         };

         struct win32_file_mapper
         {
            explicit win32_file_mapper( const char* filename )
               : win32_file_mapper( win32_file_opener( filename ) )
            {
            }

            explicit win32_file_mapper( const win32_file_opener& reader )
               : m_size( reader.size() ),
                 m_handle( open( reader ) )
            {
            }

            win32_file_mapper( const win32_file_mapper& ) = delete;
            win32_file_mapper( win32_file_mapper&& ) = delete;

            ~win32_file_mapper() noexcept
            {
               ::CloseHandle( m_handle );
            }

            void operator=( const win32_file_mapper& ) = delete;
            void operator=( win32_file_mapper&& ) = delete;

            const size_t m_size;
            const HANDLE m_handle;

         private:
            HANDLE open( const win32_file_opener& reader ) const
            {
               const uint64_t file_size = reader.size();
               SetLastError( 0 );
               // Use `CreateFileMappingW` because a) we're not specifying a
               // mapping name, so the character type is of no consequence, and
               // b) it's defined in `memoryapi.h`, unlike
               // `CreateFileMappingA`(?!)
               const HANDLE handle = ::CreateFileMappingW( reader.m_handle,
                                                           nullptr,
                                                           PAGE_READONLY,
                                                           DWORD( file_size >> 32 ),
                                                           DWORD( file_size & 0xffffffff ),
                                                           nullptr );
               if( handle != NULL || file_size == 0 ) {
                  return handle;
               }
               TAO_PEGTL_THROW_INPUT_WIN32_ERROR( "unable to CreateFileMappingW() file " << reader.m_source << " for reading" );
            }
         };

         class file_mapper
         {
         public:
            explicit file_mapper( const char* filename )
               : file_mapper( win32_file_mapper( filename ) )
            {
            }

            explicit file_mapper( const win32_file_mapper& mapper )
               : m_size( mapper.m_size ),
                 m_data( static_cast< const char* >( ::MapViewOfFile( mapper.m_handle,
                                                                      FILE_MAP_READ,
                                                                      0,
                                                                      0,
                                                                      0 ) ) )
            {
               if( ( m_size != 0 ) && ( intptr_t( m_data ) == 0 ) ) {
                  TAO_PEGTL_THROW_INPUT_WIN32_ERROR( "unable to MapViewOfFile() file mapping object with handle " << mapper.m_handle );
               }
            }

            file_mapper( const file_mapper& ) = delete;
            file_mapper( file_mapper&& ) = delete;

            ~file_mapper() noexcept
            {
               ::UnmapViewOfFile( LPCVOID( m_data ) );
            }

            void operator=( const file_mapper& ) = delete;
            void operator=( file_mapper&& ) = delete;

            bool empty() const noexcept
            {
               return m_size == 0;
            }

            std::size_t size() const noexcept
            {
               return m_size;
            }

            using iterator = const char*;
            using const_iterator = const char*;

            iterator data() const noexcept
            {
               return m_data;
            }

            iterator begin() const noexcept
            {
               return m_data;
            }

            iterator end() const noexcept
            {
               return m_data + m_size;
            }

            std::string string() const
            {
               return std::string( m_data, m_size );
            }

         private:
            const std::size_t m_size;
            const char* const m_data;
         };

      } // namespace internal

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 23 "tao/pegtl/mmap_input.hpp"
#else
#endif

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         struct mmap_holder
         {
            const std::string filename;
            const file_mapper data;

            template< typename T >
            explicit mmap_holder( T&& in_filename )
               : filename( std::forward< T >( in_filename ) ),
                 data( filename.c_str() )
            {
            }

            mmap_holder( const mmap_holder& ) = delete;
            mmap_holder( mmap_holder&& ) = delete;

            ~mmap_holder() = default;

            void operator=( const mmap_holder& ) = delete;
            void operator=( mmap_holder&& ) = delete;
         };

      } // namespace internal

      template< tracking_mode P = tracking_mode::eager, typename Eol = eol::lf_crlf >
      struct mmap_input
         : private internal::mmap_holder,
           public memory_input< P, Eol, const char* >
      {
         template< typename T >
         explicit mmap_input( T&& in_filename )
            : internal::mmap_holder( std::forward< T >( in_filename ) ),
              memory_input< P, Eol, const char* >( data.begin(), data.end(), filename.c_str() )
         {
         }

         mmap_input( const mmap_input& ) = delete;
         mmap_input( mmap_input&& ) = delete;

         ~mmap_input() = default;

         void operator=( const mmap_input& ) = delete;
         void operator=( mmap_input&& ) = delete;
      };

#ifdef __cpp_deduction_guides
      template< typename... Ts >
      explicit mmap_input( Ts&&... )->mmap_input<>;
#endif

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 17 "tao/pegtl/file_input.hpp"
#else

#endif

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
#if defined( _POSIX_MAPPED_FILES ) || defined( _WIN32 )
      template< tracking_mode P = tracking_mode::eager, typename Eol = eol::lf_crlf >
      struct file_input
         : mmap_input< P, Eol >
      {
         using mmap_input< P, Eol >::mmap_input;
      };
#else
      template< tracking_mode P = tracking_mode::eager, typename Eol = eol::lf_crlf >
      struct file_input
         : read_input< P, Eol >
      {
         using read_input< P, Eol >::read_input;
      };
#endif

#ifdef __cpp_deduction_guides
      template< typename... Ts >
      explicit file_input( Ts&&... )->file_input<>;
#endif

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 33 "tao/pegtl.hpp"

#line 1 "tao/pegtl/change_action.hpp"
       
#line 1 "tao/pegtl/change_action.hpp"



#ifndef TAO_PEGTL_CHANGE_ACTION_HPP
#define TAO_PEGTL_CHANGE_ACTION_HPP

#include <type_traits>






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< template< typename... > class NewAction >
      struct change_action
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static bool match( Input& in, States&&... st )
         {
            static_assert( !std::is_same< Action< void >, NewAction< void > >::value, "old and new action class templates are identical" );
            return Control< Rule >::template match< A, M, NewAction, Control >( in, st... );
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 35 "tao/pegtl.hpp"
#line 1 "tao/pegtl/change_action_and_state.hpp"
       
#line 1 "tao/pegtl/change_action_and_state.hpp"



#ifndef TAO_PEGTL_CHANGE_ACTION_AND_STATE_HPP
#define TAO_PEGTL_CHANGE_ACTION_AND_STATE_HPP

#include <type_traits>






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< template< typename... > class NewAction, typename NewState >
      struct change_action_and_state
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static auto match( Input& in, States&&... st )
            -> typename std::enable_if< ( A == apply_mode::action ), bool >::type
         {
            static_assert( !std::is_same< Action< void >, NewAction< void > >::value, "old and new action class templates are identical" );
            NewState s( static_cast< const Input& >( in ), st... );
            if( Control< Rule >::template match< A, M, NewAction, Control >( in, s ) ) {
               Action< Rule >::success( static_cast< const Input& >( in ), s, st... );
               return true;
            }
            return false;
         }

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States,
                   int = 1 >
         static auto match( Input& in, States&&... st )
            -> typename std::enable_if< ( A == apply_mode::nothing ), bool >::type
         {
            static_assert( !std::is_same< Action< void >, NewAction< void > >::value, "old and new action class templates are identical" );
            NewState s( static_cast< const Input& >( in ), st... );
            return Control< Rule >::template match< A, M, NewAction, Control >( in, s );
         }

         template< typename Input,
                   typename... States >
         static void success( const Input& in, NewState& s, States&&... st ) noexcept( noexcept( s.success( in, st... ) ) )
         {
            s.success( in, st... );
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 36 "tao/pegtl.hpp"
#line 1 "tao/pegtl/change_action_and_states.hpp"
       
#line 1 "tao/pegtl/change_action_and_states.hpp"



#ifndef TAO_PEGTL_CHANGE_ACTION_AND_STATES_HPP
#define TAO_PEGTL_CHANGE_ACTION_AND_STATES_HPP

#include <tuple>
#include <type_traits>
#line 17 "tao/pegtl/change_action_and_states.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< template< typename... > class NewAction, typename... NewStates >
      struct change_action_and_states
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   std::size_t... Ns,
                   typename Input,
                   typename... States >
         static bool match( TAO_PEGTL_NAMESPACE::internal::index_sequence< Ns... >, Input& in, States&&... st )
         {
            auto t = std::tie( st... );
            if( Control< Rule >::template match< A, M, NewAction, Control >( in, std::get< Ns >( t )... ) ) {
               Action< Rule >::success( static_cast< const Input& >( in ), st... );
               return true;
            }
            return false;
         }

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static auto match( Input& in, States&&... st )
            -> typename std::enable_if< ( A == apply_mode::action ), bool >::type
         {
            static_assert( !std::is_same< Action< void >, NewAction< void > >::value, "old and new action class templates are identical" );
            return match< Rule, A, M, Action, Control >( TAO_PEGTL_NAMESPACE::internal::index_sequence_for< NewStates... >(), in, NewStates()..., st... );
         }

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States,
                   int = 1 >
         static auto match( Input& in, States&&... /*unused*/ )
            -> typename std::enable_if< ( A == apply_mode::nothing ), bool >::type
         {
            static_assert( !std::is_same< Action< void >, NewAction< void > >::value, "old and new action class templates are identical" );
            return Control< Rule >::template match< A, M, NewAction, Control >( in, NewStates()... );
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 37 "tao/pegtl.hpp"
#line 1 "tao/pegtl/change_control.hpp"
       
#line 1 "tao/pegtl/change_control.hpp"



#ifndef TAO_PEGTL_CHANGE_CONTROL_HPP
#define TAO_PEGTL_CHANGE_CONTROL_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
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
                   typename Input,
                   typename... States >
         static bool match( Input& in, States&&... st )
         {
            return TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, NewControl >( in, st... );
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 38 "tao/pegtl.hpp"
#line 1 "tao/pegtl/change_state.hpp"
       
#line 1 "tao/pegtl/change_state.hpp"



#ifndef TAO_PEGTL_CHANGE_STATE_HPP
#define TAO_PEGTL_CHANGE_STATE_HPP

#include <type_traits>







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename NewState >
      struct change_state
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static auto match( Input& in, States&&... st )
            -> typename std::enable_if< ( A == apply_mode::action ), bool >::type
         {
            NewState s( static_cast< const Input& >( in ), st... );
            if( TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, s ) ) {
               Action< Rule >::success( static_cast< const Input& >( in ), s, st... );
               return true;
            }
            return false;
         }

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States,
                   int = 1 >
         static auto match( Input& in, States&&... st )
            -> typename std::enable_if< ( A == apply_mode::nothing ), bool >::type
         {
            NewState s( static_cast< const Input& >( in ), st... );
            return TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, s );
         }

         template< typename Input,
                   typename... States >
         static void success( const Input& in, NewState& s, States&&... st ) noexcept( noexcept( s.success( in, st... ) ) )
         {
            s.success( in, st... );
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 39 "tao/pegtl.hpp"
#line 1 "tao/pegtl/change_states.hpp"
       
#line 1 "tao/pegtl/change_states.hpp"



#ifndef TAO_PEGTL_CHANGE_STATES_HPP
#define TAO_PEGTL_CHANGE_STATES_HPP

#include <tuple>
#line 17 "tao/pegtl/change_states.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename... NewStates >
      struct change_states
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   std::size_t... Ns,
                   typename Input,
                   typename... States >
         static bool match( TAO_PEGTL_NAMESPACE::internal::index_sequence< Ns... >, Input& in, States&&... st )
         {
            auto t = std::tie( st... );
            if( TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, std::get< Ns >( t )... ) ) {
               Action< Rule >::success( static_cast< const Input& >( in ), st... );
               return true;
            }
            return false;
         }

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static auto match( Input& in, States&&... st )
            -> typename std::enable_if< ( A == apply_mode::action ), bool >::type
         {
            return match< Rule, A, M, Action, Control >( TAO_PEGTL_NAMESPACE::internal::index_sequence_for< NewStates... >(), in, NewStates()..., st... );
         }

         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States,
                   int = 1 >
         static auto match( Input& in, States&&... /*unused*/ )
            -> typename std::enable_if< ( A == apply_mode::nothing ), bool >::type
         {
            return TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, NewStates()... );
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 40 "tao/pegtl.hpp"

#line 1 "tao/pegtl/disable_action.hpp"
       
#line 1 "tao/pegtl/disable_action.hpp"



#ifndef TAO_PEGTL_DISABLE_ACTION_HPP
#define TAO_PEGTL_DISABLE_ACTION_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct disable_action
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static bool match( Input& in, States&&... st )
         {
            return TAO_PEGTL_NAMESPACE::match< Rule, apply_mode::nothing, M, Action, Control >( in, st... );
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 42 "tao/pegtl.hpp"
#line 1 "tao/pegtl/enable_action.hpp"
       
#line 1 "tao/pegtl/enable_action.hpp"



#ifndef TAO_PEGTL_ENABLE_ACTION_HPP
#define TAO_PEGTL_ENABLE_ACTION_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct enable_action
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static bool match( Input& in, States&&... st )
         {
            return TAO_PEGTL_NAMESPACE::match< Rule, apply_mode::action, M, Action, Control >( in, st... );
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 43 "tao/pegtl.hpp"

#line 1 "tao/pegtl/discard_input.hpp"
       
#line 1 "tao/pegtl/discard_input.hpp"



#ifndef TAO_PEGTL_DISCARD_INPUT_HPP
#define TAO_PEGTL_DISCARD_INPUT_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct discard_input
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static bool match( Input& in, States&&... st )
         {
            const bool result = TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, st... );
            in.discard();
            return result;
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 45 "tao/pegtl.hpp"
#line 1 "tao/pegtl/discard_input_on_failure.hpp"
       
#line 1 "tao/pegtl/discard_input_on_failure.hpp"



#ifndef TAO_PEGTL_DISCARD_INPUT_ON_FAILURE_HPP
#define TAO_PEGTL_DISCARD_INPUT_ON_FAILURE_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct discard_input_on_failure
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static bool match( Input& in, States&&... st )
         {
            const bool result = TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, st... );
            if( !result ) {
               in.discard();
            }
            return result;
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 46 "tao/pegtl.hpp"
#line 1 "tao/pegtl/discard_input_on_success.hpp"
       
#line 1 "tao/pegtl/discard_input_on_success.hpp"



#ifndef TAO_PEGTL_DISCARD_INPUT_ON_SUCCESS_HPP
#define TAO_PEGTL_DISCARD_INPUT_ON_SUCCESS_HPP







namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct discard_input_on_success
         : maybe_nothing
      {
         template< typename Rule,
                   apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static bool match( Input& in, States&&... st )
         {
            const bool result = TAO_PEGTL_NAMESPACE::match< Rule, A, M, Action, Control >( in, st... );
            if( result ) {
               in.discard();
            }
            return result;
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 47 "tao/pegtl.hpp"

// The following are not included by
// default because they include <iostream>.

// #include "pegtl/analyze.hpp"

#endif
#line 2 "amalgamated.hpp"
#line 1 "tao/pegtl/analyze.hpp"
       
#line 1 "tao/pegtl/analyze.hpp"



#ifndef TAO_PEGTL_ANALYZE_HPP
#define TAO_PEGTL_ANALYZE_HPP



#line 1 "tao/pegtl/analysis/analyze_cycles.hpp"
       
#line 1 "tao/pegtl/analysis/analyze_cycles.hpp"



#ifndef TAO_PEGTL_ANALYSIS_ANALYZE_CYCLES_HPP
#define TAO_PEGTL_ANALYSIS_ANALYZE_CYCLES_HPP

#include <cassert>

#include <map>
#include <set>
#include <stdexcept>

#include <iostream>
#include <utility>




#line 1 "tao/pegtl/analysis/insert_guard.hpp"
       
#line 1 "tao/pegtl/analysis/insert_guard.hpp"



#ifndef TAO_PEGTL_ANALYSIS_INSERT_GUARD_HPP
#define TAO_PEGTL_ANALYSIS_INSERT_GUARD_HPP

#include <utility>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         template< typename C >
         class insert_guard
         {
         public:
            insert_guard( C& container, const typename C::value_type& value )
               : m_i( container.insert( value ) ),
                 m_c( &container )
            {
            }

            insert_guard( const insert_guard& ) = delete;

            insert_guard( insert_guard&& other ) noexcept
               : m_i( other.m_i ),
                 m_c( other.m_c )
            {
               other.m_c = nullptr;
            }

            ~insert_guard()
            {
               if( m_c && m_i.second ) {
                  m_c->erase( m_i.first );
               }
            }

            void operator=( const insert_guard& ) = delete;
            void operator=( insert_guard&& ) = delete;

            explicit operator bool() const noexcept
            {
               return m_i.second;
            }

         private:
            const std::pair< typename C::iterator, bool > m_i;
            C* m_c;
         };

         template< typename C >
         insert_guard< C > make_insert_guard( C& container, const typename C::value_type& value )
         {
            return insert_guard< C >( container, value );
         }

      } // namespace analysis

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 20 "tao/pegtl/analysis/analyze_cycles.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace analysis
      {
         class analyze_cycles_impl
         {
         protected:
            explicit analyze_cycles_impl( const bool verbose ) noexcept
               : m_verbose( verbose ),
                 m_problems( 0 )
            {
            }

            const bool m_verbose;
            unsigned m_problems;
            grammar_info m_info;
            std::set< std::string > m_stack;
            std::map< std::string, bool > m_cache;
            std::map< std::string, bool > m_results;

            std::map< std::string, rule_info >::const_iterator find( const std::string& name ) const noexcept
            {
               const auto iter = m_info.map.find( name );
               assert( iter != m_info.map.end() );
               return iter;
            }

            bool work( const std::map< std::string, rule_info >::const_iterator& start, const bool accum )
            {
               const auto j = m_cache.find( start->first );

               if( j != m_cache.end() ) {
                  return j->second;
               }
               if( const auto g = make_insert_guard( m_stack, start->first ) ) {
                  switch( start->second.type ) {
                     case rule_type::any: {
                        bool a = false;
                        for( const auto& r : start->second.rules ) {
                           a = a || work( find( r ), accum || a );
                        }
                        return m_cache[ start->first ] = true;
                     }
                     case rule_type::opt: {
                        bool a = false;
                        for( const auto& r : start->second.rules ) {
                           a = a || work( find( r ), accum || a );
                        }
                        return m_cache[ start->first ] = false;
                     }
                     case rule_type::seq: {
                        bool a = false;
                        for( const auto& r : start->second.rules ) {
                           a = a || work( find( r ), accum || a );
                        }
                        return m_cache[ start->first ] = a;
                     }
                     case rule_type::sor: {
                        bool a = true;
                        for( const auto& r : start->second.rules ) {
                           a = a && work( find( r ), accum );
                        }
                        return m_cache[ start->first ] = a;
                     }
                  }
                  throw std::logic_error( "code should be unreachable: invalid rule_type value" ); // NOLINT, LCOV_EXCL_LINE
               }
               if( !accum ) {
                  ++m_problems;
                  if( m_verbose ) {
                     std::cout << "problem: cycle without progress detected at rule class " << start->first << std::endl; // LCOV_EXCL_LINE
                  }
               }
               return m_cache[ start->first ] = accum;
            }
         };

         template< typename Grammar >
         class analyze_cycles
            : private analyze_cycles_impl
         {
         public:
            explicit analyze_cycles( const bool verbose )
               : analyze_cycles_impl( verbose )
            {
               Grammar::analyze_t::template insert< Grammar >( m_info );
            }

            std::size_t problems()
            {
               for( auto i = m_info.map.begin(); i != m_info.map.end(); ++i ) {
                  m_results[ i->first ] = work( i, false );
                  m_cache.clear();
               }
               return m_problems;
            }

            template< typename Rule >
            bool consumes() const noexcept
            {
               const auto i = m_results.find( internal::demangle< Rule >() );
               assert( i != m_results.end() );
               return i->second;
            }
         };

      } // namespace analysis

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "tao/pegtl/analyze.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      template< typename Rule >
      std::size_t analyze( const bool verbose = true )
      {
         return analysis::analyze_cycles< Rule >( verbose ).problems();
      }

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 3 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/abnf.hpp"
       
#line 1 "tao/pegtl/contrib/abnf.hpp"



#ifndef TAO_PEGTL_CONTRIB_ABNF_HPP
#define TAO_PEGTL_CONTRIB_ABNF_HPP




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace abnf
      {
         // Core ABNF rules according to RFC 5234, Appendix B

         // clang-format off
         struct ALPHA : internal::ranges< internal::peek_char, 'a', 'z', 'A', 'Z' > {};
         struct BIT : internal::one< internal::result_on_found::success, internal::peek_char, '0', '1' > {};
         struct CHAR : internal::range< internal::result_on_found::success, internal::peek_char, char( 1 ), char( 127 ) > {};
         struct CR : internal::one< internal::result_on_found::success, internal::peek_char, '\r' > {};
         struct CRLF : internal::string< '\r', '\n' > {};
         struct CTL : internal::ranges< internal::peek_char, char( 0 ), char( 31 ), char( 127 ) > {};
         struct DIGIT : internal::range< internal::result_on_found::success, internal::peek_char, '0', '9' > {};
         struct DQUOTE : internal::one< internal::result_on_found::success, internal::peek_char, '"' > {};
         struct HEXDIG : internal::ranges< internal::peek_char, '0', '9', 'a', 'f', 'A', 'F' > {};
         struct HTAB : internal::one< internal::result_on_found::success, internal::peek_char, '\t' > {};
         struct LF : internal::one< internal::result_on_found::success, internal::peek_char, '\n' > {};
         struct LWSP : internal::star< internal::sor< internal::string< '\r', '\n' >, internal::one< internal::result_on_found::success, internal::peek_char, ' ', '\t' > >, internal::one< internal::result_on_found::success, internal::peek_char, ' ', '\t' > > {};
         struct OCTET : internal::any< internal::peek_char > {};
         struct SP : internal::one< internal::result_on_found::success, internal::peek_char, ' ' > {};
         struct VCHAR : internal::range< internal::result_on_found::success, internal::peek_char, char( 33 ), char( 126 ) > {};
         struct WSP : internal::one< internal::result_on_found::success, internal::peek_char, ' ', '\t' > {};
         // clang-format on

      } // namespace abnf

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 4 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/alphabet.hpp"
       
#line 1 "tao/pegtl/contrib/alphabet.hpp"



#ifndef TAO_PEGTL_CONTRIB_ALPHABET_HPP
#define TAO_PEGTL_CONTRIB_ALPHABET_HPP



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace alphabet
      {
         static const int a = 'a'; // NOLINT
         static const int b = 'b'; // NOLINT
         static const int c = 'c'; // NOLINT
         static const int d = 'd'; // NOLINT
         static const int e = 'e'; // NOLINT
         static const int f = 'f'; // NOLINT
         static const int g = 'g'; // NOLINT
         static const int h = 'h'; // NOLINT
         static const int i = 'i'; // NOLINT
         static const int j = 'j'; // NOLINT
         static const int k = 'k'; // NOLINT
         static const int l = 'l'; // NOLINT
         static const int m = 'm'; // NOLINT
         static const int n = 'n'; // NOLINT
         static const int o = 'o'; // NOLINT
         static const int p = 'p'; // NOLINT
         static const int q = 'q'; // NOLINT
         static const int r = 'r'; // NOLINT
         static const int s = 's'; // NOLINT
         static const int t = 't'; // NOLINT
         static const int u = 'u'; // NOLINT
         static const int v = 'v'; // NOLINT
         static const int w = 'w'; // NOLINT
         static const int x = 'x'; // NOLINT
         static const int y = 'y'; // NOLINT
         static const int z = 'z'; // NOLINT

         static const int A = 'A'; // NOLINT
         static const int B = 'B'; // NOLINT
         static const int C = 'C'; // NOLINT
         static const int D = 'D'; // NOLINT
         static const int E = 'E'; // NOLINT
         static const int F = 'F'; // NOLINT
         static const int G = 'G'; // NOLINT
         static const int H = 'H'; // NOLINT
         static const int I = 'I'; // NOLINT
         static const int J = 'J'; // NOLINT
         static const int K = 'K'; // NOLINT
         static const int L = 'L'; // NOLINT
         static const int M = 'M'; // NOLINT
         static const int N = 'N'; // NOLINT
         static const int O = 'O'; // NOLINT
         static const int P = 'P'; // NOLINT
         static const int Q = 'Q'; // NOLINT
         static const int R = 'R'; // NOLINT
         static const int S = 'S'; // NOLINT
         static const int T = 'T'; // NOLINT
         static const int U = 'U'; // NOLINT
         static const int V = 'V'; // NOLINT
         static const int W = 'W'; // NOLINT
         static const int X = 'X'; // NOLINT
         static const int Y = 'Y'; // NOLINT
         static const int Z = 'Z'; // NOLINT

      } // namespace alphabet

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 5 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/counter.hpp"
       
#line 1 "tao/pegtl/contrib/counter.hpp"



#ifndef TAO_PEGTL_CONTRIB_COUNTER_HPP
#define TAO_PEGTL_CONTRIB_COUNTER_HPP

#include <map>
#include <string>






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      struct counter_data
      {
         unsigned start = 0;
         unsigned success = 0;
         unsigned failure = 0;
      };

      struct counter_state
      {
         std::map< std::string, counter_data > counts;
      };

      template< typename Rule >
      struct counter
         : normal< Rule >
      {
         template< typename Input >
         static void start( const Input& /*unused*/, counter_state& ts )
         {
            ++ts.counts[ internal::demangle< Rule >() ].start;
         }

         template< typename Input >
         static void success( const Input& /*unused*/, counter_state& ts )
         {
            ++ts.counts[ internal::demangle< Rule >() ].success;
         }

         template< typename Input >
         static void failure( const Input& /*unused*/, counter_state& ts )
         {
            ++ts.counts[ internal::demangle< Rule >() ].failure;
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 6 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/http.hpp"
       
#line 1 "tao/pegtl/contrib/http.hpp"



#ifndef TAO_PEGTL_CONTRIB_HTTP_HPP
#define TAO_PEGTL_CONTRIB_HTTP_HPP
#line 14 "tao/pegtl/contrib/http.hpp"
#line 1 "tao/pegtl/contrib/remove_first_state.hpp"
       
#line 1 "tao/pegtl/contrib/remove_first_state.hpp"



#ifndef TAO_PEGTL_CONTRIB_REMOVE_FIRST_STATE_HPP
#define TAO_PEGTL_CONTRIB_REMOVE_FIRST_STATE_HPP



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      // Applies to start(), success(), failure(), raise(), apply(), and apply0():
      // The first state is removed when the call is forwarded to Base.
      template< typename Base >
      struct remove_first_state
         : Base
      {
         template< typename Input, typename State, typename... States >
         static void start( const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::start( in, st... ) ) )
         {
            Base::start( in, st... );
         }

         template< typename Input, typename State, typename... States >
         static void success( const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::success( in, st... ) ) )
         {
            Base::success( in, st... );
         }

         template< typename Input, typename State, typename... States >
         static void failure( const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::failure( in, st... ) ) )
         {
            Base::failure( in, st... );
         }

         template< typename Input, typename State, typename... States >
         static void raise( const Input& in, State&& /*unused*/, States&&... st )
         {
            Base::raise( in, st... );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename State, typename... States >
         static auto apply( const Iterator& begin, const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::template apply< Action >( begin, in, st... ) ) )
            -> decltype( Base::template apply< Action >( begin, in, st... ) )
         {
            return Base::template apply< Action >( begin, in, st... );
         }

         template< template< typename... > class Action, typename Input, typename State, typename... States >
         static auto apply0( const Input& in, State&& /*unused*/, States&&... st ) noexcept( noexcept( Base::template apply0< Action >( in, st... ) ) )
            -> decltype( Base::template apply0< Action >( in, st... ) )
         {
            return Base::template apply0< Action >( in, st... );
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 15 "tao/pegtl/contrib/http.hpp"
#line 1 "tao/pegtl/contrib/uri.hpp"
       
#line 1 "tao/pegtl/contrib/uri.hpp"



#ifndef TAO_PEGTL_CONTRIB_URI_HPP
#define TAO_PEGTL_CONTRIB_URI_HPP
#line 14 "tao/pegtl/contrib/uri.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace uri
      {
         // URI grammar according to RFC 3986.

         // This grammar is a direct PEG translation of the original URI grammar.
         // It should be considered experimental -- in case of any issues, in particular
         // missing rules for attached actions, please contact the developers.

         // Note that this grammar has multiple top-level rules.

         using dot = one< '.' >;
         using colon = one< ':' >;

         // clang-format off
         struct dec_octet : sor< one< '0' >,
                                 rep_min_max< 1, 2, abnf::DIGIT >,
                                 seq< one< '1' >, abnf::DIGIT, abnf::DIGIT >,
                                 seq< one< '2' >, range< '0', '4' >, abnf::DIGIT >,
                                 seq< string< '2', '5' >, range< '0', '5' > > > {};

         struct IPv4address : seq< dec_octet, dot, dec_octet, dot, dec_octet, dot, dec_octet > {};

         struct h16 : rep_min_max< 1, 4, abnf::HEXDIG > {};
         struct ls32 : sor< seq< h16, colon, h16 >, IPv4address > {};

         struct dcolon : two< ':' > {};

         struct IPv6address : sor< seq< rep< 6, h16, colon >, ls32 >,
                                   seq< dcolon, rep< 5, h16, colon >, ls32 >,
                                   seq< opt< h16 >, dcolon, rep< 4, h16, colon >, ls32 >,
                                   seq< opt< h16, opt< colon, h16 > >, dcolon, rep< 3, h16, colon >, ls32 >,
                                   seq< opt< h16, rep_opt< 2, colon, h16 > >, dcolon, rep< 2, h16, colon >, ls32 >,
                                   seq< opt< h16, rep_opt< 3, colon, h16 > >, dcolon, h16, colon, ls32 >,
                                   seq< opt< h16, rep_opt< 4, colon, h16 > >, dcolon, ls32 >,
                                   seq< opt< h16, rep_opt< 5, colon, h16 > >, dcolon, h16 >,
                                   seq< opt< h16, rep_opt< 6, colon, h16 > >, dcolon > > {};

         struct gen_delims : one< ':', '/', '?', '#', '[', ']', '@' > {};
         struct sub_delims : one< '!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '=' > {};

         struct unreserved : sor< abnf::ALPHA, abnf::DIGIT, one< '-', '.', '_', '~' > > {};
         struct reserved : sor< gen_delims, sub_delims > {};

         struct IPvFuture : if_must< one< 'v' >, plus< abnf::HEXDIG >, dot, plus< sor< unreserved, sub_delims, colon > > > {};

         struct IP_literal : if_must< one< '[' >, sor< IPvFuture, IPv6address >, one< ']' > > {};

         struct pct_encoded : if_must< one< '%' >, abnf::HEXDIG, abnf::HEXDIG > {};
         struct pchar : sor< unreserved, pct_encoded, sub_delims, one< ':', '@' > > {};

         struct query : star< sor< pchar, one< '/', '?' > > > {};
         struct fragment : star< sor< pchar, one< '/', '?' > > > {};

         struct segment : star< pchar > {};
         struct segment_nz : plus< pchar > {};
         struct segment_nz_nc : plus< sor< unreserved, pct_encoded, sub_delims, one< '@' > > > {}; // non-zero-length segment without any colon ":"

         struct path_abempty : star< one< '/' >, segment > {};
         struct path_absolute : seq< one< '/' >, opt< segment_nz, star< one< '/' >, segment > > > {};
         struct path_noscheme : seq< segment_nz_nc, star< one< '/' >, segment > > {};
         struct path_rootless : seq< segment_nz, star< one< '/' >, segment > > {};
         struct path_empty : success {};

         struct path : sor< path_noscheme, // begins with a non-colon segment
                            path_rootless, // begins with a segment
                            path_absolute, // begins with "/" but not "//"
                            path_abempty > {}; // begins with "/" or is empty

         struct reg_name : star< sor< unreserved, pct_encoded, sub_delims > > {};

         struct port : star< abnf::DIGIT > {};
         struct host : sor< IP_literal, IPv4address, reg_name > {};
         struct userinfo : star< sor< unreserved, pct_encoded, sub_delims, colon > > {};
         struct opt_userinfo : opt< userinfo, one< '@' > > {};
         struct authority : seq< opt_userinfo, host, opt< colon, port > > {};

         struct scheme : seq< abnf::ALPHA, star< sor< abnf::ALPHA, abnf::DIGIT, one< '+', '-', '.' > > > > {};

         using dslash = two< '/' >;
         using opt_query = opt_must< one< '?' >, query >;
         using opt_fragment = opt_must< one< '#' >, fragment >;

         struct hier_part : sor< if_must< dslash, authority, path_abempty >, path_rootless, path_absolute, path_empty > {};
         struct relative_part : sor< if_must< dslash, authority, path_abempty >, path_noscheme, path_absolute, path_empty > {};
         struct relative_ref : seq< relative_part, opt_query, opt_fragment > {};

         struct URI : seq< scheme, one< ':' >, hier_part, opt_query, opt_fragment > {};
         struct URI_reference : sor< URI, relative_ref > {};
         struct absolute_URI : seq< scheme, one< ':' >, hier_part, opt_query > {};
         // clang-format on

      } // namespace uri

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 16 "tao/pegtl/contrib/http.hpp"

namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace http
      {
         // HTTP 1.1 grammar according to RFC 7230.

         // This grammar is a direct PEG translation of the original HTTP grammar.
         // It should be considered experimental -- in case of any issues, in particular
         // missing rules for attached actions, please contact the developers.

         using OWS = star< abnf::WSP >; // optional whitespace
         using RWS = plus< abnf::WSP >; // required whitespace
         using BWS = OWS; // "bad" whitespace

         using obs_text = not_range< 0x00, 0x7F >;
         using obs_fold = seq< abnf::CRLF, plus< abnf::WSP > >;

         // clang-format off
         struct tchar : sor< abnf::ALPHA, abnf::DIGIT, one< '!', '#', '$', '%', '&', '\'', '*', '+', '-', '.', '^', '_', '`', '|', '~' > > {};
         struct token : plus< tchar > {};

         struct field_name : token {};

         struct field_vchar : sor< abnf::VCHAR, obs_text > {};
         struct field_content : list< field_vchar, plus< abnf::WSP > > {};
         struct field_value : star< sor< field_content, obs_fold > > {};

         struct header_field : seq< field_name, one< ':' >, OWS, field_value, OWS > {};

         struct method : token {};

         struct absolute_path : plus< one< '/' >, uri::segment > {};

         struct origin_form : seq< absolute_path, uri::opt_query > {};
         struct absolute_form : uri::absolute_URI {};
         struct authority_form : uri::authority {};
         struct asterisk_form : one< '*' > {};

         struct request_target : sor< origin_form, absolute_form, authority_form, asterisk_form > {};

         struct status_code : rep< 3, abnf::DIGIT > {};
         struct reason_phrase : star< sor< abnf::VCHAR, obs_text, abnf::WSP > > {};

         struct HTTP_version : if_must< string< 'H', 'T', 'T', 'P', '/' >, abnf::DIGIT, one< '.' >, abnf::DIGIT > {};

         struct request_line : if_must< method, abnf::SP, request_target, abnf::SP, HTTP_version, abnf::CRLF > {};
         struct status_line : if_must< HTTP_version, abnf::SP, status_code, abnf::SP, reason_phrase, abnf::CRLF > {};
         struct start_line : sor< status_line, request_line > {};

         struct message_body : star< abnf::OCTET > {};
         struct HTTP_message : seq< start_line, star< header_field, abnf::CRLF >, abnf::CRLF, opt< message_body > > {};

         struct Content_Length : plus< abnf::DIGIT > {};

         struct uri_host : uri::host {};
         struct port : uri::port {};

         struct Host : seq< uri_host, opt< one< ':' >, port > > {};

         // PEG are different from CFGs! (this replaces ctext and qdtext)
         using text = sor< abnf::HTAB, range< 0x20, 0x7E >, obs_text >;

         struct quoted_pair : if_must< one< '\\' >, sor< abnf::VCHAR, obs_text, abnf::WSP > > {};
         struct quoted_string : if_must< abnf::DQUOTE, until< abnf::DQUOTE, sor< quoted_pair, text > > > {};

         struct transfer_parameter : seq< token, BWS, one< '=' >, BWS, sor< token, quoted_string > > {};
         struct transfer_extension : seq< token, star< OWS, one< ';' >, OWS, transfer_parameter > > {};
         struct transfer_coding : sor< istring< 'c', 'h', 'u', 'n', 'k', 'e', 'd' >,
                                       istring< 'c', 'o', 'm', 'p', 'r', 'e', 's', 's' >,
                                       istring< 'd', 'e', 'f', 'l', 'a', 't', 'e' >,
                                       istring< 'g', 'z', 'i', 'p' >,
                                       transfer_extension > {};

         struct rank : sor< seq< one< '0' >, opt< one< '.' >, rep_opt< 3, abnf::DIGIT > > >,
                            seq< one< '1' >, opt< one< '.' >, rep_opt< 3, one< '0' > > > > > {};

         struct t_ranking : seq< OWS, one< ';' >, OWS, one< 'q', 'Q' >, one< '=' >, rank > {};
         struct t_codings : sor< istring< 't', 'r', 'a', 'i', 'l', 'e', 'r', 's' >, seq< transfer_coding, opt< t_ranking > > > {};

         struct TE : opt< sor< one< ',' >, t_codings >, star< OWS, one< ',' >, opt< OWS, t_codings > > > {};

         template< typename T >
         using make_comma_list = seq< star< one< ',' >, OWS >, T, star< OWS, one< ',' >, opt< OWS, T > > >;

         struct connection_option : token {};
         struct Connection : make_comma_list< connection_option > {};

         struct Trailer : make_comma_list< field_name > {};

         struct Transfer_Encoding : make_comma_list< transfer_coding > {};

         struct protocol_name : token {};
         struct protocol_version : token {};
         struct protocol : seq< protocol_name, opt< one< '/' >, protocol_version > > {};
         struct Upgrade : make_comma_list< protocol > {};

         struct pseudonym : token {};

         struct received_protocol : seq< opt< protocol_name, one< '/' > >, protocol_version > {};
         struct received_by : sor< seq< uri_host, opt< one< ':' >, port > >, pseudonym > {};

         struct comment : if_must< one< '(' >, until< one< ')' >, sor< comment, quoted_pair, text > > > {};

         struct Via : make_comma_list< seq< received_protocol, RWS, received_by, opt< RWS, comment > > > {};

         struct http_URI : if_must< istring< 'h', 't', 't', 'p', ':', '/', '/' >, uri::authority, uri::path_abempty, uri::opt_query, uri::opt_fragment > {};
         struct https_URI : if_must< istring< 'h', 't', 't', 'p', 's', ':', '/', '/' >, uri::authority, uri::path_abempty, uri::opt_query, uri::opt_fragment > {};

         struct partial_URI : seq< uri::relative_part, uri::opt_query > {};

         // clang-format on
         struct chunk_size
         {
            using analyze_t = plus< abnf::HEXDIG >::analyze_t;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, std::size_t& size, States&&... /*unused*/ )
            {
               size = 0;
               std::size_t i = 0;
               while( in.size( i + 1 ) >= i + 1 ) {
                  const auto c = in.peek_char( i );
                  if( ( '0' <= c ) && ( c <= '9' ) ) {
                     size <<= 4;
                     size |= std::size_t( c - '0' );
                     ++i;
                     continue;
                  }
                  if( ( 'a' <= c ) && ( c <= 'f' ) ) {
                     size <<= 4;
                     size |= std::size_t( c - 'a' + 10 );
                     ++i;
                     continue;
                  }
                  if( ( 'A' <= c ) && ( c <= 'F' ) ) {
                     size <<= 4;
                     size |= std::size_t( c - 'A' + 10 );
                     ++i;
                     continue;
                  }
                  break;
               }
               in.bump_in_this_line( i );
               return i > 0;
            }
         };
         // clang-format off

         struct chunk_ext_name : token {};
         struct chunk_ext_val : sor< quoted_string, token > {};
         struct chunk_ext : star_must< one< ';' >, chunk_ext_name, if_must< one< '=' >, chunk_ext_val > > {};

         // clang-format on
         struct chunk_data
         {
            using analyze_t = star< abnf::OCTET >::analyze_t;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, const std::size_t size, States&&... /*unused*/ )
            {
               if( in.size( size ) >= size ) {
                  in.bump( size );
                  return true;
               }
               return false;
            }
         };

         namespace internal
         {
            namespace chunk_helper
            {
               template< typename Base >
               struct control;

               template< template< typename... > class Control, typename Rule >
               struct control< Control< Rule > >
                  : Control< Rule >
               {
                  template< apply_mode A,
                            rewind_mode M,
                            template< typename... >
                            class Action,
                            template< typename... >
                            class,
                            typename Input,
                            typename State,
                            typename... States >
                  static bool match( Input& in, State&& /*unused*/, States&&... st )
                  {
                     return Control< Rule >::template match< A, M, Action, Control >( in, st... );
                  }
               };

               template< template< typename... > class Control >
               struct control< Control< chunk_size > >
                  : remove_first_state< Control< chunk_size > >
               {};

               template< template< typename... > class Control >
               struct control< Control< chunk_data > >
                  : remove_first_state< Control< chunk_data > >
               {};

               template< template< typename... > class Control >
               struct bind
               {
                  template< typename Rule >
                  using type = control< Control< Rule > >;
               };

            } // namespace chunk_helper

         } // namespace internal

         struct chunk
         {
            using impl = seq< chunk_size, chunk_ext, abnf::CRLF, chunk_data, abnf::CRLF >;
            using analyze_t = impl::analyze_t;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, States&&... st )
            {
               std::size_t size{};
               return impl::template match< A, M, Action, internal::chunk_helper::bind< Control >::template type >( in, size, st... );
            }
         };

         // clang-format off
         struct last_chunk : seq< plus< one< '0' > >, not_at< digit >, chunk_ext, abnf::CRLF > {};

         struct trailer_part : star< header_field, abnf::CRLF > {};

         struct chunked_body : seq< until< last_chunk, chunk >, trailer_part, abnf::CRLF > {};
         // clang-format on

      } // namespace http

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 7 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/if_then.hpp"
       
#line 1 "tao/pegtl/contrib/if_then.hpp"



#ifndef TAO_PEGTL_CONTRIB_IF_THEN_HPP
#define TAO_PEGTL_CONTRIB_IF_THEN_HPP

#include <type_traits>
#line 16 "tao/pegtl/contrib/if_then.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Cond, typename Then >
         struct if_pair
         {
         };

         template< typename... Pairs >
         struct if_then;

         template< typename Cond, typename Then, typename... Pairs >
         struct if_then< if_pair< Cond, Then >, Pairs... >
            : if_then_else< Cond, Then, if_then< Pairs... > >
         {
            template< typename ElseCond, typename... Thens >
            using else_if_then = if_then< if_pair< Cond, Then >, Pairs..., if_pair< ElseCond, seq< Thens... > > >;

            template< typename... Thens >
            using else_then = if_then_else< Cond, Then, if_then< Pairs..., if_pair< trivial< true >, seq< Thens... > > > >;
         };

         template<>
         struct if_then<>
            : trivial< false >
         {
         };

         template< typename... Pairs >
         struct skip_control< if_then< Pairs... > > : std::true_type
         {
         };

      } // namespace internal

      template< typename Cond, typename... Thens >
      using if_then = internal::if_then< internal::if_pair< Cond, internal::seq< Thens... > > >;

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 8 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/integer.hpp"
       
#line 1 "tao/pegtl/contrib/integer.hpp"



#ifndef TAO_PEGTL_CONTRIB_INTEGER_HPP
#define TAO_PEGTL_CONTRIB_INTEGER_HPP

#include <limits>
#include <type_traits>





namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace integer
      {
         namespace internal
         {
            template< typename I, I Limit, typename Input >
            I actual_convert( const Input& in, std::size_t index )
            {
               static constexpr I cutoff = Limit / 10;
               static constexpr I cutlim = Limit % 10;

               I out = in.peek_char( index ) - '0';
               while( ++index < in.size() ) {
                  const I c = in.peek_char( index ) - '0';
                  if( ( out > cutoff ) || ( ( out == cutoff ) && ( c > cutlim ) ) ) {
                     throw parse_error( "integer out of range", in );
                  }
                  out *= 10;
                  out += c;
               }
               return out;
            }

            template< typename I, typename Input >
            I convert_positive( const Input& in, std::size_t index )
            {
               static constexpr I limit = ( std::numeric_limits< I >::max )();
               return actual_convert< I, limit >( in, index );
            }

            template< typename I, typename Input >
            I convert_negative( const Input& in, std::size_t index )
            {
               using U = typename std::make_unsigned< I >::type;
               static constexpr U limit = static_cast< U >( ( std::numeric_limits< I >::max )() ) + 1;
               return static_cast< I >( ~actual_convert< U, limit >( in, index ) ) + 1;
            }

         } // namespace internal

         struct unsigned_rule
            : plus< digit >
         {
         };

         struct unsigned_action
         {
            // Assumes that 'in' contains a non-empty sequence of ASCII digits.

            template< typename Input, typename State, typename... States >
            static void apply( const Input& in, State& st, States&&... /*unused*/ )
            {
               using T = typename std::decay< decltype( st.converted ) >::type;
               static_assert( std::is_integral< T >::value, "need integral type" );
               static_assert( std::is_unsigned< T >::value, "need unsigned type" );
               st.converted = internal::convert_positive< T >( in, 0 );
            }
         };

         struct signed_rule
            : seq< opt< one< '+', '-' > >, plus< digit > >
         {
         };

         struct signed_action
         {
            // Assumes that 'in' contains a non-empty sequence of ASCII digits,
            // with optional leading sign; with sign, in.size() must be >= 2.

            template< typename Input, typename State, typename... States >
            static void apply( const Input& in, State& st, States&&... /*unused*/ )
            {
               using T = typename std::decay< decltype( st.converted ) >::type;
               static_assert( std::is_integral< T >::value, "need integral type" );
               static_assert( std::is_signed< T >::value, "need signed type" );
               const auto c = in.peek_char();
               if( c == '-' ) {
                  st.converted = internal::convert_negative< T >( in, 1 );
               }
               else {
                  st.converted = internal::convert_positive< T >( in, std::size_t( c == '+' ) );
               }
            }
         };

      } // namespace integer

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 9 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/json.hpp"
       
#line 1 "tao/pegtl/contrib/json.hpp"



#ifndef TAO_PEGTL_CONTRIB_JSON_HPP
#define TAO_PEGTL_CONTRIB_JSON_HPP
#line 14 "tao/pegtl/contrib/json.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace json
      {
         // JSON grammar according to RFC 8259

         // clang-format off
         struct ws : one< ' ', '\t', '\n', '\r' > {};

         template< typename R, typename P = ws >
         struct padr : internal::seq< R, internal::star< P > > {};

         struct begin_array : padr< one< '[' > > {};
         struct begin_object : padr< one< '{' > > {};
         struct end_array : one< ']' > {};
         struct end_object : one< '}' > {};
         struct name_separator : pad< one< ':' >, ws > {};
         struct value_separator : padr< one< ',' > > {};

         struct false_ : string< 'f', 'a', 'l', 's', 'e' > {}; // NOLINT
         struct null : string< 'n', 'u', 'l', 'l' > {};
         struct true_ : string< 't', 'r', 'u', 'e' > {}; // NOLINT

         struct digits : plus< abnf::DIGIT > {};
         struct exp : seq< one< 'e', 'E' >, opt< one< '-', '+'> >, must< digits > > {};
         struct frac : if_must< one< '.' >, digits > {};
         struct int_ : sor< one< '0' >, digits > {}; // NOLINT
         struct number : seq< opt< one< '-' > >, int_, opt< frac >, opt< exp > > {};

         struct xdigit : abnf::HEXDIG {};
         struct unicode : list< seq< one< 'u' >, rep< 4, must< xdigit > > >, one< '\\' > > {};
         struct escaped_char : one< '"', '\\', '/', 'b', 'f', 'n', 'r', 't' > {};
         struct escaped : sor< escaped_char, unicode > {};
         struct unescaped : utf8::range< 0x20, 0x10FFFF > {};
         struct char_ : if_then_else< one< '\\' >, must< escaped >, unescaped > {}; // NOLINT

         struct string_content : until< at< one< '"' > >, must< char_ > > {};
         struct string : seq< one< '"' >, must< string_content >, any >
         {
            using content = string_content;
         };

         struct key_content : until< at< one< '"' > >, must< char_ > > {};
         struct key : seq< one< '"' >, must< key_content >, any >
         {
            using content = key_content;
         };

         struct value;

         struct array_element;
         struct array_content : opt< list_must< array_element, value_separator > > {};
         struct array : seq< begin_array, array_content, must< end_array > >
         {
            using begin = begin_array;
            using end = end_array;
            using element = array_element;
            using content = array_content;
         };

         struct member : if_must< key, name_separator, value > {};
         struct object_content : opt< list_must< member, value_separator > > {};
         struct object : seq< begin_object, object_content, must< end_object > >
         {
            using begin = begin_object;
            using end = end_object;
            using element = member;
            using content = object_content;
         };

         struct value : padr< sor< string, number, object, array, false_, true_, null > > {};
         struct array_element : seq< value > {};

         struct text : seq< star< ws >, value > {};
         // clang-format on

      } // namespace json

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 10 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/json_pointer.hpp"
       
#line 1 "tao/pegtl/contrib/json_pointer.hpp"



#ifndef TAO_PEGTL_CONTRIB_JSON_POINTER_HPP
#define TAO_PEGTL_CONTRIB_JSON_POINTER_HPP






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace json_pointer
      {
         // JSON pointer grammar according to RFC 6901

         // clang-format off
         struct unescaped : utf8::ranges< 0x0, 0x2E, 0x30, 0x7D, 0x7F, 0x10FFFF > {};
         struct escaped : seq< one< '~' >, one< '0', '1' > > {};

         struct reference_token : star< sor< unescaped, escaped > > {};
         struct json_pointer : star< one< '/' >, reference_token > {};
         // clang-format on

         // relative JSON pointer, see ...

         // clang-format off
         struct non_negative_integer : sor< one< '0' >, plus< digit > > {};
         struct relative_json_pointer : seq< non_negative_integer, sor< one< '#' >, json_pointer > > {};
         // clang-format on

      } // namespace json_pointer

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 11 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/parse_tree.hpp"
       
#line 1 "tao/pegtl/contrib/parse_tree.hpp"



#ifndef TAO_PEGTL_CONTRIB_PARSE_TREE_HPP
#define TAO_PEGTL_CONTRIB_PARSE_TREE_HPP

#include <cassert>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>


#line 1 "tao/pegtl/contrib/shuffle_states.hpp"
       
#line 1 "tao/pegtl/contrib/shuffle_states.hpp"



#ifndef TAO_PEGTL_CONTRIB_SHUFFLE_STATES_HPP
#define TAO_PEGTL_CONTRIB_SHUFFLE_STATES_HPP

#include <tuple>
#include <utility>




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< std::size_t N >
         struct rotate_left
         {
            template< std::size_t I, std::size_t S >
            using type = std::integral_constant< std::size_t, ( I + N ) % S >;
         };

         template< std::size_t N >
         struct rotate_right
         {
            template< std::size_t I, std::size_t S >
            using type = std::integral_constant< std::size_t, ( I + S - N ) % S >;
         };

         struct reverse
         {
            template< std::size_t I, std::size_t S >
            using type = std::integral_constant< std::size_t, ( S - 1 ) - I >;
         };

      } // namespace internal

      // Applies 'Shuffle' to the states of start(), success(), failure(), raise(), apply(), and apply0()
      template< typename Base, typename Shuffle >
      struct shuffle_states
         : Base
      {
         template< typename Input, typename Tuple, std::size_t... Is >
         static void start_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::start( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) ) )
         {
            Base::start( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< typename Input, typename... States >
         static void start( const Input& in, States&&... st ) noexcept( noexcept( start_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) ) )
         {
            start_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< typename Input, typename State >
         static void start( const Input& in, State&& st ) noexcept( noexcept( Base::start( in, st ) ) )
         {
            Base::start( in, st );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void success_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::success( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) ) )
         {
            Base::success( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< typename Input, typename... States >
         static void success( const Input& in, States&&... st ) noexcept( noexcept( success_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) ) )
         {
            success_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< typename Input, typename State >
         static void success( const Input& in, State&& st ) noexcept( noexcept( Base::success( in, st ) ) )
         {
            Base::success( in, st );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void failure_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::failure( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) ) )
         {
            Base::failure( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< typename Input, typename... States >
         static void failure( const Input& in, States&&... st ) noexcept( noexcept( failure_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) ) )
         {
            failure_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< typename Input, typename State >
         static void failure( const Input& in, State&& st ) noexcept( noexcept( Base::failure( in, st ) ) )
         {
            Base::failure( in, st );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void raise_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ )
         {
            Base::raise( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< typename Input, typename... States >
         static void raise( const Input& in, States&&... st )
         {
            raise_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< typename Input, typename State >
         static void raise( const Input& in, State&& st )
         {
            Base::raise( in, st );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename Tuple, std::size_t... Is >
         static auto apply_impl( const Iterator& begin, const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::template apply< Action >( begin, in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) ) )
            -> decltype( Base::template apply< Action >( begin, in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) )
         {
            return Base::template apply< Action >( begin, in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename... States >
         static auto apply( const Iterator& begin, const Input& in, States&&... st ) noexcept( noexcept( apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) ) )
            -> decltype( apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) )
         {
            return apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename State >
         static auto apply( const Iterator& begin, const Input& in, State&& st ) noexcept( noexcept( Base::template apply< Action >( begin, in, st ) ) )
            -> decltype( Base::template apply< Action >( begin, in, st ) )
         {
            return Base::template apply< Action >( begin, in, st );
         }

         template< template< typename... > class Action, typename Input, typename Tuple, std::size_t... Is >
         static auto apply0_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::template apply0< Action >( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) ) )
            -> decltype( Base::template apply0< Action >( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... ) )
         {
            return Base::template apply0< Action >( in, std::get< Shuffle::template type< Is, sizeof...( Is ) >::value >( t )... );
         }

         template< template< typename... > class Action, typename Input, typename... States >
         static auto apply0( const Input& in, States&&... st ) noexcept( noexcept( apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) ) )
            -> decltype( apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() ) )
         {
            return apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) >() );
         }

         template< template< typename... > class Action, typename Input, typename State >
         static auto apply0( const Input& in, State&& st ) noexcept( noexcept( Base::template apply0< Action >( in, st ) ) )
            -> decltype( Base::template apply0< Action >( in, st ) )
         {
            return Base::template apply0< Action >( in, st );
         }
      };

      template< typename Base, std::size_t N = 1 >
      using rotate_states_left = shuffle_states< Base, internal::rotate_left< N > >;

      template< typename Base, std::size_t N = 1 >
      using rotate_states_right = shuffle_states< Base, internal::rotate_right< N > >;

      template< typename Base >
      using reverse_states = shuffle_states< Base, internal::reverse >;

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 18 "tao/pegtl/contrib/parse_tree.hpp"
#line 35 "tao/pegtl/contrib/parse_tree.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace parse_tree
      {
         template< typename T >
         struct basic_node
         {
            using node_t = T;
            using children_t = std::vector< std::unique_ptr< node_t > >;
            children_t children;

            std::type_index id = std::type_index( typeid( void ) );
            std::string source;

            TAO_PEGTL_NAMESPACE::internal::iterator m_begin;
            TAO_PEGTL_NAMESPACE::internal::iterator m_end;

            // each node will be default constructed
            basic_node() = default;

            // no copy/move is necessary
            // (nodes are always owned/handled by a std::unique_ptr)
            basic_node( const basic_node& ) = delete;
            basic_node( basic_node&& ) = delete;

            ~basic_node() = default;

            // no assignment either
            basic_node& operator=( const basic_node& ) = delete;
            basic_node& operator=( basic_node&& ) = delete;

            bool is_root() const noexcept
            {
               return id == typeid( void );
            }

            template< typename U >
            bool is() const noexcept
            {
               return id == typeid( U );
            }

            std::string name() const
            {
               assert( !is_root() );
               return TAO_PEGTL_NAMESPACE::internal::demangle( id.name() );
            }

            position begin() const
            {
               return position( m_begin, source );
            }

            position end() const
            {
               return position( m_end, source );
            }

            bool has_content() const noexcept
            {
               return m_end.data != nullptr;
            }

            std::string string() const
            {
               assert( has_content() );
               return std::string( m_begin.data, m_end.data );
            }

            // Compatibility, remove with 3.0.0
            std::string content() const
            {
               return string();
            }

            template< tracking_mode P = tracking_mode::eager, typename Eol = eol::lf_crlf >
            memory_input< P, Eol > as_memory_input() const
            {
               assert( has_content() );
               return { m_begin.data, m_end.data, source, m_begin.byte, m_begin.line, m_begin.byte_in_line };
            }

            template< typename... States >
            void remove_content( States&&... /*unused*/ ) noexcept
            {
               m_end.reset();
            }

            // all non-root nodes are initialized by calling this method
            template< typename Rule, typename Input, typename... States >
            void start( const Input& in, States&&... /*unused*/ )
            {
               id = typeid( Rule );
               source = in.source();
               m_begin = TAO_PEGTL_NAMESPACE::internal::iterator( in.iterator() );
            }

            // if parsing of the rule succeeded, this method is called
            template< typename Rule, typename Input, typename... States >
            void success( const Input& in, States&&... /*unused*/ ) noexcept
            {
               m_end = TAO_PEGTL_NAMESPACE::internal::iterator( in.iterator() );
            }

            // if parsing of the rule failed, this method is called
            template< typename Rule, typename Input, typename... States >
            void failure( const Input& /*unused*/, States&&... /*unused*/ ) noexcept
            {
            }

            // if parsing succeeded and the (optional) transform call
            // did not discard the node, it is appended to its parent.
            // note that "child" is the node whose Rule just succeeded
            // and "*this" is the parent where the node should be appended.
            template< typename... States >
            void emplace_back( std::unique_ptr< node_t >&& child, States&&... /*unused*/ )
            {
               assert( child );
               children.emplace_back( std::move( child ) );
            }
         };

         struct node
            : basic_node< node >
         {
         };

         namespace internal
         {
            template< typename >
            struct is_try_catch_type
               : std::false_type
            {
            };

            template< typename Exception, typename... Rules >
            struct is_try_catch_type< TAO_PEGTL_NAMESPACE::internal::try_catch_type< Exception, Rules... > >
               : std::true_type
            {
            };

            template< typename Node >
            struct state
            {
               std::vector< std::unique_ptr< Node > > stack;

               state()
               {
                  emplace_back();
               }

               void emplace_back()
               {
                  stack.emplace_back( std::unique_ptr< Node >( new Node ) );
               }

               std::unique_ptr< Node >& back() noexcept
               {
                  assert( !stack.empty() );
                  return stack.back();
               }

               void pop_back() noexcept
               {
                  assert( !stack.empty() );
                  return stack.pop_back();
               }
            };

            template< typename Selector, typename... Parameters >
            void transform( Parameters&&... /*unused*/ ) noexcept
            {
            }

            template< typename Selector, typename Input, typename Node, typename... States >
            auto transform( const Input& in, std::unique_ptr< Node >& n, States&&... st ) noexcept( noexcept( Selector::transform( in, n, st... ) ) )
               -> decltype( Selector::transform( in, n, st... ), void() )
            {
               Selector::transform( in, n, st... );
            }

            template< typename Selector, typename Input, typename Node, typename... States >
            auto transform( const Input& /*unused*/, std::unique_ptr< Node >& n, States&&... st ) noexcept( noexcept( Selector::transform( n, st... ) ) )
               -> decltype( Selector::transform( n, st... ), void() )
            {
               Selector::transform( n, st... );
            }

            template< typename Rule, template< typename... > class Selector >
            struct is_selected_node
               : std::integral_constant< bool, !TAO_PEGTL_NAMESPACE::internal::skip_control< Rule >::value && Selector< Rule >::value >
            {
            };

            template< unsigned Level, typename Analyse, template< typename... > class Selector >
            struct is_leaf
               : std::false_type
            {
            };

            template< analysis::rule_type Type, template< typename... > class Selector >
            struct is_leaf< 0, analysis::generic< Type >, Selector >
               : std::true_type
            {
            };

            template< analysis::rule_type Type, std::size_t Count, template< typename... > class Selector >
            struct is_leaf< 0, analysis::counted< Type, Count >, Selector >
               : std::true_type
            {
            };

            template< analysis::rule_type Type, typename... Rules, template< typename... > class Selector >
            struct is_leaf< 0, analysis::generic< Type, Rules... >, Selector >
               : std::false_type
            {
            };

            template< analysis::rule_type Type, std::size_t Count, typename... Rules, template< typename... > class Selector >
            struct is_leaf< 0, analysis::counted< Type, Count, Rules... >, Selector >
               : std::false_type
            {
            };

            template< bool... >
            struct bool_sequence;

            template< bool... Bs >
            struct is_all
               : std::is_same< bool_sequence< Bs..., true >, bool_sequence< true, Bs... > >
            {
            };

            template< bool... Bs >
            struct is_none
               : std::integral_constant< bool, !is_all< !Bs... >::value >
            {
            };

            template< unsigned Level, typename Rule, template< typename... > class Selector >
            using is_unselected_leaf = std::integral_constant< bool, !is_selected_node< Rule, Selector >::value && is_leaf< Level, typename Rule::analyze_t, Selector >::value >;

            template< unsigned Level, analysis::rule_type Type, typename... Rules, template< typename... > class Selector >
            struct is_leaf< Level, analysis::generic< Type, Rules... >, Selector >
               : is_all< is_unselected_leaf< Level - 1, Rules, Selector >::value... >
            {
            };

            template< unsigned Level, analysis::rule_type Type, std::size_t Count, typename... Rules, template< typename... > class Selector >
            struct is_leaf< Level, analysis::counted< Type, Count, Rules... >, Selector >
               : is_all< is_unselected_leaf< Level - 1, Rules, Selector >::value... >
            {
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            struct make_control
            {
               template< typename Rule, bool, bool >
               struct state_handler;

               template< typename Rule >
               using type = rotate_states_right< state_handler< Rule, is_selected_node< Rule, Selector >::value, is_leaf< 8, typename Rule::analyze_t, Selector >::value > >;
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            template< typename Rule >
            struct make_control< Node, Selector, Control >::state_handler< Rule, false, true >
               : remove_first_state< Control< Rule > >
            {
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            template< typename Rule >
            struct make_control< Node, Selector, Control >::state_handler< Rule, false, false >
               : remove_first_state< Control< Rule > >
            {
               template< apply_mode A,
                         rewind_mode M,
                         template< typename... >
                         class Action,
                         template< typename... >
                         class Control2,
                         typename Input,
                         typename... States >
               static bool match( Input& in, States&&... st )
               {
                  auto& state = std::get< sizeof...( st ) - 1 >( std::tie( st... ) );
                  if( is_try_catch_type< Rule >::value ) {
                     internal::state< Node > tmp;
                     tmp.emplace_back();
                     tmp.stack.swap( state.stack );
                     const bool result = Control< Rule >::template match< A, M, Action, Control2 >( in, st... );
                     tmp.stack.swap( state.stack );
                     if( result ) {
                        for( auto& c : tmp.back()->children ) {
                           state.back()->children.emplace_back( std::move( c ) );
                        }
                     }
                     return result;
                  }
                  state.emplace_back();
                  const bool result = Control< Rule >::template match< A, M, Action, Control2 >( in, st... );
                  if( result ) {
                     auto n = std::move( state.back() );
                     state.pop_back();
                     for( auto& c : n->children ) {
                        state.back()->children.emplace_back( std::move( c ) );
                     }
                  }
                  else {
                     state.pop_back();
                  }
                  return result;
               }
            };

            template< typename Node, template< typename... > class Selector, template< typename... > class Control >
            template< typename Rule, bool B >
            struct make_control< Node, Selector, Control >::state_handler< Rule, true, B >
               : remove_first_state< Control< Rule > >
            {
               template< typename Input, typename... States >
               static void start( const Input& in, state< Node >& state, States&&... st )
               {
                  Control< Rule >::start( in, st... );
                  state.emplace_back();
                  state.back()->template start< Rule >( in, st... );
               }

               template< typename Input, typename... States >
               static void success( const Input& in, state< Node >& state, States&&... st )
               {
                  Control< Rule >::success( in, st... );
                  auto n = std::move( state.back() );
                  state.pop_back();
                  n->template success< Rule >( in, st... );
                  transform< Selector< Rule > >( in, n, st... );
                  if( n ) {
                     state.back()->emplace_back( std::move( n ), st... );
                  }
               }

               template< typename Input, typename... States >
               static void failure( const Input& in, state< Node >& state, States&&... st ) noexcept( noexcept( Control< Rule >::failure( in, st... ) ) && noexcept( std::declval< Node& >().template failure< Rule >( in, st... ) ) )
               {
                  Control< Rule >::failure( in, st... );
                  state.back()->template failure< Rule >( in, st... );
                  state.pop_back();
               }
            };

            template< typename >
            using store_all = std::true_type;

            template< typename >
            struct selector;

            template<>
            struct selector< std::tuple<> >
            {
               using type = std::false_type;
            };

            template< typename T >
            struct selector< std::tuple< T > >
            {
               using type = typename T::type;
            };

            template< typename... Ts >
            struct selector< std::tuple< Ts... > >
            {
               static_assert( sizeof...( Ts ) == 0, "multiple matches found" );
            };

            template< typename Rule, typename Collection >
            using select_tuple = typename std::conditional< Collection::template contains< Rule >::value, std::tuple< Collection >, std::tuple<> >::type;

         } // namespace internal

         template< typename Rule, typename... Collections >
         struct selector
            : internal::selector< decltype( std::tuple_cat( std::declval< internal::select_tuple< Rule, Collections > >()... ) ) >::type
         {};

         template< typename Base >
         struct apply
            : std::true_type
         {
            template< typename... Rules >
            struct on
            {
               using type = Base;

               template< typename Rule >
               using contains = internal::is_none< std::is_same< Rule, Rules >::value... >;
            };
         };

         struct store_content
            : apply< store_content >
         {};

         // some nodes don't need to store their content
         struct remove_content
            : apply< remove_content >
         {
            template< typename Node, typename... States >
            static void transform( std::unique_ptr< Node >& n, States&&... st ) noexcept( noexcept( n->Node::remove_content( st... ) ) )
            {
               n->remove_content( st... );
            }
         };

         // if a node has only one child, replace the node with its child, otherwise remove content
         struct fold_one
            : apply< fold_one >
         {
            template< typename Node, typename... States >
            static void transform( std::unique_ptr< Node >& n, States&&... st ) noexcept( noexcept( n->children.size(), n->Node::remove_content( st... ) ) )
            {
               if( n->children.size() == 1 ) {
                  n = std::move( n->children.front() );
               }
               else {
                  n->remove_content( st... );
               }
            }
         };

         // if a node has no children, discard the node, otherwise remove content
         struct discard_empty
            : apply< discard_empty >
         {
            template< typename Node, typename... States >
            static void transform( std::unique_ptr< Node >& n, States&&... st ) noexcept( noexcept( n->children.empty(), n->Node::remove_content( st... ) ) )
            {
               if( n->children.empty() ) {
                  n.reset();
               }
               else {
                  n->remove_content( st... );
               }
            }
         };

         template< typename Rule,
                   typename Node,
                   template< typename... > class Selector = internal::store_all,
                   template< typename... > class Action = nothing,
                   template< typename... > class Control = normal,
                   typename Input,
                   typename... States >
         std::unique_ptr< Node > parse( Input&& in, States&&... st )
         {
            internal::state< Node > state;
            if( !TAO_PEGTL_NAMESPACE::parse< Rule, Action, internal::make_control< Node, Selector, Control >::template type >( in, st..., state ) ) {
               return nullptr;
            }
            assert( state.stack.size() == 1 );
            return std::move( state.back() );
         }

         template< typename Rule,
                   template< typename... > class Selector = internal::store_all,
                   template< typename... > class Action = nothing,
                   template< typename... > class Control = normal,
                   typename Input,
                   typename... States >
         std::unique_ptr< node > parse( Input&& in, States&&... st )
         {
            return parse< Rule, node, Selector, Action, Control >( in, st... );
         }

      } // namespace parse_tree

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 12 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/parse_tree_to_dot.hpp"
       
#line 1 "tao/pegtl/contrib/parse_tree_to_dot.hpp"



#ifndef TAO_PEGTL_CONTRIB_PARSE_TREE_TO_DOT_HPP
#define TAO_PEGTL_CONTRIB_PARSE_TREE_TO_DOT_HPP

#include <cassert>
#include <ostream>
#include <string>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace parse_tree
      {
         namespace internal
         {
            inline void escape( std::ostream& os, const char* p, const std::size_t s )
            {
               static const char* h = "0123456789abcdef";

               const char* l = p;
               const char* const e = p + s;
               while( p != e ) {
                  const unsigned char c = *p;
                  if( c == '\\' ) {
                     os.write( l, p - l );
                     l = ++p;
                     os << "\\\\";
                  }
                  else if( c == '"' ) {
                     os.write( l, p - l );
                     l = ++p;
                     os << "\\\"";
                  }
                  else if( c < 32 ) {
                     os.write( l, p - l );
                     l = ++p;
                     switch( c ) {
                        case '\b':
                           os << "\\b";
                           break;
                        case '\f':
                           os << "\\f";
                           break;
                        case '\n':
                           os << "\\n";
                           break;
                        case '\r':
                           os << "\\r";
                           break;
                        case '\t':
                           os << "\\t";
                           break;
                        default:
                           os << "\\u00" << h[ ( c & 0xf0 ) >> 4 ] << h[ c & 0x0f ];
                     }
                  }
                  else if( c == 127 ) {
                     os.write( l, p - l );
                     l = ++p;
                     os << "\\u007f";
                  }
                  else {
                     ++p;
                  }
               }
               os.write( l, p - l );
            }

            inline void escape( std::ostream& os, const std::string& s )
            {
               escape( os, s.data(), s.size() );
            }

            template< typename Node >
            void print_dot_node( std::ostream& os, const Node& n, const std::string& s )
            {
               os << "  x" << &n << " [ label=\"";
               escape( os, s );
               if( n.has_content() ) {
                  os << "\\n";
                  escape( os, n.m_begin.data, n.m_end.data - n.m_begin.data );
               }
               os << "\" ]\n";
               if( !n.children.empty() ) {
                  os << "  x" << &n << " -> { ";
                  for( auto& up : n.children ) {
                     os << "x" << up.get() << ( ( up == n.children.back() ) ? " }\n" : ", " );
                  }
                  for( auto& up : n.children ) {
                     print_dot_node( os, *up, up->name() );
                  }
               }
            }

         } // namespace internal

         template< typename Node >
         void print_dot( std::ostream& os, const Node& n )
         {
            os << "digraph parse_tree\n{\n";
            internal::print_dot_node( os, n, n.is_root() ? "ROOT" : n.name() );
            os << "}\n";
         }

      } // namespace parse_tree

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 13 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/raw_string.hpp"
       
#line 1 "tao/pegtl/contrib/raw_string.hpp"



#ifndef TAO_PEGTL_CONTRIB_RAW_STRING_HPP
#define TAO_PEGTL_CONTRIB_RAW_STRING_HPP

#include <cstddef>
#include <type_traits>
#line 25 "tao/pegtl/contrib/raw_string.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< char Open, char Marker >
         struct raw_string_open
         {
            using analyze_t = analysis::generic< analysis::rule_type::any >;

            template< apply_mode A,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, std::size_t& marker_size, States&&... /*unused*/ ) noexcept( noexcept( in.size( 0 ) ) )
            {
               if( in.empty() || ( in.peek_char( 0 ) != Open ) ) {
                  return false;
               }
               for( std::size_t i = 1; i < in.size( i + 1 ); ++i ) {
                  switch( const auto c = in.peek_char( i ) ) {
                     case Open:
                        marker_size = i + 1;
                        in.bump_in_this_line( marker_size );
                        eol::match( in );
                        return true;
                     case Marker:
                        break;
                     default:
                        return false;
                  }
               }
               return false;
            }
         };

         template< char Open, char Marker >
         struct skip_control< raw_string_open< Open, Marker > > : std::true_type
         {
         };

         template< char Marker, char Close >
         struct at_raw_string_close
         {
            using analyze_t = analysis::generic< analysis::rule_type::opt >;

            template< apply_mode A,
                      rewind_mode,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, const std::size_t& marker_size, States&&... /*unused*/ ) noexcept( noexcept( in.size( 0 ) ) )
            {
               if( in.size( marker_size ) < marker_size ) {
                  return false;
               }
               if( in.peek_char( 0 ) != Close ) {
                  return false;
               }
               if( in.peek_char( marker_size - 1 ) != Close ) {
                  return false;
               }
               for( std::size_t i = 0; i < ( marker_size - 2 ); ++i ) {
                  if( in.peek_char( i + 1 ) != Marker ) {
                     return false;
                  }
               }
               return true;
            }
         };

         template< char Marker, char Close >
         struct skip_control< at_raw_string_close< Marker, Close > > : std::true_type
         {
         };

         template< typename Cond, typename... Rules >
         struct raw_string_until;

         template< typename Cond >
         struct raw_string_until< Cond >
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, star< not_at< Cond >, not_at< eof >, bytes< 1 > >, Cond >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, const std::size_t& marker_size, States&&... st )
            {
               auto m = in.template mark< M >();

               while( !Control< Cond >::template match< A, rewind_mode::required, Action, Control >( in, marker_size, st... ) ) {
                  if( in.empty() ) {
                     return false;
                  }
                  in.bump();
               }
               return m( true );
            }
         };

         template< typename Cond, typename... Rules >
         struct raw_string_until
         {
            using analyze_t = analysis::generic< analysis::rule_type::seq, star< not_at< Cond >, not_at< eof >, Rules... >, Cond >;

            template< apply_mode A,
                      rewind_mode M,
                      template< typename... >
                      class Action,
                      template< typename... >
                      class Control,
                      typename Input,
                      typename... States >
            static bool match( Input& in, const std::size_t& marker_size, States&&... st )
            {
               auto m = in.template mark< M >();
               using m_t = decltype( m );

               while( !Control< Cond >::template match< A, rewind_mode::required, Action, Control >( in, marker_size, st... ) ) {
                  if( in.empty() || ( !Control< seq< Rules... > >::template match< A, m_t::next_rewind_mode, Action, Control >( in, st... ) ) ) {
                     return false;
                  }
               }
               return m( true );
            }
         };

         template< typename Cond, typename... Rules >
         struct skip_control< raw_string_until< Cond, Rules... > > : std::true_type
         {
         };

      } // namespace internal

      // raw_string matches Lua-style long literals.
      //
      // The following description was taken from the Lua documentation
      // (see http://www.lua.org/docs.html):
      //
      // - An "opening long bracket of level n" is defined as an opening square
      //   bracket followed by n equal signs followed by another opening square
      //   bracket. So, an opening long bracket of level 0 is written as `[[`,
      //   an opening long bracket of level 1 is written as `[=[`, and so on.
      // - A "closing long bracket" is defined similarly; for instance, a closing
      //   long bracket of level 4 is written as `]====]`.
      // - A "long literal" starts with an opening long bracket of any level and
      //   ends at the first closing long bracket of the same level. It can
      //   contain any text except a closing bracket of the same level.
      // - Literals in this bracketed form can run for several lines, do not
      //   interpret any escape sequences, and ignore long brackets of any other
      //   level.
      // - For convenience, when the opening long bracket is immediately followed
      //   by a newline, the newline is not included in the string.
      //
      // Note that unlike Lua's long literal, a raw_string is customizable to use
      // other characters than `[`, `=` and `]` for matching. Also note that Lua
      // introduced newline-specific replacements in Lua 5.2, which we do not
      // support on the grammar level.

      template< char Open, char Marker, char Close, typename... Contents >
      struct raw_string
      {
         // This is used for binding the apply()-method and for error-reporting
         // when a raw string is not closed properly or has invalid content.
         struct content
            : internal::raw_string_until< internal::at_raw_string_close< Marker, Close >, Contents... >
         {
         };

         using analyze_t = typename internal::seq< internal::bytes< 1 >, content, internal::bytes< 1 > >::analyze_t;

         template< apply_mode A,
                   rewind_mode M,
                   template< typename... >
                   class Action,
                   template< typename... >
                   class Control,
                   typename Input,
                   typename... States >
         static bool match( Input& in, States&&... st )
         {
            std::size_t marker_size;
            if( internal::raw_string_open< Open, Marker >::template match< A, M, Action, Control >( in, marker_size, st... ) ) {
               internal::must< content >::template match< A, M, Action, Control >( in, marker_size, st... );
               in.bump_in_this_line( marker_size );
               return true;
            }
            return false;
         }
      };

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 14 "amalgamated.hpp"

#line 1 "tao/pegtl/contrib/remove_last_states.hpp"
       
#line 1 "tao/pegtl/contrib/remove_last_states.hpp"



#ifndef TAO_PEGTL_CONTRIB_REMOVE_LAST_STATES_HPP
#define TAO_PEGTL_CONTRIB_REMOVE_LAST_STATES_HPP

#include <tuple>
#include <utility>




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      // Remove the last N states of start(), success(), failure(), raise(), apply(), and apply0()
      template< typename Base, std::size_t N >
      struct remove_last_states
         : Base
      {
         template< typename Input, typename Tuple, std::size_t... Is >
         static void start_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::start( in, std::get< Is >( t )... ) ) )
         {
            Base::start( in, std::get< Is >( t )... );
         }

         template< typename Input, typename... States >
         static void start( const Input& in, States&&... st ) noexcept( noexcept( start_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) ) )
         {
            start_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void success_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::success( in, std::get< Is >( t )... ) ) )
         {
            Base::success( in, std::get< Is >( t )... );
         }

         template< typename Input, typename... States >
         static void success( const Input& in, States&&... st ) noexcept( noexcept( success_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) ) )
         {
            success_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void failure_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::failure( in, std::get< Is >( t )... ) ) )
         {
            Base::failure( in, std::get< Is >( t )... );
         }

         template< typename Input, typename... States >
         static void failure( const Input& in, States&&... st ) noexcept( noexcept( failure_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) ) )
         {
            failure_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }

         template< typename Input, typename Tuple, std::size_t... Is >
         static void raise_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ )
         {
            Base::raise( in, std::get< Is >( t )... );
         }

         template< typename Input, typename... States >
         static void raise( const Input& in, States&&... st )
         {
            raise_impl( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename Tuple, std::size_t... Is >
         static auto apply_impl( const Iterator& begin, const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::template apply< Action >( begin, in, std::get< Is >( t )... ) ) )
            -> decltype( Base::template apply< Action >( begin, in, std::get< Is >( t )... ) )
         {
            return Base::template apply< Action >( begin, in, std::get< Is >( t )... );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename... States >
         static auto apply( const Iterator& begin, const Input& in, States&&... st ) noexcept( noexcept( apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) ) )
            -> decltype( apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) )
         {
            return apply_impl< Action >( begin, in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }

         template< template< typename... > class Action, typename Input, typename Tuple, std::size_t... Is >
         static auto apply0_impl( const Input& in, const Tuple& t, internal::index_sequence< Is... > /*unused*/ ) noexcept( noexcept( Base::template apply0< Action >( in, std::get< Is >( t )... ) ) )
            -> decltype( Base::template apply0< Action >( in, std::get< Is >( t )... ) )
         {
            return Base::template apply0< Action >( in, std::get< Is >( t )... );
         }

         template< template< typename... > class Action, typename Input, typename... States >
         static auto apply0( const Input& in, States&&... st ) noexcept( noexcept( apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) ) )
            -> decltype( apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() ) )
         {
            return apply0_impl< Action >( in, std::tie( st... ), internal::make_index_sequence< sizeof...( st ) - N >() );
         }
      };

      template< typename Base >
      using remove_last_state = remove_last_states< Base, 1 >;

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 16 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/rep_one_min_max.hpp"
       
#line 1 "tao/pegtl/contrib/rep_one_min_max.hpp"



#ifndef TAO_PEGTL_CONTRIB_REP_ONE_MIN_MAX_HPP
#define TAO_PEGTL_CONTRIB_REP_ONE_MIN_MAX_HPP

#include <algorithm>
#line 16 "tao/pegtl/contrib/rep_one_min_max.hpp"
namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< unsigned Min, unsigned Max, char C >
         struct rep_one_min_max
         {
            using analyze_t = analysis::counted< analysis::rule_type::any, Min >;

            static_assert( Min <= Max, "invalid rep_one_min_max rule (maximum number of repetitions smaller than minimum)" );

            template< typename Input >
            static bool match( Input& in )
            {
               const auto size = in.size( Max + 1 );
               if( size < Min ) {
                  return false;
               }
               std::size_t i = 0;
               while( ( i < size ) && ( in.peek_char( i ) == C ) ) {
                  ++i;
               }
               if( ( Min <= i ) && ( i <= Max ) ) {
                  bump_help< result_on_found::success, Input, char, C >( in, i );
                  return true;
               }
               return false;
            }
         };

         template< unsigned Min, unsigned Max, char C >
         struct skip_control< rep_one_min_max< Min, Max, C > > : std::true_type
         {
         };

      } // namespace internal

      inline namespace ascii
      {
         template< unsigned Min, unsigned Max, char C >
         struct rep_one_min_max : internal::rep_one_min_max< Min, Max, C >
         {
         };

      } // namespace ascii

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 17 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/rep_string.hpp"
       
#line 1 "tao/pegtl/contrib/rep_string.hpp"



#ifndef TAO_PEGTL_CONTRIB_REP_STRING_HPP
#define TAO_PEGTL_CONTRIB_REP_STRING_HPP

#include <cstddef>




namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< std::size_t, typename, char... >
         struct make_rep_string;

         template< char... Ss, char... Cs >
         struct make_rep_string< 0, string< Ss... >, Cs... >
         {
            using type = string< Ss... >;
         };

         template< std::size_t N, char... Ss, char... Cs >
         struct make_rep_string< N, string< Ss... >, Cs... >
            : make_rep_string< N - 1, string< Ss..., Cs... >, Cs... >
         {};

      } // namespace internal

      inline namespace ascii
      {
         template< std::size_t N, char... Cs >
         struct rep_string
            : internal::make_rep_string< N, internal::string<>, Cs... >::type
         {};

      } // namespace ascii

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 18 "amalgamated.hpp"

#line 1 "tao/pegtl/contrib/to_string.hpp"
       
#line 1 "tao/pegtl/contrib/to_string.hpp"



#ifndef TAO_PEGTL_CONTRIB_TO_STRING_HPP
#define TAO_PEGTL_CONTRIB_TO_STRING_HPP

#include <string>



namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename >
         struct to_string;

         template< template< char... > class X, char... Cs >
         struct to_string< X< Cs... > >
         {
            static std::string get()
            {
               const char s[] = { Cs..., 0 };
               return std::string( s, sizeof...( Cs ) );
            }
         };

      } // namespace internal

      template< typename T >
      std::string to_string()
      {
         return internal::to_string< T >::get();
      }

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 20 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/tracer.hpp"
       
#line 1 "tao/pegtl/contrib/tracer.hpp"



#ifndef TAO_PEGTL_CONTRIB_TRACER_HPP
#define TAO_PEGTL_CONTRIB_TRACER_HPP

#include <cassert>
#include <iomanip>
#include <iostream>
#include <utility>
#include <vector>






namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace internal
      {
         template< typename Input >
         void print_current( const Input& in )
         {
            if( in.empty() ) {
               std::cerr << "<eof>";
            }
            else {
               const auto c = in.peek_uint8();
               switch( c ) {
                  case 0:
                     std::cerr << "<nul> = ";
                     break;
                  case 9:
                     std::cerr << "<ht> = ";
                     break;
                  case 10:
                     std::cerr << "<lf> = ";
                     break;
                  case 13:
                     std::cerr << "<cr> = ";
                     break;
                  default:
                     if( isprint( c ) ) {
                        std::cerr << '\'' << c << "' = ";
                     }
               }
               std::cerr << "(char)" << unsigned( c );
            }
         }

      } // namespace internal

      struct trace_state
      {
         unsigned rule = 0;
         unsigned line = 0;
         std::vector< unsigned > stack;
      };

#if defined( _MSC_VER ) && ( _MSC_VER < 1910 )

      template< typename Rule >
      struct tracer
         : normal< Rule >
      {
         template< typename Input, typename... States >
         static void start( const Input& in, States&&... /*unused*/ )
         {
            std::cerr << in.position() << "  start  " << internal::demangle< Rule >() << "; current ";
            print_current( in );
            std::cerr << std::endl;
         }

         template< typename Input, typename... States >
         static void start( const Input& in, trace_state& ts, States&&... st )
         {
            std::cerr << std::setw( 6 ) << ++ts.line << " " << std::setw( 6 ) << ++ts.rule << " ";
            start( in, st... );
            ts.stack.push_back( ts.rule );
         }

         template< typename Input, typename... States >
         static void success( const Input& in, States&&... /*unused*/ )
         {
            std::cerr << in.position() << " success " << internal::demangle< Rule >() << "; next ";
            print_current( in );
            std::cerr << std::endl;
         }

         template< typename Input, typename... States >
         static void success( const Input& in, trace_state& ts, States&&... st )
         {
            assert( !ts.stack.empty() );
            std::cerr << std::setw( 6 ) << ++ts.line << " " << std::setw( 6 ) << ts.stack.back() << " ";
            success( in, st... );
            ts.stack.pop_back();
         }

         template< typename Input, typename... States >
         static void failure( const Input& in, States&&... /*unused*/ )
         {
            std::cerr << in.position() << " failure " << internal::demangle< Rule >() << std::endl;
         }

         template< typename Input, typename... States >
         static void failure( const Input& in, trace_state& ts, States&&... st )
         {
            assert( !ts.stack.empty() );
            std::cerr << std::setw( 6 ) << ++ts.line << " " << std::setw( 6 ) << ts.stack.back() << " ";
            failure( in, st... );
            ts.stack.pop_back();
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename... States >
         static auto apply( const Iterator& begin, const Input& in, States&&... st )
            -> decltype( normal< Rule >::template apply< Action >( begin, in, st... ) )
         {
            std::cerr << in.position() << "  apply  " << internal::demangle< Rule >() << std::endl;
            return normal< Rule >::template apply< Action >( begin, in, st... );
         }

         template< template< typename... > class Action, typename Iterator, typename Input, typename... States >
         static auto apply( const Iterator& begin, const Input& in, trace_state& ts, States&&... st )
            -> decltype( apply< Action >( begin, in, st... ) )
         {
            std::cerr << std::setw( 6 ) << ++ts.line << "        ";
            return apply< Action >( begin, in, st... );
         }

         template< template< typename... > class Action, typename Input, typename... States >
         static auto apply0( const Input& in, States&&... st )
            -> decltype( normal< Rule >::template apply0< Action >( in, st... ) )
         {
            std::cerr << in.position() << "  apply0 " << internal::demangle< Rule >() << std::endl;
            return normal< Rule >::template apply0< Action >( in, st... );
         }

         template< template< typename... > class Action, typename Input, typename... States >
         static auto apply0( const Input& in, trace_state& ts, States&&... st )
            -> decltype( apply0< Action >( in, st... ) )
         {
            std::cerr << std::setw( 6 ) << ++ts.line << "        ";
            return apply0< Action >( in, st... );
         }
      };

#else

      template< template< typename... > class Base >
      struct trace
      {
         template< typename Rule >
         struct control
            : Base< Rule >
         {
            template< typename Input, typename... States >
            static void start( const Input& in, States&&... st )
            {
               std::cerr << in.position() << "  start  " << internal::demangle< Rule >() << "; current ";
               print_current( in );
               std::cerr << std::endl;
               Base< Rule >::start( in, st... );
            }

            template< typename Input, typename... States >
            static void start( const Input& in, trace_state& ts, States&&... st )
            {
               std::cerr << std::setw( 6 ) << ++ts.line << " " << std::setw( 6 ) << ++ts.rule << " ";
               start( in, st... );
               ts.stack.push_back( ts.rule );
            }

            template< typename Input, typename... States >
            static void success( const Input& in, States&&... st )
            {
               std::cerr << in.position() << " success " << internal::demangle< Rule >() << "; next ";
               print_current( in );
               std::cerr << std::endl;
               Base< Rule >::success( in, st... );
            }

            template< typename Input, typename... States >
            static void success( const Input& in, trace_state& ts, States&&... st )
            {
               assert( !ts.stack.empty() );
               std::cerr << std::setw( 6 ) << ++ts.line << " " << std::setw( 6 ) << ts.stack.back() << " ";
               success( in, st... );
               ts.stack.pop_back();
            }

            template< typename Input, typename... States >
            static void failure( const Input& in, States&&... st )
            {
               std::cerr << in.position() << " failure " << internal::demangle< Rule >() << std::endl;
               Base< Rule >::failure( in, st... );
            }

            template< typename Input, typename... States >
            static void failure( const Input& in, trace_state& ts, States&&... st )
            {
               assert( !ts.stack.empty() );
               std::cerr << std::setw( 6 ) << ++ts.line << " " << std::setw( 6 ) << ts.stack.back() << " ";
               failure( in, st... );
               ts.stack.pop_back();
            }

            template< template< typename... > class Action, typename Iterator, typename Input, typename... States >
            static auto apply( const Iterator& begin, const Input& in, States&&... st )
               -> decltype( Base< Rule >::template apply< Action >( begin, in, st... ) )
            {
               std::cerr << in.position() << "  apply  " << internal::demangle< Rule >() << std::endl;
               return Base< Rule >::template apply< Action >( begin, in, st... );
            }

            template< template< typename... > class Action, typename Iterator, typename Input, typename... States >
            static auto apply( const Iterator& begin, const Input& in, trace_state& ts, States&&... st )
               -> decltype( apply< Action >( begin, in, st... ) )
            {
               std::cerr << std::setw( 6 ) << ++ts.line << "        ";
               return apply< Action >( begin, in, st... );
            }

            template< template< typename... > class Action, typename Input, typename... States >
            static auto apply0( const Input& in, States&&... st )
               -> decltype( Base< Rule >::template apply0< Action >( in, st... ) )
            {
               std::cerr << in.position() << "  apply0 " << internal::demangle< Rule >() << std::endl;
               return Base< Rule >::template apply0< Action >( in, st... );
            }

            template< template< typename... > class Action, typename Input, typename... States >
            static auto apply0( const Input& in, trace_state& ts, States&&... st )
               -> decltype( apply0< Action >( in, st... ) )
            {
               std::cerr << std::setw( 6 ) << ++ts.line << "        ";
               return apply0< Action >( in, st... );
            }
         };
      };

      template< typename Rule >
      using tracer = trace< normal >::control< Rule >;

#endif

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 21 "amalgamated.hpp"
#line 1 "tao/pegtl/contrib/unescape.hpp"
       
#line 1 "tao/pegtl/contrib/unescape.hpp"



#ifndef TAO_PEGTL_CONTRIB_UNESCAPE_HPP
#define TAO_PEGTL_CONTRIB_UNESCAPE_HPP

#include <cassert>
#include <string>





namespace tao
{
   namespace TAO_PEGTL_NAMESPACE
   {
      namespace unescape
      {
         // Utility functions for the unescape actions.

         inline bool utf8_append_utf32( std::string& string, const unsigned utf32 )
         {
            if( utf32 <= 0x7f ) {
               string += char( utf32 & 0xff );
               return true;
            }
            if( utf32 <= 0x7ff ) {
               char tmp[] = { char( ( ( utf32 & 0x7c0 ) >> 6 ) | 0xc0 ), // NOLINT
                              char( ( ( utf32 & 0x03f ) ) | 0x80 ) };
               string.append( tmp, sizeof( tmp ) );
               return true;
            }
            if( utf32 <= 0xffff ) {
               if( utf32 >= 0xd800 && utf32 <= 0xdfff ) {
                  // nope, this is a UTF-16 surrogate
                  return false;
               }
               char tmp[] = { char( ( ( utf32 & 0xf000 ) >> 12 ) | 0xe0 ), // NOLINT
                              char( ( ( utf32 & 0x0fc0 ) >> 6 ) | 0x80 ),
                              char( ( ( utf32 & 0x003f ) ) | 0x80 ) };
               string.append( tmp, sizeof( tmp ) );
               return true;
            }
            if( utf32 <= 0x10ffff ) {
               char tmp[] = { char( ( ( utf32 & 0x1c0000 ) >> 18 ) | 0xf0 ), // NOLINT
                              char( ( ( utf32 & 0x03f000 ) >> 12 ) | 0x80 ),
                              char( ( ( utf32 & 0x000fc0 ) >> 6 ) | 0x80 ),
                              char( ( ( utf32 & 0x00003f ) ) | 0x80 ) };
               string.append( tmp, sizeof( tmp ) );
               return true;
            }
            return false;
         }

         // This function MUST only be called for characters matching TAO_PEGTL_NAMESPACE::ascii::xdigit!
         template< typename I >
         I unhex_char( const char c )
         {
            switch( c ) {
               case '0':
               case '1':
               case '2':
               case '3':
               case '4':
               case '5':
               case '6':
               case '7':
               case '8':
               case '9':
                  return I( c - '0' );
               case 'a':
               case 'b':
               case 'c':
               case 'd':
               case 'e':
               case 'f':
                  return I( c - 'a' + 10 );
               case 'A':
               case 'B':
               case 'C':
               case 'D':
               case 'E':
               case 'F':
                  return I( c - 'A' + 10 );
               default: // LCOV_EXCL_LINE
                  throw std::runtime_error( "invalid character in unhex" ); // NOLINT, LCOV_EXCL_LINE
            }
         }

         template< typename I >
         I unhex_string( const char* begin, const char* end )
         {
            I r = 0;
            while( begin != end ) {
               r <<= 4;
               r += unhex_char< I >( *begin++ );
            }
            return r;
         }

         // Actions for common unescape situations.

         struct append_all
         {
            template< typename Input >
            static void apply( const Input& in, std::string& s )
            {
               s.append( in.begin(), in.size() );
            }
         };

         // This action MUST be called for a character matching T which MUST be TAO_PEGTL_NAMESPACE::one< ... >.
         template< typename T, char... Rs >
         struct unescape_c
         {
            template< typename Input >
            static void apply( const Input& in, std::string& s )
            {
               assert( in.size() == 1 );
               s += apply_one( in, static_cast< const T* >( nullptr ) );
            }

            template< typename Input, char... Qs >
            static char apply_one( const Input& in, const one< Qs... >* /*unused*/ )
            {
               static_assert( sizeof...( Qs ) == sizeof...( Rs ), "size mismatch between escaped characters and their mappings" );
               return apply_two( in, { Qs... }, { Rs... } );
            }

            template< typename Input >
            static char apply_two( const Input& in, const std::initializer_list< char >& q, const std::initializer_list< char >& r )
            {
               const char c = *in.begin();
               for( std::size_t i = 0; i < q.size(); ++i ) {
                  if( *( q.begin() + i ) == c ) {
                     return *( r.begin() + i );
                  }
               }
               throw parse_error( "invalid character in unescape", in ); // NOLINT, LCOV_EXCL_LINE
            }
         };

         // See src/example/pegtl/unescape.cpp for why the following two actions
         // skip the first input character. They also MUST be called
         // with non-empty matched inputs!

         struct unescape_u
         {
            template< typename Input >
            static void apply( const Input& in, std::string& s )
            {
               assert( !in.empty() ); // First character MUST be present, usually 'u' or 'U'.
               if( !utf8_append_utf32( s, unhex_string< unsigned >( in.begin() + 1, in.end() ) ) ) {
                  throw parse_error( "invalid escaped unicode code point", in );
               }
            }
         };

         struct unescape_x
         {
            template< typename Input >
            static void apply( const Input& in, std::string& s )
            {
               assert( !in.empty() ); // First character MUST be present, usually 'x'.
               s += unhex_string< char >( in.begin() + 1, in.end() );
            }
         };

         // The unescape_j action is similar to unescape_u, however unlike
         // unescape_u it
         // (a) assumes exactly 4 hexdigits per escape sequence,
         // (b) accepts multiple consecutive escaped 16-bit values.
         // When applied to more than one escape sequence, unescape_j
         // translates UTF-16 surrogate pairs in the input into a single
         // UTF-8 sequence in s, as required for JSON by RFC 8259.

         struct unescape_j
         {
            template< typename Input >
            static void apply( const Input& in, std::string& s )
            {
               assert( ( ( in.size() + 1 ) % 6 ) == 0 ); // Expects multiple "\\u1234", starting with the first "u".
               for( const char* b = in.begin() + 1; b < in.end(); b += 6 ) {
                  const auto c = unhex_string< unsigned >( b, b + 4 );
                  if( ( 0xd800 <= c ) && ( c <= 0xdbff ) && ( b + 6 < in.end() ) ) {
                     const auto d = unhex_string< unsigned >( b + 6, b + 10 );
                     if( ( 0xdc00 <= d ) && ( d <= 0xdfff ) ) {
                        b += 6;
                        (void)utf8_append_utf32( s, ( ( ( c & 0x03ff ) << 10 ) | ( d & 0x03ff ) ) + 0x10000 );
                        continue;
                     }
                  }
                  if( !utf8_append_utf32( s, c ) ) {
                     throw parse_error( "invalid escaped unicode code point", in );
                  }
               }
            }
         };

      } // namespace unescape

   } // namespace TAO_PEGTL_NAMESPACE

} // namespace tao

#endif
#line 22 "amalgamated.hpp"
