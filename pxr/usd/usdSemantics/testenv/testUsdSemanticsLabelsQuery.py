#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import dataclasses
import typing
import unittest

from pxr import Tf
from pxr import Gf
from pxr import Usd
from pxr import UsdSemantics

class TestInvalid(unittest.TestCase):
    def testEmptyInterval(self):
        with self.assertRaises(Tf.ErrorException):
            UsdSemantics.LabelsQuery("instance_name", Gf.Interval())

    def testEmptyTaxonomyInterval(self):
        with self.assertRaises(Tf.ErrorException):
            UsdSemantics.LabelsQuery("", Gf.Interval.GetFullInterval())

    def testEmptyTaxonomyTimeCode(self):
        with self.assertRaises(Tf.ErrorException):
            UsdSemantics.LabelsQuery("", Usd.TimeCode.Default())


class TestStageWithoutLabels(unittest.TestCase):
    queryTimes = (Usd.TimeCode.Default(),
                  Usd.TimeCode(1.0),
                  Gf.Interval(-10.0, 10.0))

    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory()
        self.queryPrims = (
            self.stage.DefinePrim("/Grandparent"),
            self.stage.DefinePrim("/Grandparent/Parent"),
            self.stage.DefinePrim("/Grandparent/Parent/Child"),
            self.stage.GetPseudoRoot(),
        )

    def testComputeDirectLabels(self):
        for taxonomy in ("category", "style"):
            for time in self.queryTimes:
                for prim in self.queryPrims:
                    with self.subTest(taxonomy=taxonomy, time=time, prim=prim):
                        query = UsdSemantics.LabelsQuery(taxonomy, time)
                        self.assertFalse(query.ComputeUniqueDirectLabels(prim))

    def testComputeInheritedLabels(self):
        for taxonomy in ("category", "style"):
            for time in self.queryTimes:
                for prim in self.queryPrims:
                    with self.subTest(taxonomy=taxonomy, time=time, prim=prim):
                        query = UsdSemantics.LabelsQuery(taxonomy, time)
                        self.assertFalse(query.ComputeUniqueInheritedLabels(prim))

    def testHasDirectLabeled(self):
        for taxonomy, label in (("category", "object"),
                                ("style", "chic")):
            for time in self.queryTimes:
                for prim in self.queryPrims:
                    with self.subTest(taxonomy=taxonomy, label=label,
                                      time=time, prim=prim):
                        query = UsdSemantics.LabelsQuery(taxonomy, time)
                        self.assertFalse(query.HasDirectLabel(prim, label))

    def testHasInheritedLabeled(self):
        for taxonomy, label in (("category", "object"),
                                ("style", "chic")):
            for time in self.queryTimes:
                for prim in self.queryPrims:
                    with self.subTest(taxonomy=taxonomy, label=label,
                                      time=time, prim=prim):
                        query = UsdSemantics.LabelsQuery(taxonomy, time)
                        self.assertFalse(query.HasInheritedLabel(prim, label))


class TestUnlabeledIntermediateDefaultsOnly(unittest.TestCase):
    queryTimes = (Usd.TimeCode.Default(),
                  Usd.TimeCode(1.0),
                  Gf.Interval(-10.0, 10.0))

    style = "style"
    category = "category"
    grandparentStyleLabels = ["big", "large"]
    grandparentCategoryLabels = ["object"]
    childStyleLabels = ["red"]

    def setUp(self):
        """Create a stage with a grandparent and child with style labels
        no labels on the parent prim, and category labels on the grandparent
        prim."""
        self.stage = Usd.Stage.CreateInMemory()
        
        self.grandparent = self.stage.DefinePrim("/Grandparent")
        self.parent = self.stage.DefinePrim("/Grandparent/Parent")
        self.child = self.stage.DefinePrim("/Grandparent/Parent/Child")

        grandparentStyleSchema = UsdSemantics.LabelsAPI.Apply(
            self.grandparent, self.style)
        grandparentStyleSchema.GetLabelsAttr().Set(self.grandparentStyleLabels)

        grandparentCategorySchema = UsdSemantics.LabelsAPI.Apply(
            self.grandparent, self.category)
        grandparentCategorySchema.GetLabelsAttr().Set(
            self.grandparentCategoryLabels)

        childStyleSchema = UsdSemantics.LabelsAPI.Apply(self.child, self.style)
        childStyleSchema.GetLabelsAttr().Set(self.childStyleLabels)

    def testComputeDirectLabels(self):
        expected = {
            self.style : (
                (self.stage.GetPseudoRoot(), []),
                (self.child, self.childStyleLabels),
                (self.parent, []),
                (self.grandparent, self.grandparentStyleLabels)
            ),
            self.category : (
                (self.stage.GetPseudoRoot(), []),
                (self.child, []),
                (self.parent, []),
                (self.grandparent, self.grandparentCategoryLabels)
            )}
        for time in self.queryTimes:
            for taxonomy in expected:
                for prim, labels in expected[taxonomy]:
                    with self.subTest(taxonomy=taxonomy, time=time,
                                      prim=prim, labels=labels):
                        query = UsdSemantics.LabelsQuery(taxonomy, time)
                        self.assertCountEqual(
                            query.ComputeUniqueDirectLabels(prim), labels)

    def testComputeInheritedLabels(self):
        expected = {
            self.style : (
                (self.stage.GetPseudoRoot(), []),
                (self.child, self.grandparentStyleLabels + self.childStyleLabels),
                (self.parent, self.grandparentStyleLabels),
                (self.grandparent, self.grandparentStyleLabels)
            ),
            self.category : (
                (self.stage.GetPseudoRoot(), []),
                (self.child, self.grandparentCategoryLabels),
                (self.parent, self.grandparentCategoryLabels),
                (self.grandparent, self.grandparentCategoryLabels)
            )}
        for time in self.queryTimes:
            for taxonomy in expected:
                for prim, labels in expected[taxonomy]:
                    with self.subTest(taxonomy=taxonomy, time=time,
                                      prim=prim, labels=labels):
                        query = UsdSemantics.LabelsQuery(taxonomy, time)
                        self.assertCountEqual(
                            query.ComputeUniqueInheritedLabels(prim), labels)

    def testHasDirectLabel(self):
        expected = {
            self.style : (
                (self.stage.GetPseudoRoot(), [], ["big", "red", "large"]),
                (self.child, ["red"], ["big", "large"]),
                (self.parent, [], ["big", "large"]),
                (self.grandparent, ["big", "large"], ["red"]),
            ),
            self.category : (
                (self.stage.GetPseudoRoot(), [], ["object"]),
                (self.child, [], ["object"]),
                (self.parent, [], ["object"]),
                (self.grandparent, ["object"], []),
            )
        }
        for time in self.queryTimes:
            for taxonomy in expected:
                for prim, appliedLabels, unappliedLabels in expected[taxonomy]:
                    for appliedLabel in appliedLabels:
                        with self.subTest(taxonomy=taxonomy, time=time,
                                          prim=prim, label=appliedLabel):
                            query = UsdSemantics.LabelsQuery(taxonomy, time)
                            self.assertTrue(
                                query.HasDirectLabel(prim, appliedLabel))
                    for unappliedLabel in unappliedLabels:
                        with self.subTest(taxonomy=taxonomy, time=time,
                                          prim=prim, label=unappliedLabel):
                            query = UsdSemantics.LabelsQuery(taxonomy, time)
                            self.assertFalse(
                                query.HasDirectLabel(prim, unappliedLabel))

    def testHasAncestorLabel(self):
        expected = {
            self.style : (
                (self.stage.GetPseudoRoot(), [], ["big", "red", "large"]),
                (self.child, ["big", "large", "red"], []),
                (self.parent,["big", "large"], ["red"]),
                (self.grandparent, ["big", "large"], ["red"]),
            ),
            self.category : (
                (self.stage.GetPseudoRoot(), [], ["object"]),
                (self.child, ["object"], []),
                (self.parent, ["object"], []),
                (self.grandparent, ["object"], []),
            )
        }
        for time in self.queryTimes:
            for taxonomy in expected:
                for prim, appliedLabels, unappliedLabels in expected[taxonomy]:
                    for appliedLabel in appliedLabels:
                        with self.subTest(taxonomy=taxonomy, time=time,
                                          prim=prim, label=appliedLabel):
                            query = UsdSemantics.LabelsQuery(taxonomy, time)
                            self.assertTrue(
                                query.HasInheritedLabel(prim, appliedLabel))
                    for unappliedLabel in unappliedLabels:
                        with self.subTest(taxonomy=taxonomy, time=time,
                                          prim=prim, label=unappliedLabel):
                            query = UsdSemantics.LabelsQuery(taxonomy, time)
                            self.assertFalse(
                                query.HasInheritedLabel(prim, unappliedLabel))


class TestUnlabeledIntermediateTimeSamplesAndDefaults(unittest.TestCase):
    """Tests a stage with a grandparent and child with default labels only,
    no labels on the parent prim, and both time sampled and default labels on
    the grandparent prim."""

    size = "size"
    grandparentDefaultLabels = ["big", "large"]
    grandparentTimeSampleLabels100 = ["small"]
    grandparentTimeSampleLabels200 = ["tiny"]

    childDefaultLabels = ["teeny"]

    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory()
        
        self.grandparent = self.stage.DefinePrim("/Grandparent")
        self.parent = self.stage.DefinePrim("/Grandparent/Parent")
        self.child = self.stage.DefinePrim("/Grandparent/Parent/Child")

        grandparentSchema = UsdSemantics.LabelsAPI.Apply(
            self.grandparent, self.size)
        grandparentSchema.GetLabelsAttr().Set(
            self.grandparentTimeSampleLabels100, 100)
        grandparentSchema.GetLabelsAttr().Set(
            self.grandparentTimeSampleLabels200, 200)

        childSchema = UsdSemantics.LabelsAPI.Apply(self.child, self.size)
        childSchema.GetLabelsAttr().Set(self.childDefaultLabels)

    def testComputeDirectLabelsBeforeInterval(self):
        expected = [
            (self.stage.GetPseudoRoot(), []),
            (self.child, ["teeny"]),
            (self.parent, []),
            (self.grandparent, ["small"])
        ]

        for prim, labels in expected:
            with self.subTest(prim=prim, labels=labels):
                query = UsdSemantics.LabelsQuery(self.size, Usd.TimeCode(-100))
                self.assertCountEqual(
                    query.ComputeUniqueDirectLabels(prim), labels)

    def testComputeDirectLabelsAfterInterval(self):
        expected = [
            (self.stage.GetPseudoRoot(), []),
            (self.child, ["teeny"]),
            (self.parent, []),
            (self.grandparent, ["tiny"])
        ]

        query = UsdSemantics.LabelsQuery(self.size, Usd.TimeCode(3000))
        for prim, labels in expected:
            with self.subTest(prim=prim, labels=labels):
                self.assertCountEqual(
                    query.ComputeUniqueDirectLabels(prim), labels)

    def testComputeDirectLabelsIncludingAll(self):
        expected = [
            (self.stage.GetPseudoRoot(), []),
            (self.child, ["teeny"]),
            (self.parent, []),
            (self.grandparent, ["tiny", "small"])
        ]

        for prim, labels in expected:
            with self.subTest(prim=prim, labels=labels):
                query = UsdSemantics.LabelsQuery(self.size, Gf.Interval(100, 200))
                self.assertCountEqual(
                    query.ComputeUniqueDirectLabels(prim), labels)

    def testComputeDirectLabelsFullInterval(self):
        expected = [
            (self.stage.GetPseudoRoot(), []),
            (self.child, ["teeny"]),
            (self.parent, []),
            (self.grandparent, ["tiny", "small"])
        ]

        for prim, labels in expected:
            with self.subTest(prim=prim, labels=labels):
                query = UsdSemantics.LabelsQuery(self.size,
                                                 Gf.Interval.GetFullInterval())
                self.assertCountEqual(
                    query.ComputeUniqueDirectLabels(prim), labels)

    def testComputeDirectLabelsSubinterval(self):
        expected = [
            (self.stage.GetPseudoRoot(), []),
            (self.child, ["teeny"]),
            (self.parent, []),
            (self.grandparent, ["small"])
        ]

        for prim, labels in expected:
            with self.subTest(prim=prim, labels=labels):
                query = UsdSemantics.LabelsQuery(self.size, Gf.Interval(150, 175))
                self.assertCountEqual(
                    query.ComputeUniqueDirectLabels(prim), labels)


class TestAncestorPopulation(unittest.TestCase):
    """Tests a stage where parent and grandparent both have labels.

    An early version of LabelsQuery would early exit during ancestor
    population, causing this test case to fail. This exercises
    different access patterns (top down / bottom up) to make sure
    the cache gets populated correctly."""

    size = "size"
    grandparentDefaultLabels = ["big", "large"]
    parentDefaultLabels = ["small"]

    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory()
        
        self.grandparent = self.stage.DefinePrim("/Grandparent")
        self.parent = self.stage.DefinePrim("/Grandparent/Parent")
        self.child = self.stage.DefinePrim("/Grandparent/Parent/Child")

        grandparentSchema = UsdSemantics.LabelsAPI.Apply(
            self.grandparent, self.size)
        grandparentSchema.GetLabelsAttr().Set(self.grandparentDefaultLabels)

        parentSchema = UsdSemantics.LabelsAPI.Apply(self.parent, self.size)
        parentSchema.GetLabelsAttr().Set(self.parentDefaultLabels)

    def testComputeInheritedLabelsUniqueCache(self):
        """Ensure that each query gets a unique cache"""

        expected = [
            (self.stage.GetPseudoRoot(), []),
            (self.grandparent, ["big", "large"]),
            (self.parent, ["small", "big", "large"]),
            (self.child, ["small", "big", "large"]),
        ]

        for prim, labels in expected:
            with self.subTest(prim=prim, labels=labels):
                query = UsdSemantics.LabelsQuery(self.size, Usd.TimeCode.Default())
                self.assertCountEqual(
                    query.ComputeUniqueInheritedLabels(prim), labels)

    def testComputeInheritedLabelsTopDown(self):
        """Share the query cache across tests, traversing top down"""

        expected = [
            (self.stage.GetPseudoRoot(), []),
            (self.grandparent, ["big", "large"]),
            (self.parent, ["small", "big", "large"]),
            (self.child, ["small", "big", "large"]),
        ]

        query = UsdSemantics.LabelsQuery(self.size, Usd.TimeCode.Default())
        for prim, labels in expected:
            with self.subTest(prim=prim, labels=labels):
                self.assertCountEqual(query.ComputeUniqueInheritedLabels(prim), labels)

    def testComputeInheritedLabelsBottomUp(self):
        """Share the query cache across tests, traversing bottom up"""

        expected = [
            (self.stage.GetPseudoRoot(), []),
            (self.grandparent, ["big", "large"]),
            (self.parent, ["small", "big", "large"]),
            (self.child, ["small", "big", "large"]),
        ]

        query = UsdSemantics.LabelsQuery(self.size, Usd.TimeCode.Default())
        for prim, labels in reversed(expected):
            with self.subTest(prim=prim, labels=labels):
                self.assertCountEqual(query.ComputeUniqueInheritedLabels(prim), labels)


if __name__ == "__main__":
    unittest.main()