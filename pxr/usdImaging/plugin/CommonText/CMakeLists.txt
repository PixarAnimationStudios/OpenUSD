set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE CommonText)

set(optionalLibs "")
set(optionalCPPFiles "")
if (WIN32)
    list(APPEND optionalLibs USP10)
    list(APPEND optionalCPPFiles
        codePage.cpp
        multiLanguageHandlerImplWin.cpp
    )
elseif (APPLE)
    list(APPEND optionalLibs "-framework CoreGraphics")
endif()

pxr_plugin(CommonText
    LIBRARIES
        ar
        arch
        gf
        tf
        sdf
        usdImaging
        ${FREETYPE_LIB}
        ${optionalLibs}

    INCLUDE_DIRS
        ${FREETYPE_INCLUDE_DIR}

    CPPFILES
        commonText.cpp
        characterSupport.cpp
        freeTypeFontDevice.cpp
        freeTypeFontDeviceImp.cpp
        freeTypeFontList.cpp
        genericLayout.cpp
        intermediateInfo.cpp
        languageAttribute.cpp
        lineLayoutManager.cpp
        multiLanguageHandler.cpp
        simpleLayout.cpp
        system.cpp
        utilities.cpp
        ${optionalCPPFiles}

    RESOURCE_FILES
        plugInfo.json
)

add_compile_definitions(UNICODE _UNICODE)