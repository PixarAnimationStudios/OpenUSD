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

import sys, unittest
from pxr import Sdf,Usd,Tf

allFormats = ['usd' + x for x in 'ac']

class TestUsdPrim(unittest.TestCase):
    def test_Basic(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('Basics.'+fmt)
            p = s.GetPrimAtPath('/')
            q = s.GetPrimAtPath('/')
            assert p is not q
            assert p == q
            assert hash(p) == hash(q)

            # Check that unicode objects convert to sdfpaths.
            p = s.GetPrimAtPath(u'/')
            q = s.GetPrimAtPath(u'/')
            assert p is not q
            assert p == q
            assert hash(p) == hash(q)

            p = s.OverridePrim('/foo')
            p.CreateAttribute('attr', Sdf.ValueTypeNames.String)
            a = p.GetAttribute('attr')
            b = p.GetAttribute('attr')
            assert a and b
            assert a is not b
            assert a == b
            assert hash(a) == hash(b)

            p.CreateRelationship('relationship')
            a = p.GetRelationship('relationship')
            b = p.GetRelationship('relationship')
            assert a and b
            assert a is not b
            assert a == b
            assert hash(a) == hash(b)

    def test_OverrideMetadata(self):
        for fmt in allFormats:
            weak = Sdf.Layer.CreateAnonymous('OverrideMetadataTest.'+fmt)
            strong = Sdf.Layer.CreateAnonymous('OverrideMetadataTest.'+fmt)
            stage = Usd.Stage.Open(weak.identifier)
            assert stage.DefinePrim("/Mesh/Child", "Mesh")

            stage = Usd.Stage.Open(strong.identifier)

            p = stage.OverridePrim("/Mesh")
            p.GetReferences().AddReference(Sdf.Reference(weak.identifier, "/Mesh"))
            p = stage.GetPrimAtPath("/Mesh/Child")
            assert p
            assert p.SetMetadata(
                "hidden", False), "Failed to set metadata in stronger layer" 
            assert p.GetName() == p.GetPath().name

    def test_GetPrimStack(self):
        layers = [Sdf.Layer.CreateAnonymous('base.usda'),
                  Sdf.Layer.CreateAnonymous('sublayer.usda'),
                  Sdf.Layer.CreateAnonymous('payload.usda'),
                  Sdf.Layer.CreateAnonymous('basic_over.usda')]

        base = layers[0]
        sublayer = layers[1]
        payload = layers[2]
        basicOver = layers[3]

        primPath = '/root'

        stage = Usd.Stage.Open(sublayer)
        over = stage.OverridePrim(primPath)

        stage = Usd.Stage.Open(basicOver)
        over = stage.OverridePrim(primPath)

        stage = Usd.Stage.Open(payload)
        over = stage.OverridePrim(primPath)
        over.GetReferences().AddReference(Sdf.Reference(basicOver.identifier, primPath))

        stage = Usd.Stage.Open(base)
        prim = stage.DefinePrim(primPath)
        prim.SetPayload(payload, primPath)
        stage.GetRootLayer().subLayerPaths.append(sublayer.identifier) 

        expectedPrimStack = [layer.GetPrimAtPath(primPath) for layer in layers]
        stage = Usd.Stage.Open(base)
        prim = stage.GetPrimAtPath(primPath)

        assert prim.GetPrimStack() == expectedPrimStack

    def test_GetCachedPrimBits(self):
        layerFile = 'test.usda'
        layer = Sdf.Layer.FindOrOpen(layerFile)
        assert layer, 'failed to find "%s"' % layerFile

        stage = Usd.Stage.Open(layer, load=Usd.Stage.LoadNone)
        assert stage, 'failed to create stage for %s' % layerFile

        # Check various bits.
        root = stage.GetPrimAtPath('/')
        globalClass = stage.GetPrimAtPath('/GlobalClass')
        abstractSubscope = stage.GetPrimAtPath('/GlobalClass/AbstractSubscope')
        abstractOver = stage.GetPrimAtPath('/GlobalClass/AbstractOver')
        pureOver = stage.GetPrimAtPath('/PureOver')
        undefSubscope = stage.GetPrimAtPath('/PureOver/UndefinedSubscope')
        group = stage.GetPrimAtPath('/Group')
        modelChild = stage.GetPrimAtPath('/Group/ModelChild')
        localChild = stage.GetPrimAtPath('/Group/LocalChild')
        undefModelChild = stage.GetPrimAtPath('/Group/UndefinedModelChild')
        deactivatedScope = stage.GetPrimAtPath('/Group/DeactivatedScope')
        deactivatedModel = stage.GetPrimAtPath('/Group/DeactivatedModel')
        deactivatedOver = stage.GetPrimAtPath('/Group/DeactivatedOver')
        propertyOrder = stage.GetPrimAtPath('/PropertyOrder')

        # Named child access API
        assert group.GetChild('ModelChild') == modelChild
        assert group.GetChild('LocalChild') == localChild
        assert not group.GetChild('__NoSuchChild__')

        # Check filtered children access.
        self.assertEqual(list(root.GetAllChildren()), [
            globalClass, pureOver, group, propertyOrder])
        self.assertEqual(list(root.GetChildren()), [propertyOrder])

        # Manually construct the "normal" view using the default predicate.
        self.assertEqual(list(root.GetFilteredChildren(
            Usd.PrimDefaultPredicate)), [propertyOrder])

        # Manually construct the "normal" view using the individual terms
        # from the default predicate.
        self.assertEqual(list(root.GetFilteredChildren(
            Usd.PrimIsActive & Usd.PrimIsLoaded &
            Usd.PrimIsDefined & ~Usd.PrimIsAbstract)), [propertyOrder])

        # Only abstract prims.
        self.assertEqual(list(root.GetFilteredChildren(Usd.PrimIsAbstract)),
                    [globalClass])

        # Abstract & defined prims -- still just the class.
        self.assertEqual(list(root.GetFilteredChildren(
            Usd.PrimIsAbstract & Usd.PrimIsDefined)), [globalClass])

        # Abstract | unloaded prims -- the class and the group.
        self.assertEqual(list(root.GetFilteredChildren(
            Usd.PrimIsAbstract | ~Usd.PrimIsLoaded)), [globalClass, group])

        # Models only.
        self.assertEqual(list(root.GetFilteredChildren(Usd.PrimIsModel)), [group])

        # Non-models only.
        self.assertEqual(list(root.GetFilteredChildren(~Usd.PrimIsModel)),
                    [globalClass, pureOver, propertyOrder])

        # Models or undefined.
        self.assertEqual(list(root.GetFilteredChildren(
            Usd.PrimIsModel | ~Usd.PrimIsDefined)), [pureOver, group])

        # Check individual flags.
        assert root.IsActive()
        assert root.IsLoaded()
        assert root.IsModel()
        assert root.IsGroup()
        assert not root.IsAbstract()
        assert root.IsDefined()

        assert globalClass.IsActive()
        assert globalClass.IsLoaded()
        assert not globalClass.IsModel()
        assert not globalClass.IsGroup()
        assert globalClass.IsAbstract()
        assert globalClass.IsDefined()

        assert abstractSubscope.IsActive()
        assert abstractSubscope.IsLoaded()
        assert not abstractSubscope.IsModel()
        assert not abstractSubscope.IsGroup()
        assert abstractSubscope.IsAbstract()
        assert abstractSubscope.IsDefined()

        assert abstractOver.IsActive()
        assert abstractOver.IsLoaded()
        assert not abstractOver.IsModel()
        assert not abstractOver.IsGroup()
        assert abstractOver.IsAbstract()
        assert not abstractOver.IsDefined()

        assert pureOver.IsActive()
        assert pureOver.IsLoaded()
        assert not pureOver.IsModel()
        assert not pureOver.IsGroup()
        assert not pureOver.IsAbstract()
        assert not pureOver.IsDefined()

        assert undefSubscope.IsActive()
        assert undefSubscope.IsLoaded()
        assert not undefSubscope.IsModel()
        assert not undefSubscope.IsGroup()
        assert not undefSubscope.IsAbstract()
        assert not undefSubscope.IsDefined()

        assert group.IsActive()
        assert not group.IsLoaded()
        assert group.IsModel()
        assert group.IsGroup()
        assert not group.IsAbstract()
        assert group.IsDefined()

        assert modelChild.IsActive()
        assert not modelChild.IsLoaded()
        assert modelChild.IsModel()
        assert not modelChild.IsGroup()
        assert not modelChild.IsAbstract()
        assert modelChild.IsDefined()

        assert localChild.IsActive()
        assert not localChild.IsLoaded()
        assert not localChild.IsModel()
        assert not localChild.IsGroup()
        assert not localChild.IsAbstract()
        assert localChild.IsDefined()

        assert undefModelChild.IsActive()
        assert not undefModelChild.IsLoaded()
        assert not undefModelChild.IsModel()
        assert not undefModelChild.IsGroup()
        assert not undefModelChild.IsAbstract()
        assert not undefModelChild.IsDefined()

        assert not deactivatedScope.IsActive()
        assert not deactivatedScope.IsLoaded()
        assert not deactivatedScope.IsModel()
        assert not deactivatedScope.IsGroup()
        assert not deactivatedScope.IsAbstract()
        assert deactivatedScope.IsDefined()
        assert not stage.GetPrimAtPath(
            deactivatedScope.GetPath().AppendChild('child'))

        # activate it.
        deactivatedScope.SetActive(True)
        assert deactivatedScope.IsActive()
        assert deactivatedScope.HasAuthoredActive()
        assert not deactivatedScope.IsLoaded()
        assert not deactivatedScope.IsModel()
        assert not deactivatedScope.IsGroup()
        assert not deactivatedScope.IsAbstract()
        assert deactivatedScope.IsDefined()
        assert stage.GetPrimAtPath(
            deactivatedScope.GetPath().AppendChild('child'))

        # clear active.
        deactivatedScope.ClearActive()
        assert deactivatedScope.IsActive()
        assert not deactivatedScope.HasAuthoredActive()
        assert not deactivatedScope.IsLoaded()
        assert not deactivatedScope.IsModel()
        assert not deactivatedScope.IsGroup()
        assert not deactivatedScope.IsAbstract()
        assert deactivatedScope.IsDefined()
        assert stage.GetPrimAtPath(
            deactivatedScope.GetPath().AppendChild('child'))

        # deactivate it again.
        deactivatedScope.SetActive(False)
        assert not deactivatedScope.IsActive()
        assert deactivatedScope.HasAuthoredActive()
        assert not deactivatedScope.IsLoaded()
        assert not deactivatedScope.IsModel()
        assert not deactivatedScope.IsGroup()
        assert not deactivatedScope.IsAbstract()
        assert deactivatedScope.IsDefined()
        assert not stage.GetPrimAtPath(
            deactivatedScope.GetPath().AppendChild('child'))

        assert not deactivatedModel.IsActive()
        assert not deactivatedModel.IsLoaded()
        assert deactivatedModel.IsModel()
        assert not deactivatedModel.IsGroup()
        assert not deactivatedModel.IsAbstract()
        assert deactivatedModel.IsDefined()
        assert not stage.GetPrimAtPath(
            deactivatedModel.GetPath().AppendChild('child'))

        assert not deactivatedOver.IsActive()
        assert not deactivatedOver.IsLoaded()
        assert not deactivatedOver.IsModel()
        assert not deactivatedOver.IsGroup()
        assert not deactivatedOver.IsAbstract()
        assert not deactivatedOver.IsDefined()
        assert not stage.GetPrimAtPath(
            deactivatedOver.GetPath().AppendChild('child'))

        # Load the model and recheck.
        stage.Load('/Group')

        assert group.IsActive()
        assert group.IsLoaded()
        assert group.IsModel()
        assert group.IsGroup()
        assert not group.IsAbstract()
        assert group.IsDefined()

        # child should be loaded now.
        assert localChild.IsActive()
        assert localChild.IsLoaded()
        assert not localChild.IsModel()
        assert not localChild.IsGroup()
        assert not localChild.IsAbstract()
        assert localChild.IsDefined()

        # undef child should be loaded and defined, due to payload inclusion.
        assert undefModelChild.IsActive()
        assert undefModelChild.IsLoaded()
        assert not undefModelChild.IsModel()
        assert not undefModelChild.IsGroup()
        assert not undefModelChild.IsAbstract()
        assert undefModelChild.IsDefined()

        # check prim defined entirely inside payload.
        payloadChild = stage.GetPrimAtPath('/Group/PayloadChild')
        assert payloadChild
        assert payloadChild.IsActive()
        assert payloadChild.IsLoaded()
        assert not payloadChild.IsModel()
        assert not payloadChild.IsGroup()
        assert not payloadChild.IsAbstract()
        assert payloadChild.IsDefined()

        # check deactivated scope again.
        assert not deactivatedScope.IsActive()
        assert not deactivatedScope.IsLoaded()
        assert not deactivatedScope.IsModel()
        assert not deactivatedScope.IsGroup()
        assert not deactivatedScope.IsAbstract()
        assert deactivatedScope.IsDefined()
        assert not stage.GetPrimAtPath(
            deactivatedScope.GetPath().AppendChild('child'))

        # activate it.
        deactivatedScope.SetActive(True)
        assert deactivatedScope.IsActive()
        assert deactivatedScope.IsLoaded()
        assert not deactivatedScope.IsModel()
        assert not deactivatedScope.IsGroup()
        assert not deactivatedScope.IsAbstract()
        assert deactivatedScope.IsDefined()
        assert stage.GetPrimAtPath(
            deactivatedScope.GetPath().AppendChild('child'))

        # deactivate it again.
        deactivatedScope.SetActive(False)
        assert not deactivatedScope.IsActive()
        assert not deactivatedScope.IsLoaded()
        assert not deactivatedScope.IsModel()
        assert not deactivatedScope.IsGroup()
        assert not deactivatedScope.IsAbstract()
        assert deactivatedScope.IsDefined()
        assert not stage.GetPrimAtPath(
            deactivatedScope.GetPath().AppendChild('child'))


    def test_ChangeTypeName(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('ChangeTypeName.'+fmt)
            foo = s.OverridePrim("/Foo")

            # Initialize
            self.assertEqual(foo.GetTypeName(), "")
            self.assertEqual(foo.GetMetadata("typeName"), None)
            assert not foo.HasAuthoredTypeName()

            # Set via public API 
            assert foo.SetTypeName("Mesh")
            assert foo.HasAuthoredTypeName()
            self.assertEqual(foo.GetTypeName(), "Mesh")
            self.assertEqual(foo.GetMetadata("typeName"), "Mesh")
            self.assertEqual(foo.GetTypeName(), "Mesh")

            foo.ClearTypeName()
            self.assertEqual(foo.GetTypeName(), "")
            self.assertEqual(foo.GetMetadata("typeName"), None)
            assert not foo.HasAuthoredTypeName()

            # Set via metadata
            assert foo.SetMetadata("typeName", "Scope")
            assert foo.HasAuthoredTypeName()
            self.assertEqual(foo.GetTypeName(), "Scope")
            self.assertEqual(foo.GetMetadata("typeName"), "Scope")

    def test_HasAuthoredReferences(self):
        for fmt in allFormats:
            s1 = Usd.Stage.CreateInMemory('HasAuthoredReferences.'+fmt)
            s1.DefinePrim("/Foo", "Mesh")
            s1.DefinePrim("/Bar", "Mesh")
            baz = s1.DefinePrim("/Foo/Baz", "Mesh")
            assert baz.GetReferences().AddReference(s1.GetRootLayer().identifier, "/Bar")

            s2 = Usd.Stage.CreateInMemory('HasAuthoredReferences.'+fmt)
            foo = s2.OverridePrim("/Foo")
            baz = s2.GetPrimAtPath("/Foo/Baz")

            assert not baz
            assert not foo.HasAuthoredReferences()
            assert foo.GetReferences().AddReference(s1.GetRootLayer().identifier, "/Foo")
            items = foo.GetMetadata("references").ApplyOperations([])
            assert foo.HasAuthoredReferences()

            # Make sure references are detected across composition arcs.
            baz = s2.OverridePrim("/Foo/Baz")
            assert baz.HasAuthoredReferences()

            # Clear references out.
            assert foo.GetReferences().ClearReferences()
            assert not foo.HasAuthoredReferences()
            # Child should be gone.
            baz = s2.GetPrimAtPath("/Foo/Baz")
            assert not baz

            # Set the references explicitly.
            assert foo.GetReferences().SetReferences(items)
            assert foo.HasAuthoredReferences()
            # Child is back.
            baz = s2.GetPrimAtPath("/Foo/Baz")
            assert baz.HasAuthoredReferences()

    def test_GoodAndBadReferences(self):
        for fmt in allFormats:
            # Sub-root references are allowed
            s1 = Usd.Stage.CreateInMemory('References.'+fmt)
            s1.DefinePrim("/Foo", "Mesh")
            s1.DefinePrim("/Bar/Bazzle", "Mesh")
            baz = s1.DefinePrim("/Foo/Baz", "Mesh")
            bazRefs = baz.GetReferences()
            bazRefs.AddReference(s1.GetRootLayer().identifier, "/Bar/Bazzle")

            # Test that both in-memory identifiers, relative paths, and absolute
            # paths all resolve properly
            s2 = Usd.Stage.CreateNew("refTest1."+fmt)
            sphere = s2.DefinePrim("/Sphere", "Sphere")
            sphereRefs = sphere.GetReferences()
            s3 = Usd.Stage.CreateNew("refTest2."+fmt)
            box = s3.DefinePrim("/Box", "Cube")
            assert s2.ResolveIdentifierToEditTarget(
                s1.GetRootLayer().identifier) != ""
            assert s2.ResolveIdentifierToEditTarget("./refTest2."+fmt) != ""
            assert s2.ResolveIdentifierToEditTarget(
                s2.GetRootLayer().realPath) != ""
            # but paths to non-existent files fail
            assert s2.ResolveIdentifierToEditTarget("./noFile."+fmt) == ""
            # and paths relative to in-memory layers fail (expected errors?)
            print "bazRefs = " + s1.ResolveIdentifierToEditTarget("./refTest2."+fmt)
            assert s1.ResolveIdentifierToEditTarget("./refTest2."+fmt) == "" 

            # A good reference generates no errors or exceptions
            assert bazRefs.AddReference(s2.GetRootLayer().identifier, "/Sphere")

            # A bad reference succeeds, but generates warnings from compose errors.
            assert not sphere.HasAuthoredReferences()
            assert sphereRefs.AddReference("./refTest2."+fmt, "/noSuchPrim")
            assert sphere.HasAuthoredReferences()

    def test_PropertyOrder(self):
        layerFile = 'test.usda'
        layer = Sdf.Layer.FindOrOpen(layerFile)
        assert layer, 'failed to find "%s"' % layerFile

        stage = Usd.Stage.Open(layer, load=Usd.Stage.LoadNone)
        assert stage, 'failed to create stage for %s' % layerFile

        po = stage.GetPrimAtPath('/PropertyOrder')
        assert po
        attrs = po.GetAttributes()
        # expected order:
        expected = ['A0', 'a1', 'a2', 'A3', 'a4', 'a5', 'a10', 'A20']
        assert [a.GetName() for a in attrs] == expected, \
            '%s != %s' % ([a.GetName() for a in attrs], expected)

        rels = po.GetRelationships()
        # expected order:
        expected = ['R0', 'r1', 'r2', 'R3', 'r4', 'r5', 'r10', 'R20']
        assert [r.GetName() for r in rels] == expected, \
            '%s != %s' % ([r.GetName() for r in rels], expected)
        
        
    def test_PropertyReorder(self):
        def l(chars):
            return list(x for x in chars)

        for fmt in allFormats:
            sl = Sdf.Layer.CreateAnonymous(fmt)
            s = Usd.Stage.CreateInMemory('PropertyReorder.'+fmt, sl)
            f = s.OverridePrim('/foo')

            s.SetEditTarget(s.GetRootLayer())
            for name in reversed(l('abcd')):
                f.CreateAttribute(name, Sdf.ValueTypeNames.Int)

            s.SetEditTarget(s.GetSessionLayer())
            for name in reversed(l('defg')):
                f.CreateAttribute(name, Sdf.ValueTypeNames.Int)

            self.assertEqual(f.GetPropertyNames(), l('abcdefg'))

            pred = lambda tok : tok in ['a', 'd', 'f']
            self.assertEqual(f.GetPropertyNames(predicate=pred),
                             l('adf'))

            f.SetPropertyOrder(l('edc'))
            self.assertEqual(f.GetPropertyNames(), l('edcabfg'))

            f.SetPropertyOrder(l('a'))
            self.assertEqual(f.GetPropertyNames(), l('abcdefg'))

            f.SetPropertyOrder([])
            self.assertEqual(f.GetPropertyNames(), l('abcdefg'))

            f.SetPropertyOrder(l('g'))
            self.assertEqual(f.GetPropertyNames(), l('gabcdef'))

            f.SetPropertyOrder(l('d'))
            self.assertEqual(f.GetPropertyNames(), l('dabcefg'))

            self.assertEqual(f.GetPropertyNames(predicate=pred),
                             l('daf'))

            f.SetPropertyOrder(l('xyz'))
            self.assertEqual(f.GetPropertyNames(), l('abcdefg'))

            f.SetPropertyOrder(l('xcydze'))
            self.assertEqual(f.GetPropertyNames(), l('cdeabfg'))

            f.SetPropertyOrder(l('gfedcba'))
            self.assertEqual(f.GetPropertyNames(), l('gfedcba'))

    def test_DefaultPrim(self):
        for fmt in allFormats:
            # No default prim to start.
            s = Usd.Stage.CreateInMemory('DefaultPrim.'+fmt)
            assert not s.GetDefaultPrim()

            # Set defaultPrim metadata on root layer, but no prim in scene
            # description.
            s.GetRootLayer().defaultPrim = 'foo'
            assert not s.GetDefaultPrim()

            # Create the prim, should pick it up.
            fooPrim = s.OverridePrim('/foo')
            assert s.GetDefaultPrim() == fooPrim

            # Change defaultPrim, ensure it picks up again.
            s.GetRootLayer().defaultPrim = 'bar'
            assert not s.GetDefaultPrim()
            barPrim = s.OverridePrim('/bar')
            assert s.GetDefaultPrim() == barPrim

            # Try error cases.
            s.GetRootLayer().defaultPrim = 'foo/bar'
            assert not s.GetDefaultPrim()
            s.OverridePrim('/foo/bar')
            assert not s.GetDefaultPrim()
            s.defaultPrim = ''
            assert not s.GetDefaultPrim()

            # Try stage-level authoring API.
            s.SetDefaultPrim(fooPrim)
            assert s.GetDefaultPrim() == fooPrim
            assert s.HasDefaultPrim()
            s.ClearDefaultPrim()
            assert not s.GetDefaultPrim()
            assert not s.HasDefaultPrim()

    def test_GetNextSibling(self):
        import random, time
        seed = int(time.time())
        print 'GetNextSibling() random seed:', seed
        random.seed(seed)

        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('GetNextSibling.'+fmt)

            # Create a stage with some prims, some defined, others not, at random.
            names = tuple('abcd')
            def make(stage, names, depth, prefix=None):
                prefix = prefix if prefix else Sdf.Path.absoluteRootPath
                for name in names:
                    if depth:
                        make(stage, names, depth-1, prefix.AppendChild(name))
                    else:
                        if random.random() <= 0.25:
                            s.DefinePrim(prefix)
                        else:
                            s.OverridePrim(prefix)

            # Now walk every prim on the stage, and ensure that obtaining children
            # by GetChildren() and walking siblings via GetNextSibling() returns the
            # same results.
            def test(root):
                def checkKids(p):
                    direct = p.GetChildren()
                    bySib = []
                    if len(direct):
                        bySib.append(direct[0])
                        while bySib[-1].GetNextSibling():
                            bySib.append(bySib[-1].GetNextSibling())
                    self.assertEqual(direct, bySib)
                checkKids(root)
                for child in root.GetChildren():
                    test(child)

            make(s, names, 4)
            test(s.GetPseudoRoot())

    def test_PrimOrder(self):
        # Create a stage with three root prims.
        orderBefore = ['Foo', 'Bar', 'Baz']
        orderAfter = ['Baz', 'Foo', 'Bar']

        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('PrimOrder.'+fmt)
            children = [s.DefinePrim('/' + p) for p in orderBefore]
            self.assertEqual(s.GetPseudoRoot().GetChildren(), children)

            # Author reorder, assert they are reordered.
            s.GetPseudoRoot().SetMetadata('primOrder', orderAfter)
            self.assertEqual(s.GetPseudoRoot().GetChildren(),
                        [s.GetPrimAtPath('/' + p) for p in orderAfter])

            # Try the same thing with non-root prims.
            s = Usd.Stage.CreateInMemory('PrimOrder.'+fmt)
            children = [s.DefinePrim('/Root/' + p) for p in orderBefore]
            self.assertEqual(s.GetPrimAtPath('/Root').GetChildren(), children)

            # Author reorder, assert they are reordered.
            s.GetPrimAtPath('/Root').SetMetadata('primOrder', orderAfter)
            self.assertEqual(s.GetPrimAtPath('/Root').GetChildren(),
                        [s.GetPrimAtPath('/Root/' + p) for p in orderAfter])

    def test_Instanceable(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('Instanceable.'+fmt)
            p = s.DefinePrim('/Instanceable', 'Mesh')
            assert not p.IsInstanceable()
            assert p.GetMetadata('instanceable') == None
            assert not p.HasAuthoredInstanceable()

            p.SetInstanceable(True)
            assert p.IsInstanceable()
            assert p.GetMetadata('instanceable') == True
            assert p.HasAuthoredInstanceable()

            p.SetInstanceable(False)
            assert not p.IsInstanceable()
            assert p.GetMetadata('instanceable') == False
            assert p.HasAuthoredInstanceable()

            p.ClearInstanceable()
            assert not p.IsInstanceable()
            assert p.GetMetadata('instanceable') == None
            assert not p.HasAuthoredInstanceable()

    def test_GetComposedPrimChildrenAsMetadataTest(self):
        stage = Usd.Stage.Open('MilkCartonA.usda')
        self.assertTrue(stage)

        prim = stage.GetPrimAtPath('/MilkCartonA')
        self.assertTrue(prim)

        self.assertEqual(prim.GetAllMetadata()['typeName'], "Xform")

    def test_GetPrimIndex(self):
        def _CreateTestStage(fmt):
            s = Usd.Stage.CreateInMemory('GetPrimIndex.'+fmt)

            c = s.DefinePrim('/Class')

            r = s.DefinePrim('/Ref')
            s.DefinePrim('/Ref/Child')

            p = s.DefinePrim('/Instance')
            p.GetInherits().AddInherit('/Class')
            p.GetReferences().AddInternalReference('/Ref')
            p.SetInstanceable(True)
            return s

        def _ValidatePrimIndexes(prim):
            # Assert that the prim indexes for the prim at this path
            # are valid. Also dump them to a string just to force
            # all nodes in the prim index to be touched.
            self.assertTrue(prim.GetPrimIndex().IsValid())
            self.assertTrue(prim.GetPrimIndex().DumpToString())
            self.assertTrue(prim.ComputeExpandedPrimIndex().IsValid())
            self.assertTrue(prim.ComputeExpandedPrimIndex().DumpToString())

        def _ValidateNoPrimIndexes(prim):
            self.assertFalse(prim.GetPrimIndex().IsValid())
            self.assertFalse(prim.GetPrimIndex().DumpToString())

        for fmt in allFormats:
            s = _CreateTestStage(fmt)

            _ValidatePrimIndexes(s.GetPseudoRoot())
            _ValidatePrimIndexes(s.GetPrimAtPath('/Ref'))
            _ValidatePrimIndexes(s.GetPrimAtPath('/Ref/Child'))

            # Master prims do not expose a valid prim index.
            master = s.GetMasters()[0]
            _ValidateNoPrimIndexes(master)

            # However, prims beneath masters do expose a valid prim index.
            # Note this prim index may change from run to run depending on
            # which is selected as the source for the master.
            _ValidatePrimIndexes(master.GetChild('Child'))
            
    def test_PseudoRoot(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('PseudoRoot.%s' % fmt)
            w = s.DefinePrim('/World')
            p = s.GetPrimAtPath('/')
            self.assertTrue(p.IsPseudoRoot())
            self.assertFalse(Usd.Prim().IsPseudoRoot())
            self.assertFalse(w.IsPseudoRoot())
            self.assertTrue(w.GetParent().IsPseudoRoot())
            self.assertFalse(p.GetParent().IsPseudoRoot())

    def test_Deactivation(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('Deactivation.%s' % fmt)
            child = s.DefinePrim('/Root/Group/Child')

            group = s.GetPrimAtPath('/Root/Group')
            self.assertEqual(group.GetAllChildren(), [child])
            self.assertTrue(s._GetPcpCache().FindPrimIndex('/Root/Group/Child'))

            group.SetActive(False)

            # Deactivating a prim removes all of its children from the stage.
            # Note that the deactivated prim itself still exists on the stage;
            # this allows users to reactivate it.
            self.assertEqual(group.GetAllChildren(), [])

            # Deactivating a prim should also cause the underlying prim 
            # indexes for its children to be removed.
            self.assertFalse(
                s._GetPcpCache().FindPrimIndex('/Root/Group/Child'))

    def test_AppliedSchemas(self):
        for fmt in allFormats:
            sessionLayer = Sdf.Layer.CreateNew("SessionLayer.%s" % fmt)
            s = Usd.Stage.CreateInMemory('AppliedSchemas.%s' % fmt, sessionLayer)

            s.SetEditTarget(Usd.EditTarget(s.GetRootLayer()))

            root = s.OverridePrim('/hello')
            self.assertEqual([], root.GetAppliedSchemas())

            rootModelAPI = Usd.ModelAPI.Apply(root)
            self.assertTrue(rootModelAPI)

            root = rootModelAPI.GetPrim()
            self.assertTrue(root)

            self.assertTrue(root.HasAPI(Usd.ModelAPI))

            self.assertEqual(['ModelAPI'], root.GetAppliedSchemas())

            # Switch the edit target to the session layer and test bug 156929
            s.SetEditTarget(Usd.EditTarget(s.GetSessionLayer()))
            sessionClipsAPI = Usd.ClipsAPI.Apply(root)
            self.assertTrue(sessionClipsAPI)
            self.assertEqual(['ClipsAPI', 'ModelAPI'], root.GetAppliedSchemas())

            self.assertTrue(root.HasAPI(Usd.ClipsAPI))

            # Ensure duplicates aren't picked up
            anotherSessionClipsAPI = Usd.ClipsAPI.Apply(root)
            self.assertTrue(anotherSessionClipsAPI)
            self.assertEqual(['ClipsAPI', 'ModelAPI'], root.GetAppliedSchemas())

            # Add a duplicate in the root layer and ensure that there are no 
            # duplicates in the composed result.
            s.SetEditTarget(Usd.EditTarget(s.GetRootLayer()))
            rootClipsAPI = Usd.ClipsAPI.Apply(root)
            self.assertTrue(rootClipsAPI)
            self.assertEqual(['ClipsAPI', 'ModelAPI'], 
                             root.GetAppliedSchemas())


if __name__ == "__main__":
    unittest.main()
