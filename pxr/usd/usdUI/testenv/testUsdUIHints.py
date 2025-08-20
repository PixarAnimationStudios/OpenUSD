#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Sdf, Usd, UsdUI
import unittest

class TestUsdUIHints(unittest.TestCase):
    def setUp(self):
        self.stage = Usd.Stage.Open('hints.usda')
        assert self.stage

    #
    # Helpers
    #

    def _ValidateObjectHints(self, hints, obj, displayName, hidden):
        # Make sure we either get the expected object back from `hints`, or
        # that they're both invalid. Some of the tests below will pass e.g., an
        # invalid UsdPrim for obj, which won't compare equal to the invalid
        # UsdObject returned by `hints`.
        assert hints.GetObject() == obj or \
               (not hints.GetObject().IsValid() and not obj.IsValid())

        self.assertEqual(hints.GetDisplayName(), displayName)
        self.assertEqual(hints.GetHidden(), hidden)

    def _ValidatePrimHints(self, hints, prim, expandedDict, shownIfDict):
        self.assertEqual(hints.GetPrim(), prim)

        # Group expansion state
        self.assertEqual(hints.GetDisplayGroupsExpanded(), expandedDict)
        for k, v in expandedDict.items():
            self.assertEqual(hints.GetDisplayGroupExpanded(k), v)
        self.assertEqual(hints.GetDisplayGroupExpanded('non-existent'), False);

        # Group shownIf
        self.assertEqual(hints.GetDisplayGroupsShownIf(), shownIfDict)
        for k, v in shownIfDict.items():
            self.assertEqual(hints.GetDisplayGroupShownIf(k), v)
        self.assertEqual(hints.GetDisplayGroupShownIf('non-existent'), '');

    def _ValidatePropertyHints(self, hints, prop, displayGroup, shownIf):
        # Make sure we either get the expected property back from `hints`, or
        # that they're both invalid. Some of the tests below will pass e.g., an
        # invalid UsdAttribute for prop, which won't compare equal to the
        # invalid UsdProperty returned by `hints`.
        assert hints.GetProperty() == prop or \
               (not hints.GetProperty().IsValid() and not prop.IsValid())

        self.assertEqual(hints.GetDisplayGroup(), displayGroup)
        self.assertEqual(hints.GetShownIf(), shownIf)

    def _ValidateAttributeHints(
            self, hints, attr,
            valueLabels, valueLabelsOrder):

        self.assertEqual(hints.GetAttribute(), attr)

        # Value labels
        self.assertEqual(
            hints.GetValueLabels(),
            valueLabels)
        self.assertEqual(
            hints.GetValueLabelsOrder(),
            valueLabelsOrder)

    #
    # Test cases
    #

    def test_Fallbacks(self):
        # Verify that default-constructed and blank-object hints return the
        # fallbacks for each field

        # Prim
        hintlessPrim = self.stage.GetPrimAtPath('/HintlessPrim')
        assert hintlessPrim

        def _checkPrim(hints, prim):
            self._ValidatePrimHints(
                hints, prim,
                expandedDict = {},
                shownIfDict = {})

            self._ValidateObjectHints(
                hints, prim,
                displayName = '',
                hidden = False)

        # Check default-constructed and empty cases
        _checkPrim(UsdUI.PrimHints(), Usd.Prim())
        _checkPrim(UsdUI.PrimHints(hintlessPrim), hintlessPrim)

        # Attribute
        hintlessAttr = \
            self.stage.GetAttributeAtPath('/HintlessPrim.hintlessAttribute')
        assert hintlessAttr

        def _checkAttr(hints, attr):
            self._ValidateAttributeHints(
                hints, attr,
                valueLabels = {},
                valueLabelsOrder = [])

            self._ValidatePropertyHints(
                hints, attr,
                displayGroup = '',
                shownIf = '')

            self._ValidateObjectHints(
                hints, attr,
                displayName = '',
                hidden = False)

        # Check default-constructed and empty cases
        _checkAttr(UsdUI.AttributeHints(), Usd.Attribute())
        _checkAttr(UsdUI.AttributeHints(hintlessAttr), hintlessAttr)

    def test_InvalidHints(self):
        # Verify that "set" operations on invalid hint objects raise exceptions

        # Object
        hints = UsdUI.ObjectHints()
        assert not hints

        with self.assertRaises(RuntimeError):
            hints.SetDisplayName('name')
        with self.assertRaises(RuntimeError):
            hints.SetHidden(True)

        # Prim
        hints = UsdUI.PrimHints()
        assert not hints

        with self.assertRaises(RuntimeError):
            hints.SetDisplayGroupsExpanded({'a group' : True})
        with self.assertRaises(RuntimeError):
            hints.SetDisplayGroupExpanded('a group', True)
        with self.assertRaises(RuntimeError):
            hints.SetDisplayGroupsShownIf({'a group' : 'x == 5'})
        with self.assertRaises(RuntimeError):
            hints.SetDisplayGroupShownIf('a group', 'x == 5')

        # Property
        hints = UsdUI.PropertyHints()
        assert not hints

        with self.assertRaises(RuntimeError):
            hints.SetDisplayGroup('group')
        with self.assertRaises(RuntimeError):
            hints.SetShownIf('x == 5')

        # Attribute
        hints = UsdUI.AttributeHints()
        assert not hints

        with self.assertRaises(RuntimeError):
            hints.SetValueLabels({ 'a' : 1 })
        with self.assertRaises(RuntimeError):
            hints.SetValueLabelsOrder(['a'])
        with self.assertRaises(RuntimeError):
            hints.ApplyValueLabel('a')

    def test_Errors(self):
        # Verify that we get errors when passing invalid data

        prim = self.stage.DefinePrim('/test_Errors')
        assert prim
        attr = prim.CreateAttribute('attr', Sdf.ValueTypeNames.Int)
        assert attr

        primHints = UsdUI.PrimHints(prim)
        attrHints = UsdUI.AttributeHints(attr)

        # Set non-bool value for DisplayGroupsExpanded
        with self.assertRaises(RuntimeError):
            primHints.SetDisplayGroupsExpanded({ 'group' : 'expanded' })

        # Set non-string value for DisplayGroupsShownIf
        with self.assertRaises(RuntimeError):
            primHints.SetDisplayGroupsShownIf({ 'group' : 5 })

    def test_PrimHintsFromAsset(self):
        prim = self.stage.GetPrimAtPath('/HintsPrim')
        assert prim

        hints = UsdUI.PrimHints(prim)
        assert hints

        self._ValidatePrimHints(
            hints, prim,
            expandedDict = { 'a group' : True,
                             'a group:a nested group' : False },
            shownIfDict = { 'a group' : 'x == 1' })

        self._ValidateObjectHints(
            hints, prim,
            displayName = 'a prim',
            hidden = True)

    def test_AttributeHintsFromAsset(self):
        attr = self.stage.GetAttributeAtPath('/HintsPrim.attribute')
        assert attr

        hints = UsdUI.AttributeHints(attr)
        assert hints

        self._ValidateAttributeHints(
            hints, attr,
            valueLabels = { 'low' : 1, 'med' : 2, 'high' : 3 },
            valueLabelsOrder = ['low', 'med', 'high'])

        self._ValidatePropertyHints(
            hints, attr,
            displayGroup = 'a group',
            shownIf = 'x == 2')

        self._ValidateObjectHints(
            hints, attr,
            displayName = 'an attr',
            hidden = True)

    def test_RelationshipHintsFromAsset(self):
        rel = self.stage.GetRelationshipAtPath('/HintsPrim.relationship')
        assert rel

        hints = UsdUI.PropertyHints(rel)
        assert hints

        self._ValidatePropertyHints(
            hints, rel,
            displayGroup = 'a group:a nested group',
            shownIf = 'x == 3')

        self._ValidateObjectHints(
            hints, rel,
            displayName = 'a rel',
            hidden = True)

    def test_ObjectHintsSetters(self):
        obj = self.stage.DefinePrim('/test_ObjectHintsSetters')
        assert obj

        hints = UsdUI.ObjectHints(obj)
        assert hints

        self.assertEqual(hints.GetDisplayName(), '')
        assert hints.SetDisplayName('displayName')
        self.assertEqual(hints.GetDisplayName(), 'displayName')

        self.assertEqual(hints.GetHidden(), False)
        assert hints.SetHidden(True)
        self.assertEqual(hints.GetHidden(), True)

    def test_PrimHintsSetters(self):
        prim = self.stage.DefinePrim('/test_PrimHintsSetters')
        assert prim

        hints = UsdUI.PrimHints(prim)
        assert hints

        # Group expansion state
        self.assertEqual(hints.GetDisplayGroupsExpanded(), {})
        self.assertEqual(hints.GetDisplayGroupExpanded('group'), False)
        self.assertEqual(hints.GetDisplayGroupExpanded('group:subgroup'), False)
        self.assertEqual(hints.GetDisplayGroupExpanded('group2'), False)

        assert hints.SetDisplayGroupsExpanded(
            { 'group' : True, 'group:subgroup' : True })

        self.assertEqual(
            hints.GetDisplayGroupsExpanded(),
            { 'group' : True, 'group:subgroup' : True })
        self.assertEqual(hints.GetDisplayGroupExpanded('group'), True)
        self.assertEqual(hints.GetDisplayGroupExpanded('group:subgroup'), True)
        self.assertEqual(hints.GetDisplayGroupExpanded('group2'), False)

        assert hints.SetDisplayGroupExpanded('group2', True)
        self.assertEqual(
            hints.GetDisplayGroupsExpanded(),
            { 'group' : True, 'group:subgroup' : True, 'group2' : True })
        self.assertEqual(hints.GetDisplayGroupExpanded('group2'), True)

        # Group shownIf
        self.assertEqual(hints.GetDisplayGroupsShownIf(), {})
        self.assertEqual(hints.GetDisplayGroupShownIf('group'), '')
        self.assertEqual(hints.GetDisplayGroupShownIf('group2'), '')

        assert hints.SetDisplayGroupsShownIf({ 'group' : 'x == 1' })

        self.assertEqual(hints.GetDisplayGroupsShownIf(),
                         { 'group' : 'x == 1' })
        self.assertEqual(hints.GetDisplayGroupShownIf('group'), 'x == 1')
        self.assertEqual(hints.GetDisplayGroupShownIf('group2'), '')

        assert hints.SetDisplayGroupShownIf('group2', 'x == 2')
        self.assertEqual(hints.GetDisplayGroupsShownIf(),
            { 'group' : 'x == 1', 'group2' : 'x == 2' })
        self.assertEqual(hints.GetDisplayGroupShownIf('group2'), 'x == 2')

    def test_AttributeHintsSetters(self):
        prim = self.stage.DefinePrim('/test_AttributeHintsSetters')
        assert prim
        attr = prim.CreateAttribute('newAttribute', Sdf.ValueTypeNames.Int)
        assert attr

        hints = UsdUI.AttributeHints(attr)
        assert hints

        # Value labels
        self.assertEqual(hints.GetValueLabels(), {})
        assert hints.SetValueLabels({ 'round' : 1, 'square' : 2 })
        self.assertEqual(hints.GetValueLabels(), { 'round' : 1, 'square' : 2 })

        self.assertEqual(hints.GetValueLabelsOrder(), [])
        assert hints.SetValueLabelsOrder(['square', 'round'])
        self.assertEqual(hints.GetValueLabelsOrder(), ['square', 'round'])

    def test_AuthorValueLabels(self):
        def _validate(attr, initialValue):
            assert attr
            hints = UsdUI.AttributeHints(attr)
            assert hints

            # Run through all value labels
            self.assertEqual(hints.GetAttribute().Get(), initialValue)
            for label, value in hints.GetValueLabels().items():
                assert hints.ApplyValueLabel(label)
                self.assertEqual(hints.GetAttribute().Get(), value)

            # Apply a non-existent label, which should be a no-op
            curVal = attr.Get()
            self.assertFalse(hints.ApplyValueLabel('non-existent'))
            self.assertEqual(attr.Get(), curVal)

        _validate(
            self.stage.GetAttributeAtPath('/HintsPrim.attribute'), 1)
        _validate(
            self.stage.GetAttributeAtPath('/HintsPrim.tokenArrayAttr'), [])

if __name__ == "__main__":
    unittest.main()
