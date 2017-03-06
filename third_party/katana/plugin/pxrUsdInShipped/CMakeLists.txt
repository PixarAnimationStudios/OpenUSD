set(PXR_PACKAGE pxrUsdInShipped)

pxr_plugin(${PXR_PACKAGE}
    KATANA_PLUGIN
    LIBRARIES
        katanaAttrfncApi
        katanaOpApi
        katanaPluginApi
        kind
        usd
        usdGeom
        usdKatana
        usdShade

    PRIVATE_CLASSES
        attrfnc_materialReference

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
        pointInstancer.cpp
        points.cpp
        scope.cpp
        uiUtils.cpp
        xform.cpp
)
