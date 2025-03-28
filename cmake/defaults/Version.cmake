#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Versioning information
set(PXR_MAJOR_VERSION "0")
set(PXR_MINOR_VERSION "25")
set(PXR_PATCH_VERSION "5") # NOTE: Must not have leading 0 for single digits

math(EXPR PXR_VERSION "${PXR_MAJOR_VERSION} * 10000 + ${PXR_MINOR_VERSION} * 100 + ${PXR_PATCH_VERSION}")
