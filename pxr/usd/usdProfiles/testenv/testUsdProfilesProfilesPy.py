#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest
from pxr import Usd, UsdProfiles

QS = UsdProfiles.ProfileRegistry.QueryStatus
PR = UsdProfiles.ProfileRegistry
CA = UsdProfiles.ClaimsAPI


def _load(filename):
    return PR._TestLoadFromFile(filename)

def _resultContains(results, cap):
    return any(r.capability == cap for r in results)

def _resultStatus(results, cap):
    for r in results:
        if r.capability == cap:
            return r.status
    raise AssertionError(f"Token '{cap}' not found in results")

def _resultTokensWithStatus(results, status):
    return [r.capability for r in results if r.status == status]


class TestGetAllProfiles(unittest.TestCase):
    """GetAllProfiles() returns only capabilities tagged isProfile=true."""

    def setUp(self):
        PR._TestClear()
        ok, errors = _load("testIsProfile.json")
        self.assertTrue(ok)
        self.assertEqual(errors, [])

    def test_ReturnsOnlyProfileNodes(self):
        profiles = PR.GetAllProfiles()
        self.assertEqual(len(profiles), 2)
        self.assertIn("usd.core.26.03", profiles)
        self.assertIn("usd.core.26.11", profiles)

    def test_NonProfilesExcluded(self):
        profiles = PR.GetAllProfiles()
        self.assertNotIn("usd", profiles)
        self.assertNotIn("usd.geom", profiles)
        self.assertNotIn("usd.geom.mesh", profiles)

    def test_EmptyRegistry(self):
        PR._TestClear()
        ok, errors = _load("testEmpty.json")
        self.assertTrue(ok)
        self.assertEqual(PR.GetAllProfiles(), [])

    def test_SubsetOfAllCapabilities(self):
        profiles = set(PR.GetAllProfiles())
        all_caps = set(PR.GetAllCapabilities())
        self.assertTrue(profiles.issubset(all_caps))
        self.assertLess(len(profiles), len(all_caps))


class TestIsProfile(unittest.TestCase):
    """Factored from testUsdProfilesRegistry: IsProfile() reflects isProfile flag."""

    def setUp(self):
        PR._TestClear()
        ok, errors = _load("testIsProfile.json")
        self.assertTrue(ok)
        self.assertEqual(errors, [])

    def test_ProfileNodes(self):
        self.assertTrue(PR.IsProfile("usd.core.26.03"))
        self.assertTrue(PR.IsProfile("usd.core.26.11"))

    def test_NonProfileNodes(self):
        self.assertFalse(PR.IsProfile("usd"))
        self.assertFalse(PR.IsProfile("usd.geom"))
        self.assertFalse(PR.IsProfile("usd.geom.mesh"))
        self.assertFalse(PR.IsProfile("no.such.capability"))
        self.assertFalse(PR.IsProfile(""))

    def test_NormalQueriesUnaffected(self):
        self.assertTrue(PR.HasCapability("usd.core.26.03"))
        self.assertTrue(PR.HasCapability("usd.core.26.11"))
        self.assertEqual(PR.HasPredecessor("usd.core.26.03", "usd"), QS.ValidPath)
        self.assertEqual(
            PR.HasPredecessor("usd.core.26.11", "usd.core.26.03"), QS.Deprecated)


class TestCoversCapabilities(unittest.TestCase):
    """Factored from testUsdProfilesRegistry: full CoversCapabilities contract."""

    def setUp(self):
        PR._TestClear()
        ok, errors = _load("testProfileVersions.json")
        self.assertTrue(ok)
        self.assertEqual(errors, [])

    def test_AllValid(self):
        required = ["usd.geom.mesh", "usd"]
        status, results = PR.CoversCapabilities("profile.25.11", required)
        self.assertEqual(status, QS.ValidPath)
        self.assertEqual(len(results), 2)
        self.assertEqual(results[0].capability, "usd.geom.mesh")
        self.assertEqual(results[0].status, QS.ValidPath)
        self.assertEqual(results[1].capability, "usd")
        self.assertEqual(results[1].status, QS.ValidPath)

    def test_MixedDeprecation(self):
        required = ["usd.geom.nurbs", "usd.geom.mesh"]
        status, results = PR.CoversCapabilities("profile.26.08", required)
        self.assertEqual(status, QS.DeprecationConflict)
        self.assertEqual(results[0].capability, "usd.geom.nurbs")
        self.assertEqual(results[0].status, QS.ValidPath)
        self.assertEqual(results[1].capability, "usd.geom.mesh")
        self.assertEqual(results[1].status, QS.DeprecationConflict)

    def test_Unreachable(self):
        required = ["usd.geom.mesh", "no.such.cap"]
        status, results = PR.CoversCapabilities("profile.25.11", required)
        self.assertEqual(status, QS.NoPath)
        self.assertEqual(len(results), 2)
        self.assertEqual(results[0].status, QS.ValidPath)
        self.assertEqual(results[1].status, QS.NoPath)

    def test_EmptyRequired(self):
        status, _ = PR.CoversCapabilities("profile.25.11", [])
        self.assertEqual(status, QS.NoPath)

    def test_UnknownPerspective(self):
        status, _ = PR.CoversCapabilities("no.such.profile", ["usd.geom.mesh"])
        self.assertEqual(status, QS.NoPath)

    def test_DeprecatedOnly(self):
        required = ["profile.25.11"]
        status, results = PR.CoversCapabilities("profile.26.08", required)
        self.assertEqual(status, QS.Deprecated)
        self.assertEqual(results[0].status, QS.Deprecated)

    def test_Excepted(self):
        required = ["usd.geom.mesh", "usd"]
        excepted = ["usd.geom.mesh"]
        status, results = PR.CoversCapabilities("profile.25.11", required, excepted)
        self.assertEqual(status, QS.NoPath)
        self.assertEqual(results[0].capability, "usd.geom.mesh")
        self.assertEqual(results[0].status, QS.Excepted)
        self.assertEqual(results[1].capability, "usd")
        self.assertEqual(results[1].status, QS.ValidPath)

    def test_ExceptedNotInRequired(self):
        required = ["usd.geom.mesh"]
        excepted = ["usd.geom.nurbs"]
        status, _ = PR.CoversCapabilities("profile.25.11", required, excepted)
        self.assertEqual(status, QS.ValidPath)


class TestDeprecationPerspective(unittest.TestCase):
    """Factored from testUsdProfilesRegistry: deprecation is perspective-aware."""

    def setUp(self):
        PR._TestClear()
        ok, errors = _load("testProfileVersions.json")
        self.assertTrue(ok)
        self.assertEqual(errors, [])

    def test_Perspective(self):
        self.assertEqual(
            PR.HasPredecessor("profile.25.11", "usd.geom.mesh"), QS.ValidPath)
        self.assertEqual(
            PR.HasPredecessor("profile.26.08", "profile.25.11"), QS.Deprecated)
        self.assertEqual(
            PR.HasPredecessor("profile.26.08", "usd.geom.nurbs"), QS.ValidPath)
        self.assertEqual(
            PR.HasPredecessor("profile.26.08", "usd.geom.mesh"), QS.DeprecationConflict)


class TestIsCompatibleWith(unittest.TestCase):
    """End-to-end ClaimsAPI.IsCompatibleWith via in-memory stage + loaded registry."""

    def setUp(self):
        PR._TestClear()
        ok, errors = _load("testProfileVersions.json")
        self.assertTrue(ok)
        self.assertEqual(errors, [])
        self.stage = Usd.Stage.CreateInMemory()
        prim = self.stage.DefinePrim("/TestPrim")
        self.api = CA.Apply(prim)
        self.api.SetCapabilityUsage("usd.geom.mesh", "hard")
        self.api.SetCapabilityUsage("usd", "hard")

    def test_ProfileNotDeclared(self):
        status, _ = self.api.IsCompatibleWith("profile.25.11")
        self.assertEqual(status, QS.NoPath)

    def test_ValidPath(self):
        self.api.SetProfileCompatible("profile.25.11")
        status, results = self.api.IsCompatibleWith("profile.25.11")
        self.assertEqual(status, QS.ValidPath)
        self.assertEqual(len(results), 2)
        caps = [r.capability for r in results]
        self.assertIn("usd.geom.mesh", caps)
        self.assertIn("usd", caps)
        self.assertTrue(all(r.status == QS.ValidPath for r in results))

    def test_WithExceptions(self):
        self.api.SetProfileCompatibleWithExceptions(
            "profile.26.08", ["usd.geom.mesh"])
        status, results = self.api.IsCompatibleWith("profile.26.08")
        self.assertEqual(status, QS.NoPath)
        excepted = [r.capability for r in results if r.status == QS.Excepted]
        self.assertIn("usd.geom.mesh", excepted)

    def test_NoCaps_NoPath(self):
        prim2 = self.stage.DefinePrim("/NoCaps")
        api2 = CA.Apply(prim2)
        api2.SetProfileCompatible("profile.25.11")
        status, _ = api2.IsCompatibleWith("profile.25.11")
        self.assertEqual(status, QS.NoPath)


if __name__ == '__main__':
    unittest.main()
