set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

if (NOT PXR_ENABLE_PYTHON_SUPPORT)
    message(STATUS "Skipping building usdgenschemafromsdr, Python modules required.")
elseif (NOT JINJA2_FOUND)
    message(WARNING "Skipping building usdgenschemafromsdr due to missing dependency: Jinja2")
else()
    pxr_python_bin(usdgenschemafromsdr
        DEPENDENCIES
            tf
            sdf
            usd
            usdUtils
    )
endif()
