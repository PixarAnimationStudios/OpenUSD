#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
if (NOT PYTHON_EXECUTABLE)
    message(STATUS "Unable to find Python executable - PySide not present")
    set(PYSIDE_FOUND False)
    return()
endif()

execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" "-c" "import PySide"
    RESULT_VARIABLE pySideImportResult 
)

if (pySideImportResult EQUAL 1)
    execute_process(
        COMMAND "${PYTHON_EXECUTABLE}" "-c" "import PySide2"
        RESULT_VARIABLE pySideImportResult 
    )
    if (pySideImportResult EQUAL 0)
        set(pySideImportResult "PySide2")
        set(pySideUIC pyside2-uic python2-pyside2-uic pyside2-uic-2.7)
    else()
        set(pySideImportResult 0)
    endif()
  else()
    set(pySideImportResult "PySide")
    set(pySideUIC pyside-uic python2-pyside-uic pyside-uic-2.7)
endif()

find_program(PYSIDEUICBINARY NAMES ${pySideUIC} HINTS ${PYSIDE_BIN_DIR})

if (pySideImportResult)
    if (EXISTS ${PYSIDEUICBINARY})
        message(STATUS "Found ${pySideImportResult}: with ${PYTHON_EXECUTABLE}, will use ${PYSIDEUICBINARY} for pyside-uic binary")
        set(PYSIDE_FOUND True)
        if (pySideImportResult STREQUAL "PySide2")
            message(STATUS "Building against PySide2 is currently experimental.  "
                           "See https://bugreports.qt.io/browse/PYSIDE-357 if "
                           "'No module named Compiler' errors are encountered"
            )
        endif()
    else()
        message(STATUS "Found ${pySideImportResult} but NOT pyside-uic binary")
        set(PYSIDE_FOUND False)
    endif()
else()
    message(STATUS "Did not find PySide with ${PYTHON_EXECUTABLE}")
    set(PYSIDE_FOUND False)
endif()
