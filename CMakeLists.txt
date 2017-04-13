project(usd)

if (WIN32)
    cmake_minimum_required(VERSION 3.1.1)
else()
    cmake_minimum_required(VERSION 2.8.8)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/defaults
                      ${CMAKE_SOURCE_DIR}/cmake/modules
                      ${CMAKE_SOURCE_DIR}/cmake/macros)

include(Options)
include(ProjectDefaults)
include(Packages)

# This has to be defined after Packages is included, because it relies on the
# discovered path to the python executable.
set(PXR_PYTHON_SHEBANG "${PYTHON_EXECUTABLE}" 
    CACHE 
    STRING
    "Replacement path for Python #! line."
)

# CXXDefaults will set a variety of variables for the project.
# Consume them here. This is an effort to keep the most common
# build files readable.
include(CXXDefaults)
add_definitions(${_PXR_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS ${_PXR_CXX_FLAGS})

include(Public)

add_subdirectory(pxr)
add_subdirectory(extras)

function(_addIfExists pkg pkgCmakeLists)
    if (EXISTS ${pkgCmakeLists})
        add_subdirectory(${pkg})
    else()
        message(WARNING "Specified third party package '${pkgName}' was not found. Please check that '${pkgCmakeLists}' exists")
    endif()
endfunction()

function(_enableThirdPartyPlugin identifier)
    set(pkg third_party/${identifier}) 
    set(pkgCmakeLists ${CMAKE_CURRENT_SOURCE_DIR}/${pkg}/CMakeLists.txt)
    _addIfExists(${pkg} ${pkgCmakeLists})
endfunction()

if (${PXR_BUILD_KATANA_PLUGIN})
    if (NOT ${PXR_BUILD_USD_IMAGING})
        message(WARNING "Skipping third_party/katana because PXR_BUILD_USD_IMAGING=OFF")
    else()
        _enableThirdPartyPlugin("katana")
    endif()
endif()

if (${PXR_BUILD_MAYA_PLUGIN})
    if (NOT ${PXR_BUILD_USD_IMAGING})
        message(WARNING "Skipping third_party/maya because PXR_BUILD_USD_IMAGING=OFF")
    else()
        _enableThirdPartyPlugin("maya")
    endif()
endif()

if (${PXR_BUILD_DOCUMENTATION})                                                  

    # Ensure we have doxygen+graphviz available
    find_program(DOXYGEN_EXECUTABLE                                              
        NAMES doxygen                                                            
    )                                                                            
    if (NOT EXISTS ${DOXYGEN_EXECUTABLE})                                        
        message(WARNING "Cannot run doc generation, doxygen not found.")           
        return()                                                                 
    endif()                                                                      

    find_program(DOT_EXECUTABLE
        NAMES dot 
    )                                                                            
    if (NOT EXISTS ${DOT_EXECUTABLE})                                        
        message(WARNING "Cannot run doc generation, dot not found.")           
        return()                                                                 
    endif()                          
    
    set(DOCS_DIR "${CMAKE_BINARY_DIR}") 
    set(GEN_SCRIPT "${PROJECT_SOURCE_DIR}/cmake/macros/generateDocs.py")         
    set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/pxr")
    set(DOXYGEN_CONFIG "${CMAKE_SOURCE_DIR}/pxr/usd/lib/usd/Doxyfile")
                                                                                 
    add_custom_target(                                                           
        documentation                                                            
        ALL                                                                      
        COMMAND ${PYTHON_EXECUTABLE} ${GEN_SCRIPT} ${SOURCE_DIR} ${DOCS_DIR} ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG} ${DOT_EXECUTABLE}
    ) 

    set(BUILT_DOCS "${DOCS_DIR}/docs/doxy_html")
    set(INST_DOCS  "${CMAKE_INSTALL_PREFIX}/docs")
    install(
        DIRECTORY ${BUILT_DOCS}
        DESTINATION ${INST_DOCS}
    )
endif()   
