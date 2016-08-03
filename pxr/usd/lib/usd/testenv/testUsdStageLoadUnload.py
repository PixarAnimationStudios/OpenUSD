#!/pxrpythonsubst

import os, sys
from Mentor.Runtime import (SetAssertMode, MTR_EXIT_TEST, FindDataFile,
                            Assert, AssertTrue, AssertFalse, AssertEqual,
                            AssertNotEqual, RequiredException,
                            ExpectedErrors, ExitTest)

from pxr import Gf, Tf, Sdf, Pcp, Usd

allFormats = ['usd' + x for x in 'abc']

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
        self.payload1 = Usd.Stage.CreateInMemory("payload1"+ext)
        p = self.payload1.DefinePrim("/Sad/Panda", "Scope")

        # Create payload3.usda
        self.payload3 = Usd.Stage.CreateInMemory("payload3"+ext)
        p = self.payload3.DefinePrim("/Garply/Qux", "Scope")

        # Create payload2.usda
        self.payload2 = Usd.Stage.CreateInMemory("payload2"+ext)
        p = self.payload2.DefinePrim("/Baz/Garply", "Scope")
        p.SetPayload(self.payload3.GetRootLayer(), "/Garply")

        #
        # Create the scene that references payload1 and payload2
        #
        self.stage = Usd.Stage.CreateInMemory("scene"+ext)
        p = self.stage.DefinePrim("/Sad", "Scope")
        p.SetPayload(self.payload1.GetRootLayer(), "/Sad")

        p = self.stage.DefinePrim("/Foo/Baz", "Scope")
        p.SetPayload(self.payload2.GetRootLayer(), "/Baz")


    def PrintPaths(self, msg=""):
        print("    Paths: "+msg)
        for p in self.stage.Traverse():
            print "    ", p
        print("")


def TestLoadAndUnload():
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


def TestLoad():
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

        p.stage.Load("/Sad")
        p.PrintPaths()
        assert p.stage.GetPrimAtPath("/Sad/Panda")
        assert p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")

        p.stage.Unload("/Sad")
        p.PrintPaths()
        assert not p.stage.GetPrimAtPath("/Sad/Panda")
        assert p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")

        p.stage.Unload("/")
        p.PrintPaths()
        assert not p.stage.GetPrimAtPath("/Sad/Panda")
        assert not p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")

        p.stage.Load("/")
        p.PrintPaths()
        assert p.stage.GetPrimAtPath("/Sad/Panda")
        assert p.stage.GetPrimAtPath("/Foo/Baz/Garply/Qux")


def TestOpen():
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


def TestErrors():
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

        with RequiredException(Tf.ErrorException):
            p.stage.Load("/Non/Existent/Prim")

        with RequiredException(Tf.ErrorException):
            p.stage.Unload("/Non/Existent/Prim")

        set1 = (Sdf.Path("/Non/Existent/Prim"),)
        set2 = (Sdf.Path("/Other/Existent/Prim"),)
        with RequiredException(Tf.ErrorException):
            p.stage.LoadAndUnload(set1, set())
        with RequiredException(Tf.ErrorException):
            p.stage.LoadAndUnload(set(), set1)
        with RequiredException(Tf.ErrorException):
            p.stage.LoadAndUnload(set1, set2)

        p.PrintPaths()


def TestRedundantLoads():
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


def TestStageCreateNew():
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
        s.Close()
        del s
        assert not lyr

        assert os.path.exists(layerName)
        # No error here, we expect to clobber the existing file.
        s1 = Usd.Stage.CreateNew(layerName)
        assert not s1.GetPrimAtPath("/Foo")
        # We expect this to error, because the layer is already in memory.
        with RequiredException(Tf.ErrorException):
            Usd.Stage.CreateNew(layerName)
        assert s1.GetRootLayer()
        s1.Close()
        del s1

        # Make sure session layers work
        session = Usd.Stage.CreateInMemory()
        session.OverridePrim("/Foo")
        s = Usd.Stage.CreateNew(layerName, session.GetRootLayer())
        assert s.GetPrimAtPath("/Foo")
        session.Close()
        s.Close()
        del s, session

        assert os.path.exists(layerName)
        os.unlink(layerName)

def TestStageReload():
    """Tests Usd.Stage.Reload
    """
    import tempfile

    # Reloading anonymous layers clears content.
    s = Usd.Stage.CreateInMemory()
    s.OverridePrim('/foo')
    s.Reload()
    assert not s.GetPrimAtPath('/foo')

    # Try with a real file -- saved changes preserved, unsaved changes get
    # discarded.
    def _TestStageReload(fmt):
        f = tempfile.NamedTemporaryFile(suffix='.%s' % fmt)
        s = Usd.Stage.CreateNew(f.name)
        s.OverridePrim('/foo')
        s.GetRootLayer().Save()
        s.OverridePrim('/bar')

        assert s.GetPrimAtPath('/foo')
        assert s.GetPrimAtPath('/bar')
        s.Reload()
        assert s.GetPrimAtPath('/foo')
        assert not s.GetPrimAtPath('/bar')

    for fmt in allFormats:
        # XXX: This verifies current behavior, however this is probably
        #      not the behavior we ultimately want -- see bug 102444.
        _TestStageReload(fmt)

def TestStageLayerReload():
    """Test that Usd.Stage responds correctly when one of its 
    layer is directly reloaded."""
    import tempfile, shutil

    def _TestLayerReload(fmt):
        # First, test case where the reloaded layer is in the
        # stage's root LayerStack.
        l1name = tempfile.NamedTemporaryFile(suffix='.%s' % fmt)
        l2name = tempfile.NamedTemporaryFile(suffix='.%s' % fmt)

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
        
        # Now test the case where the reloaded layer is in a referenced
        # LayerStack.
        rootLayerName = tempfile.NamedTemporaryFile(suffix='.%s' % fmt)
        refLayerName = tempfile.NamedTemporaryFile(suffix='.%s' % fmt)

        refLayer = Sdf.Layer.CreateAnonymous()
        Sdf.CreatePrimInLayer(refLayer, '/foo/bar')
        refLayer.Export(refLayerName.name)

        rootLayer = Sdf.Layer.CreateAnonymous()
        rootPrim = Sdf.CreatePrimInLayer(rootLayer, '/foo')
        rootPrim.referenceList.Add(
            Sdf.Reference(refLayerName.name, '/foo'))
        rootLayer.Export(rootLayerName.name)

        s = Usd.Stage.Open(rootLayerName.name)
        assert s.GetPrimAtPath('/foo/bar')
        
        del refLayer.GetPrimAtPath('/foo').nameChildren['bar']
        refLayer.Export(refLayerName.name)
        Sdf.Layer.Find(refLayerName.name).Reload(force = True)
        assert s.GetPrimAtPath('/foo')
        assert not s.GetPrimAtPath('/foo/bar')

    for fmt in allFormats:
        # XXX: This verifies current behavior, however this is probably
        #      not the behavior we ultimately want -- see bug 102444.
        _TestLayerReload(fmt)

if __name__ == "__main__":
    TestLoadAndUnload()
    TestLoad()
    TestOpen()
    TestErrors()
    TestRedundantLoads()
    TestStageCreateNew()
    TestStageReload()
    TestStageLayerReload()
    print("OK")

