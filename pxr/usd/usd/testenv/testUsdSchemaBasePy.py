#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
