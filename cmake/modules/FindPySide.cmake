#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
if (NOT PYTHON_EXECUTABLE)
    message(FATAL_ERROR "Unable to find Python executable - PySide not present")
    return()
endif()

# Prefer PySide6 over PySide2
execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" "-c" "import PySide6"
    RESULT_VARIABLE pySideImportResult 
)
if (pySideImportResult EQUAL 0)
    set(pySideImportResult "PySide6")
    set(pySideUIC pyside6-uic python3-pyside6-uic)
endif()

# PySide6 not found OR PYSIDE2 explicitly requested
if (pySideImportResult EQUAL 1 OR PYSIDE_USE_PYSIDE2)
    execute_process(
        COMMAND "${PYTHON_EXECUTABLE}" "-c" "import PySide2"
        RESULT_VARIABLE pySideImportResult 
    )
    if (pySideImportResult EQUAL 0)
        set(pySideImportResult "PySide2")
        set(pySideUIC pyside2-uic)
    endif()
endif()

# If nothing is found, the result will be <VAR>-NOTFOUND.
find_program(PYSIDEUICBINARY NAMES ${pySideUIC} HINTS ${PYSIDE_BIN_DIR})

if (pySideImportResult)
    # False if the constant ends in the suffix -NOTFOUND.
    if (PYSIDEUICBINARY)
        message(STATUS "Found ${pySideImportResult}: with ${PYTHON_EXECUTABLE}, will use ${PYSIDEUICBINARY} for pyside-uic binary")
        set(PYSIDE_AVAILABLE True)
    else()
        message(STATUS "Found ${pySideImportResult} but NOT pyside-uic binary")
        set(PYSIDE_AVAILABLE False)
    endif()
else()
    if (PYSIDE_USE_PYSIDE2)
        message(STATUS "Did not find PySide2 with ${PYTHON_EXECUTABLE}")
    else()
        message(STATUS "Did not find PySide6 with ${PYTHON_EXECUTABLE}")
    endif()
    set(PYSIDE_AVAILABLE False)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(PySide
    REQUIRED_VARS
        PYSIDE_AVAILABLE
        PYSIDEUICBINARY
)
