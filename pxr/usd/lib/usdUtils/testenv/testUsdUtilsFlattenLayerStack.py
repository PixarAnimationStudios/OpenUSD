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
from pxr import UsdUtils, Sdf, Usd, Gf, Vt
import unittest

class TestUsdUtilsFlattenLayerStack(unittest.TestCase):
    def test_Basic(self):
        src_stage = Usd.Stage.Open('root.usda')
        layer = UsdUtils.FlattenLayerStack(src_stage)
        result_stage = Usd.Stage.Open(layer)

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
            self.assertTrue(p.HasPayload())
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
            self.assertEqual( a.GetTimeSamples(),  [11.0, 20.0] )

            # Layer offsets get folded into reference arcs
            p = stage.GetPrimAtPath('/Sphere/ChildFromReference')
            a = p.GetAttribute('timeSamplesAcrossRef')
            self.assertEqual( a.GetTimeSamples(),  [11.0, 20.0] )

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

        # Confirm relative asset path handling in the output stage.
        p = result_stage.GetPrimAtPath('/Sphere')
        a = p.GetAttribute('relativePath')
        # It should have become an absolute path.
        self.assertTrue( a.Get().path.startswith('/') )
        # Check arrays of paths.
        a = p.GetAttribute('relativePathVec')
        for path in a.Get():
            self.assertTrue( path.path.startswith('/') )

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
        p = layer.GetPrimAtPath('/SphereUsingClip_LegacyForm')
        self.assertEqual( p.GetInfo('clipActive'),
           Vt.Vec2dArray(1, [(11, 0)]) )
        self.assertEqual( p.GetInfo('clipTimes'),
           Vt.Vec2dArray(2, [(11, 1), (20, 10)]) )
        p = layer.GetPrimAtPath('/SphereUsingClip')
        self.assertEqual( p.GetInfo('clips')['default']['active'],
           Vt.Vec2dArray(1, [(11, 0)]) )
        self.assertEqual( p.GetInfo('clips')['default']['times'],
           Vt.Vec2dArray(2, [(11, 1), (20, 10)]) )

        # Confirm nested variant sets still exist
        self.assertTrue(layer.GetObjectAtPath(
            '/Sphere{vset_1=default}{vset_2=default}ChildFromNestedVariant'))

if __name__=="__main__":
    unittest.main()
