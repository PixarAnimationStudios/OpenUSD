#!/pxrpythonsubst
#
# Copyright 2021 Pixar
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
