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

import unittest

class TestUsdBugs(unittest.TestCase):
    def test_153956(self):
        from pixar import Sdf

        # Create a crate-backed .usd file and populate it with an
        # attribute connection. These files do not store specs for
        # targets/connections, so there will be an entry in the
        # connectionChildren list but no corresponding spec.
        layer = Sdf.Layer.CreateAnonymous(".usd")
        primSpec = Sdf.CreatePrimInLayer(layer, "/Test")
        attrSpec = Sdf.AttributeSpec(primSpec, "attr", Sdf.ValueTypeNames.Float)

        # -- Adding item to prependedItems list..."
        attrSpec.connectionPathList.prependedItems.append("/Test.prependedItem")

        # Transfer the contents of the crate-backed .usd file into an
        # memory-backed .usda file. These file *do* store specs for
        # targets/connections.
        newLayer = Sdf.Layer.CreateAnonymous(".usda")
        newLayer.TransferContent(layer)

        primSpec = newLayer.GetPrimAtPath("/Test")
        attrSpec = primSpec.properties["attr"]

        # Adding an item to the explicitItems list changes to listOp to
        # explicit mode, but does not clear any existing connectionChildren.
        attrSpec.connectionPathList.explicitItems.append("/Test.explicitItem")

        # Prior to the fix, this caused a failed verify b/c an entry exists in
        # the connectionChildren list for which there is no corresponding spec.
        primSpec.name = "Test2"

    def test_141718(self):
        from pxr import Sdf
        crateLayer = Sdf.Layer.CreateAnonymous('.usdc')
        prim = Sdf.CreatePrimInLayer(crateLayer, '/Prim')
        rel = Sdf.RelationshipSpec(prim, 'myRel', custom=False)
        rel.targetPathList.explicitItems.append('/Prim2')
        asciiLayer = Sdf.Layer.CreateAnonymous('.usda')
        asciiLayer.TransferContent(crateLayer)
        p = asciiLayer.GetPrimAtPath('/Prim')
        p.RemoveProperty(p.relationships['myRel'])

    def test_155392(self):
        from pxr import Sdf, Usd
        # Usd should maintain load state across instancing changes.
        l1 = Sdf.Layer.CreateAnonymous('.usda')
        l2 = Sdf.Layer.CreateAnonymous('.usda')

        xpay = Sdf.CreatePrimInLayer(l2, '/xpay')
        ypay = Sdf.CreatePrimInLayer(l2, '/ypay')

        p1 = Sdf.CreatePrimInLayer(l1, '/p1')
        p1.specifier = Sdf.SpecifierDef
        p1.referenceList.Add(Sdf.Reference('','/x'))

        p2 = Sdf.CreatePrimInLayer(l1, '/p2')
        p2.specifier = Sdf.SpecifierDef
        p2.referenceList.Add(Sdf.Reference('','/x'))

        x = Sdf.CreatePrimInLayer(l1, '/x')
        x.instanceable = True
        x.specifier = Sdf.SpecifierDef
        x.payload = Sdf.Payload(l2.identifier, '/xpay')

        y = Sdf.CreatePrimInLayer(l1, '/x/y')
        y.specifier = Sdf.SpecifierDef
        y.payload = Sdf.Payload(l2.identifier, '/ypay');

        s = Usd.Stage.Open(l1, Usd.Stage.LoadAll)

        self.assertTrue(
            all([x.IsLoaded() for x in [
                s.GetPrimAtPath('/p1'),
                s.GetPrimAtPath('/p1/y'),
                s.GetPrimAtPath('/p2'),
                s.GetPrimAtPath('/p2/y')]]))

        # Now uninstance, and assert that load state is preserved.
        s.GetPrimAtPath('/p2').SetInstanceable(False)

        self.assertTrue(
            all([x.IsLoaded() for x in [
                s.GetPrimAtPath('/p1'),
                s.GetPrimAtPath('/p1/y'),
                s.GetPrimAtPath('/p2'),
                s.GetPrimAtPath('/p2/y')]]))

        # Reinstance /p2 for next test.
        s.GetPrimAtPath('/p2').SetInstanceable(True)

        self.assertTrue(
            all([x.IsLoaded() for x in [
                s.GetPrimAtPath('/p1'),
                s.GetPrimAtPath('/p1/y'),
                s.GetPrimAtPath('/p2'),
                s.GetPrimAtPath('/p2/y')]]))

        # Now do the same but nested-instance everything.
        l3 = Sdf.Layer.CreateAnonymous('.usda')
        l3.comment = 'l3'

        outer = Sdf.CreatePrimInLayer(l3, '/outer')
        outer.specifier = Sdf.SpecifierDef
        outer.instanceable = True

        outerc = Sdf.CreatePrimInLayer(l3, '/outer/c')
        outerc.specifier = Sdf.SpecifierDef
        outerc.referenceList.Add(Sdf.Reference(l1.identifier, '/p1'))

        i1 = Sdf.CreatePrimInLayer(l3, '/i1')
        i1.specifier = Sdf.SpecifierDef
        i1.referenceList.Add(Sdf.Reference('', '/outer'))

        i2 = Sdf.CreatePrimInLayer(l3, '/i2')
        i2.specifier = Sdf.SpecifierDef
        i2.referenceList.Add(Sdf.Reference('', '/outer'))

        s2 = Usd.Stage.Open(l3, Usd.Stage.LoadAll)

        self.assertTrue(
            all([x.IsLoaded() for x in [
                s2.GetPrimAtPath('/i1'),
                s2.GetPrimAtPath('/i1/c'), 
                s2.GetPrimAtPath('/i1/c/y'),
                s2.GetPrimAtPath('/i2'),
                s2.GetPrimAtPath('/i2/c'), 
                s2.GetPrimAtPath('/i2/c/y')
            ]]))

        # Uninstance outer.
        s2.GetPrimAtPath('/i1').SetInstanceable(False)
        
        self.assertTrue(
            all([x.IsLoaded() for x in [
                s2.GetPrimAtPath('/i1'),
                s2.GetPrimAtPath('/i1/c'), 
                s2.GetPrimAtPath('/i1/c/y'),
                s2.GetPrimAtPath('/i2'),
                s2.GetPrimAtPath('/i2/c'), 
                s2.GetPrimAtPath('/i2/c/y')
            ]]))

        # Uninstance inner.
        s2.GetPrimAtPath('/i1/c').SetInstanceable(False)

        self.assertTrue(
            all([x.IsLoaded() for x in [
                s2.GetPrimAtPath('/i1'),
                s2.GetPrimAtPath('/i1/c'), 
                s2.GetPrimAtPath('/i1/c/y'),
                s2.GetPrimAtPath('/i2'),
                s2.GetPrimAtPath('/i2/c'), 
                s2.GetPrimAtPath('/i2/c/y')
            ]]))

    def test_156222(self):
        from pxr import Sdf, Usd

        # Test that removing all instances for a master prim and adding a new
        # instance with the same instancing key as the master causes the new
        # instance to be assigned to the master.
        l = Sdf.Layer.CreateAnonymous('.usda')
        Sdf.CreatePrimInLayer(l, '/Ref')

        # The bug is non-deterministic because of threaded composition,
        # but it's easier to reproduce with more instance prims.
        numInstancePrims = 50
        instancePrimPaths = [Sdf.Path('/Instance_{}'.format(i))
                             for i in xrange(numInstancePrims)]
        for path in instancePrimPaths:
            instancePrim = Sdf.CreatePrimInLayer(l, path)
            instancePrim.instanceable = True
            instancePrim.referenceList.Add(Sdf.Reference(primPath = '/Ref'))

        nonInstancePrim = Sdf.CreatePrimInLayer(l, '/NonInstance')
        nonInstancePrim.referenceList.Add(Sdf.Reference(primPath = '/Ref'))

        s = Usd.Stage.Open(l)
        self.assertEqual(len(s.GetMasters()), 1)

        # Check that the master prim is using one of the instanceable prim
        # index for its source.
        master = s.GetMasters()[0]
        masterPath = master.GetPath()
        self.assertIn(master._GetSourcePrimIndex().rootNode.path,
                      instancePrimPaths)

        # In a single change block, uninstance all of the instanceable prims,
        # but mark the non-instance prim as instanceable.
        with Sdf.ChangeBlock():
            for path in instancePrimPaths:
                l.GetPrimAtPath(path).instanceable = False
            nonInstancePrim.instanceable = True

        # This should not cause a new master prim to be generated; instead, 
        # the master prim should now be using the newly-instanced prim index 
        # as its source.
        master = s.GetMasters()[0]
        self.assertEqual(master.GetPath(), masterPath)
        self.assertEqual(master._GetSourcePrimIndex().rootNode.path,
                         nonInstancePrim.path)

    def test_157758(self):
        # Test that setting array values with various python sequences works.
        from pxr import Usd, Sdf, Vt
        s = Usd.Stage.CreateInMemory()
        p = s.DefinePrim('/testPrim')
        a = p.CreateAttribute('points', Sdf.ValueTypeNames.Float3Array)
        a.Set([(1,2,3), (2,3,4), (3,4,5)])
        self.assertEqual(a.Get(), Vt.Vec3fArray(3, [(1,2,3), (2,3,4), (3,4,5)]))
        a.Set(((3,2,1), (4,3,2), (5,4,3)))
        self.assertEqual(a.Get(), Vt.Vec3fArray(3, [(3,2,1), (4,3,2), (5,4,3)]))
        a.Set(zip(range(3), range(3), range(3)))
        self.assertEqual(a.Get(), Vt.Vec3fArray(3, [(0,0,0), (1,1,1), (2,2,2)]))


    def test_160884(self):
        # Test that opening a stage that has a mask pointing beneath an instance
        # doesn't crash.
        from pxr import Usd, Sdf
        import random
        allFormats = ['usd' + x for x in 'ac']
        for fmt in allFormats:
            l = Sdf.Layer.CreateAnonymous('_bug160884.'+fmt)
            l.ImportFromString('''#usda 1.0
                (
                    endTimeCode = 150
                    startTimeCode = 100
                    upAxis = "Y"
                )

                def Sphere "test"
                {
                    def Scope "scope1" {}
                    def Scope "scope2" {}
                    def Scope "scope3" {}
                    def Scope "scope4" {}
                    def Scope "scope5" {}
                    def Scope "scope6" {}
                    def Scope "scope7" {}
                    def Scope "scope8" {}
                    def Scope "scope9" {}
                    def Scope "scope10" {}
                    def Scope "scope11" {}
                    def Scope "scope12" {}
                    def Scope "scope13" {}
                    def Scope "scope14" {}
                    def Scope "scope15" {}
                    def Scope "scope16" {}
                    def Scope "scope17" {}
                    def Scope "scope18" {}
                    def Scope "scope19" {}
                    def Scope "scope20" {}
                }

                def Scope "Location"
                {

                  def "asset1" (
                      instanceable = True
                      add references = </test>
                  )
                  {
                  }

                  def "asset2" (
                      add references = </test>
                  )
                  {
                  }

                }

                def Scope "Loc1" (
                    instanceable = True
                    add references = </Location>
                )
                {

                }

                def Scope "Loc2" (
                    add references = </Location>
                )
                {

                }
                ''')

            for i in range(1024):
                stage = Usd.Stage.OpenMasked(
                    l, Usd.StagePopulationMask(['/Loc%s/asset1/scope%s' %
                                                (str(random.randint(1,20)),
                                                 str(random.randint(1,2)))]))
    def test_USD_4712(self):
        # Test that activating a prim auto-includes payloads of new descendants
        # if the ancestors' payloads were already included.
        from pxr import Usd, Sdf
        l1 = Sdf.Layer.CreateAnonymous('.usd')
        l1.ImportFromString('''#usda 1.0
            (
                defaultPrim = "shot"
            )

            def "shot" {
                def "camera" {
                    def "cache"(
                        active = false
                    ){
                        def "cam" {
                        }
                    }
                }
            }

            def "cam_extra" {
                def "cache_payload" {}
            }

            def "cam" {
                def "cam_payload" {}
            }''')

        l2 = Sdf.Layer.CreateAnonymous()
        Sdf.CreatePrimInLayer(l2, '/cam_extra').specifier = Sdf.SpecifierDef
        Sdf.CreatePrimInLayer(
            l2, '/cam_extra/cache_payload').specifier = Sdf.SpecifierDef
        Sdf.CreatePrimInLayer(l2, '/cam').specifier = Sdf.SpecifierDef
        Sdf.CreatePrimInLayer(
            l2, '/cam/cam_payload').specifier = Sdf.SpecifierDef

        l1.GetPrimAtPath('/shot/camera/cache').payload = Sdf.Payload(
            l2.identifier, '/cam_extra')
        l1.GetPrimAtPath('/shot/camera/cache/cam').payload = Sdf.Payload(
            l2.identifier, '/cam')
        
        stage = Usd.Stage.Open(l1)
        stage.SetEditTarget(stage.GetSessionLayer())
        cachePrim = stage.GetPrimAtPath('/shot/camera/cache')
    
        # Activating the cachePrim should auto-load the cam payload since its
        # nearest loadable ancestor is loaded.
        cachePrim.SetActive(True)
        cachePayloadPrim = stage.GetPrimAtPath(
            '/shot/camera/cache/cache_payload')
        self.assertTrue(cachePayloadPrim.IsValid())
        cameraPayloadPrim = stage.GetPrimAtPath(
            '/shot/camera/cache/cam/cam_payload')
        self.assertTrue(cameraPayloadPrim.IsValid())

if __name__ == '__main__':
    unittest.main()
