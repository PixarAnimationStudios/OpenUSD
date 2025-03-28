#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Find Draco Compression Library using DRACO_ROOT as a hint location and
# provides the results by defining the variables DRACO_INCLUDES and
# DRACO_LIBRARY.
#

find_library(DRACO_LIBRARY NAMES draco PATHS "${DRACO_ROOT}/lib")
find_path(DRACO_INCLUDES draco/compression/decode.h PATHS "${DRACO_ROOT}/include")
