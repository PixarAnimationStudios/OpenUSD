#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

include(gccclangshareddefaults)

if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6)
    if (Boost_VERSION LESS 106200)
        # gcc-6 introduces a placement-new warning, which causes problems
        # in boost-1.61 or less, in the boost::function code.
        # boost-1.62 fixes the warning
        _disable_warning("placement-new")
    endif()
endif()

# gcc's maybe-uninitialized warning often generates false positives.
# We disable this warning globally rather than trying to chase them
# down as they occur.
#
# See https://gcc.gnu.org/bugzilla/buglist.cgi?quicksearch=maybe%20uninitialized&list_id=394666
_disable_warning("maybe-uninitialized")

set(_PXR_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS}")
