set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE vt)

pxr_library(vt
    LIBRARIES
        arch
        tf
        gf
        trace
        TBB::tbb

    PUBLIC_CLASSES
        array
        arrayEdit
        arrayEditBuilder
        arrayEditOps
        debugCodes
        dictionary
        hash
        streamOut
        types
        value

    PYTHON_PUBLIC_CLASSES
        arrayPyBuffer
        valueFromPython
        wrapArray

    PUBLIC_HEADERS
        api.h
        traits.h
        typeHeaders.h
        visitValue.h
    
    PYTHON_PUBLIC_HEADERS
        pyOperators.h
        wrapArrayEdit.h

    PYMODULE_CPPFILES
        module.cpp
        wrapArrayBase.cpp
        wrapArrayDualQuaternion.cpp
        wrapArrayFloat.cpp
        wrapArrayIntegral.cpp
        wrapArrayMatrix.cpp
        wrapArrayQuaternion.cpp
        wrapArrayRange.cpp
        wrapArrayString.cpp
        wrapArrayToken.cpp
        wrapArrayVec.cpp
        wrapDictionary.cpp
        wrapValue.cpp

    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        overview.dox
)

pxr_build_test(testVtCpp
    LIBRARIES
        tf
        gf
        vt
    CPPFILES
        testenv/testVtCpp.cpp
)
pxr_build_test(testVtArrayEditCpp
    LIBRARIES
        tf
        gf
        vt
    CPPFILES
        testenv/testVtArrayEdit.cpp
)
pxr_test_scripts(
    testenv/testVtArray.py
    testenv/testVtArrayEdit.py
    testenv/testVtValue.py
)
pxr_register_test(testVtValue
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVtValue"
)
pxr_register_test(testVtArray
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVtArray"
)
pxr_register_test(testVtArrayEdit
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVtArrayEdit"
)
pxr_register_test(testVtCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVtCpp"
)
pxr_register_test(testVtArrayEditCpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVtArrayEditCpp"
)
