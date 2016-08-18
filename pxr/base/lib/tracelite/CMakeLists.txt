set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE tracelite)

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_shared_library(tracelite
    LIBRARIES
        arch

    PUBLIC_CLASSES
        trace
        
    PUBLIC_HEADERS
        api.h
        trace.h

    CPPFILES
        ${MODULE_ENTRYPOINT_CPP}
)
