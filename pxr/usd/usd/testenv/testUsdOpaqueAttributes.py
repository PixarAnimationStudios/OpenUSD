#!/pxrpythonsubst
#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

import unittest
from pxr import Sdf, Tf, Usd

class TestUsdDataFormats(unittest.TestCase):
    def test_NoAuthoredValue(self):
        """
        Make sure we can't access or author values for opaque attributes.
        """
        s = Usd.Stage.CreateInMemory()
        p = s.DefinePrim("/X", "Scope")
        attr1 = p.CreateAttribute("Attr1", Sdf.ValueTypeNames.Opaque)

        self.assertEqual(attr1.Get(), None)

        # Make sure we can't author a default value
        with self.assertRaises(Tf.ErrorException):
            attr1.Set(Sdf.OpaqueValue())
            self.assertEqual(attr1.Get(), None)

        # Make sure we can't author time samples
        with self.assertRaises(Tf.ErrorException):
            attr1.Set(Sdf.OpaqueValue(), Usd.TimeCode(10.0))

        # After all that failed authoring, we should still not have a value.
        self.assertFalse(attr1.HasValue())
        self.assertEqual(attr1.Get(), None)

    def _RunSerializeTest(self, fmt):
        """
        Make sure the given format can read and write metadata and connections
        for opaque attributes.
        """
        # Set up scene
        s = Usd.Stage.CreateNew('test_OpaqueAttributes.' + fmt)
        p = s.DefinePrim("/X", "Scope")
        attr1 = p.CreateAttribute("Attr1", Sdf.ValueTypeNames.Opaque)
        attr1.SetHidden(True)
        attr2 = p.CreateAttribute("Attr2", Sdf.ValueTypeNames.Opaque)
        attr2.AddConnection(attr1.GetPath())

        # Save to disk and reload
        s.Save()
        # Use the layer reload method so we can force the reload
        s.GetRootLayer().Reload(force=True)

        # Make sure we preserved everything we wanted to preserve
        self.assertTrue(s.GetAttributeAtPath("/X.Attr1").IsHidden())
        conns = s.GetAttributeAtPath("/X.Attr2").GetConnections()
        self.assertEqual(len(conns), 1)
        self.assertEqual(conns[0], "/X.Attr1")

    def test_SerializableUsda(self):
        self._RunSerializeTest("usda")

    def test_SerializableUsdc(self):
        self._RunSerializeTest("usdc")

if __name__ == '__main__':
    unittest.main()
