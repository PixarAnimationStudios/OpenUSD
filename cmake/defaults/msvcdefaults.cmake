#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Enable exception handling.
set(_PXR_CXX_FLAGS "${_PXR_CXX_FLAGS} /EHsc")

# Standards compliant.
set(_PXR_CXX_FLAGS "${_PXR_CXX_FLAGS} /Zc:rvalueCast /Zc:strictStrings")

# The /Zc:inline option strips out the "arch_ctor_<name>" symbols used for
# library initialization by ARCH_CONSTRUCTOR starting in Visual Studio 2019, 
# causing release builds to fail. Disable the option for this and later 
# versions.
# 
# For more details, see:
# https://developercommunity.visualstudio.com/content/problem/914943/zcinline-removes-extern-symbols-inside-anonymous-n.html
if (MSVC_VERSION GREATER_EQUAL 1920)
    set(_PXR_CXX_FLAGS "${_PXR_CXX_FLAGS} /Zc:inline-")
else()
    set(_PXR_CXX_FLAGS "${_PXR_CXX_FLAGS} /Zc:inline")
endif()

# Turn on all but informational warnings.
set(_PXR_CXX_FLAGS "${_PXR_CXX_FLAGS} /W3")

# Warnings are errors in strict build mode.
if (${PXR_STRICT_BUILD_MODE})
    set(_PXR_CXX_FLAGS "${_PXR_CXX_FLAGS} /WX")
endif()

# The Visual Studio preprocessor does not conform to the C++ standard,
# resulting in warnings like:
#
#     warning C4003: not enough arguments for function-like macro invocation '_TF_PP_IS_PARENS'
#
# These warnings are harmless and can be ignored. They affect a number of
# code sites that tricky to guard with individual pragmas, so we opt to
# disable them throughout the build here.
#
# Note that these issues are apparently fixed with the "new" preprocessor
# present in Visual Studio 2019 version 16.5. If/when we enable that option,
# we should revisit this.
#
# https://developercommunity.visualstudio.com/t/standard-conforming-preprocessor-invalid-warning-c/364698
_disable_warning("4003")

# truncation from 'double' to 'float' due to matrix and vector classes in `Gf`
_disable_warning("4244")
_disable_warning("4305")

# conversion from size_t to int. While we don't want this enabled
# it's in the Python headers. So all the Python wrap code is affected.
_disable_warning("4267")

# no definition for inline function
# this affects Glf only
_disable_warning("4506")

# 'typedef ': ignored on left of '' when no variable is declared
# XXX:figure out why we need this
_disable_warning("4091")

# c:\python27\include\pymath.h(22): warning C4273: 'round': inconsistent dll linkage 
# XXX:figure out real fix
_disable_warning("4273")

# qualifier applied to function type has no meaning; ignored
# tbb/parallel_for_each.h
_disable_warning("4180")

# '<<': result of 32-bit shift implicitly converted to 64 bits
# tbb/enumerable_thread_specific.h
_disable_warning("4334")

# Disable warning C4996 regarding fopen(), strcpy(), etc.
_add_define("_CRT_SECURE_NO_WARNINGS")

# Disable warning C4996 regarding unchecked iterators for std::transform,
# std::copy, std::equal, et al.
_add_define("_SCL_SECURE_NO_WARNINGS")

# Make sure WinDef.h does not define min and max macros which
# will conflict with std::min() and std::max().
_add_define("NOMINMAX")

# Forces all libraries that have separate source to be linked as
# DLL's rather than static libraries on Microsoft Windows, unless
# explicitly told otherwise.
if (NOT Boost_USE_STATIC_LIBS)
    _add_define("BOOST_ALL_DYN_LINK")
endif()

# Suppress automatic boost linking via pragmas, as we must not rely on
# a heuristic, but upon the tool set we have specified in our build.
_add_define("BOOST_ALL_NO_LIB")

if(${PXR_USE_DEBUG_PYTHON})
    _add_define("BOOST_DEBUG_PYTHON")
    _add_define("BOOST_LINKING_PYTHON")
endif()

# Need half::_toFloat and half::_eLut.
_add_define("OPENEXR_DLL")

# Exclude headers from unnecessary Windows APIs to improve build
# times and avoid annoying conflicts with macros defined in those
# headers.
_add_define("WIN32_LEAN_AND_MEAN")

# These files require /bigobj compiler flag
#   Vt/arrayPyBuffer.cpp
#   Usd/crateFile.cpp
#   Usd/stage.cpp
# Until we can set the flag on a per file basis, we'll have to enable it
# for all translation units.
set(_PXR_CXX_FLAGS "${_PXR_CXX_FLAGS} /bigobj")

# Enable PDB generation.
set(_PXR_CXX_FLAGS "${_PXR_CXX_FLAGS} /Zi")

# Enable multiprocessor builds.
set(_PXR_CXX_FLAGS "${_PXR_CXX_FLAGS} /MP")
set(_PXR_CXX_FLAGS "${_PXR_CXX_FLAGS} /Gm-")

# Ignore LNK4221.  This happens when making an archive with a object file
# with no symbols in it.  We do this a lot because of a pattern of having
# a C++ source file for many header-only facilities, e.g. tf/bitUtils.cpp.
set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /IGNORE:4221")
