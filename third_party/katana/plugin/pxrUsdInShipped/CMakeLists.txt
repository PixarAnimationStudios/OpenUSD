set(PXR_PACKAGE pxrUsdInShipped)

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
        look.cpp
        looksGroup.cpp
        mesh.cpp
        model.cpp
        nurbsPatch.cpp
        points.cpp
        scope.cpp
        uiUtils.cpp
        xform.cpp
)
