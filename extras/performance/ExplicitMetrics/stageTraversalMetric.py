#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Usd, UsdUtils, Usdviewq

def testUsdviewInputFunction(appController):
    with Usdviewq.Timer("traverse stage", True):
        stage = appController._dataModel.stage
        for prim in stage.Traverse():
            pass  # Just traverse all prims

