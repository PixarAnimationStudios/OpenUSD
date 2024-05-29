#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# set a timestamp id to identify this run of tests in the environment, if not
# already set
if (NOT DEFINED ENV{PXR_CTEST_RUN_ID})
    # otherwise, 
    # Note - can't use default format for TIMESTAMP, as it contains ":", which
    # isn't allowed in windows filepaths
    string(TIMESTAMP _current_time "%Y-%m-%dT%H.%M.%S")
    set(ENV{PXR_CTEST_RUN_ID} ${_current_time})
endif()
