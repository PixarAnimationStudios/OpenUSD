#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import UsdUtils, Sdf, Usd
import unittest

class TestUsdUtilsStageCache(unittest.TestCase):
    def test_Basic(self):
        # Verify that the UsdUtils.StageCache singleton 
        # does not crash upon system teardown.
        lyr = Sdf.Layer.CreateAnonymous()
        with Usd.StageCacheContext(UsdUtils.StageCache.Get()):
            stage = Usd.Stage.Open(lyr)

if __name__=="__main__":
    unittest.main()
