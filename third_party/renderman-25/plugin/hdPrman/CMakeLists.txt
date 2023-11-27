# Install hdPrman's headers in pxr/imaging alongside other shared
# libraries for Hydra backends, like hdSt.
set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdPrman)

# RiEntryPoints.h incorrectly makes libloadprman symbols DLL import
# This is only temporary and will be removed shortly
add_definitions(-DPRMANBUILDINGAPI)

# TODO The pxr cmake macros don't provide a good way to link prman with link_directories
# on Linux/OSX without resorting to absolute rpath. For now, simply avoid linking
# libprman on Linux/OSX.
set(optionalLibs "")
if(WIN32)
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:libmmd.lib /NODEFAULTLIB:libirc.lib /NODEFAULTLIB:svml_dispmd.lib /NODEFAULTLIB:libdecimal.lib" )
    list(APPEND optionalLibs ${PXRCORE_LIBRARY})
    list(APPEND optionalLibs ${PRMAN_LIBRARY})
    list(APPEND optionalLibs ${PRMAN_STATS_LIBRARY})
elseif(APPLE)
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup -headerpad_max_install_names" )
endif()

set(optionalIncludeDirs "")
set(optionalPublicClasses "")
if (${PXR_ENABLE_MATERIALX_SUPPORT})
    list(APPEND optionalLibs
        MaterialXCore
        MaterialXFormat
        MaterialXGenShader
        MaterialXGenOsl
        MaterialXRender
        hdMtlx
    )
    list(APPEND optionalPublicClasses matfiltMaterialX)

    if (${PXR_ENABLE_OSL_SUPPORT})
        if (DEFINED OSL_SHADER_INSTALL_DIR)
            add_compile_definitions(
              OSL_SHADER_INSTALL_DIR=\"${OSL_SHADER_INSTALL_DIR}\"
            )
        endif()

        list(APPEND optionalLibs ${OSL_COMP_LIBRARY})
        list(APPEND optionalIncludeDirs ${OSL_INCLUDE_DIR})
    endif()
endif()

if (${PXR_ENABLE_OPENVDB_SUPPORT})
    list(APPEND optionalLibs hioOpenVDB)
endif()

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        ar
        arch
        js
        plug
        tf
        vt
        gf
        hd
        hdsi
        hio
        usdRi
        usdVol
        usdVolImaging
        ${optionalLibs}

    INCLUDE_DIRS
        ${RENDERMAN_INCLUDE_DIR}
        ${optionalIncludeDirs}

    PUBLIC_CLASSES
        basisCurves
        camera
        cone
        cylinder
        coordSys
        coordSysPrimSceneIndexPlugin
        debugCodes
        dependencyForwardingSceneIndexPlugin
        displayFilter
        gprim
        gprimbase
        instancer
        integrator
        light
        lightFilter
        material
        matfiltConvertPreviewMaterial
        matfiltResolveVstructs
        mesh
        renderTerminalOutputInvalidatingSceneIndexPlugin
        points
        renderDelegate
        renderSettings
        rileyRenderOutputSchema
        rileyRenderTargetSchema
        rixStrings
        sampleFilter
        sphere
        tokens
        volume
        xcpt
        ${optionalPublicClasses}

    PRIVATE_CLASSES
        cameraContext
        debugUtil
        extComputationPrimvarPruningSceneIndexPlugin
        framebuffer
        implicitSurfaceSceneIndexPlugin
        materialPrimvarTransferSceneIndexPlugin
        matfiltSceneIndexPlugins
        pinnedCurveExpandingSceneIndexPlugin
        renderBuffer
        renderParam
        renderPass
        renderSettingsFilteringSceneIndexPlugin
        renderViewContext
        resourceRegistry
        rileyPrimBase
        rileyPrimFactory
        rileyRenderOutputPrim
        rileyRenderTargetPrim
        terminalSceneIndexObserver
        utils
        motionBlurSceneIndexPlugin
        virtualStructResolvingSceneIndex

    PUBLIC_HEADERS
        api.h

    PRIVATE_HEADERS
        prmanArchDefs.h
        rileyPrimUtil.h
        sceneIndexObserverApi.h
        virtualStructConditionalGrammar.tab.h

    CPPFILES
        virtualStructConditionalGrammar.tab.cpp
        virtualStructConditionalGrammar.lex.cpp
        rendererPlugin.cpp

    RESOURCE_FILES
        plugInfo.json

    DISABLE_PRECOMPILED_HEADERS
)

# Include paths to hdPrman headers within hdPrman itself look like
# #include "hdPrman/...", so we need to add include/pxr/imaging
# to the include paths for this library. We use PROJECT_BINARY_DIR
# to include headers out of the build directory, which is what
# normally happens in the USD build.
if(TARGET hdPrman)
    target_include_directories(${PXR_PACKAGE}
        PRIVATE
            "${PROJECT_BINARY_DIR}/include/pxr/imaging"
    )
endif()

if(WIN32 AND NOT BUILD_SHARED_LIBS)
    # XXX: Temporarily disable this test on static Windows builds
    #      due to symbol visibility issues.
else()
    pxr_build_test(testHdPrman
        LIBRARIES
            cameraUtil
            hdPrman
            hd
            usd
            usdImaging
            usdRender
            ${PXRCORE_LIBRARY}
            ${PRMAN_LIBRARY}
        CPPFILES
            testenv/testHdPrman.cpp
    )
endif()
