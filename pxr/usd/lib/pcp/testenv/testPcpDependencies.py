#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

"""
This test exercises the Pcp dependency API in specific scenarios
that represent interesting cases that it needs to handle.

It cherry picks particular examples from the Pcp museum
to exercise.
"""

from pxr import Sdf, Pcp, Tf
import os, unittest

class TestPcpDependencies(unittest.TestCase):
    # Fully populate a PcpCache so we can examine its full dependencies.
    def _ForcePopulateCache(self, cache):
        def walk(cache, path=Sdf.Path('/')):
            primIndex, primIndexErrors = cache.ComputePrimIndex(path)
            childNames, prohibitedChildNames = primIndex.ComputePrimChildNames()
            for childName in childNames:
                walk(cache, path.AppendChild(childName))
        walk(cache)

    # Wrapper to query deps on a given site.
    def _FindSiteDeps(self, rootLayerPath, siteLayerPath, sitePath,
                      depMask = Pcp.DependencyTypeAnyNonVirtual,
                      recurseOnSite = False,
                      recurseOnIndex = False):
        sitePath = Sdf.Path(sitePath)
        self.assertFalse(sitePath.isEmpty)
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerPath)
        self.assertTrue(rootLayer)
        siteLayer = Sdf.Layer.FindOrOpen(siteLayerPath)
        self.assertTrue(siteLayer)
        cache = Pcp.Cache( Pcp.LayerStackIdentifier(rootLayer) )
        siteLayerStack = cache.ComputeLayerStack(
            Pcp.LayerStackIdentifier(siteLayer))[0]
        self.assertTrue(siteLayerStack)
        self._ForcePopulateCache(cache)
        return cache.FindSiteDependencies( siteLayerStack, sitePath, depMask,
                                           recurseOnSite, recurseOnIndex,
                                           False )

    # Helper to compare dep results and print diffs.
    def _AssertDepsEqual(self, deps_lhs, deps_rhs):
        def _LessThan(a, b):
            if a.indexPath < b.indexPath:
                return -1
            if a.indexPath > b.indexPath:
                return 1
            if a.sitePath < b.sitePath:
                return -1
            if a.sitePath > b.sitePath:
                return 1
            return cmp(id(a.mapFunc), id(b.mapFunc))
        a = sorted(deps_lhs, _LessThan)
        b = sorted(deps_rhs, _LessThan)
        if a != b:
            print 'Only in a:'
            for i in a:
                if i not in b:
                    print i
            print 'Only in b:'
            for i in b:
                if i not in a:
                    print i
            self.assertEqual(a,b)

    def test_Basic(self):
        ########################################################################
        # A basic reference arc.
        #
        self._AssertDepsEqual( self._FindSiteDeps(
             'BasicReference/root.sdf',
             'BasicReference/ref.sdf',
             '/PrimA'),
            [
            Pcp.Dependency(
                '/PrimWithReferences',
                '/PrimA',
                Pcp.MapFunction({'/PrimA': '/PrimWithReferences'})
                )
            ])

        ########################################################################
        # A reference to a defaultPrim.
        #
        self._AssertDepsEqual( self._FindSiteDeps(
             'BasicReference/root.sdf',
             'BasicReference/defaultRef.sdf',
             '/Default'),
            [
            Pcp.Dependency(
                '/PrimWithDefaultReferenceTarget',
                '/Default',
                Pcp.MapFunction({
                    '/Default': '/PrimWithDefaultReferenceTarget'})
                )
            ])

        ########################################################################
        # A reference case showing the distinction between ancestral and
        # direct dependencies.
        #
        self._AssertDepsEqual( self._FindSiteDeps(
             'BasicAncestralReference/root.sdf',
             'BasicAncestralReference/A.sdf',
             '/A'),
            [
            Pcp.Dependency(
                '/A',
                '/A',
                Pcp.MapFunction({
                    '/A': '/A'})
                )
            ])
        self._AssertDepsEqual( self._FindSiteDeps(
             'BasicAncestralReference/root.sdf',
             'BasicAncestralReference/B.sdf',
             '/B',
             depMask = Pcp.DependencyTypeDirect | Pcp.DependencyTypeNonVirtual
             ),
            [
            Pcp.Dependency(
                '/A/B',
                '/B',
                Pcp.MapFunction({
                    '/B': '/A/B'})
                )
            ])
        self._AssertDepsEqual( self._FindSiteDeps(
             'BasicAncestralReference/root.sdf',
             'BasicAncestralReference/A.sdf',
             '/A/B',
             depMask = Pcp.DependencyTypeAncestral | Pcp.DependencyTypeNonVirtual
             ),
            [
            Pcp.Dependency(
                '/A/B',
                '/A/B',
                Pcp.MapFunction({
                    '/A': '/A'})
                )
            ])

        ########################################################################
        # An example showing how we can use existing structure to reason about
        # deps that would apply to sites that do not contain any specs yet.
        # This case is important for change processing the addition of a new
        # child prim.
        #
        self._AssertDepsEqual( self._FindSiteDeps(
             'BasicAncestralReference/root.sdf',
             'BasicAncestralReference/A.sdf',
             '/A.HypotheticalProperty',
             ),
            [
            Pcp.Dependency(
                '/A.HypotheticalProperty',
                '/A.HypotheticalProperty',
                Pcp.MapFunction({
                    '/A': '/A'})
                )
            ])
        self._AssertDepsEqual( self._FindSiteDeps(
             'BasicAncestralReference/root.sdf',
             'BasicAncestralReference/A.sdf',
             '/A.HypotheticalProperty',
             recurseOnSite = True, # This should have no effect on the results.
             ),
            [
            Pcp.Dependency(
                '/A.HypotheticalProperty',
                '/A.HypotheticalProperty',
                Pcp.MapFunction({
                    '/A': '/A'})
                )
            ])
        self._AssertDepsEqual( self._FindSiteDeps(
             'BasicAncestralReference/root.sdf',
             'BasicAncestralReference/A.sdf',
             '/A/B/HypotheticalChildPrim',
             depMask = Pcp.DependencyTypeAncestral | Pcp.DependencyTypeNonVirtual
             ),
            [
            Pcp.Dependency(
                '/A/B/HypotheticalChildPrim',
                '/A/B/HypotheticalChildPrim',
                Pcp.MapFunction({
                    '/A': '/A'})
                )
            ])

        ########################################################################
        # Using recurseOnSite to find inbound dependencies on child sites.
        # This is important for the case of making a significant change to
        # a prim that contains children inherited elsewhere.
        self._AssertDepsEqual( self._FindSiteDeps(
            'BasicLocalAndGlobalClassCombination/root.sdf',
            'BasicLocalAndGlobalClassCombination/model.sdf',
            '/_class_Model/_class_Nested',
            depMask = Pcp.DependencyTypeAnyIncludingVirtual,
            recurseOnSite = True
            ),
            [
            Pcp.Dependency(
                '/Model_1/_class_Nested',
                '/_class_Model/_class_Nested',
                Pcp.MapFunction({
                    '/_class_Model': '/Model_1',
                    })
                ),
            Pcp.Dependency(
                '/Model_1/_class_Nested/Left',
                '/_class_Model/_class_Nested/Sym',
                Pcp.MapFunction({
                    '/_class_Model': '/Model_1',
                    '/_class_Model/_class_Nested/Sym': '/Model_1/_class_Nested/Left',
                    })
                ),
            Pcp.Dependency(
                '/Model_1/Instance',
                '/_class_Model/_class_Nested',
                Pcp.MapFunction({
                    '/_class_Model': '/Model_1',
                    '/_class_Model/_class_Nested': '/Model_1/Instance',
                    })
                ),
            Pcp.Dependency(
                '/Model_1/Instance/Left',
                '/_class_Model/_class_Nested/Sym',
                Pcp.MapFunction({
                    '/_class_Model': '/Model_1',
                    '/_class_Model/_class_Nested': '/Model_1/Instance',
                    '/_class_Model/_class_Nested/Sym': '/Model_1/Instance/Left',
                    })
                ),
            ])

        ########################################################################
        # Because deps are analyzed in terms of prim arcs, recurseOnSite needs
        # to be careful to work correctly when querying deps on a property path.
        self._AssertDepsEqual( self._FindSiteDeps(
                'BasicVariantWithConnections/root.sdf',
                'BasicVariantWithConnections/camera.sdf',
                '/camera.HypotheticalProperty',
                recurseOnSite = True,
            ),
            [
            Pcp.Dependency(
                '/main_cam.HypotheticalProperty',
                '/camera.HypotheticalProperty',
                Pcp.MapFunction({
                    '/camera': '/main_cam'})
                )
            ])

        ########################################################################
        # A reference arc, using recurseOnIndex to pick up child indexes
        # that do not introduce any new deps.
        #
        self._AssertDepsEqual( self._FindSiteDeps(
             'BasicReference/root.sdf',
             'BasicReference/ref2.sdf',
             '/PrimB',
             recurseOnIndex = True),
            [
            Pcp.Dependency(
                '/PrimWithReferences',
                '/PrimB',
                Pcp.MapFunction({'/PrimB': '/PrimWithReferences'})
                ),
            Pcp.Dependency(
                '/PrimWithReferences/PrimA_Child',
                '/PrimB/PrimA_Child',
                Pcp.MapFunction({'/PrimB': '/PrimWithReferences'})
                ),
            Pcp.Dependency(
                '/PrimWithReferences/PrimB_Child',
                '/PrimB/PrimB_Child',
                Pcp.MapFunction({'/PrimB': '/PrimWithReferences'})
                ),
            Pcp.Dependency(
                '/PrimWithReferences/PrimC_Child',
                '/PrimB/PrimC_Child',
                Pcp.MapFunction({'/PrimB': '/PrimWithReferences'})
                )
            ])

        ########################################################################
        # A relocation that affects a connection target path.
        # This requires path translation below the dependency arc.
        #
        self._AssertDepsEqual( self._FindSiteDeps(
             'TrickyConnectionToRelocatedAttribute/root.sdf',
             'TrickyConnectionToRelocatedAttribute/eye_rig.sdf',
             '/EyeRig/rig/Mover.bar[/EyeRig/Anim.foo]'),
            [
            Pcp.Dependency(
                '/HumanRig/rig/Face/rig/LEyeRig/rig/Mover.bar[/HumanRig/Anim/Face/LEye.foo]',
                '/EyeRig/rig/Mover.bar[/EyeRig/Anim.foo]',
                Pcp.MapFunction({
                    '/EyeRig': '/HumanRig/rig/Face/rig/LEyeRig',
                    '/EyeRig/Anim': '/HumanRig/Anim/Face/LEye',
                    })
                ),
            Pcp.Dependency(
                '/HumanRig/rig/Face/rig/REyeRig/rig/Mover.bar[/HumanRig/Anim/Face/REye.foo]',
                '/EyeRig/rig/Mover.bar[/EyeRig/Anim.foo]',
                Pcp.MapFunction({
                    '/EyeRig': '/HumanRig/rig/Face/rig/REyeRig',
                    '/EyeRig/Anim': '/HumanRig/Anim/Face/REye',
                    })
                ),
            Pcp.Dependency(
                '/HumanRig/rig/Face/rig/SymEyeRig/rig/Mover.bar[/HumanRig/rig/Face/rig/SymEyeRig/Anim.foo]',
                '/EyeRig/rig/Mover.bar[/EyeRig/Anim.foo]',
                Pcp.MapFunction({
                    '/EyeRig': '/HumanRig/rig/Face/rig/SymEyeRig',
                    })
                ),
            ])

        ########################################################################
        # A relocation that causes a virtual (aka spooky) dependency.
        #
        # Virtual deps are not returned when not requested:
        self._AssertDepsEqual( self._FindSiteDeps(
            'TrickyConnectionToRelocatedAttribute/root.sdf',
            'TrickyConnectionToRelocatedAttribute/root.sdf',
            '/HumanRig/rig/Face/rig/LEyeRig/Anim',
            depMask = (Pcp.DependencyTypeDirect | Pcp.DependencyTypeAncestral
                        | Pcp.DependencyTypeNonVirtual)
            ),
            [])

        # Virtual deps are introduced by relocations:
        self._AssertDepsEqual( self._FindSiteDeps(
            'TrickyConnectionToRelocatedAttribute/root.sdf',
            'TrickyConnectionToRelocatedAttribute/root.sdf',
            '/HumanRig/rig/Face/rig/LEyeRig/Anim',
            depMask = (Pcp.DependencyTypeDirect | Pcp.DependencyTypeAncestral
                        | Pcp.DependencyTypeVirtual)
            ),
            # XXX These deps are duplicated 2x due to the presence of relocates
            # nodes representing virtual dependencies.  Leaving here for now,
            # but this raises an interesting question w/r/t whether we should
            # unique the deps.
            [
            Pcp.Dependency(
                '/HumanRig/Anim/Face/LEye',
                '/HumanRig/rig/Face/rig/LEyeRig/Anim',
                Pcp.MapFunction.Identity()),
            Pcp.Dependency(
                '/HumanRig/rig/Face/Anim/LEye',
                '/HumanRig/rig/Face/rig/LEyeRig/Anim',
                Pcp.MapFunction.Identity()),
            Pcp.Dependency(
                '/HumanRig/Anim/Face/LEye',
                '/HumanRig/rig/Face/rig/LEyeRig/Anim',
                Pcp.MapFunction.Identity()),
            Pcp.Dependency(
                '/HumanRig/rig/Face/Anim/LEye',
                '/HumanRig/rig/Face/rig/LEyeRig/Anim',
                Pcp.MapFunction.Identity()),
            ])

if __name__ == "__main__":
    unittest.main()
