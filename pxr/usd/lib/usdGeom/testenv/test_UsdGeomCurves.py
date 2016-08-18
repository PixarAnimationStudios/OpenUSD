#!/pxrpythonsubst

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
