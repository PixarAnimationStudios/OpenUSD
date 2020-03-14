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
        x.payloadList.explicitItems.append(Sdf.Payload(l2.identifier, '/xpay'))

        y = Sdf.CreatePrimInLayer(l1, '/x/y')
        y.specifier = Sdf.SpecifierDef
        x.payloadList.explicitItems.append(Sdf.Payload(l2.identifier, '/ypay'))

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

        l1.GetPrimAtPath('/shot/camera/cache').payloadList.Prepend(
            Sdf.Payload(l2.identifier, '/cam_extra'))
        l1.GetPrimAtPath('/shot/camera/cache/cam').payloadList.Prepend(
            Sdf.Payload(l2.identifier, '/cam'))
        
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

    def test_USD_4936(self):
        # Test that relationships resolve correctly with nested instancing and
        # instance proxies within masters.
        from pxr import Usd, Sdf
        l1 = Sdf.Layer.CreateAnonymous('.usd')
        l1.ImportFromString('''#usda 1.0
            def "W" {
                def "A" (
                    instanceable = true
                    prepend references = </M>
                )
                {
                }
            }

            def "M" {
                def "B" (
                    instanceable = true
                    prepend references = </M2>
                )
                {
                }
            }

            def "M2" {
                rel r = </M2/D>
                def "D" {
                }
            }''')
        stage = Usd.Stage.Open(l1)
        wab = stage.GetPrimAtPath('/W/A/B')
        # prior to fixing this bug, the resulting target would be '/W/A/B/B/D'.
        self.assertEqual(wab.GetRelationship('r').GetTargets(), [Sdf.Path('/W/A/B/D')])

    def test_USD_5196(self):
        from pxr import Usd, Sdf, Vt, Tf
        import os, random
        # Test that usdc files corrupted by truncation (such that the table of
        # contents is past the end of the file) are detected and fail to open
        # with an error.
        with Tf.NamedTemporaryFile(suffix=".usdc") as f:
            layer = Sdf.Layer.CreateNew(f.name)
            foo = Sdf.CreatePrimInLayer(layer, '/foo')
            attr = Sdf.AttributeSpec(foo, 'attr', Sdf.ValueTypeNames.IntArray)
            ints = range(1024**2)
            random.shuffle(ints)
            attr.default = Vt.IntArray(ints)
            layer.Save()
            del layer
            # Now truncate layer to corrupt it.
            fobj = open(f.name, "rw+")
            size = os.path.getsize(f.name)
            fobj.truncate(size / 2)
            fobj.close()
            # Attempting to open the file should raise an exception.
            with self.assertRaises(Tf.ErrorException):
                layer = Sdf.Layer.FindOrOpen(f.name)

    def test_USD_5045(self):
        # USD-5045 is github issue #753
        from pxr import Usd
        nullPrim = Usd.Prim()
        with self.assertRaises(RuntimeError):
            nullPrim.IsDefined()

    def test_PIPE_6232(self):
        # This interaction between nested instancing, load/unload, activation,
        # and inherits triggered a corruption of instancing data structures and
        # ultimately a crash bug in the USD core.
        from pxr import Usd, Sdf
        lpay = Sdf.Layer.CreateAnonymous('.usda')
        lpay.ImportFromString('''#usda 1.0
def "innerM" (
    instanceable = true
    inherits = </_someClass>
)
{
}
''')
        l = Sdf.Layer.CreateAnonymous('.usda')
        l.ImportFromString('''#usda 1.0
def "outerM" ( instanceable = true )
{
    def "inner" ( payload = @%s@</innerM> )
    {
    }
}
def "World"
{
    def "i" ( prepend references = </outerM> )
    {
    }
}
def "OtherWorld"
{
    def "i" ( prepend references = </outerM> )
    {
    }
}'''%lpay.identifier)
        s = Usd.Stage.Open(l, load=Usd.Stage.LoadNone)
        # === Load /World/i and /OtherWorld/i ===
        s.Load('/World/i')
        s.Load('/OtherWorld/i')
        # === Deactivate /World ==='
        s.GetPrimAtPath('/World').SetActive(False)
        # === Create class /_someClass ==='
        s.CreateClassPrim('/_someClass')
        p = s.GetPrimAtPath('/OtherWorld/i/inner')
        self.assertTrue(p.IsInstance())
        self.assertTrue(p.GetMaster())

    def test_USD_5386(self):
        from pxr import Usd, Sdf
        # This is github issue #883.
        def MakeLayer(text, *args):
            l = Sdf.Layer.CreateAnonymous('.usda')
            l.ImportFromString(text % args)
            return l

        c = MakeLayer('''#usda 1.0
def Xform "geo"
{
    def Sphere "sphere1"
    {
    }
}
''')
        a = MakeLayer('''#usda 1.0
(
    defaultPrim = "geo"
    subLayers = [
        @%s@
    ]
)

def Xform "geo"
{
    def Cube "cube2"
    {
    }
}''', c.identifier)
        b = MakeLayer('''#usda 1.0
(
    defaultPrim = "geo"
    subLayers = [
        @%s@
    ]
)

def Xform "geo"
{
    def Cube "cube2"
    {
    }
}''', c.identifier)

        d = MakeLayer('''#usda 1.0
def "geo" ( append payload = @%s@ )
{
}''', a.identifier)
        e = MakeLayer('''#usda 1.0
def "geo" ( append payload = @%s@ )
{
}''', b.identifier)

        s = Usd.Stage.CreateInMemory()
        r = s.GetRootLayer()
        r.subLayerPaths.append(d.identifier)
        s2 = Usd.Stage.CreateInMemory()
        r2 = s2.GetRootLayer()
        r2.subLayerPaths.append(d.identifier)
        s.MuteAndUnmuteLayers([c.identifier], [])
        s2.MuteAndUnmuteLayers([c.identifier], [])
        r.subLayerPaths.clear()
        r.subLayerPaths.append(e.identifier)
        s.MuteAndUnmuteLayers([], [c.identifier])

    def test_USD_5709(self):
        # Population masks with paths descendant to instances were not working
        # correctly, since we incorrectly applied the mask to the _master_ prim
        # paths when populating master prim hierarchies.  This test ensures that
        # masks with paths descendant to instances work as expected.
        from pxr import Usd, Sdf
        l = Sdf.Layer.CreateAnonymous('.usda')
        l.ImportFromString('''#usda 1.0
        def Sphere "test"
        {
            def Scope "scope1" {}
            def Scope "scope2" {}
            def Scope "scope3" {}
        }

        def Scope "Model" (
            instanceable = True
            payload = </test>
        )
        {
        }

        def "M1" ( append references = </Model> )
        {
        }
        def "M2" ( append references = </Model> )
        {
        }
        def "M3" ( append references = </Model> )
        {
        }

        def Scope "Nested" (instanceable = True)
        {
            def "M1" ( append references = </Model> ) {}
            def "M2" ( append references = </Model> ) {}
            def "M3" ( append references = </Model> ) {}
        }

        def "N1" (append references = </Nested>)
        {
        }
        def "N2" (append references = </Nested>)
        {
        }
        def "N3" (append references = </Nested>)
        {
        }
        ''')
        ########################################################################
        # Non-nested instancing
        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/M1/scope2']))
        self.assertFalse(s.GetPrimAtPath('/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/M1/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/M1/scope2', '/M3']))
        self.assertFalse(s.GetPrimAtPath('/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/M1/scope3'))
        self.assertTrue(s.GetPrimAtPath('/M3/scope1'))
        self.assertTrue(s.GetPrimAtPath('/M3/scope2'))
        self.assertTrue(s.GetPrimAtPath('/M3/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/M1/scope2', '/M3/scope2']))
        self.assertFalse(s.GetPrimAtPath('/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/M1/scope3'))
        self.assertFalse(s.GetPrimAtPath('/M3/scope1'))
        self.assertTrue(s.GetPrimAtPath('/M3/scope2'))
        self.assertFalse(s.GetPrimAtPath('/M3/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/M1/scope2']),
            load=Usd.Stage.LoadNone)
        self.assertFalse(s.GetPrimAtPath('/M1/scope2'))
        s.Load('/M1')
        self.assertFalse(s.GetPrimAtPath('/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/M1/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/M1/scope2', '/M3']),
            load=Usd.Stage.LoadNone)
        self.assertFalse(s.GetPrimAtPath('/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/M3/scope1'))
        s.Load('/M1')
        s.Load('/M3')
        self.assertFalse(s.GetPrimAtPath('/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/M1/scope3'))
        self.assertTrue(s.GetPrimAtPath('/M3/scope1'))
        self.assertTrue(s.GetPrimAtPath('/M3/scope2'))
        self.assertTrue(s.GetPrimAtPath('/M3/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/M1/scope2', '/M3/scope2']),
            load=Usd.Stage.LoadNone)
        self.assertFalse(s.GetPrimAtPath('/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/M3/scope2'))
        s.Load('/M1')
        s.Load('/M3')
        self.assertFalse(s.GetPrimAtPath('/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/M1/scope3'))
        self.assertFalse(s.GetPrimAtPath('/M3/scope1'))
        self.assertTrue(s.GetPrimAtPath('/M3/scope2'))
        self.assertFalse(s.GetPrimAtPath('/M3/scope3'))

        ########################################################################
        # Nested instancing
        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2']))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2', '/N3/M3']))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))
        self.assertTrue(s.GetPrimAtPath('/N3/M3/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N3/M3/scope2'))
        self.assertTrue(s.GetPrimAtPath('/N3/M3/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2', '/N3/M3/scope2']))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))
        self.assertFalse(s.GetPrimAtPath('/N3/M3/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N3/M3/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N3/M3/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2']),
            load=Usd.Stage.LoadNone)
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope2'))
        s.Load('/N1/M1')
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2', '/N3/M3']),
            load=Usd.Stage.LoadNone)
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N3/M3/scope1'))
        s.Load('/N1/M1')
        s.Load('/N3/M3')
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))
        self.assertTrue(s.GetPrimAtPath('/N3/M3/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N3/M3/scope2'))
        self.assertTrue(s.GetPrimAtPath('/N3/M3/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2', '/N3/M3/scope2']),
            load=Usd.Stage.LoadNone)
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N3/M3/scope2'))
        s.Load('/N1/M1')
        s.Load('/N3/M3')
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))
        self.assertFalse(s.GetPrimAtPath('/N3/M3/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N3/M3/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N3/M3/scope3'))

        ########################################################################
        # Nested instancing again
        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2']))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2', '/N3/M1']))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))
        self.assertTrue(s.GetPrimAtPath('/N3/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N3/M1/scope2'))
        self.assertTrue(s.GetPrimAtPath('/N3/M1/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2', '/N3/M1/scope2']))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))
        self.assertFalse(s.GetPrimAtPath('/N3/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N3/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N3/M1/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2']),
            load=Usd.Stage.LoadNone)
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope2'))
        s.Load('/N1/M1')
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2', '/N3/M1']),
            load=Usd.Stage.LoadNone)
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N3/M1/scope1'))
        s.Load('/N1/M1')
        s.Load('/N3/M1')
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))
        self.assertTrue(s.GetPrimAtPath('/N3/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N3/M1/scope2'))
        self.assertTrue(s.GetPrimAtPath('/N3/M1/scope3'))

        s = Usd.Stage.OpenMasked(
            l, mask=Usd.StagePopulationMask(['/N1/M1/scope2', '/N3/M1/scope2']),
            load=Usd.Stage.LoadNone)
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N3/M1/scope2'))
        s.Load('/N1/M1')
        s.Load('/N3/M1')
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N1/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N1/M1/scope3'))
        self.assertFalse(s.GetPrimAtPath('/N3/M1/scope1'))
        self.assertTrue(s.GetPrimAtPath('/N3/M1/scope2'))
        self.assertFalse(s.GetPrimAtPath('/N3/M1/scope3'))
        
if __name__ == '__main__':
    unittest.main()
