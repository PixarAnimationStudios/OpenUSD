#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

include(gccclangshareddefaults)

set(_PXR_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS}")

# Prevent floating point result discrepancies on Apple platforms
# due to multiplication+additions being converted to FMA
if (APPLE)
    set(_PXR_CXX_FLAGS "${_PXR_CXX_FLAGS} -ffp-contract=off")
endif()

# clang annoyingly warns about the -pthread option if it's only linking.
if(CMAKE_USE_PTHREADS_INIT)
    _disable_warning("unused-command-line-argument")
endif()
