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
        relPath = '/World/fx/Letters/points.prototypes'
        rel = self.layers[3].GetRelationshipAtPath(relPath)
        self.assertTrue(rel, 'Relationship not stitched')

        expectedTargets = Sdf.PathListOp()
        expectedTargets.prependedItems = [
            '/World/fx/Letters/points/Prototypes/obj/proto_2',
            '/World/fx/Letters/points/Prototypes/obj/proto_3',
            '/World/fx/Letters/points/Prototypes/obj/proto_1',
            '/World/fx/Letters/points/Prototypes/obj/proto_4'
        ]
        self.assertEqual(rel.GetInfo('targetPaths'), expectedTargets)

    def test_ConnectionPathCopy(self):
        attrPath = '/World/fx/Letters/points.internalValue'
        attr = self.layers[3].GetAttributeAtPath(attrPath) 
        self.assertTrue(attr, 'Attribute not stitched')

        expectedTargets = Sdf.PathListOp()
        expectedTargets.prependedItems = [
            '/World/fx/Letters.interface:value'
        ]
        self.assertEqual(attr.GetInfo('connectionPaths'), expectedTargets)

    def test_Dictionary(self):
        layer = self.layers[3]
        self.assertEqual(layer.GetPrimAtPath('/World').GetInfo('customData'),
                         { 'zUp': True,
                           'testInt': 1,
                           'testString': 'foo' })

    def test_Variants(self):
        layer1 = Sdf.Layer.FindOrOpen('src/variants_1.usda')
        layer2 = Sdf.Layer.FindOrOpen('src/variants_2.usda')

        # First, stitch to an empty layer -- this should be equivalent
        # to making a copy of the weaker layer.
        l = Sdf.Layer.CreateAnonymous('.usda')
        UsdUtils.StitchLayers(l, layer1)
        self.assertEqual(layer1.ExportToString(), l.ExportToString(),
                         ("Expected:\n%s\nResult:\n%s" % 
                          (layer1.ExportToString(), l.ExportToString())))

        l = Sdf.Layer.CreateAnonymous('.usda')
        UsdUtils.StitchLayers(l, layer2)
        self.assertEqual(layer2.ExportToString(), l.ExportToString(),
                         ("Expected:\n%s\nResult:\n%s" % 
                          (layer2.ExportToString(), l.ExportToString())))

        # Stitch the two layers together and verify that variants
        # are merged as expected.
        UsdUtils.StitchLayers(layer1, layer2)

        self.assertTrue(layer1.GetObjectAtPath('/Root{a=x}Child'))
        self.assertTrue(layer1.GetObjectAtPath('/Root{a=x}Child.testAttr'))
        self.assertTrue(layer1.GetObjectAtPath('/Root{a=x}Child.testRel'))
        self.assertTrue(layer1.GetObjectAtPath('/Root{a=x}Child_2'))
        self.assertTrue(layer1.GetObjectAtPath('/Root{a=x}Child_2.testAttr'))
        self.assertTrue(layer1.GetObjectAtPath('/Root{a=x}Child_2.testRel'))
        self.assertTrue(layer1.GetObjectAtPath('/Root{b=x}Child'))

        self.assertEqual(
            layer1.GetAttributeAtPath('/Root{a=x}Child.testAttr')
            .GetInfo('timeSamples'),
            {1.0: 1.0, 2.0: 2.0})

        self.assertEqual(
            layer1.GetAttributeAtPath('/Root{a=x}Child_2.testAttr')
            .GetInfo('timeSamples'),
            {2.0: 2.0})

        expectedTargets = Sdf.PathListOp()
        expectedTargets.prependedItems = ['/Root/Test', '/Root/Test2']
        self.assertEqual(
            layer1.GetRelationshipAtPath('/Root{a=x}Child.testRel')
            .GetInfo('targetPaths'),
            expectedTargets)

        expectedTargets.prependedItems = ['/Root/Test2']
        self.assertEqual(
            layer1.GetRelationshipAtPath('/Root{a=x}Child_2.testRel')
            .GetInfo('targetPaths'),
            expectedTargets)

        self.assertEqual(dict(layer1.GetPrimAtPath('/Root').variantSelections), 
                         {'a':'x', 'b':'x'})

        expectedNames = Sdf.StringListOp()
        expectedNames.prependedItems = ['a', 'b']
        self.assertEqual(layer1.GetPrimAtPath('/Root').GetInfo('variantSetNames'), 
                         expectedNames)

if __name__ == '__main__':
    unittest.main()
