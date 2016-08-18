set(PXR_PREFIX examples)
set(PXR_INSTALL_SUBDIR share/usd/examples)

if(UNIX)
    add_subdirectory(usdObj)
    add_subdirectory(usdSchemaExamples)
    add_subdirectory(usdMakeFileVariantModelAsset)
endif()