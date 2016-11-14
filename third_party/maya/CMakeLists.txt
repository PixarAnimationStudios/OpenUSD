set(PXR_INSTALL_SUBDIR "third_party/maya")

pxr_setup_plugins()

# Additional defines for building against Maya
add_definitions(-D_BOOL -DREQUIRE_IOSTREAM)

if (APPLE)
   add_definitions(-DOSMac_ -DMAC_PLUGIN)
endif()

add_subdirectory(lib)
add_subdirectory(plugin)
