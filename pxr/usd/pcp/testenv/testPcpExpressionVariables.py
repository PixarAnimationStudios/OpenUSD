#!/pxrpythonsubst
#
# Copyright 2023 Pixar
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

