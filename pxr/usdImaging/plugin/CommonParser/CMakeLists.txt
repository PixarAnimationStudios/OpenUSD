set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE CommonParser)

set(optionalCPPFiles "")

pxr_plugin(CommonParser
    LIBRARIES
        ar
        arch
        gf
        tf
        sdf
        usdImaging
        ${optionalLibs}

    CPPFILES
        commonParser.cpp
        environment.cpp
        abandonmentElement.cpp
        environmentElement.cpp
        globals.cpp
        locationElement.cpp        
        structureElement.cpp        
        styleElement.cpp        
        textrunElement.cpp        
        transformElement.cpp        
        universeElement.cpp        
        markupParser.cpp
        mtextParser.cpp
        sink.cpp
        ${optionalCPPFiles}

    RESOURCE_FILES
        plugInfo.json
)

add_compile_definitions(UNICODE _UNICODE)