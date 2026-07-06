#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest
from pxr import Usd, UsdProfiles

CA = UsdProfiles.ClaimsAPI


def _makeApi():
    stage = Usd.Stage.CreateInMemory()
    prim = stage.DefinePrim("/TestPrim")
    api = CA.Apply(prim)
    return stage, api


class TestSchemaApply(unittest.TestCase):

    def test_CanApplyAndApply(self):
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim("/P")
        result, whyNot = CA.CanApply(prim)
        self.assertTrue(result)
        self.assertEqual(whyNot, "")
        api = CA.Apply(prim)
        self.assertTrue(api)

    def test_Get(self):
        stage = Usd.Stage.CreateInMemory()
        prim = stage.DefinePrim("/P")
        CA.Apply(prim)
        got = CA.Get(stage, "/P")
        self.assertTrue(got)
        self.assertEqual(got.GetPrim(), prim)


class TestCapabilityUsages(unittest.TestCase):

    def setUp(self):
        self.stage, self.api = _makeApi()

    def test_InitiallyEmpty(self):
        self.assertEqual(self.api.GetCapabilityUsages(), {})
        self.assertEqual(self.api.GetCapabilityUsage("usd.geom.mesh"), "")

    def test_SetAndGetSingle(self):
        self.api.SetCapabilityUsage("usd.geom.mesh", "hard")
        self.assertEqual(self.api.GetCapabilityUsage("usd.geom.mesh"), "hard")

    def test_SetMultiple(self):
        self.api.SetCapabilityUsage("usd.geom.mesh", "hard")
        self.api.SetCapabilityUsage("usd.shading", "soft")
        usages = self.api.GetCapabilityUsages()
        self.assertEqual(len(usages), 2)
        self.assertEqual(usages["usd.geom.mesh"], "hard")
        self.assertEqual(usages["usd.shading"], "soft")

    def test_Overwrite(self):
        self.api.SetCapabilityUsage("usd.geom.mesh", "hard")
        self.api.SetCapabilityUsage("usd.geom.mesh", "enhancement")
        self.assertEqual(self.api.GetCapabilityUsage("usd.geom.mesh"), "enhancement")

    def test_BulkReplace(self):
        self.api.SetCapabilityUsage("usd.geom.mesh", "hard")
        self.api.SetCapabilityUsages({"usd.physics": "hard"})
        usages = self.api.GetCapabilityUsages()
        self.assertEqual(len(usages), 1)
        self.assertIn("usd.physics", usages)
        self.assertNotIn("usd.geom.mesh", usages)

    def test_UnknownCapabilityEmpty(self):
        self.assertEqual(self.api.GetCapabilityUsage("no.such.cap"), "")


class TestProfileCompatibilityDeclarations(unittest.TestCase):

    def setUp(self):
        self.stage, self.api = _makeApi()

    def test_InitiallyEmpty(self):
        self.assertEqual(self.api.GetCompatibleProfiles(), [])
        self.assertEqual(self.api.GetProfileExceptions("profile.a"), [])

    def test_SetCompatible(self):
        self.api.SetProfileCompatible("profile.a")
        profiles = self.api.GetCompatibleProfiles()
        self.assertIn("profile.a", profiles)
        self.assertEqual(self.api.GetProfileExceptions("profile.a"), [])

    def test_SetCompatibleWithExceptions(self):
        self.api.SetProfileCompatibleWithExceptions(
            "profile.b", ["usd.geom.mesh", "usd.shading"])
        self.assertIn("profile.b", self.api.GetCompatibleProfiles())
        ex = self.api.GetProfileExceptions("profile.b")
        self.assertEqual(len(ex), 2)
        self.assertIn("usd.geom.mesh", ex)
        self.assertIn("usd.shading", ex)

    def test_OverwriteEntry(self):
        self.api.SetProfileCompatible("profile.a")
        self.api.SetProfileCompatibleWithExceptions("profile.a", ["usd.geom.hair"])
        ex = self.api.GetProfileExceptions("profile.a")
        self.assertEqual(len(ex), 1)
        self.assertIn("usd.geom.hair", ex)

    def test_Clear(self):
        self.api.SetProfileCompatible("profile.a")
        self.api.SetProfileCompatibleWithExceptions("profile.b", ["usd.geom.mesh"])
        self.api.ClearProfileCompatibility("profile.a")
        profiles = self.api.GetCompatibleProfiles()
        self.assertEqual(len(profiles), 1)
        self.assertIn("profile.b", profiles)
        self.assertEqual(self.api.GetProfileExceptions("profile.a"), [])

    def test_ExceptionsUnknownProfile(self):
        self.assertEqual(self.api.GetProfileExceptions("never.declared"), [])


class TestProfilesInfoRoundTrip(unittest.TestCase):

    def setUp(self):
        self.stage, self.api = _makeApi()

    def test_InitiallyEmpty(self):
        self.assertEqual(self.api.GetProfilesInfo(), {})

    def test_RoundTrip(self):
        self.api.SetProfilesInfo(
            {"capabilityUsages": {"usd.geom.mesh": "hard"}})
        got = self.api.GetProfilesInfo()
        self.assertIn("capabilityUsages", got)

    def test_SharedStorage(self):
        # High-level write is visible to raw read — same underlying storage
        self.api.SetCapabilityUsage("usd.geom.mesh", "hard")
        got = self.api.GetProfilesInfo()
        self.assertIn("capabilityUsages", got)
        self.assertIn("usd.geom.mesh", got["capabilityUsages"])


if __name__ == '__main__':
    unittest.main()
