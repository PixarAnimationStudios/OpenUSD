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

import unittest
from pxr import Sdf, Usd

class TestUsdResolveTargetPy(unittest.TestCase):
    """ 
    Test for value resolution with UsdResolverTarget in python. This test is
    to verify that the python API works but is not meant to be a full coverage
    test. Full coverage is handled by the C++ test testUsdResolveTarget.
    """

    def _VerifyAttrQuery(self, attrQuery, 
                         expectedTimeSamples = None, 
                         expectedDefaultValue = None) :
        # We expect HasAuthoredValue() to return true if we expect either time
        # samples or a default value.
        expectedHasAuthoredValue = bool(expectedTimeSamples or expectedDefaultValue)
        self.assertEqual(attrQuery.HasAuthoredValue(), expectedHasAuthoredValue)

        # We expect HasValue to return true if we expect an authored value.
        # Note that HasValue would return true if an attribute has a fallback value
        # but this whole test doesn't use attributes with fallbacks.
        expectedHasValue = expectedHasAuthoredValue
        self.assertEqual(attrQuery.HasValue(), expectedHasValue)

        # Verify that GetTimeSamples returns the expected time sample times.
        expectedTimeSampleTimes = [] if expectedTimeSamples is None else \
            [time for time, _ in expectedTimeSamples]
        self.assertEqual(attrQuery.GetTimeSamples(), expectedTimeSampleTimes)

        # Since this test currently doesn't involve clips, we expect 
        # ValueMightBeTimeVarying to be true iff we expect more than one time 
        # sample.
        if len(expectedTimeSampleTimes) > 1:
            self.assertTrue(attrQuery.ValueMightBeTimeVarying())
        else: 
            self.assertFalse(attrQuery.ValueMightBeTimeVarying())

        # Verify that calling Get at default time returns the expected default 
        # value.
        if expectedDefaultValue is None:
            self.assertIsNone(attrQuery.Get(Usd.TimeCode.Default()))
        else:
            self.assertEqual(
                attrQuery.Get(Usd.TimeCode.Default()), expectedDefaultValue)
        
        if expectedTimeSamples is None:
            # If we expect no time samples, verify that calling Get with a 
            # numeric time code returns the expected default value.
            if expectedDefaultValue is None:
                self.assertIsNone(attrQuery.Get(1.0))
            else:
                self.assertEqual(
                    attrQuery.Get(1.0), expectedDefaultValue)
        else:
            # Verify that calling Get at each expected time sample time returns 
            # the expected time sample value.
            for time, val in expectedTimeSamples:
                self.assertEqual(attrQuery.Get(time), val)

    def test_ResolveTargetFromEditTarget(self):
        """Test UsdResolveTargets created from a UsdEditTarget."""

        stage = Usd.Stage.Open("resolveTarget/root.usda")
        # Parent unculled prim stack is:
        #   /Parent : session.usda -> root.usda -> sub1.usda -> sub2.usda
        #      |
        #     ref
        #      v
        #   /InternalRef : session.usda -> root.usda -> sub1.usda -> sub2.usda
        #      |
        #     ref
        #      v
        #   /RefParent : ref.usda -> ref_sub1.usda -> ref_sub2.usda
        parentPrim = stage.GetPrimAtPath("/Parent")
        self.assertTrue(parentPrim)
        # /Parent/RefChild is just a namespace child of /Parent with no 
        # additional composition arcs of its own outside of its ancestral
        # composition.
        childPrim = stage.GetPrimAtPath("/Parent/RefChild")
        self.assertTrue(childPrim)

        # Create an edit target that targets the sub2 layer with no PcpMapping
        editTarget = Usd.EditTarget(Sdf.Layer.Find('resolveTarget/sub2.usda'))
        self.assertTrue(editTarget.IsValid())
        self.assertFalse(editTarget.IsNull())

        # Make both an "up to" and "stronger than" resolve target for 
        # /Parent/RefChild from this edit target.
        resolveUpToEditTarget = \
            childPrim.MakeResolveTargetUpToEditTarget(editTarget);
        resolveStrongerThanEditTarget = \
            childPrim.MakeResolveTargetStrongerThanEditTarget(editTarget);
        self.assertFalse(resolveUpToEditTarget.IsNull())
        self.assertFalse(resolveStrongerThanEditTarget.IsNull())

        # Using /Parent/RefChild.foo verify the attribute value resolves 
        # correctly based on "up to" and "stronger than" the edit target spec:
        #  root.usda: /Parent/RefChild -> 6.0
        #  sub1.usda: /Parent/RefChild -> 5.0
        #  sub2.usda: /Parent/RefChild -> 4.0 (edit target spec)
        #  ...
        attr = stage.GetAttributeAtPath("/Parent/RefChild.foo")
        self.assertTrue(attr)
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, resolveUpToEditTarget),
            expectedDefaultValue=4.0)
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, resolveStrongerThanEditTarget),
            expectedDefaultValue=6.0)

        # Now set /Parent/RefChild.foo to 10.0 with the edit target.
        with Usd.EditContext(stage, editTarget):
            attr.Set(10.0)

        # Like UsdPrimCompositionQuery and UsdAttributeQuery, resolve targets
        # do not listen to change notification and must be recreated if a 
        # change potentially affecting the composed scene occurs. In this case
        # authoring fooAttr's default on a layer that already has a spec for
        # it does cause recomposition, but we recreate the resolve targets 
        # anyway. 
        resolveUpToEditTarget = \
            childPrim.MakeResolveTargetUpToEditTarget(editTarget);
        resolveStrongerThanEditTarget = \
            childPrim.MakeResolveTargetStrongerThanEditTarget(editTarget);
        self.assertFalse(resolveUpToEditTarget.IsNull())
        self.assertFalse(resolveStrongerThanEditTarget.IsNull())

        # Verify the attribute value resolves correctly based on "up to" and 
        # "stronger than" the edit target spec's new value in sub2.usda:
        #  root.usda: /Parent/RefChild -> 6.0
        #  sub1.usda: /Parent/RefChild -> 5.0
        #  sub2.usda: /Parent/RefChild -> 10.0 (edit target spec)
        #  ...
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, resolveUpToEditTarget),
            expectedDefaultValue=10.0)
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, resolveStrongerThanEditTarget),
            expectedDefaultValue=6.0)

    def test_ResolveTargetFromCompositionQuery(self):
        """Test UsdResolveTargets created from a UsdPrimCompositionQuery."""

        stage = Usd.Stage.Open("resolveTarget/root.usda")
        # Parent unculled prim stack is:
        #   /Parent : session.usda -> root.usda -> sub1.usda -> sub2.usda
        #      |
        #     ref
        #      v
        #   /InternalRef : session.usda -> root.usda -> sub1.usda -> sub2.usda
        #      |
        #     ref
        #      v
        #   /RefParent : ref.usda -> ref_sub1.usda -> ref_sub2.usda
        parentPrim = stage.GetPrimAtPath("/Parent")
        self.assertTrue(parentPrim)
        # /Parent/RefChild is just a namespace child of /Parent with no 
        # additional composition arcs of its own outside of its ancestral
        # composition.
        childPrim = stage.GetPrimAtPath("/Parent/RefChild")
        self.assertTrue(childPrim)

        # Create a prim composition query for /Parent/RefChild. The prim 
        # composition query gets us every arc that could contribute specs to the
        # prim (even if the arc would be culled normally) so we use it to 
        # create all resolve targets.
        query = Usd.PrimCompositionQuery(childPrim)
        arcs = query.GetCompositionArcs()

        # Loop through every layer in each composition arc creating both the 
        # "up to" and "stronger than" resolve targets for each.
        upToResolveTargets = []
        strongerThanResolveTargets = []
        for arc in arcs:
            layers = arc.GetTargetNode().layerStack.layers
            for layer in layers:
                upToResolveTargets.append(
                    arc.MakeResolveTargetUpTo(layer))
                strongerThanResolveTargets.append(
                    arc.MakeResolveTargetStrongerThan(layer))

        self.assertEqual(len(upToResolveTargets), 11)
        self.assertEqual(len(strongerThanResolveTargets), 11)

        # /Parent/RefChild.bar
        # Has alternating time samples and default values authored:
        #    root.usda: /Parent/RefChild -> {1.0: 6, 6.0: 1}
        #    sub1.usda: /Parent/RefChild -> 5
        #    sub2.usda: /Parent/RefChild -> {1.0: 4, 4.0: 1}
        #    ref.usda: /RefParent/RefChild -> 3
        #    ref_sub1.usda: /RefParent/RefChild -> {1.0: 2, 2.0: 1}
        #    ref_sub2.usda: /RefParent/RefChild -> 1
        attr = stage.GetAttributeAtPath("/Parent/RefChild.bar")
        self.assertTrue(attr)
        
        # Node: /Parent/RefChild
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, upToResolveTargets[0]),
            expectedTimeSamples=[(1.0, 6), (6.0, 1)],
            expectedDefaultValue=5)
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, strongerThanResolveTargets[0]),
            expectedTimeSamples=None,
            expectedDefaultValue=None)

        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, upToResolveTargets[1]),
            expectedTimeSamples=[(1.0, 6), (6.0, 1)],
            expectedDefaultValue=5)
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, strongerThanResolveTargets[1]),
            expectedTimeSamples=None,
            expectedDefaultValue=None)

        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, upToResolveTargets[2]),
            expectedTimeSamples=None,
            expectedDefaultValue=5)
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, strongerThanResolveTargets[2]),
            expectedTimeSamples=[(1.0, 6), (6.0, 1)],
            expectedDefaultValue=None)

        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, upToResolveTargets[3]),
            expectedTimeSamples=[(1.0, 4), (4.0, 1)],
            expectedDefaultValue=3)
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, strongerThanResolveTargets[3]),
            expectedTimeSamples=[(1.0, 6), (6.0, 1)],
            expectedDefaultValue=5)

        # Node: /Internal/RefChild
        for i in range(4, 8):
            self._VerifyAttrQuery(
                Usd.AttributeQuery(attr, upToResolveTargets[i]),
                expectedTimeSamples=None,
                expectedDefaultValue=3)
            self._VerifyAttrQuery(
                Usd.AttributeQuery(attr, strongerThanResolveTargets[i]),
                expectedTimeSamples=[(1.0, 6), (6.0, 1)],
                expectedDefaultValue=5)

        # Node: /RefParent/RefChild
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, upToResolveTargets[8]),
            expectedTimeSamples=None,
            expectedDefaultValue=3)
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, strongerThanResolveTargets[8]),
            expectedTimeSamples=[(1.0, 6), (6.0, 1)],
            expectedDefaultValue=5)

        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, upToResolveTargets[9]),
            expectedTimeSamples=[(1.0, 2), (2.0, 1)],
            expectedDefaultValue=1)
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, strongerThanResolveTargets[9]),
            expectedTimeSamples=[(1.0, 6), (6.0, 1)],
            expectedDefaultValue=5)

        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, upToResolveTargets[10]),
            expectedTimeSamples=None,
            expectedDefaultValue=1)
        self._VerifyAttrQuery(
            Usd.AttributeQuery(attr, strongerThanResolveTargets[10]),
            expectedTimeSamples=[(1.0, 6), (6.0, 1)],
            expectedDefaultValue=5)

if __name__ == '__main__':
    unittest.main()
