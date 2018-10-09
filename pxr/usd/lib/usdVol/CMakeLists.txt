set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdVol)

pxr_library(usdVol

    LIBRARIES
        tf
        usd
        usdGeom

    PUBLIC_CLASSES
        volume
	fieldBase
	fieldAsset
	field3DAsset
	openVDBAsset
	tokens

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapVolume.cpp
	wrapFieldBase.cpp
	wrapFieldAsset.cpp
	wrapField3DAsset.cpp
	wrapOpenVDBAsset.cpp
	wrapTokens.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        generatedSchema.usda
        plugInfo.json
        schema.usda:usdVol/schema.usda
)

pxr_test_scripts(
    testenv/testUsdVolVolume.py
)

pxr_register_test(testUsdVolVolume
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdVolVolume"
    EXPECTED_RETURN_CODE 0
)


