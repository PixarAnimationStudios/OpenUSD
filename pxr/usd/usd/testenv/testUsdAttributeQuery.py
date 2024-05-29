#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import contextlib
import unittest

from pxr import Usd, Sdf, Tf

@contextlib.contextmanager
def StageChangeListener(stage):
    class _Listener(object):
        def __init__(self, stage):
            self._listener = Tf.Notice.Register(
                Usd.Notice.ObjectsChanged, self._HandleNotice, stage)
            self.resyncedPrimPaths = []
            self.changedInfoPaths = []

        def _HandleNotice(self, notice, sender):
            self.resyncedPrimPaths = notice.GetResyncedPaths()
            self.changedInfoPaths = notice.GetChangedInfoOnlyPaths()

    l = _Listener(stage)
    yield l

class TestUsdAttributeQuery(unittest.TestCase):
    def test_NoInvalidationForInsignificantChange(self):
        """Test that insignificant layer stack changes do not invalidate
        an attribute query."""

        sublayer = Sdf.Layer.CreateAnonymous("source.usda")
        sublayer.ImportFromString('''
#usda 1.0
        
def "Prim"
{
    double attr = 1.0
    double attr.timeSamples = {
        0.0: 2.0
    }
}
'''
        .strip())

        emptySublayerBefore = Sdf.Layer.CreateAnonymous("empty_before.usda")
        emptySublayerAfter = Sdf.Layer.CreateAnonymous("empty_after.usda")

        rootLayer = Sdf.Layer.CreateAnonymous("root.usda")
        rootLayer.subLayerPaths.insert(0, sublayer.identifier)

        stage = Usd.Stage.Open(rootLayer)

        query = Usd.AttributeQuery(stage.GetAttributeAtPath("/Prim.attr"))
        self.assertTrue(query)
        self.assertEqual(query.Get(), 1.0)
        self.assertEqual(query.Get(0), 2.0)

        # Test that adding and removing empty sublayers before and after the
        # sublayer with attribute values does not invalidate the attribute
        # query.
        @contextlib.contextmanager
        def _Validate():
            with StageChangeListener(stage) as l:
                yield
                self.assertEqual(l.resyncedPrimPaths, [])

            self.assertTrue(query)
            self.assertEqual(query.Get(), 1.0)
            self.assertEqual(query.Get(0), 2.0)

        with _Validate():
            rootLayer.subLayerPaths.insert(0, emptySublayerBefore.identifier)

        with _Validate():
            rootLayer.subLayerPaths.append(emptySublayerAfter.identifier)

        with _Validate():
            rootLayer.subLayerPaths.remove(emptySublayerBefore.identifier)

        with _Validate():
            rootLayer.subLayerPaths.remove(emptySublayerAfter.identifier)

if __name__ == "__main__":
    unittest.main()
