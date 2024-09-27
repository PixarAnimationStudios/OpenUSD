#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Tf
from pxr import Gf
from pxr import Usd
from pxr import UsdSemantics

class TestPseudoRoot(unittest.TestCase):
    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory()

    def testGetAppliedTaxonomies(self):
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.GetDirectTaxonomies(
                self.stage.GetPseudoRoot()), [])

    def testComputeInheritedTaxonomies(self):
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.ComputeInheritedTaxonomies(
                self.stage.GetPseudoRoot()),
            [])


class TestUnapplied(unittest.TestCase):
    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory()
        self.rootPrim = self.stage.DefinePrim("/Bookcase")
        self.unappliedSchema = UsdSemantics.LabelsAPI(self.rootPrim, "style")
        self.assertFalse(self.unappliedSchema)

    def testGetAppliedTaxonomies(self):
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.GetDirectTaxonomies(self.rootPrim),
            [])

    def testComputeInheritedTaxonomies(self):
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.ComputeInheritedTaxonomies(self.rootPrim),
            [])


class TestDirectlyApplied(unittest.TestCase):
    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory()
        self.rootPrim = self.stage.DefinePrim("/Bookcase")
        self.appliedSemantics = \
            UsdSemantics.LabelsAPI.Apply(self.rootPrim, "style")

    def testGetAppliedTaxonomies(self):
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.GetDirectTaxonomies(self.rootPrim),
            ["style"]
        )

    def testComputeInheritedTaxonomies(self):
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.ComputeInheritedTaxonomies(self.rootPrim),
            ["style"]
        )


class TestHierachy(unittest.TestCase):
    """Test helpers for computing ancestral taxonomies from the hierarchy

    The grandparent has two applications of the schema, the child has one,
    and the parent has zero.
    """
    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory()
        self.grandparent = self.stage.DefinePrim("/Grandparent")
        self.parent = self.stage.DefinePrim("/Grandparent/Parent")
        self.child = self.stage.DefinePrim("/Grandparent/Parent/Child")

        UsdSemantics.LabelsAPI.Apply(self.grandparent, "style")
        UsdSemantics.LabelsAPI.Apply(self.parent, "category")
        UsdSemantics.LabelsAPI.Apply(self.child, "style")

    def testGetAppliedTaxonomies(self):
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.GetDirectTaxonomies(self.grandparent),
            ["style"]
        )
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.GetDirectTaxonomies(self.parent),
            ["category"]
        )
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.GetDirectTaxonomies(self.child),
            ["style"]
        )

    def testComputeInheritedTaxonomies(self):
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.ComputeInheritedTaxonomies(self.grandparent),
            ["style"]
        )
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.ComputeInheritedTaxonomies(self.parent),
            ["style", "category"]
        )
        self.assertCountEqual(
            UsdSemantics.LabelsAPI.ComputeInheritedTaxonomies(self.child),
            ["style", "category"]
        )



if __name__ == "__main__":
    unittest.main()