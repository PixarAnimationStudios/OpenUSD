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

import os, sys, tempfile, unittest
from pxr import Gf, Tf, Sdf, Usd

allFormats = ['usd' + x for x in 'ac']

class PayloadedScene(object):
    def __init__(self, fmt, unload=True, loadSet=Usd.Stage.LoadAll,
                 stageCreateFn=Usd.Stage.CreateInMemory):
        # Construct the following test case:
        #
        # stage.fmt         payload1.fmt
        #   /Sad  ---(P)---> /Sad
        #   |                /Sad/Panda
        #   |
        #   |                   
        #   /Foo                payload2.fmt
        #   /Foo/Baz ---(P)---> /Baz                   payload3.fmt
        #                       /Baz/Garply ---(P)---> /Garply
        #                                              /Garply/Qux
        
        ext = '.'+fmt

        # Create payload1.fmt
        self.__payload1 = stageCreateFn("payload1"+ext)
        p = self.__payload1.DefinePrim("/Sad/Panda", "Scope")

        # Create payload3.usda
        self.__payload3 = stageCreateFn("payload3"+ext)
        p = self.__payload3.DefinePrim("/Garply/Qux", "Scope")

        # Create payload2.usda
        self.__payload2 = stageCreateFn("payload2"+ext)
        p = self.__payload2.DefinePrim("/Baz/Garply", "Scope")
        p.GetPayloads().AddPayload(
            self.__payload3.GetRootLayer().identifier, "/Garply")

        #
        # Create the scene that references payload1 and payload2
        #
        self.stage = stageCreateFn("scene"+ext, loadSet)
        p = self.stage.DefinePrim("/Sad", "Scope")
        p.GetPayloads().AddPayload(
            self.__payload1.GetRootLayer().identifier, "/Sad")

        p = self.stage.DefinePrim("/Foo/Baz", "Scope")
        p.GetPayloads().AddPayload(
            self.__payload2.GetRootLayer().identifier, "/Baz")

        # By default, tests expect that the scene starts 
        # with everything unloaded, unless specified otherwise
        if unload:
            self.stage.Unload()

    def CleanupOnDiskAssets(self, fmt):
        import os
        
        del self.stage
        del self.__payload1
        del self.__payload2
        del self.__payload3

        ext = "." + fmt
        for i in [1,2,3]:
            fname = "payload" + str(i) + ext
            if os.path.exists(fname):
                os.unlink(fname)
        fname = "scene"+ ext
        if os.path.exists(fname):
            os.unlink(fname)

    def PrintPaths(self, msg=""):
        print("    Paths: "+msg)
        for p in self.stage.Traverse():
            print "    ", p
        print("")


class TestUsdLoadUnload(unittest.TestCase):
    def test_LoadRules(self):
        """Test the UsdStageLoadRules object."""
        ################################################################
        # Basics.
        r = Usd.StageLoadRules()
        self.assertEqual(r, Usd.StageLoadRules())
        self.assertEqual(r, Usd.StageLoadRules.LoadAll())
        r.AddRule('/', Usd.StageLoadRules.NoneRule)
        self.assertEqual(r, Usd.StageLoadRules.LoadNone())
        
        r.LoadWithDescendants('/')
        r.Minimize()
        self.assertEqual(r, Usd.StageLoadRules())

        r.Unload('/')
        self.assertEqual(r, Usd.StageLoadRules.LoadNone())

        r.LoadWithoutDescendants('/')
        self.assertEqual(r.GetRules(),
                         [(Sdf.Path('/'), Usd.StageLoadRules.OnlyRule)])

        r = Usd.StageLoadRules()
        r.AddRule('/', Usd.StageLoadRules.AllRule)
        self.assertTrue(r.IsLoaded('/'))
        self.assertTrue(r.IsLoaded('/foo/bar/baz'))
        
        r.AddRule('/', Usd.StageLoadRules.NoneRule)
        self.assertFalse(r.IsLoaded('/'))
        self.assertFalse(r.IsLoaded('/foo/bar/baz'))
        
        # None for '/', All for /Foo/Bar/Baz/Garply
        r.AddRule('/Foo/Bar/Baz/Garply', Usd.StageLoadRules.AllRule)
        self.assertTrue(r.IsLoaded('/'))
        self.assertTrue(r.IsLoaded('/Foo'))
        self.assertTrue(r.IsLoaded('/Foo/Bar'))
        self.assertTrue(r.IsLoaded('/Foo/Bar/Baz'))
        self.assertTrue(r.IsLoaded('/Foo/Bar/Baz/Garply'))
        self.assertTrue(r.IsLoaded('/Foo/Bar/Baz/Garply/Child'))
        self.assertFalse(r.IsLoaded('/Foo/Bear'))
        self.assertFalse(r.IsLoaded('/Foo/Bear/Baz'))
        self.assertFalse(r.IsLoaded('/Foo/Bear/Baz/Garply'))
        self.assertFalse(r.IsLoaded('/Foo/Bear/Baz/Garply/Child'))

        # This unload creates a redundant rule, but everything should function
        # as expected wrt IsLoaded queries.
        r.Unload('/Foo/Bar/Baz')
        self.assertEqual(
            r.GetRules(),
            [(Sdf.Path('/'), Usd.StageLoadRules.NoneRule),
             (Sdf.Path('/Foo/Bar/Baz'), Usd.StageLoadRules.NoneRule)])
        self.assertFalse(r.IsLoaded('/'))
        self.assertFalse(r.IsLoaded('/Foo'))
        self.assertFalse(r.IsLoaded('/Foo/Bar'))
        self.assertFalse(r.IsLoaded('/Foo/Bar/Baz'))
        self.assertFalse(r.IsLoaded('/Foo/Bar/Baz/Garply'))
        self.assertFalse(r.IsLoaded('/Foo/Bar/Baz/Garply/Child'))
        self.assertFalse(r.IsLoaded('/Foo/Bear'))
        self.assertFalse(r.IsLoaded('/Foo/Bear/Baz'))
        self.assertFalse(r.IsLoaded('/Foo/Bear/Baz/Garply'))
        self.assertFalse(r.IsLoaded('/Foo/Bear/Baz/Garply/Child'))
        # Minimizing removes the redundant rule, all queries behave the same.
        r.Minimize()
        self.assertEqual(r.GetRules(),
                         [(Sdf.Path('/'), Usd.StageLoadRules.NoneRule)])
        self.assertFalse(r.IsLoaded('/'))
        self.assertFalse(r.IsLoaded('/Foo'))
        self.assertFalse(r.IsLoaded('/Foo/Bar'))
        self.assertFalse(r.IsLoaded('/Foo/Bar/Baz'))
        self.assertFalse(r.IsLoaded('/Foo/Bar/Baz/Garply'))
        self.assertFalse(r.IsLoaded('/Foo/Bar/Baz/Garply/Child'))
        self.assertFalse(r.IsLoaded('/Foo/Bear'))
        self.assertFalse(r.IsLoaded('/Foo/Bear/Baz'))
        self.assertFalse(r.IsLoaded('/Foo/Bear/Baz/Garply'))
        self.assertFalse(r.IsLoaded('/Foo/Bear/Baz/Garply/Child'))

        ################################################################
        # LoadAndUnload
        r = Usd.StageLoadRules()
        r.LoadAndUnload(
            loadSet = ['/Load/All', '/Another/Load/All'],
            unloadSet = ['/Unload/All',
                         '/Another/Unload/All',
                         '/Load/All/UnloadIneffective'],
            policy = Usd.LoadWithDescendants)
        r.Minimize()
        self.assertEqual(
            r.GetRules(),
            [(Sdf.Path('/Another/Unload/All'), Usd.StageLoadRules.NoneRule),
             (Sdf.Path('/Unload/All'), Usd.StageLoadRules.NoneRule)])

        r = Usd.StageLoadRules()
        r.LoadAndUnload(
            loadSet = ['/Load/All', '/Another/Load/All'],
            unloadSet = ['/Unload/All',
                         '/Another/Unload/All',
                         '/Load/All/UnloadIneffective'],
            policy = Usd.LoadWithoutDescendants)
        r.Minimize()
        self.assertEqual(
            r.GetRules(),
            [(Sdf.Path('/Another/Load/All'), Usd.StageLoadRules.OnlyRule),
             (Sdf.Path('/Another/Unload/All'), Usd.StageLoadRules.NoneRule),
             (Sdf.Path('/Load/All'), Usd.StageLoadRules.OnlyRule),
             (Sdf.Path('/Unload/All'), Usd.StageLoadRules.NoneRule)])

        r2 = Usd.StageLoadRules()
        r2.SetRules(r.GetRules())
        self.assertEqual(r, r2)
        self.assertEqual(r.GetRules(), r2.GetRules())

        ################################################################
        # GetEffectiveRuleForPath
    
        r = Usd.StageLoadRules.LoadNone()
        self.assertEqual(r.GetEffectiveRuleForPath('/any/path'),
                         Usd.StageLoadRules.NoneRule)
        r.AddRule('/any', Usd.StageLoadRules.OnlyRule)
        # Root is now included as OnlyRule due to being in the ancestor chain.
        self.assertEqual(r.GetEffectiveRuleForPath('/'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/any'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/any/path'),
                         Usd.StageLoadRules.NoneRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/outside/path'),
                         Usd.StageLoadRules.NoneRule)

        self.assertTrue(r.IsLoadedWithNoDescendants('/any'))
        self.assertFalse(r.IsLoadedWithNoDescendants('/any/path'))
        self.assertFalse(r.IsLoadedWithAllDescendants('/any'))
        self.assertFalse(r.IsLoadedWithAllDescendants('/any/path'))

        # Root and /other are OnlyRule like above, /other/child and descendants
        # are AllRule.
        r.AddRule('/other/child', Usd.StageLoadRules.AllRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child'),
                         Usd.StageLoadRules.AllRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/descndt/path'),
                         Usd.StageLoadRules.AllRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/outside/path'),
                         Usd.StageLoadRules.NoneRule)

        self.assertTrue(r.IsLoadedWithNoDescendants('/any'))
        self.assertFalse(r.IsLoadedWithNoDescendants('/any/path'))
        self.assertTrue(r.IsLoadedWithAllDescendants('/other/child'))
        self.assertTrue(r.IsLoadedWithAllDescendants(
            '/other/child/descndt/path'))

        # Now add an Only and a None under /other/child.
        r.AddRule('/other/child/only', Usd.StageLoadRules.OnlyRule)
        r.AddRule('/other/child/none', Usd.StageLoadRules.NoneRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child'),
                         Usd.StageLoadRules.AllRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/descndt/path'),
                         Usd.StageLoadRules.AllRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/only'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/only/child'),
                         Usd.StageLoadRules.NoneRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/none'),
                         Usd.StageLoadRules.NoneRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/none/child'),
                         Usd.StageLoadRules.NoneRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/outside/path'),
                         Usd.StageLoadRules.NoneRule)

        # One more level, an All under a nested None.
        r.AddRule('/other/child/none/child/all', Usd.StageLoadRules.AllRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child'),
                         Usd.StageLoadRules.AllRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/descndt/path'),
                         Usd.StageLoadRules.AllRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/none'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/none/child'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(
            r.GetEffectiveRuleForPath('/other/child/none/child/all'),
            Usd.StageLoadRules.AllRule)

        # Minimize, queries should be the same.
        r.Minimize()
        self.assertEqual(r.GetEffectiveRuleForPath('/'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child'),
                         Usd.StageLoadRules.AllRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/descndt/path'),
                         Usd.StageLoadRules.AllRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/none'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(r.GetEffectiveRuleForPath('/other/child/none/child'),
                         Usd.StageLoadRules.OnlyRule)
        self.assertEqual(
            r.GetEffectiveRuleForPath('/other/child/none/child/all'),
            Usd.StageLoadRules.AllRule)

        self.assertEqual(
            r.GetRules(),
            [(Sdf.Path('/'), Usd.StageLoadRules.NoneRule),
             (Sdf.Path('/any'), Usd.StageLoadRules.OnlyRule),
             (Sdf.Path('/other/child'), Usd.StageLoadRules.AllRule),
             (Sdf.Path('/other/child/none'), Usd.StageLoadRules.NoneRule),
             (Sdf.Path('/other/child/none/child/all'),
              Usd.StageLoadRules.AllRule),
             (Sdf.Path('/other/child/only'), Usd.StageLoadRules.OnlyRule)])

        ################################################################
        # Swap.
        r1 = Usd.StageLoadRules.LoadNone()
        r2 = Usd.StageLoadRules.LoadAll()
        
        r1.swap(r2)
        self.assertEqual(r1, Usd.StageLoadRules.LoadAll())
        self.assertEqual(r2, Usd.StageLoadRules.LoadNone())

        r1.AddRule('/foo', Usd.StageLoadRules.NoneRule)
        r2.AddRule('/bar', Usd.StageLoadRules.AllRule)

        r1.swap(r2)
        self.assertEqual(
            r1.GetRules(),
            [(Sdf.Path('/'), Usd.StageLoadRules.NoneRule),
             (Sdf.Path('/bar'), Usd.StageLoadRules.AllRule)])
        self.assertEqual(
            r2.GetRules(),
            [(Sdf.Path('/foo'), Usd.StageLoadRules.NoneRule)])

        ################################################################
        # More minimize testing.
        r = Usd.StageLoadRules()
        r.Minimize()
        self.assertEqual(r, Usd.StageLoadRules())
        r.AddRule('/', Usd.StageLoadRules.AllRule)
        r.Minimize()
        self.assertEqual(r, Usd.StageLoadRules())
        
        r = Usd.StageLoadRules()
        r.AddRule('/Foo/Bar/Only', Usd.StageLoadRules.OnlyRule)
        r.AddRule('/Foo/Bar', Usd.StageLoadRules.AllRule)
        r.AddRule('/Foo', Usd.StageLoadRules.AllRule)
        r.AddRule('/World/anim', Usd.StageLoadRules.NoneRule)
        r.AddRule('/World/anim/chars/group', Usd.StageLoadRules.OnlyRule)
        r.AddRule('/World/anim/sim', Usd.StageLoadRules.AllRule)
        r.AddRule('/World/anim/sim/other/prim', Usd.StageLoadRules.AllRule)
        r.AddRule('/World/anim/sim/another/prim', Usd.StageLoadRules.OnlyRule)
        r.Minimize()
        self.assertEqual(
            r.GetRules(),
            [(Sdf.Path('/Foo/Bar/Only'), Usd.StageLoadRules.OnlyRule),
             (Sdf.Path('/World/anim'), Usd.StageLoadRules.NoneRule),
             (Sdf.Path('/World/anim/chars/group'), Usd.StageLoadRules.OnlyRule),
             (Sdf.Path('/World/anim/sim'), Usd.StageLoadRules.AllRule),
             (Sdf.Path('/World/anim/sim/another/prim'),
              Usd.StageLoadRules.OnlyRule)])

    def test_GetSetLoadRules(self):
        """Test calling GetLoadRules and SetLoadRules on a UsdStage"""
        r = Usd.StageLoadRules.LoadNone()
        r.AddRule('/any', Usd.StageLoadRules.OnlyRule)
        r.AddRule('/other/child', Usd.StageLoadRules.AllRule)
        r.AddRule('/other/child/only', Usd.StageLoadRules.OnlyRule)
        r.AddRule('/other/child/none', Usd.StageLoadRules.NoneRule)
        r.AddRule('/other/child/none/child/all', Usd.StageLoadRules.AllRule)

        s = Usd.Stage.CreateInMemory()
        s.DefinePrim('/any/child')
        s.DefinePrim('/other/child/prim')
        s.DefinePrim('/other/child/only/prim')
        s.DefinePrim('/other/child/only/loaded/prim')
        s.DefinePrim('/other/child/none/prim')
        s.DefinePrim('/other/child/none/unloaded/prim')
        s.DefinePrim('/other/child/none/child/all/one/prim')
        s.DefinePrim('/other/child/none/child/all/two/prim')

        payload = s.OverridePrim('/__payload')

        def addPayload(prim):
            prim.GetPayloads().AddInternalPayload(payload.GetPath())

        # Add payloads to all prims except leaf 'prim's and '__payload'.
        map(addPayload, [prim for prim in s.TraverseAll()
                         if prim.GetName() not in ('prim', '__payload')])

        # Create a new stage, with nothing loaded.
        testStage = Usd.Stage.Open(s.GetRootLayer(), load=Usd.Stage.LoadNone)

        self.assertEqual(list(testStage.Traverse()), [])
        
        # Now set the load rules and assert that they produce the expected set
        # of loaded prims.
        testStage.SetLoadRules(r)
        self.assertEqual(
            [prim.GetPath() for prim in testStage.Traverse()],
            ['/any',
             '/other',
             '/other/child',
             '/other/child/prim',
             '/other/child/only',
             '/other/child/only/prim',
             '/other/child/none',
             '/other/child/none/prim',
             '/other/child/none/child',
             '/other/child/none/child/all',
             '/other/child/none/child/all/one',
             '/other/child/none/child/all/one/prim',
             '/other/child/none/child/all/two',
             '/other/child/none/child/all/two/prim'])

        self.assertEqual(testStage.GetLoadRules(), r)
        

    def test_LoadAndUnload(self):
        """Test Stage::LoadUnload thoroughly, as all other requests funnel into it.
        """

        print sys._getframe().f_code.co_name

        for fmt in allFormats:
            p = PayloadedScene(fmt)

            #
            # Everything is unloaded
            #
            p.PrintPaths("All unloaded")
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert len(p.stage.GetLoadSet()) == 0
            assert len(p.stage.FindLoadable()) == 2
            assert Sdf.Path("/Sad") in p.stage.FindLoadable()
            assert Sdf.Path("/Foo/Baz") in p.stage.FindLoadable()
            assert Sdf.Path("/Foo/Baz/Garply") not in p.stage.FindLoadable()

            #
            # Load /Foo without descendants, which will pull in nothing new.
            # 
            p.stage.LoadAndUnload((Sdf.Path("/Foo"),), tuple(),
                                  policy=Usd.LoadWithoutDescendants)
            p.PrintPaths("/Foo loaded without descendants")
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert Sdf.Path("/Foo") not in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz") not in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz/Garply") not in p.stage.GetLoadSet()
            assert len(p.stage.FindLoadable()) == 2
            assert Sdf.Path("/Foo/Baz/Garply") not in p.stage.FindLoadable()

            #
            # Load /Foo/Baz without descendants, which will pull in /Foo/Baz but
            # not /Foo/Baz/Garply
            # 
            p.stage.LoadAndUnload((Sdf.Path("/Foo/Baz"),), tuple(),
                                  policy=Usd.LoadWithoutDescendants)
            p.PrintPaths("/Foo/Baz loaded without descendants")
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert Sdf.Path("/Foo") not in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz") in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz/Garply") not in p.stage.GetLoadSet()
            assert len(p.stage.FindLoadable()) == 3
            assert Sdf.Path("/Foo/Baz/Garply") in p.stage.FindLoadable()

            #
            # Load /Foo which will pull in /Foo/Baz and /Foo/Baz/Garply
            # 
            p.stage.LoadAndUnload((Sdf.Path("/Foo"),), tuple())
            p.PrintPaths("/Foo loaded")
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert Sdf.Path("/Foo") not in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz") in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz/Garply") in p.stage.GetLoadSet()
            assert len(p.stage.FindLoadable()) == 3
            assert Sdf.Path("/Foo/Baz/Garply") in p.stage.FindLoadable()

            #
            # Load /Foo/Baz without descendants, which should pull in just
            # /Foo/Baz.
            # 
            p.stage.LoadAndUnload((Sdf.Path("/Foo/Baz"),), tuple(),
                                  policy=Usd.LoadWithoutDescendants)
            p.PrintPaths("/Foo/Baz loaded w/o descendants")
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert Sdf.Path("/Foo") not in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz") in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz/Garply") not in p.stage.GetLoadSet()
            assert len(p.stage.FindLoadable()) == 3
            assert Sdf.Path("/Foo/Baz/Garply") in p.stage.FindLoadable()

            #
            # Load /Foo again, which will pull in /Foo/Baz and /Foo/Baz/Garply
            # 
            p.stage.LoadAndUnload((Sdf.Path("/Foo"),), tuple())
            p.PrintPaths("/Foo loaded")
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert Sdf.Path("/Foo") not in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz") in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz/Garply") in p.stage.GetLoadSet()
            assert len(p.stage.FindLoadable()) == 3
            assert Sdf.Path("/Foo/Baz/Garply") in p.stage.FindLoadable()

            #
            # Unload /Foo/Baz/Garply and load /Foo/Baz without descendants,
            # which should pull in just /Foo/Baz.
            # 
            p.stage.LoadAndUnload((Sdf.Path("/Foo/Baz"),),
                                  (Sdf.Path("/Foo/Baz/Garply"),),
                                  policy=Usd.LoadWithoutDescendants)
            p.PrintPaths("/Foo/Baz/Garply unloaded, "
                         "/Foo/Baz loaded w/o descendants")
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert Sdf.Path("/Foo") not in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz") in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz/Garply") not in p.stage.GetLoadSet()
            assert len(p.stage.FindLoadable()) == 3
            assert Sdf.Path("/Foo/Baz/Garply") in p.stage.FindLoadable()
            
            #
            # Unload /Foo, unloading everything
            #
            p.stage.LoadAndUnload(tuple(), (Sdf.Path("/Foo"),))
            p.PrintPaths("/Foo unloaded")
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert len(p.stage.GetLoadSet()) == 0
            # /Foo/Baz/Garply's payload is no longer visible, loadable count = 2
            assert len(p.stage.FindLoadable()) == 2

            #
            # Explicitly load /Foo/Baz, which will implicitly pull in
            # /Foo/Baz/Garply
            #
            p.stage.LoadAndUnload((Sdf.Path("/Foo/Baz"),), tuple())
            p.PrintPaths("/Foo/Baz loaded")
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert len(p.stage.GetLoadSet()) == 2

            #
            # Unload /Foo, which unloads everything recursively
            #
            p.stage.LoadAndUnload(tuple(), (Sdf.Path("/Foo"),))
            p.PrintPaths("/Foo unloaded")
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert len(p.stage.GetLoadSet()) == 0, str(p.stage.GetLoadSet())

            #
            # Load /Foo, but unload /Foo/Baz. Verify that the loading of /Foo
            # overrides the unloading of /Foo/Baz/Garply.
            #
            p.PrintPaths("/Foo loaded, /Foo/Baz/Garply unloaded")
            p.stage.LoadAndUnload((Sdf.Path("/Foo"),),
                                  (Sdf.Path("/Foo/Baz/Garply"),))
            p.PrintPaths()
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert len(p.stage.GetLoadSet()) == 2
            p.stage.Unload("/")
            assert len(p.stage.GetLoadSet()) == 0

            #
            # Load only /Foo/Baz/Garply, which will load /Foo, but not /Sad/Panda
            #
            p.PrintPaths("/Foo/Baz/Garply loaded")
            p.stage.LoadAndUnload((Sdf.Path("/Foo/Baz/Garply"),), tuple())
            p.PrintPaths()
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert p.stage.GetPrimAtPath("/Foo/Baz")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert len(p.stage.GetLoadSet()) == 2


    def test_Load(self):
        """Tests UsdStage::Load/Unload.
        """
        print sys._getframe().f_code.co_name
        for fmt in allFormats:
            p = PayloadedScene(fmt)
            p.PrintPaths()
            assert not p.stage.GetPrimAtPath("/Sad/Panda")

            p.stage.Load("/Foo")
            p.PrintPaths()
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert (set(p.stage.GetLoadSet()) ==
                    set([Sdf.Path("/Foo/Baz"), 
                         Sdf.Path("/Foo/Baz/Garply")]))

            p.stage.Load("/Sad")
            p.PrintPaths()
            assert p.stage.GetPrimAtPath("/Sad/Panda")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert (set(p.stage.GetLoadSet()) ==
                    set([Sdf.Path("/Sad"), 
                         Sdf.Path("/Foo/Baz"), 
                         Sdf.Path("/Foo/Baz/Garply")]))

            p.stage.Unload("/Sad")
            p.PrintPaths()
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            self.assertEqual(set(p.stage.GetLoadSet()),
                             set([Sdf.Path("/Foo/Baz"), 
                                  Sdf.Path("/Foo/Baz/Garply")]))

            p.stage.Unload("/")
            p.PrintPaths()
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            self.assertEqual(set(p.stage.GetLoadSet()), set([]))

            p.stage.Load("/")
            p.PrintPaths()
            assert p.stage.GetPrimAtPath("/Sad/Panda")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert (set(p.stage.GetLoadSet()) ==
                    set([Sdf.Path("/Sad"), 
                         Sdf.Path("/Foo/Baz"), 
                         Sdf.Path("/Foo/Baz/Garply")]))

            # If a loaded prim is deactivated, it will no longer have
            # any child prims (just like any other deactivated prim), 
            # but it is still considered part of the stage's load set
            # since its payload is still loaded.
            p.stage.GetPrimAtPath("/Sad").SetActive(False)
            p.PrintPaths()
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert (set(p.stage.GetLoadSet()) ==
                    set([Sdf.Path("/Sad"), 
                         Sdf.Path("/Foo/Baz"), 
                         Sdf.Path("/Foo/Baz/Garply")]))

            # If an ancestor of a loaded prim is deactivated, the 
            # loaded prim will no longer exist on the stage (since
            # inactive prims have no descendents), but will still
            # be considered part of the stage's load set since
            # its payload is still loaded.
            p.stage.GetPrimAtPath("/Foo").SetActive(False)
            p.PrintPaths()
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert (set(p.stage.GetLoadSet()) ==
                    set([Sdf.Path("/Sad"), 
                         Sdf.Path("/Foo/Baz"), 
                         Sdf.Path("/Foo/Baz/Garply")]))

    def test_Create(self):
        """Test the behavior of UsdStage::Create WRT load behavior"""
        print sys._getframe().f_code.co_name

        # Exercise creating an in memory stage
        for fmt in allFormats:
            # Try loading none
            p = PayloadedScene(fmt, unload=False, loadSet=Usd.Stage.LoadNone)
                
            p.PrintPaths()
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert len(p.stage.GetLoadSet()) == 0
            assert len(p.stage.FindLoadable()) == 2

            # Try loading all
            p = PayloadedScene(fmt, unload=False, loadSet=Usd.Stage.LoadAll)
            p.PrintPaths()
            assert p.stage.GetPrimAtPath("/Sad/Panda")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert len(p.stage.GetLoadSet()) == 3, str(p.stage.GetLoadSet())
            assert len(p.stage.FindLoadable()) == 3

        # Exercise creating an on-disk stage
        for fmt in allFormats:
            # Try loading none
            p = PayloadedScene(fmt, unload=False, loadSet=Usd.Stage.LoadNone,
                               stageCreateFn=Usd.Stage.CreateNew)
                
            p.PrintPaths()
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert len(p.stage.GetLoadSet()) == 0
            assert len(p.stage.FindLoadable()) == 2

            p.CleanupOnDiskAssets(fmt) 

            # Try loading all
            p = PayloadedScene(fmt, unload=False, loadSet=Usd.Stage.LoadAll,
                               stageCreateFn=Usd.Stage.CreateNew)
            p.PrintPaths()
            assert p.stage.GetPrimAtPath("/Sad/Panda")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert len(p.stage.GetLoadSet()) == 3, str(p.stage.GetLoadSet())
            assert len(p.stage.FindLoadable()) == 3

            p.CleanupOnDiskAssets(fmt)

    def test_Open(self):
        """Test the behavior of UsdStage::Open WRT load behavior.
        """
        print sys._getframe().f_code.co_name
        
        for fmt in allFormats:
            p = PayloadedScene(fmt)

            # reopen the stage with nothing loaded
            p.stage = Usd.Stage.Open(p.stage.GetRootLayer().identifier,
                                     load=Usd.Stage.LoadNone)
            p.PrintPaths()
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert len(p.stage.GetLoadSet()) == 0
            assert len(p.stage.FindLoadable()) == 2

            # reopen the stage with everything loaded
            p.stage = Usd.Stage.Open(p.stage.GetRootLayer().identifier,
                                     load=Usd.Stage.LoadAll)
            p.PrintPaths()
            assert p.stage.GetPrimAtPath("/Sad/Panda")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply")
            assert p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert len(p.stage.GetLoadSet()) == 3, str(p.stage.GetLoadSet())
            assert len(p.stage.FindLoadable()) == 3


    def test_Errors(self):
        """Tests error conditions.
        """

        # TODO: test loading / unloading of inactive prims once we have
        # correct active composition semantics.

        # TODO: assert that inactive paths do not show up in loaded or
        # loadable sets.

        print sys._getframe().f_code.co_name
        for fmt in allFormats:
            p = PayloadedScene(fmt)
            p.PrintPaths()

            with self.assertRaises(Tf.ErrorException):
                p.stage.Load("/Non/Existent/Prim")

            # It is explicitly okay to unload nonexistent prim paths.
            p.stage.Unload("/Non/Existent/Prim")

            set1 = (Sdf.Path("/Non/Existent/Prim"),)
            set2 = (Sdf.Path("/Other/Existent/Prim"),)
            with self.assertRaises(Tf.ErrorException):
                p.stage.LoadAndUnload(set1, set())
            with self.assertRaises(Tf.ErrorException):
                p.stage.LoadAndUnload(set1, set2)

            # In the past, when it was illegal to unload a deactivated prim we'd
            # get an error for this.  Now it is legal.
            p.stage.Load('/Foo/Baz')
            foo = p.stage.GetPrimAtPath('/Foo')
            baz = p.stage.GetPrimAtPath('/Foo/Baz')
            assert foo
            assert baz
            foo.SetActive(False)
            assert not baz
            p.stage.Unload('/Foo/Baz')
            with self.assertRaises(Tf.ErrorException):
                p.stage.Load('/Foo/Baz')
            foo.SetActive(True)
            baz = p.stage.GetPrimAtPath('/Foo/Baz')
            assert not baz.IsLoaded()
            p.PrintPaths()


    def test_RedundantLoads(self):
        """Ensure that calling load or unload redundantly is not an error
        """
        print sys._getframe().f_code.co_name
        for fmt in allFormats:
            p = PayloadedScene(fmt)
            p.PrintPaths()

            p.stage.Load("/Sad")
            p.PrintPaths()
            p.stage.Load("/Sad")
            p.PrintPaths()

            p.stage.Load("/Foo")
            p.PrintPaths()
            p.stage.Load("/Foo")
            p.PrintPaths()

            p.stage.Unload("/Foo")
            p.PrintPaths()
            p.stage.Unload("/Foo")
            p.PrintPaths()


    def test_StageCreateNew(self):
        """Tests the behavior of Usd.Stage.Create
        """
        for fmt in allFormats:
            layerName = "testLayer." + fmt
            if os.path.exists(layerName):
                os.unlink(layerName)

            s = Usd.Stage.CreateNew(layerName)
            assert s
            assert os.path.exists(layerName)
            lyr = s.GetRootLayer()
            s.OverridePrim("/Foo")
            lyr.Save()
            # Test the layer expiration behavior. Because we only have a weak ptr,
            # we expect it to expire when the stage goes away.
            assert lyr
            del s
            assert not lyr

            assert os.path.exists(layerName)
            # No error here, we expect to clobber the existing file.
            s1 = Usd.Stage.CreateNew(layerName)
            assert not s1.GetPrimAtPath("/Foo")
            # We expect this to error, because the layer is already in memory.
            with self.assertRaises(Tf.ErrorException):
                Usd.Stage.CreateNew(layerName)
            assert s1.GetRootLayer()
            del s1

            # Make sure session layers work
            session = Usd.Stage.CreateInMemory()
            session.OverridePrim("/Foo")
            s = Usd.Stage.CreateNew(layerName, session.GetRootLayer())
            assert s.GetPrimAtPath("/Foo")
            del s, session

            assert os.path.exists(layerName)
            os.unlink(layerName)

    def test_StageReload(self):
        """Tests Usd.Stage.Reload
        """
        # Reloading anonymous layers clears content.
        s = Usd.Stage.CreateInMemory()
        s.OverridePrim('/foo')
        s.Reload()
        assert not s.GetPrimAtPath('/foo')

        # Try with a real file -- saved changes preserved, unsaved changes get
        # discarded.
        def _TestStageReload(fmt):
            with tempfile.NamedTemporaryFile(suffix='.%s' % fmt) as f:
                f.close()

                s = Usd.Stage.CreateNew(f.name)
                s.OverridePrim('/foo')
                s.GetRootLayer().Save()
                s.OverridePrim('/bar')

                assert s.GetPrimAtPath('/foo')
                assert s.GetPrimAtPath('/bar')
                s.Reload()
                assert s.GetPrimAtPath('/foo')
                assert not s.GetPrimAtPath('/bar')

                # NOTE: f will want to delete the underlying file on
                #       __exit__ from the context manager.  But stage s
                #       may have the file open.  If so the deletion will
                #       fail on Windows.  Explicitly release our reference
                #       to the stage to close the file.
                del s

        for fmt in allFormats:
            # XXX: This verifies current behavior, however this is probably
            #      not the behavior we ultimately want -- see bug 102444.
            _TestStageReload(fmt)

    def test_StageLayerReload(self):
        """Test that Usd.Stage responds correctly when one of its 
        layer is directly reloaded."""
        import shutil

        def _TestLayerReload(fmt):
            # First, test case where the reloaded layer is in the
            # stage's root LayerStack.
            with tempfile.NamedTemporaryFile(suffix='.%s' % fmt) as l1name, \
                 tempfile.NamedTemporaryFile(suffix='.%s' % fmt) as l2name:
                l1name.close()
                l2name.close()

                l1 = Sdf.Layer.CreateAnonymous()
                Sdf.CreatePrimInLayer(l1, '/foo')
                l1.Export(l1name.name)

                l2 = Sdf.Layer.CreateAnonymous()
                Sdf.CreatePrimInLayer(l2, '/bar')
                l2.Export(l2name.name)

                s = Usd.Stage.Open(l1name.name)
                assert s.GetPrimAtPath('/foo')
                assert not s.GetPrimAtPath('/bar')

                shutil.copyfile(l2name.name, l1name.name)
                # Force layer reload to avoid issues where mtime does not
                # change after file copy due to low resolution.
                s.GetRootLayer().Reload(force = True)

                assert not s.GetPrimAtPath('/foo')
                assert s.GetPrimAtPath('/bar')
                
                # NOTE: l1name will want to delete the underlying file
                #       on __exit__ from the context manager.  But stage s
                #       may have the file open.  If so the deletion will
                #       fail on Windows.  Explicitly release our reference
                #       to the stage to close the file.
                del s

            # Now test the case where the reloaded layer is in a referenced
            # LayerStack.
            with tempfile.NamedTemporaryFile(suffix='.%s' % fmt) as rootLayerName, \
                 tempfile.NamedTemporaryFile(suffix='.%s' % fmt) as refLayerName:
                rootLayerName.close()
                refLayerName.close()

                refLayer = Sdf.Layer.CreateAnonymous()
                Sdf.CreatePrimInLayer(refLayer, '/foo/bar')
                refLayer.Export(refLayerName.name)

                rootLayer = Sdf.Layer.CreateAnonymous()
                rootPrim = Sdf.CreatePrimInLayer(rootLayer, '/foo')
                # The resolver wants references with forward slashes.
                rootPrim.referenceList.Add(
                    Sdf.Reference(refLayerName.name.replace('\\', '/'), '/foo'))
                rootLayer.Export(rootLayerName.name)

                s = Usd.Stage.Open(rootLayerName.name)
                assert s.GetPrimAtPath('/foo/bar')
                
                del refLayer.GetPrimAtPath('/foo').nameChildren['bar']
                refLayer.Export(refLayerName.name)
                Sdf.Layer.Find(refLayerName.name).Reload(force = True)
                assert s.GetPrimAtPath('/foo')
                assert not s.GetPrimAtPath('/foo/bar')

                # NOTE: rootLayerName will want to delete the underlying file
                #       on __exit__ from the context manager.  But stage s
                #       may have the file open.  If so the deletion will
                #       fail on Windows.  Explicitly release our reference
                #       to the stage to close the file.
                del s

        import platform
        for fmt in allFormats:
            # XXX: This verifies current behavior, however this is probably
            #      not the behavior we ultimately want -- see bug 102444.
            # Can't test Reload() for usdc on Windows because the system
            # won't allow us to modify the memory-mapped file.
            if not (platform.system() == 'Windows' and fmt == 'usdc'):
                _TestLayerReload(fmt)

if __name__ == "__main__":
    unittest.main()
