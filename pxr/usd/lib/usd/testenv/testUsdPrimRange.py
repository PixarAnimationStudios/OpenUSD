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
from pxr import Sdf, Usd

allFormats = ['usd' + x for x in 'ac']

class TestUsdPrimRange(unittest.TestCase):
    def test_PrimIsDefined(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestPrimIsDefined.'+fmt)
            pseudoRoot = s.GetPrimAtPath("/")
            foo = s.DefinePrim("/Foo", "Mesh")
            bar = s.OverridePrim("/Bar")
            faz = s.DefinePrim("/Foo/Faz", "Mesh")
            baz = s.DefinePrim("/Bar/Baz", "Mesh")

            # Change specifier so that /Bar is an over prim with no type.
            s.GetRootLayer().GetPrimAtPath("/Bar").specifier = Sdf.SpecifierOver

            # Default tree iterator should not hit undefined prims.
            x = list(Usd.PrimRange(pseudoRoot))
            self.assertEqual(x, [pseudoRoot, foo, faz])

            # Create a tree iterator and ensure that /Bar and its descendants aren't
            # traversed by it.
            x = list(Usd.PrimRange(pseudoRoot, Usd.PrimIsDefined))
            self.assertEqual(x, [pseudoRoot, foo, faz])

            # When we ask for undefined prim rooted at /Bar, verify that bar and baz
            # are returned.
            x = list(Usd.PrimRange(bar, ~Usd.PrimIsDefined))
            self.assertEqual(x, [bar, baz])

    def test_PrimHasDefiningSpecifier(self):
        for fmt in allFormats:
            stageFile = 'testHasDefiningSpecifier.' + fmt
            stage = Usd.Stage.Open(stageFile)

            # In the case of nested overs and defs, the onus is
            # on the client to manually prune their results either by
            # restarting iteration upon hitting an over, or by iterating
            # through all prims.
            root = stage.GetPrimAtPath('/a1')
            actual = []
            expected = [stage.GetPrimAtPath(x) for x in ['/a1', '/a1/a2']]
            for prim in Usd.PrimRange.AllPrims(root):
                if prim.HasDefiningSpecifier():
                    actual.append(prim)
            self.assertEqual(actual, expected)

            root = stage.GetPrimAtPath('/b1')
            actual = []
            expected = [stage.GetPrimAtPath(x) for x in 
                        ['/b1/b2', '/b1/b2/b3/b4/b5/b6']]
            for prim in Usd.PrimRange(root, Usd.PrimIsActive):
                if prim.HasDefiningSpecifier():
                    actual.append(prim)
            self.assertEqual(actual, expected)

            # Note that the over is not included in our traversal.
            root = stage.GetPrimAtPath('/c1')
            actual = list(Usd.PrimRange(root, Usd.PrimHasDefiningSpecifier))
            expected = [stage.GetPrimAtPath(x) for x in 
                        ['/c1', '/c1/c2', '/c1/c2/c3']]
            self.assertEqual(actual, expected)

    def test_PrimIsActive(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestPrimIsActive.'+fmt)
            foo = s.DefinePrim("/Foo", "Mesh")
            bar = s.DefinePrim("/Foo/Bar", "Mesh")
            baz = s.DefinePrim("/Foo/Bar/Baz", "Mesh")
            baz.SetActive(False)

            # Create a tree iterator and ensure that /Bar and its descendants aren't
            # traversed by it.
            x1 = list(Usd.PrimRange(foo, Usd.PrimIsActive))
            self.assertEqual(x1, [foo, bar])

            # Test to make sure the predicate is checked on the iteration root.
            x2 = list(Usd.PrimRange(baz, Usd.PrimIsActive))
            self.assertEqual(x2, [])

            x3 = list(Usd.PrimRange(baz, ~Usd.PrimIsActive))
            self.assertEqual(x3, [baz])

            x4 = list(Usd.PrimRange(bar, Usd.PrimIsActive))
            self.assertEqual(x4, [bar])
       
    def test_PrimIsModelOrGroup(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestPrimIsModelOrGroup.'+fmt)
            group = s.DefinePrim("/Group", "Xform")
            Usd.ModelAPI(group).SetKind('group')
            model = s.DefinePrim("/Group/Model", "Model")
            Usd.ModelAPI(model).SetKind('model')
            mesh = s.DefinePrim("/Group/Model/Sbdv", "Mesh")

            x1 = list(Usd.PrimRange(group, Usd.PrimIsModel))
            self.assertEqual(x1, [group, model])

            x2 = list(Usd.PrimRange(group, Usd.PrimIsGroup))
            self.assertEqual(x2, [group])

            x3 = list(Usd.PrimRange(group, ~Usd.PrimIsModel))
            self.assertEqual(x3, [])

            x4 = list(Usd.PrimRange(mesh, ~Usd.PrimIsModel))
            self.assertEqual(x4, [mesh])

            x5 = list(Usd.PrimRange(group, 
                Usd.PrimIsModel | Usd.PrimIsGroup))
            self.assertEqual(x5, [group, model])

            x6 = list(Usd.PrimRange(group, 
                Usd.PrimIsModel & Usd.PrimIsGroup))
            self.assertEqual(x6, [group])

    def test_PrimIsAbstract(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestPrimIsAbstract.'+fmt)
            group = s.DefinePrim("/Group", "Xform")
            c = s.CreateClassPrim("/class_Model")

            x1 = list(Usd.PrimRange(group, Usd.PrimIsAbstract))
            self.assertEqual(x1, [])

            x2 = list(Usd.PrimRange(group, ~Usd.PrimIsAbstract))
            self.assertEqual(x2, [group])

            x3 = list(Usd.PrimRange(c, Usd.PrimIsAbstract))
            self.assertEqual(x3, [c])

            x4 = list(Usd.PrimRange(c, ~Usd.PrimIsAbstract))
            self.assertEqual(x4, [])

    def test_PrimIsLoaded(self):
        for fmt in allFormats:
            payloadStage = Usd.Stage.CreateInMemory("payload."+fmt)
            p = payloadStage.DefinePrim("/Payload", "Scope")

            stage = Usd.Stage.CreateInMemory("scene."+fmt)
            foo = stage.DefinePrim("/Foo")
            foo.SetPayload(payloadStage.GetRootLayer(), "/Payload")

            self.assertEqual(stage.GetLoadSet(), ["/Foo"])

            stage.Unload("/Foo")

            x1 = list(Usd.PrimRange(foo, ~Usd.PrimIsLoaded))
            self.assertEqual(x1, [foo])

            x2 = list(Usd.PrimRange(foo, Usd.PrimIsLoaded))
            self.assertEqual(x2, [])

            stage.Load("/Foo")

            x3 = list(Usd.PrimRange(foo, ~Usd.PrimIsLoaded))
            self.assertEqual(x3, [])

            x4 = list(Usd.PrimRange(foo, Usd.PrimIsLoaded))
            self.assertEqual(x4, [foo])

    def test_PrimIsInstanceOrMasterOrRoot(self):
        for fmt in allFormats:
            refStage = Usd.Stage.CreateInMemory("reference."+fmt)
            refStage.DefinePrim("/Ref/Child")

            stage = Usd.Stage.CreateInMemory("scene."+fmt)
            root = stage.DefinePrim("/Root")

            i = stage.DefinePrim("/Root/Instance")
            i.GetReferences().AppendReference(refStage.GetRootLayer().identifier, "/Ref")
            i.SetMetadata("instanceable", True)

            n = stage.DefinePrim("/Root/NonInstance")
            n.GetReferences().AppendReference(refStage.GetRootLayer().identifier, "/Ref")
            nChild = stage.GetPrimAtPath('/Root/NonInstance/Child')

            # Test Usd.PrimIsInstance
            self.assertEqual(list(Usd.PrimRange(i, Usd.PrimIsInstance)), 
                        [i])
            self.assertEqual(list(Usd.PrimRange(i, ~Usd.PrimIsInstance)), [])

            self.assertEqual(list(Usd.PrimRange(n, Usd.PrimIsInstance)), [])
            self.assertEqual(list(Usd.PrimRange(n, ~Usd.PrimIsInstance)), 
                        [n, nChild])

            self.assertEqual(list(Usd.PrimRange(root, Usd.PrimIsInstance)), 
                        [])
            self.assertEqual(list(Usd.PrimRange(root, ~Usd.PrimIsInstance)), 
                        [root, n, nChild])

    def test_RoundTrip(self):
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestRoundTrip.'+fmt)
            prims = map(stage.DefinePrim, ['/foo', '/bar', '/baz'])

            treeRange = Usd.PrimRange(stage.GetPseudoRoot())
            tripped = Usd._TestPrimRangeRoundTrip(treeRange)
            self.assertEqual(treeRange, tripped)
            self.assertEqual(list(treeRange), list(tripped))

            treeRange = Usd.PrimRange.PreAndPostVisit(stage.GetPseudoRoot())
            tripped = Usd._TestPrimRangeRoundTrip(treeRange)
            self.assertEqual(treeRange, tripped)
            self.assertEqual(list(treeRange), list(tripped))

    def test_StageTraverse(self):
        for fmt in allFormats:
            s = Usd.Stage.CreateInMemory('TestStageTraverse.'+fmt)
            pseudoRoot = s.GetPrimAtPath("/")
            foo = s.DefinePrim("/Foo", "Mesh")
            bar = s.OverridePrim("/Bar")
            faz = s.DefinePrim("/Foo/Faz", "Mesh")
            baz = s.DefinePrim("/Bar/Baz", "Mesh")

            # Change speicifer so that /Bar is an over with no type.
            s.GetRootLayer().GetPrimAtPath("/Bar").specifier = Sdf.SpecifierOver

            # Default traverse should not hit undefined prims.
            x = list(s.Traverse())
            self.assertEqual(x, [foo, faz])
            x = list(Usd.PrimRange.Stage(s))
            self.assertEqual(x, [foo, faz])

            # Traverse all.
            x = list(s.TraverseAll())
            self.assertEqual(x, [foo, faz, bar, baz])
            x = list(Usd.PrimRange.Stage(
                s, predicate=Usd._PrimFlagsPredicate.Tautology()))
            self.assertEqual(x, [foo, faz, bar, baz])

            # Traverse undefined prims.
            x = list(s.Traverse(~Usd.PrimIsDefined))
            self.assertEqual(x, [bar, baz])
            x = list(Usd.PrimRange.Stage(s, predicate=~Usd.PrimIsDefined))
            self.assertEqual(x, [bar, baz])

    def test_WithInstancing(self):
        for fmt in allFormats:
            refStage = Usd.Stage.CreateInMemory("reference."+fmt)
            refStage.DefinePrim("/Ref/Child")

            stage = Usd.Stage.CreateInMemory("scene."+fmt)
            root = stage.DefinePrim("/Root")

            i = stage.DefinePrim("/Root/Instance")
            i.GetReferences().AppendReference(refStage.GetRootLayer().identifier, "/Ref")
            i.SetMetadata("instanceable", True)

            n = stage.DefinePrim("/Root/NonInstance")
            n.GetReferences().AppendReference(refStage.GetRootLayer().identifier, "/Ref")
            nChild = stage.GetPrimAtPath('/Root/NonInstance/Child')

            master = stage.GetMasters()[0]
            masterChild = master.GetChild('Child')

            # A default traversal of a stage with instances should not descend into 
            # instance masters
            self.assertEqual(list(Usd.PrimRange.Stage(stage)), [root, i, n, nChild])
            self.assertEqual(list(Usd.PrimRange(stage.GetPseudoRoot())),
                        [stage.GetPseudoRoot(), root, i, n, nChild])

            # But the tree iterator should allow traversal of the masters if
            # explicitly specified.
            self.assertEqual(list(Usd.PrimRange(master)), [master, masterChild])

if __name__ == "__main__":
    unittest.main()
