#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.

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
