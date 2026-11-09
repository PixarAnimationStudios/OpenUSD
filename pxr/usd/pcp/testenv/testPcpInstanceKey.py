#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.


from pxr import Pcp, Plug, Sdf
import os, unittest

class TestPcpInstanceKey(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register the test dynamic file format plugin used by
        # test_EquivalenceWithDynamicPayload.  This must happen before any
        # layer is opened, since file formats registered after that are not
        # found.
        pluginDir = os.path.join(
            os.path.dirname(__file__), 'PcpPlugins', 'lib')
        try:
            Plug.Registry().RegisterPlugins(pluginDir + '/*/Resources/')
        except RuntimeError:
            pass

    def _LoadPcpCache(self, layerPath, sessionLayer=None):
        rootLayer = Sdf.Layer.FindOrOpen(layerPath)
        # Keep the layers alive for as long as the cache, since the layer
        # stack identifier only holds weak references to them.
        self._layers = getattr(self, '_layers', []) + [rootLayer, sessionLayer]
        cache = Pcp.Cache(
            Pcp.LayerStackIdentifier(rootLayer, sessionLayer), usd=True)
        return cache

    def _LoadPcpCacheWithoutInstancing(self, layerPath, instancePaths):
        """Load layerPath with the given instances made non-instanceable
        through a session layer."""
        sessionLayer = Sdf.Layer.CreateAnonymous()
        for path in instancePaths:
            Sdf.CreatePrimInLayer(sessionLayer, path).instanceable = False
        return self._LoadPcpCache(layerPath, sessionLayer)

    def _GetInstanceKey(self, cache, primPath):
        (pi, err) = cache.ComputePrimIndex(primPath)
        self.assertEqual(err, [])

        key = Pcp.InstanceKey(pi)
        self.assertEqual(key, key)
        if pi.IsInstanceable():
            self.assertNotEqual(key, Pcp.InstanceKey())
        else:
            self.assertEqual(key, Pcp.InstanceKey())

        print("Pcp.InstanceKey('%s'): " % primPath)
        print(key, "\n")
        return key

    @staticmethod
    def _GetInstanceableNodes(primIndex):
        """Return the nodes that contribute to the instance key of
        primIndex, in strength order.

        XXX: This duplicates the logic of PcpInstanceKey and
        Pcp_ChildNodeIsInstanceable, and does not see nodes that stand in
        for a direct arc skipped as a duplicate.  It is sufficient for the
        cases that use it below."""
        hasPayloads = primIndex.hasAnyPayloads
        nodes = []
        def _Visit(node):
            if node.isCulled:
                return
            if (node.HasTransitiveDirectDependency() and
                    node.CanContributeSpecs() and node.hasSpecs):
                nodes.append(node)
                if not hasPayloads:
                    return
            for child in node.children:
                _Visit(child)
        for child in primIndex.rootNode.children:
            _Visit(child)
        return nodes

    def _MakeTargetMapper(self, cache, fromInstance, toInstance):
        """Return a function that maps a target path composed beneath
        fromInstance to the corresponding path beneath toInstance, for
        instances with equal keys.  The target is mapped back through each
        of fromInstance's instanceable nodes to that node's site, then
        forward through the corresponding node of toInstance.  The
        instances' maps may differ where they map to paths, but not in
        which paths they map, so the strongest node that maps the target
        determines the result."""
        fromNodes = self._GetInstanceableNodes(
            cache.ComputePrimIndex(fromInstance)[0])
        toNodes = self._GetInstanceableNodes(
            cache.ComputePrimIndex(toInstance)[0])
        # Pcp.LayerStackSite does not wrap equality, so compare its parts.
        def _Sites(nodes):
            return [(n.layerStack.identifier, n.path) for n in nodes]
        self.assertEqual(_Sites(fromNodes), _Sites(toNodes))

        def _Map(path):
            for fromNode, toNode in zip(fromNodes, toNodes):
                sourcePath = fromNode.mapToRoot.MapTargetToSource(path)
                if not sourcePath.isEmpty:
                    return toNode.mapToRoot.MapSourceToTarget(sourcePath)
            return Sdf.Path()
        return _Map

    def _ComputeDescendants(self, cache, instancePath,
                            mapTarget=None, targetInstancePath=None):
        """Return a dict mapping each descendant path of instancePath,
        relative to it, to a description of its composed results: child
        names, prim stack, property names, and relationship targets and
        attribute connections.  Paths in the root layer stack beneath
        instancePath are made relative to it so that descendants of
        different instances can be compared.  If given, mapTarget is
        applied to each target path first, and the result is made relative
        to targetInstancePath instead."""
        rootLayers = set(l.identifier for l in cache.layerStack.layers)
        relRoot = Sdf.Path('/__Instance')

        def _Relative(path, fromPath=instancePath):
            return path.ReplacePrefix(fromPath, relRoot)

        def _Site(spec):
            path = spec.path
            if spec.layer.identifier in rootLayers:
                path = _Relative(path)
            return (spec.layer.GetDisplayName(), str(path))

        def _Targets(primStack, propPath):
            # Find the strongest property spec to tell relationships from
            # attributes.
            for primSpec in primStack:
                propSpec = primSpec.layer.GetPropertyAtPath(
                    primSpec.path.AppendProperty(propPath.name))
                if propSpec:
                    break
            else:
                return ()
            if isinstance(propSpec, Sdf.RelationshipSpec):
                paths = cache.ComputeRelationshipTargetPaths(propPath)[0]
            else:
                paths = cache.ComputeAttributeConnectionPaths(propPath)[0]
            if mapTarget:
                return tuple(str(_Relative(p, targetInstancePath))
                             for p in map(mapTarget, paths) if not p.isEmpty)
            return tuple(str(_Relative(p)) for p in paths)

        result = {}
        def _Visit(path):
            (pi, _) = cache.ComputePrimIndex(path)
            if pi.hasAnyPayloads and not cache.IsPayloadIncluded(path):
                cache.RequestPayloads([path], [])
                (pi, _) = cache.ComputePrimIndex(path)
            childNames = list(pi.ComputePrimChildNames()[0])
            if path == instancePath:
                result['.'] = (tuple(childNames),)
            else:
                propNames = pi.ComputePrimPropertyNames()
                result[str(path.MakeRelativePath(instancePath))] = (
                    tuple(childNames),
                    tuple(_Site(spec) for spec in pi.primStack),
                    tuple((str(name),
                           _Targets(pi.primStack, path.AppendProperty(name)))
                          for name in propNames))
            for name in childNames:
                _Visit(path.AppendChild(name))
        _Visit(instancePath)
        return result

    def _AssertEquivalentDescendants(self, cache, instancePaths,
                                     numKeys=1):
        """Verify that the given instances have numKeys distinct instance
        keys, and that instances with equal keys have equivalent
        descendants.  Usd may choose any instance with a given key as the
        source of the prototype that all of those instances share, so
        differences mean the prototype's contents depend on that choice.

        Target paths are compared after mapping them from one instance to
        the other through corresponding instanceable nodes.  This is
        stronger than what Usd does for instance proxies, which maps only
        paths within the prototype's source by its root path."""
        groups = []
        for path in instancePaths:
            key = self._GetInstanceKey(cache, path)
            for groupKey, paths in groups:
                if groupKey == key:
                    paths.append(path)
                    break
            else:
                groups.append((key, [path]))
        self.assertEqual(len(groups), numKeys,
                         'instance key groups: %s' % [g[1] for g in groups])

        diffs = []
        for _, paths in groups:
            for source in paths:
                for other in paths:
                    if other == source:
                        continue
                    lhs = self._ComputeDescendants(cache, Sdf.Path(other))
                    rhs = self._ComputeDescendants(
                        cache, Sdf.Path(source),
                        self._MakeTargetMapper(
                            cache, Sdf.Path(source), Sdf.Path(other)),
                        Sdf.Path(other))
                    for relPath in sorted(set(lhs) | set(rhs)):
                        if lhs.get(relPath) != rhs.get(relPath):
                            diffs.append(
                                '  %s\n    <%s>: %s\n'
                                '    from <%s>: %s' % (
                                relPath, other, lhs.get(relPath),
                                source, rhs.get(relPath)))
        if diffs:
            self.fail('Instances with equal keys have different '
                      'descendants:\n' + '\n'.join(diffs))

    def _AssertSameDescendantsWithoutInstancing(
            self, cache, layerPath, instancePaths):
        """Verify that each instance composes the same descendants as it
        does when it is not instanceable."""
        uninstanced = self._LoadPcpCacheWithoutInstancing(
            layerPath, instancePaths)
        for path in instancePaths:
            self.assertFalse(
                uninstanced.ComputePrimIndex(path)[0].IsInstanceable())
            self.assertEqual(
                self._ComputeDescendants(cache, Sdf.Path(path)),
                self._ComputeDescendants(uninstanced, Sdf.Path(path)),
                'descendants of <%s> change with instancing' % path)

    def test_Default(self):
        """Test default constructed (invalid) instance key for 
        code coverage"""
        invalidKey = Pcp.InstanceKey()
        self.assertEqual(invalidKey, invalidKey)
        print("Pcp.InstanceKey(): ")
        print(invalidKey)

    def test_Basic(self):
        """Test instance key functionality on simple
        asset structure including references and inherits"""
        cache = self._LoadPcpCache('basic.usda')

        prop1Key = self._GetInstanceKey(cache, '/Set_1/Prop_1')
        prop2Key = self._GetInstanceKey(cache, '/Set_1/Prop_2')
        prop3Key = self._GetInstanceKey(cache, '/Set_1/Prop_3')

        self.assertEqual(prop1Key, prop2Key)
        self.assertNotEqual(prop1Key, prop3Key)
        self.assertNotEqual(prop2Key, prop3Key)

        # Even though /NotAnInstance is tagged with instance = True,
        # it does not introduce any instance-able data via a composition arc
        # and is not treated as a real instance. Thus, it's instance key
        # is empty.
        notAnInstanceKey = self._GetInstanceKey(cache, '/NotAnInstance')
        self.assertEqual(notAnInstanceKey, Pcp.InstanceKey())

    def test_Hashing(self):
        cache = self._LoadPcpCache("basic.usda")

        self.assertEqual(
            hash(self._GetInstanceKey(cache, "/Set_1")),
            hash(self._GetInstanceKey(cache, "/Set_1"))
        )

    def test_Variants(self):
        """Test instance key functionality on asset
        structure involving references and variants."""
        cache = self._LoadPcpCache('variants.usda')

        key1 = self._GetInstanceKey(cache, '/Model_1')
        key2 = self._GetInstanceKey(cache, '/Model_2')
        key3 = self._GetInstanceKey(cache, '/Model_3')
        key4 = self._GetInstanceKey(cache, '/Model_4')
        key5 = self._GetInstanceKey(cache, '/Model_5')
        key6 = self._GetInstanceKey(cache, '/Model_6')

        # Model_1, 2, and 3 should all have the same instance key because
        # they share the same reference and the same composed variant selection,
        # even though 1 and 2 have the variant selection authored locally and
        # 3 has the selection authored within the reference.
        self.assertEqual(key1, key2)
        self.assertEqual(key1, key3)

        # Model_4, 5, and 6 have different variant selections and composition
        # arcs, so they should have different keys.
        self.assertNotEqual(key1, key4)
        self.assertNotEqual(key1, key5)
        self.assertNotEqual(key1, key6)

        # Model_5 and 6 have locally-defined variant sets, which can introduce
        # different sets of name children. Because of this, they must also have
        # different instance keys.
        self.assertNotEqual(key5, key6)

    def test_ImpliedArcsWithNoSpecs(self):
        """Test instance key functionality with implied inherits and
        specializes."""
        cache = self._LoadPcpCache('implied_arcs/root.usda')

        # Both Model prims should share the same instance key even though
        # they are referenced from two different assets. This is because
        # there are no specs for the implied inherits or specializes in the 
        # referencing assets, which means these prims should have the 
        # exact same set of opinions.
        key1 = self._GetInstanceKey(cache, '/Set/SetA/Model')
        key2 = self._GetInstanceKey(cache, '/Set/SetB/Model')
        self.assertEqual(key1, key2)

    def test_SubrootReferences(self):
        """Test instance key functionality for subroot reference arcs, mainly
        verifying that we incorporate nodes that are technically considered 
        "ancestral" but must be considered as they are brought in through 
        the subtree that is composed for direct subroot arc."""

        cache = self._LoadPcpCache('subroot_arcs.usda')

        # For each instance it's useful to know the basic prim index graph
        # ---> = direct reference
        # -a-> = ancestral reference
        # Paths in parentheses have no specs.
        #
        # Model_overrides/Looks is the spec that defines instanceable = true
        # 
        # Instances/Ref_Override_Looks 
        #   ---> Model_overrides/Looks 
        #     -a-> Model_source/Looks
        key = self._GetInstanceKey(cache, '/Instances/Ref_Overrides_Looks')
        # Instances/Ref_A_Looks 
        #   ---> (Model_A/Looks) 
        #     -a-> Model_overrides/Looks 
        #       -a-> Model_source/Looks
        keyA = self._GetInstanceKey(cache, '/Instances/Ref_A_Looks')
        # Instances/Ref_B_Looks 
        #   ---> Model_B/Looks 
        #     -a-> Model_overrides/Looks 
        #       -a-> Model_source/Looks
        keyB = self._GetInstanceKey(cache, '/Instances/Ref_B_Looks')
        # Instances/Ref_C_Looks 
        #   ---> (Model_C/Looks) 
        #     -a-> Model_B/Looks 
        #       -a-> Model_overrides/Looks 
        #         -a-> Model_source/Looks
        keyC = self._GetInstanceKey(cache, '/Instances/Ref_C_Looks')

        # The Ref_Override_Looks and Ref_A_Looks keys are the same because 
        # Model_A/Looks provides no specs. The ancestral reference nodes are
        # still accounted for because they're under a direct reference.
        self.assertNotEqual(key, Pcp.InstanceKey())
        self.assertEqual(key, keyA)

        # The Ref_B_Looks and Ref_C_Looks keys are the same because 
        # Model_C/Looks provides no specs but they differ from the Ref_A_Looks
        # keys because Model_B/Looks does provide specs. The ancestral 
        # reference nodes here are also under a direct reference and are 
        # included.
        self.assertNotEqual(keyB, Pcp.InstanceKey())
        self.assertEqual(keyB, keyC)
        self.assertNotEqual(keyA, keyB)

        # Model_A/Looks
        #   -a-> Model_overrides/Looks 
        #     -a-> Model_source/Looks
        notAnInstanceKey = self._GetInstanceKey(cache, '/Model_A/Looks')
        # Verifying that Model_A/Looks itself is not even instanceable because
        # its ancestral reference to Model_overrides/Looks is indeed ancestral
        # only, so it is ignored for determining instanceable.
        self.assertEqual(notAnInstanceKey, Pcp.InstanceKey())

    def test_InternalReferencesAndImpliedArcs(self):
        """Test instance keys with implied arcs across internal references."""
        cache = self._LoadPcpCache('internal_ref.usda')

        # /Parent/Child and /ParentRef/Child should have the same instance
        # key since they have the same direct composition arcs, even
        # though /ParentRef/Child comes in via an ancestral reference.
        # 
        # One wrinkle for /ParentRef/Child is that the inherit arc to
        # /Class authored on /Parent/Child will be implied across the
        # ancestral reference arc from /ParentRef to /Parent. The implied
        # inherit node will be marked as inert because it duplicates the
        # original inherit node, i.e., it has the same layer stack and
        # path. That implied inherit node _should not_ contribute to
        # the instance key because it's inert.
        
        # Verify expected composition structure for /ParentRef/Child.
        expected = [
            (Pcp.ArcTypeRoot, "/ParentRef/Child"), [
                (Pcp.ArcTypeInherit, "/Class", True), [],
                (Pcp.ArcTypeReference, "/Parent/Child"), [
                    (Pcp.ArcTypeInherit, "/Class"), []
                ]
            ]
        ]

        pi, err = cache.ComputePrimIndex('/ParentRef/Child')
        for node, entry in Pcp._TestPrimIndex(pi, expected):
            arc, path, inert = (*entry, False) if len(entry) == 2 else entry
            self.assertEqual(node.arcType, arc)
            self.assertEqual(node.path, path)
            self.assertEqual(node.isInert, inert)

        # Verify instance keys match.
        key1 = self._GetInstanceKey(cache, '/Parent/Child')
        key2 = self._GetInstanceKey(cache, '/ParentRef/Child')
        self.assertEqual(key1, key2)

    # The following tests verify that instances with equal instance keys
    # compose equivalent descendants, and that instances whose descendants
    # cannot be made equivalent get different keys.

    def test_EquivalenceWithAncestralClasses(self):
        """Implied class arcs into an intermediate layer stack beneath an
        ancestral instance.  See the InstancingAndAncestralClasses museum
        case."""
        cache = self._LoadPcpCache('equivalence/ancestral_classes/root.usda')
        self._AssertEquivalentDescendants(
            cache, ['/World/set/Inst', '/World/grp/Inst'])

    def test_EquivalenceWithRootLayerStackClasses(self):
        """Implied class arcs from within an instance to paths outside it
        contribute as they do without instancing, whether or not the class
        lies beneath an ancestor of the instance.  In the root layer stack
        they apply to every instance alike.  An override of such a class in
        a set's layer stack applies to the instance in that set too, so it
        composes differently from key-equal instances elsewhere.  That is
        not yet reflected in the instance key; see GitHub issue #3201.  A
        class arc the set authors on a descendant of the instance is
        ignored.  See the InstancingAndAncestralClasses museum case."""
        layerPath = 'equivalence/root_layer_stack_classes/root.usda'
        cache = self._LoadPcpCache(layerPath)
        self._AssertEquivalentDescendants(cache, ['/World/one', '/World/two'])
        self._AssertSameDescendantsWithoutInstancing(
            cache, layerPath, ['/World/one', '/World/two'])
        self.assertEqual(
            self._GetInstanceKey(cache, '/World/set/Inst'),
            self._GetInstanceKey(cache, '/World/one'))
        (pi, err) = cache.ComputePrimIndex('/World/set/Inst/rootClassUser')
        self.assertEqual(list(pi.ComputePrimChildNames()[0]),
                         ['fromSet', 'fromRoot'])
        (pi, err) = cache.ComputePrimIndex('/World/set/Inst/worldClassUser')
        self.assertEqual(list(pi.ComputePrimChildNames()[0]), ['fromRoot'])
        (pi, err) = cache.ComputePrimIndex('/World/set/Inst/plain')
        self.assertEqual(list(pi.ComputePrimChildNames()[0]), [])

    def test_OutsideClassesAlikeInEveryContext(self):
        """An override of a root class in a set applies to instances in the
        set alike whether the set is the root layer stack or is referenced,
        directly or through another layer, and whether the instances are
        made instanceable in the set or in the referencing layer.  Each
        composes as without instancing."""
        base = 'equivalence/outside_class_contexts/'
        for layer, instancePaths in [
                ('set.usda', ['/Set/PropGroup', '/Set/PropGroup2']),
                ('shot.usda', ['/Set/PropGroup', '/Set/PropGroup2']),
                ('nested_shot.usda',
                 ['/Shot/Set/PropGroup', '/Shot/Set/PropGroup2']),
                ('instanced_in_shot.usda', ['/Set/PropGroup'])]:
            cache = self._LoadPcpCache(base + layer)
            self._AssertEquivalentDescendants(cache, instancePaths)
            self._AssertSameDescendantsWithoutInstancing(
                cache, base + layer, instancePaths)
            # The set's override of the class is in each prim stack.
            for path in instancePaths:
                (pi, err) = cache.ComputePrimIndex(path + '/Prop')
                self.assertIn(
                    '/_class_Prop',
                    [str(s.path) for s in pi.primStack
                     if s.layer.GetDisplayName().startswith('set')],
                    '%s in %s' % (path, layer))

    def test_EquivalenceWithInstanceMappedClass(self):
        """An implied class arc into the root layer stack whose path is
        mapped by the instance's own reference lands at a different path
        for each instance, so it is ignored, even where that path happens
        to equal the path in the referenced layer."""
        cache = self._LoadPcpCache(
            'equivalence/instance_mapped_class/root.usda')
        self._AssertEquivalentDescendants(cache, ['/World/one', '/World/two'])
        (pi, err) = cache.ComputePrimIndex('/World/one/widget')
        self.assertEqual(list(pi.ComputePrimChildNames()[0]), [])

    def test_EquivalenceWithRelocatedInstancesRootClass(self):
        """Instances that are relocation targets, with a class arc into the
        root layer stack at a path that does not depend on the instance.
        The class is propagated past the relocate node above each instance
        and lies outside the instances, so it contributes to both, as
        without instancing.  The instances reference different sites, so
        their keys differ."""
        layerPath = 'equivalence/relocated_instances_root_class/root.usda'
        instancePaths = ['/World/one', '/World/two']
        cache = self._LoadPcpCache(layerPath)
        self._AssertEquivalentDescendants(cache, instancePaths, numKeys=2)
        self._AssertSameDescendantsWithoutInstancing(
            cache, layerPath, instancePaths)
        for path in instancePaths:
            (pi, err) = cache.ComputePrimIndex(path + '/user')
            self.assertEqual(list(pi.ComputePrimChildNames()[0]),
                             ['fromRootCls'])

    def test_EquivalenceWithAncestralDuplicates(self):
        """An arc within an instance that duplicates an ancestral node.
        The ancestral node provides the arc's opinions at a different
        strength position, so the instances get different keys, and each
        composes the same descendants as without instancing, including
        those from arcs beneath the ancestral node (e.usda).  See the
        InstancingAndAncestralDuplicates museum case."""
        layerPath = 'equivalence/ancestral_duplicates/root.usda'
        instancePaths = ['/World/grp/Inst', '/World/plain/Inst']
        cache = self._LoadPcpCache(layerPath)
        self._AssertEquivalentDescendants(cache, instancePaths, numKeys=2)
        self._AssertSameDescendantsWithoutInstancing(
            cache, layerPath, instancePaths)

    def test_EquivalenceWithAncestralDuplicateSpecializes(self):
        """As test_EquivalenceWithAncestralDuplicates, but the ancestral
        node's opinions include a specializes arc beneath it, which was
        propagated to the root while indexing the instance's parent, outside
        the ancestral node's subtree.  That propagated node also stands in
        for the skipped arc, so fromBase is present beneath both
        instances."""
        layerPath = 'equivalence/ancestral_duplicate_specializes/root.usda'
        instancePaths = ['/World/grp/Inst', '/World/plain/Inst']
        cache = self._LoadPcpCache(layerPath)
        self._AssertEquivalentDescendants(cache, instancePaths, numKeys=2)
        self._AssertSameDescendantsWithoutInstancing(
            cache, layerPath, instancePaths)
        for path in instancePaths:
            (pi, err) = cache.ComputePrimIndex(path)
            self.assertEqual(list(pi.ComputePrimChildNames()[0]),
                             ['fromBase', 'fromC'])

    def test_EquivalenceWithRelocates(self):
        """A relocate authored outside an instance that moves one of its
        descendants changes the instance's map functions, so targets beneath
        it differ from those of an instance without the relocate.  The
        instances get different keys.  The relocate itself is ignored beneath
        the instance, like other opinions from outside it."""
        layerPath = 'equivalence/relocates/root.usda'
        instancePaths = ['/World/one', '/World/two']
        cache = self._LoadPcpCache(layerPath)
        self._AssertEquivalentDescendants(cache, instancePaths, numKeys=2)

    def test_EquivalenceWithAncestralRelocates(self):
        """A relocate in an ancestral layer stack whose source and target
        are both within an instance.  Relocates authored outside an instance
        are ignored beneath it, so instances with and without the relocate
        compose the same descendants and share a key."""
        layerPath = 'equivalence/ancestral_relocates/root.usda'
        instancePaths = ['/World/grp/Inst', '/World/plain/Inst']
        cache = self._LoadPcpCache(layerPath)
        self._AssertEquivalentDescendants(cache, instancePaths)
        for path in instancePaths:
            (pi, errs) = cache.ComputePrimIndex(path)
            self.assertEqual(errs, [])
            self.assertEqual(list(pi.ComputePrimChildNames()[0]), ['c'])

    def test_EquivalenceWithInternalReference(self):
        """An internal and an external reference to the same site map
        different paths: only the internal reference maps the rest of its
        layer stack, so a relationship target outside the referenced prim
        exists beneath one instance only, and the instances get different
        keys."""
        cache = self._LoadPcpCache(
            'equivalence/internal_reference/root.usda')
        self._AssertEquivalentDescendants(
            cache, ['/World/set/Inst1', '/World/Inst2'], numKeys=2)

    def test_EquivalenceWithRepeatedInternalReference(self):
        """The same internal reference beneath two references to a set maps
        a relationship target outside the instance to a different path for
        each, so the instances share a key and their targets correspond."""
        cache = self._LoadPcpCache(
            'equivalence/repeated_internal_reference/root.usda')
        self._AssertEquivalentDescendants(
            cache, ['/World/setA/Inst1', '/World/setB/Inst1'])

    def test_EquivalenceWithDynamicPayload(self):
        """An override of a dynamic file format argument on a descendant of
        an instance is ignored for composition but still affects the
        arguments of a payload beneath the instance."""
        self.assertTrue(Sdf.FileFormat.FindByExtension('testpcpdynamic'))
        cache = self._LoadPcpCache('equivalence/dynamic_payload/root.usda')
        self._AssertEquivalentDescendants(
            cache, ['/World/one', '/World/two'])

if __name__ == "__main__":
    unittest.main()
