set(PXR_INSTALL_SUBDIR "third_party/maya")

pxr_setup_plugins()

# Additional defines for building against Maya
add_definitions(-D_BOOL -DREQUIRE_IOSTREAM)

add_subdirectory(lib)
add_subdirectory(plugin)
