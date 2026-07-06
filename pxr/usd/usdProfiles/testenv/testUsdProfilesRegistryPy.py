#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest
from pxr import UsdProfiles

QS = UsdProfiles.ProfileRegistry.QueryStatus
PR = UsdProfiles.ProfileRegistry


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


class TestBasicDAG(unittest.TestCase):
    def setUp(self):
        PR._TestClear()
        ok, errors = _load("testBasicDAG.json")
        self.assertTrue(ok)
        self.assertEqual(errors, [])

    def test_HasCapability(self):
        self.assertTrue(PR.HasCapability("usd"))
        self.assertTrue(PR.HasCapability("chain.a"))
        self.assertTrue(PR.HasCapability("merge.point"))
        self.assertFalse(PR.HasCapability("nonexistent"))

    def test_GetAllCapabilities(self):
        all_caps = PR.GetAllCapabilities()
        self.assertEqual(len(all_caps), 6)
        self.assertIn("usd", all_caps)
        self.assertIn("merge.point", all_caps)

    def test_GetPredecessors(self):
        self.assertEqual(PR.GetPredecessors("usd"), [])

        chainAPreds = PR.GetPredecessors("chain.a")
        self.assertEqual(len(chainAPreds), 1)
        self.assertEqual(chainAPreds[0].capability, "usd")
        self.assertEqual(chainAPreds[0].status, QS.ValidPath)

        mergePreds = PR.GetPredecessors("merge.point")
        self.assertEqual(len(mergePreds), 3)
        self.assertTrue(_resultContains(mergePreds, "chain.b"))
        self.assertTrue(_resultContains(mergePreds, "branch.left"))
        self.assertTrue(_resultContains(mergePreds, "branch.right"))
        self.assertEqual(_resultTokensWithStatus(mergePreds, QS.Deprecated), [])

    def test_Metadata(self):
        self.assertEqual(PR.GetDocString("usd"), "Root capability")
        self.assertEqual(PR.GetDocString("merge.point"),
                         "Merges chain and both branches")
        self.assertEqual(PR.GetDisplayName("chain.a"), "Chain A")
        self.assertEqual(PR.GetDisplayName("merge.point"), "Merge Point")
        self.assertEqual(PR.GetStyleForCapability("usd"), "core")
        self.assertEqual(PR.GetStyleForCapability("branch.left"), "extra")
        self.assertEqual(PR.GetSubgraphForCapability("usd"), "Foundation")
        self.assertEqual(PR.GetSubgraphForCapability("merge.point"), "Extensions")
        self.assertFalse(PR.IsProfile("usd"))

    def test_GetCapabilityMetadata(self):
        md = PR.GetCapabilityMetadata("chain.b")
        self.assertIn("docstring", md)
        self.assertEqual(md["docstring"], "Second link in chain")
        self.assertEqual(md["style"], "core")

    def test_GetCapabilityStyles(self):
        styles = PR.GetCapabilityStyles()
        self.assertEqual(len(styles), 2)
        self.assertIn("core", styles)
        self.assertIn("extra", styles)
        # (styles is a list of key names)


class TestDeprecation(unittest.TestCase):
    def setUp(self):
        PR._TestClear()
        ok, errors = _load("testDeprecation.json")
        self.assertTrue(ok)
        self.assertEqual(errors, [])

    def test_HydraV2(self):
        preds = PR.GetPredecessors("hydra.v2")
        self.assertEqual(len(preds), 2)
        self.assertTrue(_resultContains(preds, "hydra.base"))
        self.assertTrue(_resultContains(preds, "hydra.v1"))
        self.assertEqual(_resultStatus(preds, "hydra.base"), QS.ValidPath)
        self.assertEqual(_resultStatus(preds, "hydra.v1"), QS.Deprecated)

    def test_ShadeMaterial(self):
        preds = PR.GetPredecessors("shade.material")
        self.assertEqual(_resultStatus(preds, "shade.look"), QS.Deprecated)
        self.assertEqual(_resultStatus(preds, "shade.base"), QS.ValidPath)

    def test_PipelineMultiDeprecated(self):
        preds = PR.GetPredecessors("pipeline.v3")
        self.assertEqual(len(preds), 3)
        self.assertEqual(_resultStatus(preds, "hydra.base"), QS.ValidPath)
        self.assertEqual(_resultStatus(preds, "pipeline.v1"), QS.Deprecated)
        self.assertEqual(_resultStatus(preds, "pipeline.v2"), QS.Deprecated)
        self.assertEqual(len(_resultTokensWithStatus(preds, QS.Deprecated)), 2)

    def test_CleanLeaf(self):
        preds = PR.GetPredecessors("clean.leaf")
        self.assertEqual(_resultTokensWithStatus(preds, QS.Deprecated), [])


class TestValidation(unittest.TestCase):
    def setUp(self):
        PR._TestClear()

    def test_CycleDetection(self):
        ok, errors = _load("testCycleDetection.json")
        self.assertFalse(ok)
        self.assertTrue(any("cycle" in e for e in errors))

    def test_MissingPredecessor(self):
        ok, errors = _load("testMissingPredecessor.json")
        self.assertFalse(ok)
        self.assertTrue(any("nonexistent.capability" in e for e in errors))

    def test_NoUsdRoot(self):
        ok, errors = _load("testNoUsdRoot.json")
        self.assertFalse(ok)
        self.assertTrue(any("usd" in e for e in errors))

    def test_FileNotFound(self):
        ok, errors = PR._TestLoadFromFile("this_file_does_not_exist.json")
        self.assertFalse(ok)
        self.assertTrue(any("Failed to open" in e for e in errors))


class TestEmptyGraph(unittest.TestCase):
    def setUp(self):
        PR._TestClear()
        ok, errors = _load("testEmpty.json")
        self.assertTrue(ok)
        self.assertEqual(errors, [])

    def test_Empty(self):
        self.assertEqual(PR.GetAllCapabilities(), [])
        self.assertFalse(PR.HasCapability("anything"))
        self.assertEqual(len(PR.GetCapabilityStyles()), 0)


class TestClear(unittest.TestCase):
    def test_Clear(self):
        PR._TestClear()
        ok, _ = _load("testBasicDAG.json")
        self.assertTrue(ok)
        self.assertNotEqual(PR.GetAllCapabilities(), [])
        PR._TestClear()
        self.assertEqual(PR.GetAllCapabilities(), [])
        self.assertFalse(PR.HasCapability("usd"))
        self.assertEqual(len(PR.GetCapabilityStyles()), 0)


class TestMultiPluginMerge(unittest.TestCase):
    def setUp(self):
        PR._TestClear()
        ok, errors = _load("testMultiPluginMerged.json")
        self.assertTrue(ok)
        self.assertEqual(errors, [])

    def test_CrossPluginEdges(self):
        geomPreds = PR.GetPredecessors("vendor.geom")
        self.assertEqual(len(geomPreds), 1)
        self.assertEqual(geomPreds[0].capability, "core.module")
        self.assertEqual(geomPreds[0].status, QS.ValidPath)

        renderPreds = PR.GetPredecessors("vendor.render")
        self.assertEqual(len(renderPreds), 2)
        self.assertTrue(_resultContains(renderPreds, "core.module"))
        self.assertTrue(_resultContains(renderPreds, "vendor.geom"))

    def test_StylesMerged(self):
        styles = PR.GetCapabilityStyles()  # list of key names
        self.assertIn("plugA", styles)
        self.assertIn("plugB", styles)
        self.assertEqual(PR.GetStyleForCapability("core.module"),
                         "plugA")
        self.assertEqual(PR.GetStyleForCapability("vendor.geom"),
                         "plugB")


class TestTransitivePredecessors(unittest.TestCase):
    def setUp(self):
        PR._TestClear()
        ok, _ = _load("testBasicDAG.json")
        self.assertTrue(ok)

    def test_Root(self):
        self.assertEqual(PR.GetTransitivePredecessors("usd"), [])

    def test_ChainA(self):
        trans = PR.GetTransitivePredecessors("chain.a")
        self.assertEqual(len(trans), 1)
        self.assertTrue(_resultContains(trans, "usd"))
        self.assertEqual(_resultStatus(trans, "usd"), QS.ValidPath)

    def test_ChainB(self):
        trans = PR.GetTransitivePredecessors("chain.b")
        self.assertEqual(len(trans), 2)
        self.assertEqual(_resultStatus(trans, "chain.a"), QS.ValidPath)
        self.assertEqual(_resultStatus(trans, "usd"), QS.ValidPath)

    def test_MergePoint(self):
        trans = PR.GetTransitivePredecessors("merge.point")
        self.assertEqual(len(trans), 5)
        for cap in ["chain.b", "chain.a", "usd", "branch.left", "branch.right"]:
            self.assertTrue(_resultContains(trans, cap))
        self.assertEqual(_resultTokensWithStatus(trans, QS.Deprecated), [])

    def test_Unknown(self):
        self.assertEqual(
            PR.GetTransitivePredecessors("nonexistent"), [])


class TestHasPredecessor(unittest.TestCase):
    def setUp(self):
        PR._TestClear()

    def test_BasicDAG(self):
        ok, _ = _load("testBasicDAG.json")
        self.assertTrue(ok)
        self.assertEqual(PR.HasPredecessor("chain.a", "usd"), QS.ValidPath)
        self.assertEqual(PR.HasPredecessor("chain.b", "usd"), QS.ValidPath)
        self.assertEqual(PR.HasPredecessor("merge.point", "usd"), QS.ValidPath)
        self.assertEqual(PR.HasPredecessor("usd", "chain.a"), QS.NoPath)
        self.assertEqual(PR.HasPredecessor("usd", "usd"), QS.NoPath)
        self.assertEqual(PR.HasPredecessor("nonexistent", "usd"), QS.NoPath)

    def test_Deprecation(self):
        ok, _ = _load("testDeprecation.json")
        self.assertTrue(ok)
        self.assertEqual(PR.HasPredecessor("hydra.v1", "hydra.base"), QS.ValidPath)
        self.assertEqual(PR.HasPredecessor("hydra.v2", "hydra.v1"), QS.Deprecated)
        self.assertEqual(PR.HasPredecessor("hydra.v2", "hydra.base"), QS.DeprecationConflict)
        self.assertEqual(PR.HasPredecessor("clean.leaf", "hydra.v1"), QS.Deprecated)
        self.assertEqual(PR.HasPredecessor("clean.leaf", "hydra.base"), QS.DeprecationConflict)



class TestUnknownCapabilityQueries(unittest.TestCase):
    def setUp(self):
        PR._TestClear()
        ok, _ = _load("testBasicDAG.json")
        self.assertTrue(ok)

    def _assertAllEmpty(self):
        bogus = "no.such.capability"
        empty = ""
        for tok in (bogus, empty):
            self.assertFalse(PR.HasCapability(tok))
            self.assertEqual(PR.GetPredecessors(tok), [])
            self.assertEqual(PR.GetTransitivePredecessors(tok), [])
            self.assertEqual(PR.HasPredecessor(tok, "usd"), QS.NoPath)
            self.assertEqual(PR.HasPredecessor("chain.b", tok), QS.NoPath)
            self.assertEqual(PR.GetDocString(tok), "")
            self.assertEqual(PR.GetDisplayName(tok), "")
            self.assertEqual(PR.GetStyleForCapability(tok), "")
            self.assertEqual(PR.GetSubgraphForCapability(tok), "")
            self.assertEqual(len(PR.GetCapabilityMetadata(tok)), 0)

    def test_WithLoadedGraph(self):
        self._assertAllEmpty()

    def test_AfterClear(self):
        PR._TestClear()
        self._assertAllEmpty()


class TestParseCapabilityVersion(unittest.TestCase):
    def setUp(self):
        PR._TestClear()

    def test_Unversioned(self):
        base, ver = PR.ParseCapabilityVersion("usd.physics")
        self.assertEqual(base, "usd.physics")
        self.assertEqual(ver, 0)

    def test_StandardSuffix(self):
        base, ver = PR.ParseCapabilityVersion("usd.physics_v2")
        self.assertEqual(base, "usd.physics")
        self.assertEqual(ver, 2)

    def test_HigherVersion(self):
        base, ver = PR.ParseCapabilityVersion("yoyo.tool_v10")
        self.assertEqual(base, "yoyo.tool")
        self.assertEqual(ver, 10)

    def test_VInMiddle(self):
        base, ver = PR.ParseCapabilityVersion("usd._v2.geom")
        self.assertEqual(base, "usd._v2.geom")
        self.assertEqual(ver, 0)

    def test_VNoDigits(self):
        base, ver = PR.ParseCapabilityVersion("usd.foo_v")
        self.assertEqual(base, "usd.foo_v")
        self.assertEqual(ver, 0)

    def test_Empty(self):
        base, ver = PR.ParseCapabilityVersion("")
        self.assertEqual(base, "")
        self.assertEqual(ver, 0)

    def test_VFollowedByNonDigit(self):
        base, ver = PR.ParseCapabilityVersion("usd.foo_v2x")
        self.assertEqual(base, "usd.foo_v2x")
        self.assertEqual(ver, 0)


class TestResolveCapability(unittest.TestCase):
    def setUp(self):
        PR._TestClear()
        ok, errors = _load("testVersioning.json")
        self.assertTrue(ok)
        self.assertEqual(errors, [])

    def test_Resolve(self):
        self.assertEqual(PR.ResolveCapability("usd.physics"),
                         "usd.physics_v2")
        self.assertEqual(PR.ResolveCapability("usd.physics_v2"),
                         "usd.physics_v2")
        self.assertEqual(PR.ResolveCapability("yoyo.tool_v2"),
                         "yoyo.tool_v5")
        self.assertEqual(PR.ResolveCapability("yoyo.tool_v5"),
                         "yoyo.tool_v5")
        self.assertEqual(PR.ResolveCapability("usd.geom"),
                         "usd.geom")
        self.assertEqual(PR.ResolveCapability("usd"), "usd")
        self.assertEqual(PR.ResolveCapability("no.such.cap"), "")
        self.assertEqual(PR.ResolveCapability(""), "")


class TestVersioningQueries(unittest.TestCase):
    def setUp(self):
        PR._TestClear()
        ok, errors = _load("testVersioning.json")
        self.assertTrue(ok)
        self.assertEqual(errors, [])

    def test_HasPredecessorVersioned(self):
        self.assertEqual(PR.HasPredecessor("profile.sim", "usd.physics"), QS.ValidPath)
        self.assertEqual(PR.HasPredecessor("profile.sim", "yoyo.tool_v2"), QS.NoPath)
        self.assertEqual(PR.HasPredecessor("usd.physics", "usd.geom"), QS.DeprecationConflict)
        self.assertEqual(PR.HasPredecessor("usd.physics", "usd"), QS.DeprecationConflict)

    def test_CoversCapabilitiesVersioned(self):
        required = ["usd.geom", "usd.physics"]
        status, results = PR.CoversCapabilities("profile.sim", required)
        self.assertEqual(status, QS.DeprecationConflict)
        self.assertEqual(len(results), 2)
        self.assertEqual(results[0].capability, "usd.geom")
        self.assertEqual(results[0].status, QS.DeprecationConflict)
        self.assertEqual(results[1].capability, "usd.physics_v2")
        self.assertEqual(results[1].status, QS.ValidPath)

    def test_UnversionedPerspective(self):
        status, _ = PR.CoversCapabilities("usd.physics", ["usd.geom"])
        self.assertEqual(status, QS.DeprecationConflict)



if __name__ == '__main__':
    unittest.main()


