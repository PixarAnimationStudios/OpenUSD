#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Jinja2 is a python library, ensure that it is available for use with our
# specified version of Python.
#
if (NOT PYTHON_EXECUTABLE)
    return()
endif()

execute_process(
    COMMAND 
        "${PYTHON_EXECUTABLE}" "-c" "import jinja2"
    RESULT_VARIABLE
        jinja2ImportResult 
)
if (jinja2ImportResult EQUAL 0)
    message(STATUS "Found Jinja2")
    set(JINJA2_FOUND True)
endif()


