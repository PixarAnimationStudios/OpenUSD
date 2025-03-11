#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at

import unittest

from pxr import Pcp, Sdf, Tf

def _ComposeLayersWithNegativeOffsetScale():
    refLayer = Sdf.Layer.CreateAnonymous("ref")
    refLayer.ImportFromString('''
    #sdf 1.4.32

    def "prim" {
        double attr.timeSamples = {
            0: 1.0,
            1: 2.0,
        }
    }
    '''.strip())

    subLayer = Sdf.Layer.CreateAnonymous("sub")
    subLayer.ImportFromString(f'''
    #sdf 1.4.32
    (
        subLayers = [
            @{refLayer.identifier}@ (scale = -1)
        ]
    )
    '''.strip())

    rootLayer = Sdf.Layer.CreateAnonymous()
    rootLayer.ImportFromString(f'''
    #sdf 1.4.32

    def "prim" (
        references = @{subLayer.identifier}@</prim> (scale = -1)
    )
    {{
    }}
    '''.strip())

    pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))
    _, errs = pcpCache.ComputePrimIndex("/prim")
    return errs

class TestPcpNegativeLayerOffsetScale(unittest.TestCase):

    # Following will not result in a composition error
    @unittest.skipIf(not Tf.GetEnvSetting("PCP_ALLOW_NEGATIVE_LAYER_OFFSET_SCALE"),
                     "Allow negative layer offset scale, no composition error")
    def test_NegativeLayerOffsetScaleAllowed(self):
        errs = _ComposeLayersWithNegativeOffsetScale()
        self.assertEqual(len(errs), 0)

    # Following will result in a composition error
    @unittest.skipIf(Tf.GetEnvSetting("PCP_ALLOW_NEGATIVE_LAYER_OFFSET_SCALE"),
                     "Do not allow negative layer offset scale, composition error")
    def test_NegativeLayerOffsetScaleNotAllowed(self):
        errs = _ComposeLayersWithNegativeOffsetScale()
        self.assertEqual(len(errs), 2)

if __name__ == "__main__":
    unittest.main()
