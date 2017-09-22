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
    def __init__(self, fmt):
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
        self.__payload1 = Usd.Stage.CreateInMemory("payload1"+ext)
        p = self.__payload1.DefinePrim("/Sad/Panda", "Scope")

        # Create payload3.usda
        self.__payload3 = Usd.Stage.CreateInMemory("payload3"+ext)
        p = self.__payload3.DefinePrim("/Garply/Qux", "Scope")

        # Create payload2.usda
        self.__payload2 = Usd.Stage.CreateInMemory("payload2"+ext)
        p = self.__payload2.DefinePrim("/Baz/Garply", "Scope")
        p.SetPayload(self.__payload3.GetRootLayer(), "/Garply")

        #
        # Create the scene that references payload1 and payload2
        #
        self.stage = Usd.Stage.CreateInMemory("scene"+ext)
        p = self.stage.DefinePrim("/Sad", "Scope")
        p.SetPayload(self.__payload1.GetRootLayer(), "/Sad")

        p = self.stage.DefinePrim("/Foo/Baz", "Scope")
        p.SetPayload(self.__payload2.GetRootLayer(), "/Baz")

        # Test expects that the scene starts with everything unloaded.
        self.stage.Unload()


    def PrintPaths(self, msg=""):
        print("    Paths: "+msg)
        for p in self.stage.Traverse():
            print "    ", p
        print("")


class TestUsdLoadUnload(unittest.TestCase):
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
            assert not Sdf.Path("/Foo/Baz/Garply") in p.stage.FindLoadable()

            #
            # Load /Foo which will pull in /Foo/Baz and /Foo/Baz/Garply
            # 
            p.stage.LoadAndUnload((Sdf.Path("/Foo"),) ,tuple())
            p.PrintPaths("/Foo loaded")
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert Sdf.Path("/Foo") not in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz") in p.stage.GetLoadSet()
            assert Sdf.Path("/Foo/Baz/Garply") in p.stage.GetLoadSet()
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
            p.stage.LoadAndUnload((Sdf.Path("/Foo/Baz"),) ,tuple())
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
            assert (set(p.stage.GetLoadSet()) ==
                    set([Sdf.Path("/Foo/Baz"), 
                         Sdf.Path("/Foo/Baz/Garply")]))

            p.stage.Unload("/")
            p.PrintPaths()
            assert not p.stage.GetPrimAtPath("/Sad/Panda")
            assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")
            assert (set(p.stage.GetLoadSet()) == set([]))

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

            # In the past, when it was illegal to unload a deactivate prim we'd get
            # an error for this.  Now it is legal.
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
            assert baz.IsLoaded()

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
