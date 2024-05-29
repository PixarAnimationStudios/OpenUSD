//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_EXPORT_H
#define PXR_BASE_ARCH_EXPORT_H

/// \file arch/export.h
/// \ingroup group_arch_SymbolVisibility
///
/// Defines symbol visibility macros.
///
/// Defines \c ARCH_EXPORT to indicate a symbol should be exported from
/// a library, \c ARCH_IMPORT to indicate an exported symbol should be
/// imported (which only matters on Windows), and \c ARCH_HIDDEN to
/// indicate that a symbol which would be exported should not be.
///
/// The correct way to use these macros is for each library to make a
/// header that looks like this (for some hypothetical library named foo):
///
/// \code
///  #ifndef FOO_API_H
///  #define FOO_API_H
///  
///  #include "pxr/base/arch/export.h"
///  
///  #if defined(FOO_STATIC)
///  #   define FOO_API
///  #   define FOO_API_TEMPLATE_CLASS(...)
///  #   define FOO_API_TEMPLATE_STRUCT(...)
///  #   define FOO_LOCAL
///  #else
///  #   if defined(FOO_EXPORTS)
///  #       define FOO_API ARCH_EXPORT
///  #       define FOO_API_TEMPLATE_CLASS(...) ARCH_EXPORT_TEMPLATE(class, __VA_ARGS__)
///  #       define FOO_API_TEMPLATE_STRUCT(...) ARCH_EXPORT_TEMPLATE(struct, __VA_ARGS__)
///  #   else
///  #       define FOO_API ARCH_IMPORT
///  #       define FOO_API_TEMPLATE_CLASS(...) ARCH_IMPORT_TEMPLATE(class, __VA_ARGS__)
///  #       define FOO_API_TEMPLATE_STRUCT(...) ARCH_IMPORT_TEMPLATE(struct, __VA_ARGS__)
///  #   endif
///  #   define FOO_LOCAL ARCH_HIDDEN
///  #endif
///  
///  #endif
/// \endcode
///
/// Note that every library has its own unique _API and _LOCAL macros and
/// the expansion of these macros depends on whether the library is being
/// built or used (indicated by the externally set macro FOO_EXPORTS).
///
/// Library headers then include this header and mark classes, methods,
/// functions and variables as part of the API or local.  Each case is
/// described below.
///
/// A class is added to the API like this:
///
/// \code
///  class FOO_API FooClass ...
/// \endcode
///
/// This will add every member to the API, including implicitly created
/// members like a default constructor, default assignment, the vtable
/// and the type_info.  The type_info is especially important.  If you
/// will dynamic_cast to the type or catch an exception using the type
/// outside of the library, you \b must put its type_info into the API.
/// To export the vtable & type_info specifically, use \c ARCH_EXPORT_TYPE.
/// However, due to compiler limitations, ARCH_EXPORT_TYPE may put the
/// whole class into the API as if ARCH_EXPORT were used.
///
/// Note that template classes do not get added to the API that way.
/// Instead they are added when explicitly instantiated like so:
///
/// \code
///  template class FOO_API FooTemplateClass<FooArgType>;
/// \endcode
///
/// It's also sometimes necessary to indicate that an instantiation exists
/// in the API and that a client should not do any of its own instantiation.
/// This is necessary, for example, when the template has static data members
/// and is done by using extern template in the header file that provides the
/// type.  Two of the macros above will do that:
///
/// \code
///  FOO_API_TEMPLATE_CLASS(FooTemplateClass<FooArgType>);
///  FOO_API_TEMPLATE_STRUCT(FooTemplateClass<FooArgType>);
/// \endcode
///
/// Which you use depends on if FooTemplateClass is a class or struct.  The
/// macro is used because we don't want to always use extern template, but
/// only when we're importing a dynamic library.
///
/// A template that is completely inlined does not need to be added to the
/// API since it's simply instantiated where needed.
///
/// Functions, methods and variables can also be put into the API:
///
/// \code
///  struct FooLocalClass {
///    ~FooLocalClass();
///    FOO_API void MoveTheThing();
///  };
///  FOO_API FooLocalClass* FooNewThing();
///  FOO_API extern int doNotUseGlobalVariables;
/// \endcode
///
/// Just because FooLocalClass is not in the API doesn't mean clients can't
/// have pointers and references to instances.  What they can't do is call
/// member functions not in the API (or, as indicated above, use RTTI for
/// it).  So, for example:
///
/// \code
///  FooLocalClass* thing = FooNewThing();
///  thing->MoveTheThing();
///  (void)dynamic_cast<FooLocalClass*>(thing); // Link error!
///  delete thing; // Link error!
/// \endcode
///
/// Deleting the FooLocalClass instance attempts to use the (non-API) d'tor
/// and fails to link.  If we had an implicitly defined d'tor then the
/// deletion would have worked, presuming FooLocalClass didn't have any
/// data members with non-API d'tors.
///
/// A method of a class added to the API is itself in the API but on some
/// platforms (i.e. not Windows) you can remove it from the API using
/// FOO_LOCAL:
///
/// \code
///  struct FOO_API FooSemiAPI {
///      void DoSomethingPublic();
///      FOO_LOCAL void _DoSomethingPrivate();
///  };
/// \endcode
///
/// Clients of the library will fail to link if they use _DoSomethingPrivate()
/// if FOO_LOCAL is supported.  If not then the symbol will remain in the API.

#include "pxr/base/arch/defines.h" 

#if defined(ARCH_OS_WINDOWS)
#   if defined(ARCH_COMPILER_GCC) && ARCH_COMPILER_GCC_MAJOR >= 4 || defined(ARCH_COMPILER_CLANG)
#       define ARCH_EXPORT __attribute__((dllexport))
#       define ARCH_IMPORT __attribute__((dllimport))
#       define ARCH_HIDDEN
#       define ARCH_EXPORT_TYPE
#   else
#       define ARCH_EXPORT __declspec(dllexport)
#       define ARCH_IMPORT __declspec(dllimport)
#       define ARCH_HIDDEN
#       define ARCH_EXPORT_TYPE
#   endif
#elif defined(ARCH_COMPILER_GCC) && ARCH_COMPILER_GCC_MAJOR >= 4 || defined(ARCH_COMPILER_CLANG)
#   define ARCH_EXPORT __attribute__((visibility("default")))
#   define ARCH_IMPORT
#   define ARCH_HIDDEN __attribute__((visibility("hidden")))
#   if defined(ARCH_COMPILER_CLANG)
#       define ARCH_EXPORT_TYPE __attribute__((type_visibility("default")))
#   else
#       define ARCH_EXPORT_TYPE __attribute__((visibility("default")))
#   endif
#else
#   define ARCH_EXPORT
#   define ARCH_IMPORT
#   define ARCH_HIDDEN
#   define ARCH_EXPORT_TYPE
#endif
#define ARCH_EXPORT_TEMPLATE(type, ...)
#define ARCH_IMPORT_TEMPLATE(type, ...) extern template type ARCH_IMPORT __VA_ARGS__

#endif // PXR_BASE_ARCH_EXPORT_H
