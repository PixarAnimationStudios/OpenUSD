#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Tf
import sys, unittest

MatchEval = Sdf._MakeBasicMatchEval

class TestSdfPathExpressionArray(unittest.TestCase):

    def test_Basics(self):
        # Create arrays
        exprs1 = Sdf.PathExpressionArray((Sdf.PathExpression('/foo'),
                                         Sdf.PathExpression('/bar')))
        with self.assertRaises(TypeError,
            msg="Implicit conversions from string should fail"):
            exprs2 = Sdf.PathExpressionArray(('/foo', '/bar'))

        # Simple use of PathExpressionArray attributes.
        l = Sdf.Layer.CreateAnonymous()
        p = Sdf.CreatePrimInLayer(l, '/foo')
        a = Sdf.AttributeSpec(p, 'a', Sdf.ValueTypeNames.PathExpressionArray)
        a.default = (Sdf.PathExpression('/foo'), Sdf.PathExpression('/bar'))
        self.assertEqual(a.default, exprs1)

if __name__ == '__main__':
    unittest.main()
