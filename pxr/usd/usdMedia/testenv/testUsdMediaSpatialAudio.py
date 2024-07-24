#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Usd, UsdMedia
import unittest, math

class TestUsdMediaSpatialAudio(unittest.TestCase):

    def test_TimeAttrs(self):
        # Create layer and stage with a SpatialAudio prim that we will reference
        refLayer = Sdf.Layer.CreateAnonymous()
        refStage = Usd.Stage.Open(refLayer)
        refAudio = UsdMedia.SpatialAudio.Define(refStage, '/RefAudio')

        # Create a new and SpatialAudio prim that references the above prim.
        # The reference has a layer offset of scale=2.0, offset=10.0
        stage = Usd.Stage.CreateInMemory()
        audio = UsdMedia.SpatialAudio.Define(stage, '/Audio')
        refs = audio.GetPrim().GetReferences()
        refs.AddReference(refLayer.identifier, Sdf.Path("/RefAudio"), 
                          layerOffset=Sdf.LayerOffset(scale=2.0, offset=10.0))

        self.assertEqual(refAudio.GetStartTimeAttr().Get(), Sdf.TimeCode(0))
        self.assertEqual(refAudio.GetEndTimeAttr().Get(), Sdf.TimeCode(0))
        self.assertEqual(refAudio.GetMediaOffsetAttr().Get(), 0.0)
        self.assertEqual(audio.GetStartTimeAttr().Get(), Sdf.TimeCode(0))
        self.assertEqual(audio.GetEndTimeAttr().Get(), Sdf.TimeCode(0))
        self.assertEqual(audio.GetMediaOffsetAttr().Get(), 0)

        refAudio.CreateStartTimeAttr().Set(Sdf.TimeCode(10.0))
        refAudio.CreateEndTimeAttr().Set(Sdf.TimeCode(200.0))
        refAudio.CreateMediaOffsetAttr().Set(5.0)
        self.assertEqual(refAudio.GetStartTimeAttr().Get(), Sdf.TimeCode(10))
        self.assertEqual(refAudio.GetEndTimeAttr().Get(), Sdf.TimeCode(200))
        self.assertEqual(refAudio.GetMediaOffsetAttr().Get(), 5.0)
        self.assertEqual(audio.GetStartTimeAttr().Get(), Sdf.TimeCode(30))
        self.assertEqual(audio.GetEndTimeAttr().Get(), Sdf.TimeCode(410))
        self.assertEqual(audio.GetMediaOffsetAttr().Get(), 5.0)

if __name__ == '__main__':
    unittest.main()
