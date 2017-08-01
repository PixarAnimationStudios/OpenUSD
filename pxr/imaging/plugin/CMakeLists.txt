add_subdirectory(hdStream)
if (${PXR_BUILD_EMBREE_PLUGIN})
    add_subdirectory(hdEmbree)
endif()
