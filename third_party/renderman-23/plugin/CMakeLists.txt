add_subdirectory(rmanArgsParser)
add_subdirectory(rmanDiscovery)
add_subdirectory(hdPrman)
add_subdirectory(hdxPrman)
add_subdirectory(rtx_glfImage)
# Only build rmanOslParser when sdrOsl is disabled
# This allows hdPrman to render OSL files without OSL pre-installed
if (NOT PXR_ENABLE_OSL_SUPPORT)
    add_subdirectory(rmanOslParser)
endif()
