#!/pxrpythonsubst
#
# Copyright 2022 Pixar
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
