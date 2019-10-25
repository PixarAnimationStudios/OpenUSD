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
from pxr import Sdf, Usd, Gf, Vt, Plug
import os, unittest

class TestUsdFlattenLayerStack(unittest.TestCase):
    def test_Basic(self):
        src_stage = Usd.Stage.Open('root.usda')
        src_layer_stack = src_stage._GetPcpCache().layerStack

        layer = Usd.FlattenLayerStack(src_layer_stack, tag="test.usda")
        result_stage = Usd.Stage.Open(layer)

        # Confirm that the tag makde it into the display name.
        self.assertTrue('test.usda' in layer.GetDisplayName())

        print '#'*72
        print 'Flattened layer:'
        print layer.ExportToString()
        print '#'*72

        # Run the same set of queries against the src and dest stages.
        # They should yield the same results.
        for stage in [src_stage, result_stage]:
            p = stage.GetPrimAtPath('/Sphere')

            # Composition arcs remain intact
            self.assertTrue(p.HasAuthoredReferences())
            self.assertTrue(p.HasAuthoredInherits())
            self.assertTrue(p.HasAuthoredPayloads())
            self.assertTrue(p.HasVariantSets())

            # Classes continue to exist
            self.assertTrue( stage.GetPrimAtPath('/_class_Sphere') )

            # Default in a strong layer wins over timeSamples in weak.
            a = p.GetAttribute('defaultOverTimeSamples')
            self.assertEqual( a.GetResolveInfo().GetSource(),
                    Usd.ResolveInfoSourceDefault )
            a = p.GetAttribute('timeSamplesOverDefault')
            self.assertEqual( a.GetResolveInfo().GetSource(),
                    Usd.ResolveInfoSourceTimeSamples)
            self.assertEqual( a.Get(), 123 ) # default
            self.assertEqual( a.Get(1), 1 ) # time sample

            # Layer offsets affect time samples
            a = p.GetAttribute('timeSamplesAcrossLayerOffset')
            if Usd.UsesInverseLayerOffset():
                self.assertEqual( a.GetTimeSamples(),  [11.0, 20.0] )
            else:
                self.assertEqual( a.GetTimeSamples(),  [-9.0, 0.0] )

            # Layer offsets get folded into reference arcs
            p = stage.GetPrimAtPath('/Sphere/ChildFromReference')
            a = p.GetAttribute('timeSamplesAcrossRef')
            if Usd.UsesInverseLayerOffset():
                self.assertEqual( a.GetTimeSamples(),  [11.0, 20.0] )
            else:
                self.assertEqual( a.GetTimeSamples(),  [-9.0, 0.0] )

            # Confirm result of list-editing.
            p = stage.GetPrimAtPath('/ListOpTest')
            r = p.GetRelationship('foo')
            self.assertEqual( r.GetTargets(),
                [Sdf.Path('/Pre_root'),
                 Sdf.Path('/Pre_sub'),
                 Sdf.Path('/App_sub'),
                 Sdf.Path('/App_root')])

            # Confirm children from across various kinds of arcs.
            for childPath in [
                '/Sphere/ChildFromPayload',
                '/Sphere/ChildFromReference',
                '/Sphere/ChildFromNestedVariant']:
                self.assertTrue(stage.GetPrimAtPath(childPath))

            # Confirm time samples coming from (offset) clips.
            p = stage.GetPrimAtPath('/SphereUsingClip_LegacyForm')
            a = p.GetAttribute('xformOp:translate')
            self.assertEqual( a.GetResolveInfo(5).GetSource(),
                    Usd.ResolveInfoSourceValueClips )
            p = stage.GetPrimAtPath('/SphereUsingClip')
            a = p.GetAttribute('xformOp:translate')
            self.assertEqual( a.GetResolveInfo(5).GetSource(),
                    Usd.ResolveInfoSourceValueClips )

            # Confirm time code values were resolve across the offset layer
            p = stage.GetPrimAtPath('/TimeCodeTest')

            # Prim metadata
            self.assertEqual(p.GetMetadata("timeCodeTest"), 0.0)
            self.assertEqual(p.GetMetadata("timeCodeArrayTest"), 
                             Sdf.TimeCodeArray([0.0, 10.0]))
            self.assertEqual(p.GetMetadata("doubleTest"), 10.0)

            self.assertEqual(
                p.GetMetadataByDictKey("customData", "timeCode"), 0.0)
            self.assertEqual(
                p.GetMetadataByDictKey("customData", "timeCodeArray"), 
                Sdf.TimeCodeArray([0.0, 10.0]))
            self.assertEqual(
                p.GetMetadataByDictKey("customData", "doubleVal"), 10.0)
            self.assertEqual(
                p.GetMetadataByDictKey("customData", "subDict:timeCode"), 0.0)
            self.assertEqual(
                p.GetMetadataByDictKey("customData", "subDict:timeCodeArray"), 
                Sdf.TimeCodeArray([0.0, 10.0]))
            self.assertEqual(
                p.GetMetadataByDictKey("customData", "subDict:doubleVal"), 10.0)

            # Atribute defaults and time samples
            a = p.GetAttribute("TimeCode")
            self.assertEqual(a.Get(), 0.0)
            self.assertEqual(a.GetTimeSamples(), [-10.0, -9.0])
            self.assertEqual(a.Get(-10.0), 0.0)
            self.assertEqual(a.Get(-9), 10.0)

            a = p.GetAttribute("TimeCodeArray")
            self.assertEqual(a.Get(), Sdf.TimeCodeArray([0.0, 10.0]))
            self.assertEqual(a.GetTimeSamples(), [-10.0, -9.0])
            self.assertEqual(a.Get(-10.0), Sdf.TimeCodeArray([0.0, 20.0]))
            self.assertEqual(a.Get(-9), Sdf.TimeCodeArray([10.0, 30.0]))

            a = p.GetAttribute("Double")
            self.assertEqual(a.Get(), 10.0)
            self.assertEqual(a.GetTimeSamples(), [-10.0, -9.0])
            self.assertEqual(a.Get(-10.0), 10.0)
            self.assertEqual(a.Get(-9), 20.0)


        # Confirm relative asset path handling in the output stage.
        p = result_stage.GetPrimAtPath('/Sphere')
        a = p.GetAttribute('relativePath')
        # It should have become an absolute path.
        self.assertTrue(os.path.isabs(a.Get().path))
        # Check arrays of paths.
        a = p.GetAttribute('relativePathVec')
        for path in a.Get():
            self.assertTrue(os.path.isabs(path.path) )

        # Confirm Sdf-level result of list-editing.
        p = layer.GetPrimAtPath('/ListOpTest')
        targets = p.relationships['foo'].targetPathList
        self.assertEqual(list(targets.prependedItems),
                ['/Pre_root', '/Pre_sub'])
        self.assertEqual(list(targets.appendedItems),
                ['/App_sub', '/App_root'])
        self.assertEqual(list(targets.deletedItems),
                ['/Del_sub', '/Del_root'])

        # Confirm offsets have been folded into value clips.
        p1 = layer.GetPrimAtPath('/SphereUsingClip_LegacyForm')
        p2 = layer.GetPrimAtPath('/SphereUsingClip')
        if Usd.UsesInverseLayerOffset():
            self.assertEqual( p1.GetInfo('clipActive'),
               Vt.Vec2dArray(1, [(11, 0)]) )
            self.assertEqual( p1.GetInfo('clipTimes'),
               Vt.Vec2dArray(2, [(11, 1), (20, 10)]) )
            self.assertEqual( p2.GetInfo('clips')['default']['active'],
               Vt.Vec2dArray(1, [(11, 0)]) )
            self.assertEqual( p2.GetInfo('clips')['default']['times'],
               Vt.Vec2dArray(2, [(11, 1), (20, 10)]) )
        else:
            self.assertEqual( p1.GetInfo('clipActive'),
               Vt.Vec2dArray(1, [(-9.0, 0.0)]) )
            self.assertEqual( p1.GetInfo('clipTimes'),
               Vt.Vec2dArray(2, [(-9.0, 1), (0.0, 10)]) )
            self.assertEqual( p2.GetInfo('clips')['default']['active'],
               Vt.Vec2dArray(1, [(-9.0, 0)]) )
            self.assertEqual( p2.GetInfo('clips')['default']['times'],
               Vt.Vec2dArray(2, [(-9.0, 1), (0.0, 10)]) )

        # Confirm nested variant sets still exist
        self.assertTrue(layer.GetObjectAtPath(
            '/Sphere{vset_1=default}{vset_2=default}ChildFromNestedVariant'))

    def test_EmptyAssetPaths(self):
        src_stage = Usd.Stage.Open('emptyAssetPaths.usda')
        src_layer_stack = src_stage._GetPcpCache().layerStack

        layer = Usd.FlattenLayerStack(src_layer_stack, tag='emptyAssetPaths')
        result_stage = Usd.Stage.Open(layer)

        # Verify that empty asset paths do not trigger coding errors during
        # flattening.

        prim = result_stage.GetPrimAtPath('/Test')
        self.assertTrue(prim.HasAuthoredReferences())

        primSpec = layer.GetPrimAtPath('/Test')
        expectedRefs = Sdf.ReferenceListOp()
        expectedRefs.explicitItems = [Sdf.Reference(primPath="/Ref")]
        self.assertEqual(primSpec.GetInfo('references'), expectedRefs)

        assetAttr = prim.GetAttribute('a')
        self.assertEqual(assetAttr.Get(), Sdf.AssetPath())

        assetArrayAttr = prim.GetAttribute('b')
        self.assertEqual(list(assetArrayAttr.Get()), 
                         [Sdf.AssetPath(), Sdf.AssetPath()])

    def test_ResolveAssetPathFn(self):
        src_stage = Usd.Stage.Open('emptyAssetPaths.usda')
        def replaceWithFoo(layer, s):
            return 'foo'

        src_layer_stack = src_stage._GetPcpCache().layerStack
        layer = Usd.FlattenLayerStack(src_layer_stack, 
                resolveAssetPathFn=replaceWithFoo,
                tag='resolveAssetPathFn')
        result_stage = Usd.Stage.Open(layer)

        # verify that we replaced asset paths with "foo"

        prim = result_stage.GetPrimAtPath('/Test')
        assetAttr = prim.GetAttribute('a')
        self.assertEqual(assetAttr.Get(), Sdf.AssetPath('foo'))

        assetArrayAttr = prim.GetAttribute('b')
        self.assertEqual(list(assetArrayAttr.Get()), 
                         [Sdf.AssetPath('foo'), Sdf.AssetPath('foo')])

if __name__=="__main__":
    # Register test plugin defining timecode metadata fields.
    testDir = os.path.abspath(os.getcwd())
    assert len(Plug.Registry().RegisterPlugins(testDir)) == 1

    unittest.main()
