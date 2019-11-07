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

import unittest

from pxr import Usd, UsdGeom, Sdf

class TestBasisCurves(unittest.TestCase):
    def setUp(self):
        self.layer = Sdf.Layer.CreateAnonymous()
        self.stage = Usd.Stage.Open(self.layer.identifier)
        self.curves = self.stage.DefinePrim("/TestCurves", "BasisCurves")
        self.schema = UsdGeom.BasisCurves(self.curves)
    
    def test_create(self):
        assert self.curves
        assert self.curves.GetName() == 'TestCurves'
        
    def test_schema(self):
        assert self.schema
        assert 'widths' in self.schema.GetSchemaAttributeNames()
        assert 'points' in self.schema.GetSchemaAttributeNames()
        assert 'normals' in self.schema.GetSchemaAttributeNames()
        assert 'curveVertexCounts' in self.schema.GetSchemaAttributeNames()
        assert 'type' in self.schema.GetSchemaAttributeNames()
        assert 'basis' in self.schema.GetSchemaAttributeNames()
        assert 'wrap' in self.schema.GetSchemaAttributeNames()
        basis = self.schema.GetBasisAttr()
        assert(basis)

class TestNurbsCurves(unittest.TestCase):
    def setUp(self):
        self.layer = Sdf.Layer.CreateAnonymous()
        self.stage = Usd.Stage.Open(self.layer.identifier)
        self.curves = self.stage.DefinePrim("/TestCurves", "NurbsCurves")
        self.schema = UsdGeom.NurbsCurves(self.curves)
    
    def test_create(self):
        assert self.curves
        assert self.curves.GetName() == 'TestCurves'
        
    def test_schema(self):
        assert self.schema
        assert 'widths' in self.schema.GetSchemaAttributeNames()
        assert 'points' in self.schema.GetSchemaAttributeNames()
        assert 'normals' in self.schema.GetSchemaAttributeNames()
        assert 'curveVertexCounts' in self.schema.GetSchemaAttributeNames()
        assert 'knots' in self.schema.GetSchemaAttributeNames()
        assert 'order' in self.schema.GetSchemaAttributeNames()
        knots = self.schema.GetKnotsAttr()
        assert(knots)

if __name__ == '__main__':
    unittest.main(verbosity=2)
