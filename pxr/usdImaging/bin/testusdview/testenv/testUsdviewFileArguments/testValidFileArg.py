#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

def testUsdviewInputFunction(appController):
    # This test tries to ensure we handle valid files supplied to usdview
    assert appController._parserData.usdFile
