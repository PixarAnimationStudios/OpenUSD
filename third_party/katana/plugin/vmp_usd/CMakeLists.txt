set(PXR_PACKAGE vmp_usd)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

pxr_plugin(${PXR_PACKAGE}
    KATANA_PLUGIN
    LIBRARIES
        arch
        tf
        work
        usd
        usdUtils
        glf
        usdImaging
        usdImagingGL
        katanaPluginApi
        usdKatana

    CPPFILES
        debugCodes.cpp
        usdVMP.cpp
)

target_compile_definitions(${PXR_PACKAGE}
    PRIVATE

    FNASSET_STATIC
    FNATTRIBUTE_STATIC
    FNATTRIBUTEFUNCTION_STATIC
    FNATTRIBUTEMODIFIER_STATIC
    FNCONFIG_STATIC
    FNDEFAULTATTRIBUTEPRODUCER_STATIC
    FNGEOLIB_STATIC
    FNGEOLIBSERVICES_STATIC
    FNLOGGING_STATIC
    FNPLUGINMANAGER_STATIC
    FNRENDEROUTPUTLOCATION_STATIC
    FNSCENEGRAPHGENERATOR_STATIC
    FNSCENEGRAPHITERATOR_STATIC
    PYSTRING_STATIC
)
