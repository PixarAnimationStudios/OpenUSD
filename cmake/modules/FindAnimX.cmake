#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Finds AnimX library.  Provides the results by defining the variables
# ANIMX_LIBRARY and ANIMX_INCLUDES.
#

find_library(ANIMX_LIBRARY AnimX)
find_path(ANIMX_INCLUDES animx.h)

find_package_handle_standard_args(
    AnimX
    REQUIRED_VARS
        ANIMX_LIBRARY
        ANIMX_INCLUDES
)
