#!/pxrpythonsubst
#
# Copyright 2024 Pixar
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
        exprs2 = Sdf.PathExpressionArray(('/foo', '/bar'))
        self.assertEqual(exprs1, exprs2)

        # Simple use of PathExpressionArray attributes.
        l = Sdf.Layer.CreateAnonymous()
        p = Sdf.CreatePrimInLayer(l, '/foo')
        a = Sdf.AttributeSpec(p, 'a', Sdf.ValueTypeNames.PathExpressionArray)
        a.default = (Sdf.PathExpression('/foo'), Sdf.PathExpression('/bar'))
        b = Sdf.AttributeSpec(p, 'b', Sdf.ValueTypeNames.PathExpressionArray)
        b.default = ('/foo', '/bar')
        self.assertEqual(a.default, b.default)

if __name__ == '__main__':
    unittest.main()
