set(PXR_PREFIX tutorials)
set(PXR_INSTALL_SUBDIR share/usd/tutorials)

if (NOT PXR_ENABLE_PYTHON_SUPPORT)
    message(STATUS "Not installing tutorial scripts because PXR_ENABLE_PYTHON_SUPPORT=OFF")
    return()
endif()
        
add_subdirectory(authoringProperties)
add_subdirectory(authoringVariants)
add_subdirectory(convertingLayerFormats)
add_subdirectory(endToEnd)
add_subdirectory(exampleModelingVariantsKatana)
add_subdirectory(helloWorld)
add_subdirectory(referencingLayers)
add_subdirectory(traversingStage)

