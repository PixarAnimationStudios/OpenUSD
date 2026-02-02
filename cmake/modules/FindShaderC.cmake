include(FindPackageHandleStandardArgs)

find_path(shaderc_INCLUDE_DIR "shaderc/shaderc.h")
find_library(shaderc_combined_LIBRARY "shaderc_combined")
find_library(shaderc_combined_DEBUG_LIBRARY "shaderc_combinedd")

set(shaderc_combined_release_or_debug "${shaderc_combined_LIBRARY}")
if (NOT shaderc_combined_release_or_debug)
    set(shaderc_combined_release_or_debug "${shaderc_combined_DEBUG_LIBRARY}")
endif()

find_package_handle_standard_args(ShaderC DEFAULT_MSG
    shaderc_combined_release_or_debug
    shaderc_INCLUDE_DIR)

if (ShaderC_FOUND)
    add_library(shaderc_combined STATIC IMPORTED)
    target_include_directories(shaderc_combined INTERFACE "${shaderc_INCLUDE_DIR}")

    if (shaderc_combined_LIBRARY AND NOT shaderc_combined_DEBUG_LIBRARY)
        # Only one un-suffixed lib found: ambiguous configuration,
        # so import using the generic IMPORTED_LOCATION.
        set_target_properties(shaderc_combined PROPERTIES
            IMPORTED_LOCATION ${shaderc_combined_LIBRARY})
    else()
        # We found a lib with a debug suffix so we can disambiguate.
        set(found_configurations)
        if (shaderc_combined_LIBRARY)
            set_target_properties(shaderc_combined PROPERTIES
                IMPORTED_LOCATION_RELEASE "${shaderc_combined_LIBRARY}")
            list(APPEND found_configurations Release)
        endif()
        if (shaderc_combined_DEBUG_LIBRARY)
            set_target_properties(shaderc_combined PROPERTIES
                IMPORTED_LOCATION_DEBUG "${shaderc_combined_DEBUG_LIBRARY}")
            list(APPEND found_configurations Debug)
        endif()
        set_target_properties(shaderc_combined PROPERTIES
            IMPORTED_CONFIGURATIONS "${found_configurations}")
    endif()
endif()
