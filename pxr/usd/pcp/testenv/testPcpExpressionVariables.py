#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Pcp, Sdf

class TestPcpExpressionVariables(unittest.TestCase):
    def test_Basic(self):
        sv = Pcp.ExpressionVariables()
        self.assertEqual(sv.GetSource(), Pcp.ExpressionVariablesSource())
        self.assertEqual(sv.GetVariables(), {})

        sv = Pcp.ExpressionVariables(
            Pcp.ExpressionVariablesSource(), {'A':'B'})
        self.assertEqual(sv.GetSource(), Pcp.ExpressionVariablesSource())
        self.assertEqual(sv.GetVariables(), {'A':'B'})

        sv2 = sv
        self.assertEqual(sv, sv2)
        self.assertNotEqual(sv, Pcp.ExpressionVariables())

    def test_Compute_Basic(self):
        rootLayer = Sdf.Layer.CreateAnonymous('root')
        rootId = Pcp.LayerStackIdentifier(rootLayer)

        self.assertEqual(
            Pcp.ExpressionVariables.Compute(rootId, rootId),
            Pcp.ExpressionVariables(Pcp.ExpressionVariablesSource(), {}))

        rootLayer.expressionVariables = {'A':'B'}
        self.assertEqual(
            Pcp.ExpressionVariables.Compute(rootId, rootId),
            Pcp.ExpressionVariables(Pcp.ExpressionVariablesSource(), {'A':'B'}))

    def test_Compute_SessionLayer(self):
        rootLayer = Sdf.Layer.CreateAnonymous('root')
        sessionLayer = Sdf.Layer.CreateAnonymous('session')
        rootId = Pcp.LayerStackIdentifier(rootLayer, sessionLayer)

        self.assertEqual(
            Pcp.ExpressionVariables.Compute(rootId, rootId),
            Pcp.ExpressionVariables(Pcp.ExpressionVariablesSource(), {}))

        rootLayer.expressionVariables = {'A':'B', 'X':'Y'}
        sessionLayer.expressionVariables = {'A':'C'}
        self.assertEqual(
            Pcp.ExpressionVariables.Compute(rootId, rootId),
            Pcp.ExpressionVariables(
                Pcp.ExpressionVariablesSource(), {'A':'C', 'X':'Y'}))

    def test_Compute_Chained(self):
        rootLayer = Sdf.Layer.CreateAnonymous('root')
        rootId = Pcp.LayerStackIdentifier(rootLayer)

        ref1Layer = Sdf.Layer.CreateAnonymous('ref1')
        ref1Id = Pcp.LayerStackIdentifier(
            ref1Layer, expressionVariablesOverrideSource=
                Pcp.ExpressionVariablesSource(rootId, rootId))

        ref2Layer = Sdf.Layer.CreateAnonymous('ref2')
        ref2Id = Pcp.LayerStackIdentifier(
            ref2Layer, expressionVariablesOverrideSource=
                Pcp.ExpressionVariablesSource(ref1Id, rootId))

        # Compute expression variables for all layer stacks. Since there are no
        # expression variables authored, these should return an empty
        # Pcp.ExpressionVariables.
        self.assertEqual(
            Pcp.ExpressionVariables.Compute(rootId, rootId),
            Pcp.ExpressionVariables(Pcp.ExpressionVariablesSource(), {}))

        self.assertEqual(
            Pcp.ExpressionVariables.Compute(ref1Id, rootId),
            Pcp.ExpressionVariables(Pcp.ExpressionVariablesSource(), {}))

        self.assertEqual(
            Pcp.ExpressionVariables.Compute(ref2Id, rootId),
            Pcp.ExpressionVariables(Pcp.ExpressionVariablesSource(), {}))

        # Author expression variable opinions and compute again. The root layer
        # stack's variables should compose over those from ref1, which
        # should compose over those from ref2.
        ref2Layer.expressionVariables = {'A':'B', 'X':'Z', 'I':'J'}
        ref1Layer.expressionVariables = {'A':'B', 'X':'Y'}
        rootLayer.expressionVariables = {'A':'C'}

        self.assertEqual(
            Pcp.ExpressionVariables.Compute(rootId, rootId),
            Pcp.ExpressionVariables(
                Pcp.ExpressionVariablesSource(rootId, rootId), 
                {'A':'C'}))

        self.assertEqual(
            Pcp.ExpressionVariables.Compute(ref1Id, rootId),
            Pcp.ExpressionVariables(
                Pcp.ExpressionVariablesSource(ref1Id, rootId), 
                {'A':'C', 'X':'Y'}))

        self.assertEqual(
            Pcp.ExpressionVariables.Compute(ref2Id, rootId),
            Pcp.ExpressionVariables(
                Pcp.ExpressionVariablesSource(ref2Id, rootId), 
                {'A':'C', 'X':'Y', 'I':'J'}))

        # Compute each layer stack's expression variables using a pre-cached
        # value for their overrides. This should give the same result as
        # computing the expression variables from scratch.
        rootExpressionVars = Pcp.ExpressionVariables.Compute(rootId, rootId)
        ref1ExpressionVars = Pcp.ExpressionVariables.Compute(ref1Id, rootId)

        self.assertEqual(
            Pcp.ExpressionVariables.Compute(rootId, rootId, rootExpressionVars),
            Pcp.ExpressionVariables.Compute(rootId, rootId))

        self.assertEqual(
            Pcp.ExpressionVariables.Compute(ref1Id, rootId, rootExpressionVars),
            Pcp.ExpressionVariables.Compute(ref1Id, rootId))

        self.assertEqual(
            Pcp.ExpressionVariables.Compute(ref2Id, rootId, ref1ExpressionVars),
            Pcp.ExpressionVariables.Compute(ref2Id, rootId))
        

if __name__ == "__main__":
    unittest.main()

