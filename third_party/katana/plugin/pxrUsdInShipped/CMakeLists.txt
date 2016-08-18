set(PXR_PACKAGE pxrUsdInShipped)

add_definitions(
    /DFNASSET_STATIC
    /DFNATTRIBUTE_STATIC
	/DFNATTRIBUTEFUNCTION_STATIC
    /DFNATTRIBUTEMODIFIER_STATIC
    /DFNCONFIG_STATIC
    /DFNDEFAULTATTRIBUTEPRODUCER_STATIC
	/DFNGEOLIB_STATIC
    /DFNGEOLIBSERVICES_STATIC
    /DFNLOGGING_STATIC
    /DFNRENDEROUTPUTLOCATION_STATIC
    /DFNSCENEGRAPHGENERATOR_STATIC
    /DFNSCENEGRAPHITERATOR_STATIC
    /DPYSTRING_STATIC
)

pxr_plugin(${PXR_PACKAGE}
    KATANA_PLUGIN
    LIBRARIES
        katanaOpApi
        katanaPluginApi
        kind
        usd
        usdGeom
        usdKatana
        usdShade

    PRIVATE_HEADERS
        declareCoreOps.h

    CPPFILES
        register.cpp

        basisCurves.cpp
        camera.cpp
        constraints.cpp
        material.cpp
        materialsGroup.cpp
        mesh.cpp
        model.cpp
        nurbsPatch.cpp
        points.cpp
        scope.cpp
        uiUtils.cpp
        xform.cpp
)
