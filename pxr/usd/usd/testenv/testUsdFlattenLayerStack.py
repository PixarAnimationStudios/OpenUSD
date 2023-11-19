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

from __future__ import print_function

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

        print('#'*72)
        print('Flattened layer:')
        print(layer.ExportToString())
        print('#'*72)

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
            self.assertEqual( a.GetTimeSamples(),  [-9.0, 0.0] )

            # Layer offsets get folded into reference arcs
            p = stage.GetPrimAtPath('/Sphere/ChildFromReference')
            a = p.GetAttribute('timeSamplesAcrossRef')
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

            # Variant sets and selections
            p = stage.GetPrimAtPath('/assetWithVariant')
            self.assertEqual(
                p.GetVariantSets().GetNames(),
                ['variantSet1', 'variantSet2'])
            self.assertFalse(stage.GetPrimAtPath('/assetWithVariant/a'))
            self.assertTrue(stage.GetPrimAtPath('/assetWithVariant/b'))
            self.assertFalse(stage.GetPrimAtPath('/assetWithVariant/c'))
            self.assertTrue(stage.GetPrimAtPath('/assetWithVariant/d'))


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
        p = layer.GetPrimAtPath('/SphereUsingClip')
        self.assertEqual( p.GetInfo('clips')['default']['active'],
           Vt.Vec2dArray(1, [(-9.0, 0)]) )
        self.assertEqual( p.GetInfo('clips')['default']['times'],
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

    def test_ResolveAssetPathAdvancedFn(self):
        src_stage = Usd.Stage.Open('emptyAssetPaths.usda')
        def replaceWithFoo(context):
            self.assertEqual(context.sourceLayer, src_stage.GetRootLayer())
            self.assertEqual(context.assetPath, '')
            self.assertEqual(context.expressionVariables, {})
            return 'foo'

        src_layer_stack = src_stage.GetPseudoRoot().GetPrimIndex().rootNode.layerStack
        layer = Usd.FlattenLayerStackAdvanced(src_layer_stack, 
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

    def test_ResolveAssetPathWithExpressions(self):
        src_stage = Usd.Stage.Open('assetPathsAndExpressions.usda')
        src_layer_stack = src_stage.GetPseudoRoot().GetPrimIndex().rootNode.layerStack

        # The simple Usd.FlattenLayerStack function should evaluate asset path
        # expressions and anchor the result to the layer where the opinion was
        # authored.
        layer = Usd.FlattenLayerStack(src_layer_stack)
        self.assertEqual(
            layer.GetAttributeAtPath('/Test.a').default,
            Sdf.ComputeAssetPathRelativeToLayer(
                src_stage.GetRootLayer(), './assetPathsTest.usda'))

        # The more advanced Usd.FlattenLayerStack function should pass the
        # evaluated asset path expression to the resolve callback to allow
        # it to perform whatever anchoring/replacement behaviors it wants.
        def replaceWithFoo(layer, assetPath):
            self.assertEqual(layer, src_stage.GetRootLayer())
            self.assertEqual(assetPath, './assetPathsTest.usda')
            return 'foo'

        layer = Usd.FlattenLayerStack(src_layer_stack,
                resolveAssetPathFn=replaceWithFoo)
        self.assertEqual(
            layer.GetAttributeAtPath('/Test.a').default,
            Sdf.AssetPath('foo'))

        # Usd.FlattenLayerStackAdvanced should pass the unevaluated asset
        # path expression along with the applicable expression variables to
        # the resolve callback to allow it to fully customize the evaluation,
        # anchoring, and replacement behaviors.
        def replaceWithFoo(context):
            self.assertEqual(context.sourceLayer, src_stage.GetRootLayer())
            self.assertEqual(context.assetPath, '`"./${NAME}.usda"`')
            self.assertEqual(context.expressionVariables, {'NAME':'assetPathsTest'})
            return 'foo'

        layer = Usd.FlattenLayerStackAdvanced(src_layer_stack,
                resolveAssetPathFn=replaceWithFoo)
        self.assertEqual(
            layer.GetAttributeAtPath('/Test.a').default,
            Sdf.AssetPath('foo'))

    def test_ResolveAssetPathWithExpressionsAcrossReference(self):
        # Create source stage with a prim that references 
        # assetPathsAndExpressions.usda but overrides the expression variable in
        # that layer stack.
        rootLayer = Sdf.Layer.CreateAnonymous('.usda')
        rootLayer.ImportFromString('''
        #usda 1.0
        (
            expressionVariables = {
                string NAME = "test_from_ref"
            }
        )

        def "TestRef" (
            references = @./assetPathsAndExpressions.usda@</Test>
        )
        {
        }
        '''.strip())

        src_stage = Usd.Stage.Open(rootLayer)

        # When we get the value of /TestRef.expression, the "NAME" variable
        # authored in our root layer should be used to evaluate the expression
        # from assetPathsAndExpressions.usda, giving us "test_from_ref.usda". 
        #
        # Note that since this layer doesn't actually exist, calling Get() gives
        # us an Sdf.AssetPath with the asset path set to the result of
        # evaluating the expression and no resolved path.
        self.assertEqual(
            src_stage.GetAttributeAtPath('/TestRef.a').Get(),
            Sdf.AssetPath('./test_from_ref.usda'))

        # Now, flatten the *referenced* layer stack retrieved from /TestRef's
        # prim index. Flattening should evaluate the expression using the
        # variables authored in the *referenced* layer stack and anchor it to
        # the layer where it was authored. So instead of 'test_from_ref.usda',
        # we expect 'assetPathsTest.usda'.
        #
        # This ensures the attribute value when loading the flattened layer on a
        # Usd.Stage will be consistent with loading the root (and session) layer
        # of the original layer stack on a Usd.Stage.
        primIndex = src_stage.GetPrimAtPath('/TestRef').GetPrimIndex()
        ref_layer_stack = primIndex.rootNode.children[0].layerStack

        layer = Usd.FlattenLayerStack(ref_layer_stack)
        self.assertEqual(
            layer.GetAttributeAtPath('/Test.a').default,
            Sdf.ComputeAssetPathRelativeToLayer(
                ref_layer_stack.layers[0], './assetPathsTest.usda'))

        # The resolve callback used by Usd.FlattenLayerStackAdvanced
        # should receive the expression variables from the referenced layer
        # stack.
        def resolveFn(context):
            self.assertEqual(context.sourceLayer, ref_layer_stack.layers[0])
            self.assertEqual(context.assetPath, '`"./${NAME}.usda"`')
            self.assertEqual(
                context.expressionVariables, 
                {'NAME':'assetPathsTest'})
            return Usd.FlattenLayerStackResolveAssetPathAdvanced(context)

        layer = Usd.FlattenLayerStackAdvanced(ref_layer_stack, resolveFn)
        self.assertEqual(
            layer.GetAttributeAtPath('/Test.a').default,
            Sdf.ComputeAssetPathRelativeToLayer(
                ref_layer_stack.layers[0], './assetPathsTest.usda'))

        # Verify the consistency guarantee mentioned above. Note that this
        # guarantee currently only applies to the resolved path; the authored
        # paths may differ.
        stage_from_flattened = Usd.Stage.Open(layer)
        stage_from_layer_stack = Usd.Stage.Open(
            ref_layer_stack.identifier.rootLayer)

        self.assertEqual(
            stage_from_flattened.GetAttributeAtPath('/Test.a').Get().resolvedPath,
            stage_from_layer_stack.GetAttributeAtPath('/Test.a').Get().resolvedPath)

    def test_ValueBlocks(self):
        src_stage = Usd.Stage.Open('valueBlocks_root.usda')
        src_layer_stack = src_stage._GetPcpCache().layerStack
        layer = Usd.FlattenLayerStack(src_layer_stack, tag='valueBlocks')
        print(layer.ExportToString())
        result_stage = Usd.Stage.Open(layer)

        # verify that value blocks worked
        prim = result_stage.GetPrimAtPath('/Human')
        a = prim.GetAttribute('a')
        self.assertEqual(a.Get(), Vt.IntArray(1, (1,)))

        # a strong value block flattens to a value block
        b = prim.GetAttribute('b')
        self.assertEqual(b.Get(), None)

        # a value block will block both defaults and time samples
        c = prim.GetAttribute('c')
        self.assertEqual(c.Get(), None)
        self.assertEqual(c.Get(1), None)

        # strong time samples will override a weaker value block
        d = prim.GetAttribute('d')
        self.assertEqual(d.Get(1), 789)

    def test_ValueTypeMismatch(self):
        src_stage = Usd.Stage.Open('valueTypeMismatch_root.usda')
        src_layer_stack = src_stage._GetPcpCache().layerStack
        layer = Usd.FlattenLayerStack(src_layer_stack, tag='valueBlocks')
        print(layer.ExportToString())
        result_stage = Usd.Stage.Open(layer)

        # Verify that the strongest value type prevailed
        prim = result_stage.GetPrimAtPath('/p')
        a = prim.GetAttribute('x')
        self.assertEqual(a.Get(), Vt.IntArray(1, (0,)))

    def test_Tags(self):
        src_stage = Usd.Stage.Open('root.usda')
        src_layer_stack = src_stage._GetPcpCache().layerStack

        tagToExtension = {
            '': '.usda',
            'test.usda': '.usda',
            'test.usdc': '.usdc',
            'test.sdf': '.sdf'
        }
        for (tag, extension) in tagToExtension.items():
            layer = Usd.FlattenLayerStack(src_layer_stack, tag=tag)
            self.assertTrue(layer.identifier.endswith(extension))

    def test_FlattenLayerStackPathsWithMissungUriResolvers(self):
        """Tests that when flattening, asset paths that contain URI schemes
        for which there is no registered resolver are left unmodified
        """

        rootLayer = Sdf.Layer.CreateAnonymous(".usda")
        rootLayer.ImportFromString("""
        #usda 1.0

        def "TestPrim"(
            assetInfo = {
                asset identifier = @test123://1.2.3.4/file3.txt@
                asset[] assetRefArr = [@test123://1.2.3.4/file6.txt@]
            }
        )
        {
            asset uriAssetRef = @test123://1.2.3.4/file1.txt@
            asset[] uriAssetRefArray = [@test123://1.2.3.4/file2.txt@]

            asset uriAssetRef.timeSamples = {
                0: @test123://1.2.3.4/file4.txt@,
                1: @test123://1.2.3.4/file5.txt@,
            }
                                   
            asset[] uriAssetRefArray.timeSamples = {
                0: [@test123://1.2.3.4/file6.txt@],
                1: [@test123://1.2.3.4/file7.txt@],               
            }
        }
        """.strip())
        
        stage = Usd.Stage.Open(rootLayer)
        flatStage = Usd.Stage.Open(stage.Flatten())

        propPath = "/TestPrim.uriAssetRef"
        stageProp = stage.GetPropertyAtPath(propPath)
        flatStageProp = flatStage.GetPropertyAtPath(propPath)
        self.assertEqual(stageProp.Get(), flatStageProp.Get())
        
        self.assertEqual(stageProp.GetTimeSamples(), 
                         flatStageProp.GetTimeSamples())

        for timeSample in stageProp.GetTimeSamples():
            self.assertEqual(stageProp.Get(timeSample), flatStageProp.Get(timeSample))

        arrayPath = "/TestPrim.uriAssetRefArray"
        self.assertEqual(stage.GetPropertyAtPath(arrayPath).Get(), 
                         flatStage.GetPropertyAtPath(arrayPath).Get())
            
        self.assertEqual(stage.GetPropertyAtPath(arrayPath).GetTimeSamples(), 
                         flatStage.GetPropertyAtPath(arrayPath).GetTimeSamples())
        
        for timeSample in stage.GetPropertyAtPath(arrayPath).GetTimeSamples():
            self.assertEqual(stage.GetPropertyAtPath(arrayPath).Get(timeSample), 
                             flatStage.GetPropertyAtPath(arrayPath).Get(timeSample))

        primPath = "/TestPrim"
        self.assertEqual(
            stage.GetPrimAtPath(primPath).GetMetadata("assetInfo").get("identifier"), 
            flatStage.GetPrimAtPath(primPath).GetMetadata("assetInfo").get("identifier"))
        
        self.assertEqual(
            stage.GetPrimAtPath(primPath).GetMetadata("assetInfo").get("assetRefArr"), 
            flatStage.GetPrimAtPath(primPath).GetMetadata("assetInfo").get("assetRefArr"))

if __name__=="__main__":
    # Register test plugin defining timecode metadata fields.
    testDir = os.path.abspath(os.getcwd())
    assert len(Plug.Registry().RegisterPlugins(testDir)) == 1

    unittest.main()
