// Copyright (c) 2020-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/

#ifndef PXR_PEGTL_INTERNAL_FILESYSTEM_HPP
#define PXR_PEGTL_INTERNAL_FILESYSTEM_HPP

#include "../config.hpp"

#if defined( PXR_PEGTL_BOOST_FILESYSTEM )

#define BOOST_FILESYSTEM_NO_DEPRECATED

#include <boost/filesystem.hpp>

namespace PXR_PEGTL_NAMESPACE::internal
{
   namespace filesystem = ::boost::filesystem;

   using error_code = ::boost::system::error_code;

   inline const auto& system_category() noexcept
   {
      return ::boost::system::system_category();
   }

}  // namespace PXR_PEGTL_NAMESPACE::internal

#elif defined( PXR_PEGTL_STD_EXPERIMENTAL_FILESYSTEM )

#include <experimental/filesystem>

namespace PXR_PEGTL_NAMESPACE::internal
{
   namespace filesystem = ::std::experimental::filesystem;

   using error_code = ::std::error_code;

   inline const auto& system_category() noexcept
   {
      return ::std::system_category();
   }

}  // namespace PXR_PEGTL_NAMESPACE::internal

#else

#include <filesystem>

namespace PXR_PEGTL_NAMESPACE::internal
{
   namespace filesystem = ::std::filesystem;

   using error_code = ::std::error_code;

   inline const auto& system_category() noexcept
   {
      return ::std::system_category();
   }

}  // namespace PXR_PEGTL_NAMESPACE::internal

#endif

#endif
