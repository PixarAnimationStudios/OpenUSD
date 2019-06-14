#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

from pxr import Gf, Tf, Sdf, Usd, Vt
import unittest, math

class TestUsdSchemaBase(unittest.TestCase):

    def test_InvalidSchemaBase(self):
        sb = Usd.SchemaBase()
        # Conversion to bool should return False.
        self.assertFalse(sb)
        # It should still be safe to get the prim, but the prim will be invalid.
        p = sb.GetPrim()
        with self.assertRaises(RuntimeError):
            p.IsActive()
        # It should still be safe to get the path, but the path will be empty.
        self.assertEqual(sb.GetPath(), Sdf.Path())
        # Try creating a CollectionAPI from it, and make sure the result is
        # a suitably invalid CollactionAPI object.
        coll = Usd.CollectionAPI(sb, 'newcollection')
        self.assertFalse(coll)
        with self.assertRaises(RuntimeError):
            coll.CreateExpansionRuleAttr()

if __name__ == "__main__":
    unittest.main()
