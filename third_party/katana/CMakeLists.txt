set(PXR_INSTALL_SUBDIR "third_party/katana")

# Transform the KATANA_API versions into defines to be used by this code
add_definitions(
    -DPX_KATANA_ASSET_API_VER=${KATANA_API_ASSET_API_VERSION}
    -DPX_KATANA_AMP_API_VER=${KATANA_API_AMP_API_VERSION}
    -DPX_KATANA_SGG_API_VER=${KATANA_API_SGG_API_VERSION}
    -DPX_KATANA_RENDER_API_VER=${KATANA_API_RENDER_API_VERSION}
    -DPX_KATANA_VMP_API_VER=${KATANA_API_VMP_API_VERSION}
    -DPX_KATANA_OP_API_VER=${KATANA_API_OP_API_VERSION}
    -DPX_KATANA_ATTRFNC_API_VER=${KATANA_API_ATTRFNC_API_VERSION}
    )

add_subdirectory(katanaAttrfncApi)
add_subdirectory(katanaOpApi)
add_subdirectory(katanaPluginApi)
add_subdirectory(usdKatana)
if (NOT ${KATANA_API_VERSION} VERSION_LESS "3.0.0")
    add_subdirectory(vtKatana)
endif()
add_subdirectory(plugin)
add_subdirectory(python)
