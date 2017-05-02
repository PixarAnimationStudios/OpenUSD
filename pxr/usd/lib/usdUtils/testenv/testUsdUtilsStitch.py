#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Tf, Sdf, UsdUtils
import unittest

class TestUsdUtilsStitch(unittest.TestCase):
    def setUp(self):
        self.layers = [Sdf.Layer.CreateAnonymous(), 
                       Sdf.Layer.FindOrOpen('src/Particles_Splash.101.usd'),
                       Sdf.Layer.FindOrOpen('src/Particles_Splash.103.usd'),
                       Sdf.Layer.CreateAnonymous(),
                       Sdf.Layer.FindOrOpen('src/letters_perFrame.101.usda'),
                       Sdf.Layer.FindOrOpen('src/letters_perFrame.102.usda'),
                       Sdf.Layer.FindOrOpen('src/letters_perFrame.103.usda')]

        self.rootprims_combined = None
        self.rootprims_a = None
        self.rootprims_b = None

        # stitch layers for basic tests
        UsdUtils.StitchLayers(self.layers[0], self.layers[1])
        UsdUtils.StitchLayers(self.layers[0], self.layers[2])

        self.rootprims_combined = self.layers[0].rootPrims
        self.rootprims_a = self.layers[1].rootPrims
        self.rootprims_b = self.layers[2].rootPrims

        # stitch layers for secondary tests
        UsdUtils.StitchLayers(self.layers[3], self.layers[4])
        UsdUtils.StitchLayers(self.layers[3], self.layers[5])
        UsdUtils.StitchLayers(self.layers[3], self.layers[6])

    def test_AllPrimsFullyCopied(self):
        for prims in [self.rootprims_a, self.rootprims_b]:
            for rootprim in prims:
                for child in rootprim.nameChildren:
                    self.assertTrue(self.layers[0].GetPrimAtPath(child.path),
                        'Missing object at ' + str(child.path))
                for attr in rootprim.attributes:
                    self.assertTrue(self.layers[0].GetAttributeAtPath(attr.path),
                        'Missing attribute at ' + str(attr.path))
                for rel in rootprim.relationships:
                    self.assertTrue(self.layers[0].GetRelationshipAtPath(rel.path),
                        'Missing relationship at ' + str(rel.path))
         
    def test_AllTimeSamplesMerged(self):
        timesamples_a = self.layers[1].ListAllTimeSamples()
        timesamples_b = self.layers[2].ListAllTimeSamples()

        # the composed layer should have the union of time samples
        self.assertTrue(len(self.layers[0].ListAllTimeSamples()) ==
                        len(set().union(*[timesamples_a, timesamples_b])))

    def test_FrameInfoMerged(self):
        hasFps = self.layers[1].framesPerSecond != None or \
                 self.layers[2].framesPerSecond != None
        hasFramePrec = self.layers[1].framePrecision != None or \
                       self.layers[2].framePrecision != None
        hasEndTimeCode = self.layers[1].endTimeCode != None or \
                         self.layers[2].endTimeCode != None
        hasStartTimeCode = self.layers[1].startTimeCode != None or \
                           self.layers[2].startTimeCode != None
        
        self.assertTrue(((hasFps and self.layers[0].framesPerSecond != None) or \
                         (not hasFps)), 'Missing fps')
        self.assertTrue((hasFramePrec and self.layers[0].framePrecision != None) or \
                        (not hasFramePrec), 'Missing fps')
        self.assertTrue(((hasStartTimeCode and self.layers[0].startTimeCode != None) or \
                         (not hasStartTimeCode)), 'Missing fps')
        self.assertTrue(((hasEndTimeCode and self.layers[0].endTimeCode != None) or \
                         (not hasEndTimeCode)), 'Missing fps')
 
    def test_ValidFileCreation(self):
        with Tf.NamedTemporaryFile(suffix='usda') as tempUsd:
            self.assertTrue(self.layers[0].Export(tempUsd.name))

    def test_TargetPathCopy(self):
        primPath = '/World/fx/Letters/points'
        primWithTargets = self.layers[3].GetPrimAtPath(primPath) 
        relationshipInPrim = primWithTargets.relationships[0]
        self.assertTrue(relationshipInPrim, 'Relationship not stitched')
        addedPaths = relationshipInPrim.targetPathList.addedItems
        self.assertEqual(len(addedPaths), 3, 'Paths missing in relationship')

    def test_ConnectionPathCopy(self):
        attrPath = '/World/fx/Letters/points.internalValue'
        attrWithConnections = self.layers[3].GetAttributeAtPath(attrPath) 
        pathList = attrWithConnections.connectionPathList.addedItems
        self.assertTrue(len(pathList) > 0,
                        'Connection missing on attribute')
        self.assertEqual(pathList,
                         [Sdf.Path('/World/fx/Letters.interface:value')],
                         'Connection on attribute has wrong target')


if __name__ == '__main__':
    unittest.main()
