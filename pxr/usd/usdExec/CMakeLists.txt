set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdExec)

pxr_library(usdExec
    LIBRARIES
        tf
        vt
        sdf
        ndr
        exec
        usd
        usdGeom

    PUBLIC_CLASSES
        tokens
        execInput
        execOutput
        execNode
        execGraph
        execConnectableAPI
        execUtils

    PUBLIC_HEADERS
        api.h
        execTypes.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapTokens.cpp
        wrapExecTypes.cpp
        wrapExecUtils.cpp
        wrapExecInput.cpp
        wrapExecOutput.cpp
        wrapExecNode.cpp
        wrapExecGraph.cpp
        wrapExecConnectableAPI.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:UsdExec/schema.usda
)