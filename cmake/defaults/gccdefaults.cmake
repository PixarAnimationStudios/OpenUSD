#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

include(gccclangshareddefaults)

# gcc's maybe-uninitialized warning often generates false positives.
# We disable this warning globally rather than trying to chase them
# down as they occur.
#
# See https://gcc.gnu.org/bugzilla/buglist.cgi?quicksearch=maybe%20uninitialized&list_id=394666
_disable_warning("maybe-uninitialized")

# TBB's concurrent_hash_map.h generates class-memaccess warnings on gcc with -Wall.
# The header is consumed in many targets, so we disable it globally for now until we
# update to a version of TBB that no longer generates this warning.
#
# See https://github.com/uxlfoundation/oneTBB/issues/307
_disable_warning("class-memaccess")

set(_PXR_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS}")
