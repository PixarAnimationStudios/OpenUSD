# Deprecated wrapper for `pxrConfig.cmake` for the old pacakge root location.
include("${CMAKE_CURRENT_LIST_DIR}/lib/cmake/pxr/pxrConfig.cmake")

# PXR_CMAKE_DIR is deprecated and only exposed in the forwarding config
# for backwards compatability
set(PXR_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})
