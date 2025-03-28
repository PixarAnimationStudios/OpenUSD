#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
if (NOT PYTHON_EXECUTABLE)
    message(FATAL_ERROR "Unable to find Python executable - PyOpenGL not present")
    return()
endif()

execute_process(
    COMMAND 
        "${PYTHON_EXECUTABLE}" "-c" "from OpenGL import *"
    RESULT_VARIABLE
        pyopenglImportResult 
)

if (pyopenglImportResult EQUAL 0)
    message(STATUS "Found PyOpenGL")
    set(PYOPENGL_AVAILABLE True)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(PyOpenGL
    REQUIRED_VARS
        PYOPENGL_AVAILABLE
)
