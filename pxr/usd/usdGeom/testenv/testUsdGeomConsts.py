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

import sys, os, unittest
from pxr import Sdf, Usd, UsdGeom

class TestUsdGeomConsts(unittest.TestCase):
    def _WriteConst(self, prim, attrName, typeStr, constVal):
        """Add a const to the prim"""
        attr = prim.CreateAttribute(attrName, typeStr)
        self.assertTrue(attr.IsDefined())
        attr.Set(constVal)

    def _CompareAuthoredToConst(self, prim, attrName, constVal):
        """Ensure the const that we previously serialized compares equal to the
        source of the value we authored (i.e. the actual schema const)
        """
        attr = prim.GetAttribute(attrName)
        self.assertTrue(attr.IsDefined())
        val = attr.Get()
        self.assertEqual(constVal, val, "Round-tripped constant '%s' did not compare "
                                   "equal to schema constant. "
                                   "(schema: %s, roundtripped: %s)" 
                                   % (attrName, repr(constVal), repr(val)))

    def _CompareAuthoredToArchived(self ,prim, attrName, archivePrim):
        """To catch ourselves if we inadvertently change the value of a const in a
        schema (which would be really bad to do unwittingly), we compare the value
        we wrote and read back in to a value from a baseline usda file of the same
        layout.
        """
        attr = prim.GetAttribute(attrName)
        self.assertTrue(attr.IsDefined())
        newVal = attr.Get()
        archAttr = archivePrim.GetAttribute(attrName)
        self.assertTrue(archAttr.IsDefined())
        archVal = archAttr.Get()
        self.assertEqual(archVal, newVal, "Baseline archived constant '%s' did not "
                                     "compare equal to schema constant "
                                     "(baseline: %s, schema: %s)." 
                                     % (attrName, repr(archVal), repr(newVal)))

    def test_Basic(self):
        lyr = Sdf.Layer.CreateNew("testConsts.usda")
        self.assertTrue(lyr)

        # To update this file, just grab the result of a run of this test (disabling
        # the archive comparison, so it will succeed) from the test-run directory
        archiveFn = 'testConsts.usda'
        archiveLyr = Sdf.Layer.FindOrOpen(archiveFn)
        self.assertTrue(archiveLyr)

        stage = Usd.Stage.Open("testConsts.usda")
        self.assertTrue(stage)

        archiveStage = Usd.Stage.Open(archiveFn)
        self.assertTrue(archiveStage)

        prim = UsdGeom.Scope.Define(stage, "/ConstRoot").GetPrim()
        self.assertTrue(prim)

        archivePrim = archiveStage.GetPrimAtPath("/ConstRoot")
        self.assertTrue(archivePrim)

        testConsts = (('SHARPNESS_INFINITE', 
                       Sdf.ValueTypeNames.Float, 
                       UsdGeom.Mesh.SHARPNESS_INFINITE), )

        #
        # First write out all the consts
        #
        for ( name, cType, cVal ) in testConsts:
            self._WriteConst( prim, name, cType, cVal )

        lyr.Save()

        #
        # Now, Rebuild the stage from a fresh read of the file
        #
        stage = None
        lyr = None

        lyr = Sdf.Layer.FindOrOpen("testConsts.usda")
        self.assertTrue(lyr)

        stage = Usd.Stage.Open("testConsts.usda")
        self.assertTrue(stage)

        prim = stage.GetPrimAtPath("/ConstRoot")
        self.assertTrue(prim)

        #
        # Finally, compare the (from code) const values with both
        # the just-serialized values (reread), and the archived values
        #
        for ( name, cType, cVal ) in testConsts:
            self._CompareAuthoredToConst(prim, name, cVal )
            self._CompareAuthoredToArchived(prim, name, archivePrim)

if __name__ == "__main__":
    unittest.main()
