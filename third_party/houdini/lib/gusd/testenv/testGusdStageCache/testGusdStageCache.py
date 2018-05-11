#
# Copyright 2018 Pixar
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

# XXX: The try-except block here is a workaround for
# Pixar-internal build system issues.
try:
    from pxr import Gusd
except ImportError:
    import Gusd

from pxr import Sdf, Tf
import unittest


def FindOrOpen(cache, path, **kwargs):
    stage = cache.FindOrOpen(path, **kwargs)
    assert stage

    assert cache.Find(path, **kwargs) == stage

    # Repeating the same query should give the same answer.
    assert cache.FindOrOpen(path, **kwargs) == stage

    # The cached stage should be discoverable by FindStages().
    assert stage in cache.FindStages([path])

    return stage


def GetPrim(cache, path, primPath, **kwargs):
    primPath = Sdf.Path(primPath)

    prim,stage = cache.GetPrim(path, primPath, **kwargs)
    assert prim
    assert stage
    assert prim.GetPath() == primPath
    
    # Repeating the same query should give the same answer.
    prim2,stage2 = cache.GetPrim(path, primPath, **kwargs)
    assert prim2 == prim
    assert stage2 == stage

    # The cached stage should be discoverable by FindStages().
    assert stage in cache.FindStages([path])
    
    return (prim,stage)


class TestGusdStageCache(unittest.TestCase):

    def test_FindOrOpen(self):
        """
        Tests basic Find() and FindOrOpen() queries.
        """
        path = "test.usda"
        cache = Gusd.StageCache()

        assert not cache.Find(path)

        stage = FindOrOpen(cache, path)

        # Stages use LoadAll by default.
        # Querying with LoadNone should give a different stage.

        assert not cache.Find(path, opts=Gusd.StageOpts.LoadNone())

        stage2 = FindOrOpen(cache, path, opts=Gusd.StageOpts.LoadNone())
        assert stage2
        assert stage2 != stage

        # Likewise, stage edits also form a unique part of the
        # cache key, and should return different stages.
        edit = Gusd.StageBasicEdit.New()
        edit.SetVariants([Sdf.Path("/foo{a=b}")])

        assert not cache.Find(path, edit=edit)
        stage3 = FindOrOpen(cache, path, edit=edit)
        assert stage3
        assert stage3 != stage and stage3 != stage2
        

    def test_StageMaskComponentExpansion(self):
        """
        Tests kind-based population mask expansion.
        """
        if not Tf.GetEnvSetting("GUSD_STAGEMASK_ENABLE"):
            return

        path = "test.usda"
        cache = Gusd.StageCache()

        prim,stage = GetPrim(cache, path, "/Root/Component/A")

        # Mask should have been expanded to include full /Root/Component.
        assert Sdf.Path("/Root/Component") in \
            stage.GetPopulationMask().GetPaths()

        # Make sure the mask composed prims as expected.
        assert stage.GetPrimAtPath("/Root/Component/B")
        assert not stage.GetPrimAtPath("/Root/NonReferencedPrim")
        assert not stage.GetPrimAtPath("/Root2")

        if not Tf.GetEnvSetting("GUSD_STAGEMASK_EXPANDRELS"):
            assert not stage.GetPrimAtPath("/Root/ExternalDependency")


    def test_StageMaskDependencyExpansion(self):
        """
        Tests dependency based (rels, attr connections) population expansion.
        """
        if not (Tf.GetEnvSetting("GUSD_STAGEMASK_ENABLE") and
                Tf.GetEnvSetting("GUSD_STAGEMASK_EXPANDRELS")):
            return

        path = "test.usda"
        cache = Gusd.StageCache()

        prim,stage = GetPrim(cache, path, "/Root/Component/B")

        # External prim should have been detected as a dependency and 
        # included in the mask.
        assert Sdf.Path("/Root/ExternalDependency") in \
            stage.GetPopulationMask().GetPaths()
        
        # Make sure the mask composed prims as expected.
        assert stage.GetPrimAtPath("/Root/ExternalDependency")
        assert not stage.GetPrimAtPath("/Root/NonReferencedPrim")
        assert not stage.GetPrimAtPath("/Root2")


    def test_StageMaskDescendantComponentExpansion(self):
        """
        Tests kind-based population mask expansion when expanding
        a primitve that contains a descendant that is a component.
        """
        if not Tf.GetEnvSetting("GUSD_STAGEMASK_ENABLE"):
            return

        path = "test.usda"
        cache = Gusd.StageCache()

        prim,stage = GetPrim(cache, path, "/Root")

        # Since /Root contains a component prim, expansion should
        # have gone no further than /Root.

        assert stage.GetPopulationMask().GetPaths() == [Sdf.Path("/Root")]

        # Make sure the mask composed prims as expected.
        assert stage.GetPrimAtPath("/Root")
        assert not stage.GetPrimAtPath("/Root2")


    def test_StageMaskRootExpansion(self):
        """
        Test the fallback behavior of expanding to the root in
        componentless hierarchcies.
        """
        if not Tf.GetEnvSetting("GUSD_STAGEMASK_ENABLE"):
            return

        path = "test.usda"
        cache = Gusd.StageCache()

        prim,stage = GetPrim(cache, path, "/Root/ComponentlessHierarchy")

        # Since there's no component as an ancestor or descendant
        # of this prim, we should have expanded out to the full stage.
        assert stage.GetPopulationMask().GetPaths() == \
            [Sdf.Path.absoluteRootPath]

        # Make sure the mask composed prims as expected.
        assert stage.GetPrimAtPath("/Root")
        assert stage.GetPrimAtPath("/Root2")


    def test_StageMaskPreemption(self):
        """
        Test that the presence of a full stage on the cache preempts
        the use of masked stage queries.
        """
        if not Tf.GetEnvSetting("GUSD_STAGEMASK_ENABLE"):
            return

        path = "test.usda"
        cache = Gusd.StageCache()

        # Pre-populate a complete stage on the cache.
        stage = FindOrOpen(cache, path)

        # Individual prim queries should now return the same stage.
        assert GetPrim(cache, path, "/Root/Component")[1] == stage
        assert GetPrim(cache, path, "/Root/NonReferencedPrim")[1] == stage
        assert GetPrim(cache, path, "/Root/ExternalDependency")[1] == stage
        assert GetPrim(cache, path, "/Root/ModelWithVariants")[1] == stage


    def test_GetPrim(self):
        """
        Tests basic stage mask queries.
        """
        path = "test.usda"
        cache = Gusd.StageCache()

        prim,stage = GetPrim(cache, path, "/Root")

        # Querying any descendant prims should give the same stage,
        # regardless of whatever masking behavior is enabled.
        assert GetPrim(cache, path, "/Root/Component")[1] == stage
        assert GetPrim(cache, path, "/Root/NonReferencedPrim")[1] == stage
        assert GetPrim(cache, path, "/Root/ExternalDependency")[1] == stage
        assert GetPrim(cache, path, "/Root/ModelWithVariants")[1] == stage


    def test_GetPrims(self):
        """
        Tests cache's GetPrims() methods for pulling in multiple prims
        across different stages, with different edits.
        """

        cache = Gusd.StageCache()
        
        # Input spec has arbitrary ordering, and a mix of complete and
        # incomplete specs (null file or prim paths)
        paths = ["test2.usda", "", "test2.usda", "test3.usda", "test2.usda", "test3.usda"]
        primPaths = [Sdf.Path(), "/foo", "/A", "/C", "/B", "/D"]

        prims = cache.GetPrims(paths, primPaths)

        self.assertEqual(len(prims), len(primPaths))

        # First two elems should be invalid (this is not an error!)
        self.assertFalse(prims[0])
        self.assertFalse(prims[1])

        # The remaining prims should be present
        self.assertTrue(all(prims[2:]))

        # Validate that all *valid* prims are what we expect.
        validPrims = prims[2:]
        validPrimPaths = primPaths[2:]
        self.assertTrue(
            all(validPrims[i].GetPath() == Sdf.Path(validPrimPaths[i])
                for i in xrange(len(validPrims))))

        # Prims should have been batched during loading, so prims referring
        # to the same stage on the input spec should be referencing
        # the same stage. Note that this is only true on a fresh cache,
        # since some prims may otherwise refer to previously cached stages.
        self.assertEqual(prims[2].GetStage(), prims[4].GetStage())
        self.assertEqual(prims[3].GetStage(), prims[5].GetStage())


    def test_GetPrims_StageMaskPreemption(self):
        """
        Test that the presence of a full stage on the cache preempts
        the use of masked stage queries, using the GetPrims() method.
        """

        cache = Gusd.StageCache()

        stage = cache.FindOrOpen("test2.usda")

        paths = ["test2.usda", "test2.usda"]
        primPaths = ["/A", "/B"]

        prims = cache.GetPrims(paths, primPaths)

        self.assertTrue(all(p.GetStage() == stage) for p in prims)
        

    def test_GetPrimWithVariants(self):
        """
        Tests queries of prims with variant selections.
        """
        path = "test.usda"
        cache = Gusd.StageCache()

        primPath = Sdf.Path("/Root/ModelWithVariants")
        variantNames = ("A", "B", "C")

        prims = set()
        for name in variantNames:
            variants = primPath.AppendVariantSelection("var",name)

            prim,stage = cache.GetPrimWithVariants(path, primPath,
                                                   variants=variants)
            assert prim.GetVariantSets().GetVariantSelection("var") == name

            prims.add(prim)

        # Should have ended up with unique prims, each on their own stage.
        # And if that is true, no prims will have expired.
        assert(all(prims))


    def test_Clear(self):
        """
        Tests cache clearing.
        """
        path = "test.usda"
        cache = Gusd.StageCache()

        stage = FindOrOpen(cache, path)
        assert stage

        stage2 = cache.Find(path)

        cache.Clear(["test.usda"])
        # should not longer find the stage on the cache.
        assert not cache.Find("test.usda")
        assert not cache.FindStages(["test.usda"])
        # but our stage variable should still be valid.
        assert stage

        if Tf.GetEnvSetting("GUSD_STAGEMASK_ENABLE"):
            # Make sure this clearing behavior extends to masked stages.

            prim,primStage = GetPrim(cache, path, "/Root/Component")

            cache.Clear(["test.usda"])

            assert not cache.Find("test.usda")
            assert not cache.FindStages(["test.usda"])

            assert prim
            assert primStage

            # the above primStage should be the only stage reference;
            # release it, and our prim should expire.
            primStage = None
            assert not prim


    def _TestLoadWithInvalidPrimPath(self, cache, path, primPath):
        # No matter how we attempt to load this prim, we should
        # always get back an invalid prim, and no stage should be loaded.

        # GetPrim()
        prim,stage = cache.GetPrim(path, primPath)

        self.assertFalse(prim)
        self.assertFalse(stage)
        self.assertFalse(cache.FindStages([path]))

        # GetPrimWithVariants()
        prim,stage = cache.GetPrimWithVariants(path, primPath,
                                               Sdf.Path("/foo{a=b}"))
        self.assertFalse(prim)
        self.assertFalse(stage)
        self.assertFalse(cache.FindStages([path]))

        # GetPrims()
        prims = cache.GetPrims([path], [primPath])
        self.assertFalse(any(prims))
        self.assertFalse(cache.FindStages([path]))


    def test_UnsafePrimPaths(self):
        """
        Tests that the cache is well-behaved given different types of
        invalid prim paths.
        """

        path = "test.usda"
        
        cache = Gusd.StageCache()

        # Empty path.
        self._TestLoadWithInvalidPrimPath(cache, path, Sdf.Path())

        # Relative path.
        # These are dangerous because algorithms that traverse parent paths
        # may have infinite loops given relative paths (eg., because
        # recursively calling GetParentPath() on a relative path will always
        #produce a new path).
        self._TestLoadWithInvalidPrimPath(cache, path, Sdf.Path("foo"))
        
        

if __name__ == "__main__":
    unittest.main()
