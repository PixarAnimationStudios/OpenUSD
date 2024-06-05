#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

def testUsdviewInputFunction(appController):
    # This test tries to ensure we handle invalid files supplied to usdview
    assert not appController._parserData.usdFile
