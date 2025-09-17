#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
function(_add_define definition)
    list(APPEND _PXR_CXX_DEFINITIONS "-D${definition}")
    set(_PXR_CXX_DEFINITIONS ${_PXR_CXX_DEFINITIONS} PARENT_SCOPE)
endfunction()

function(_disable_warning flag)
    if(MSVC)
        list(APPEND _PXR_CXX_WARNING_FLAGS "/wd${flag}")
    else()
        list(APPEND _PXR_CXX_WARNING_FLAGS "-Wno-${flag}")
    endif()
    set(_PXR_CXX_WARNING_FLAGS ${_PXR_CXX_WARNING_FLAGS} PARENT_SCOPE)
endfunction()
