#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import contextlib
import os
import shutil
import textwrap
import unittest

from pxr import Ar, Pcp, Sdf

def CreateLayer(path, layerString):
    l = Sdf.Layer.CreateAnonymous()
    l.ImportFromString(textwrap.dedent(layerString))
    l.Export(path)

def CreatePcpCache(rootLayerPath, context):
    with Ar.ResolverContextBinder(context):
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerPath)

    lsid = Pcp.LayerStackIdentifier(rootLayer, pathResolverContext=context)
    return Pcp.Cache(lsid)

def cwd(path):
    '''Decorator for running a test case inside the specified
    working directory.'''
    def wrapper(func):
        def innerwrapper(*args, **kwargs):
            if os.path.isdir(path):
                shutil.rmtree(path)
            os.makedirs(path)

            curdir = os.getcwd()
            os.chdir(path)
            try:
                return func(*args, **kwargs)
            finally:
                os.chdir(curdir)
        return innerwrapper
    return wrapper

class TestPcpResolvedPathChange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # These test cases make use of ArDefaultResolver's
        # search path functionality.
        Ar.SetPreferredResolver('ArDefaultResolver')

    def assertLayerStack(self, layerStack, expectedLayers):
        '''Helper to verify the given layer stack contains
        the layers in expectedLayers'''
        self.assertEqual(
            layerStack.layers, [Sdf.Layer.Find(l) for l in expectedLayers])

    def assertReferenceNode(self, node, expectedLayers):
        '''Helper to verify that the given node is a reference
        to a layer stack with the given expectedLayers'''
        self.assertEqual(node.arcType, Pcp.ArcTypeReference)
        self.assertLayerStack(node.layerStack, expectedLayers)
        
    @cwd('referencingWithAbsSublayers')
    def test_ReferencingLayerWithAbsoluteSublayerPaths(self):
        '''Tests behavior when the resolved path of a layer changes
        and that layer contains references to other layers and
        absolute sublayer paths.'''

        # Create test directory structure and assets.
        CreateLayer('absRef.usda', 
            '''\
            #usda 1.0
            def "Ref" { }
            '''
        )

        CreateLayer('absSublayer.usda',
            '''\
            #usda 1.0
            '''
        )

        CreateLayer('v1/root.usda',
            '''\
            #usda 1.0
            (
                subLayers = [
                    @{sublayerPath}@
                ]
            )
            
            def "AbsoluteReference" (
                references = @{absRefPath}@</Ref>
            )
            {{
            }}

            def "RelativeReference" (
                references = @./ref.usda@</Ref>
            )
            {{
            }}
            '''.format(
                sublayerPath=os.path.abspath('absSublayer.usda'),
                absRefPath=os.path.abspath('absRef.usda'))
        )

        CreateLayer('v1/ref.usda',
            '''\
            #usda 1.0
            def "Ref" { }
            '''
        )

        # Create PcpCache using a resolver context that searches
        # v2/ first, then v1/. Note that we use a search path for root.usda.
        # Since v2/root.usda doesn't exist yet, we should find v1/root.usda
        # as our root layer.
        pcpCache = CreatePcpCache("root.usda",
            Ar.DefaultResolverContext(
                searchPaths=[os.path.abspath('v2'), os.path.abspath('v1')]))

        # All relative asset paths in references/sublayers/etc. should be
        # anchored to v1/root.usda initially.
        pi, err = pcpCache.ComputePrimIndex('/AbsoluteReference')
        self.assertFalse(err)
        self.assertReferenceNode(pi.rootNode.children[0], ['absRef.usda'])

        pi, err = pcpCache.ComputePrimIndex('/RelativeReference')
        self.assertFalse(err)
        self.assertReferenceNode(pi.rootNode.children[0], ['v1/ref.usda'])

        self.assertEqual(
            pcpCache.layerStack.layers,
            [Sdf.Layer.Find('v1/root.usda'), Sdf.Layer.Find('absSublayer.usda')])

        # Copy v1/ to v2/ and reload. This should cause the resolved path of
        # root.usda to change to v2/root.usda. Any prims with references that were
        # relative to root.usda need to be resynced since those references now
        # target a different layer.
        shutil.copytree('v1', 'v2')

        with Pcp._TestChangeProcessor(pcpCache) as changes:
            pcpCache.Reload()

            self.assertEqual(
                changes.GetSignificantChanges(), ['/RelativeReference'])

        self.assertTrue(pcpCache.FindPrimIndex('/AbsoluteReference'))
        self.assertFalse(pcpCache.FindPrimIndex('/RelativeReference'))

        pi, err = pcpCache.ComputePrimIndex('/RelativeReference')
        self.assertFalse(err)
        self.assertReferenceNode(pi.rootNode.children[0], ['v2/ref.usda'])

        self.assertLayerStack(
            pcpCache.layerStack, ['v2/root.usda', 'absSublayer.usda'])

    @cwd('referencingWithRelSublayers')
    def test_ReferencingLayerWithRelativeSublayerPaths(self):
        '''Tests behavior when the resolved path of a layer changes
        and that layer contains references to other layers and
        relative sublayer paths.'''

        # Create test directory structure and assets.
        CreateLayer('absRef.usda', 
            '''\
            #usda 1.0
            def "Ref" { }
            '''
        )

        CreateLayer('v1/root.usda',
            '''\
            #usda 1.0
            (
                subLayers = [
                    @./sublayer.usda@
                ]
            )
            '''
        )

        CreateLayer('v1/sublayer.usda',
            '''\
            #usda 1.0

            def "AbsoluteReference" (
                references = @{absRefPath}@</Ref>
            )
            {{
            }}

            def "RelativeReference" (
                references = @./ref.usda@</Ref>
            )
            {{
            }}
            '''.format(
                absRefPath=os.path.abspath('absRef.usda'))
        )

        CreateLayer('v1/ref.usda',
            '''\
            #usda 1.0
            def "Ref" { }
            '''
        )

        # Create PcpCache using a resolver context that searches
        # v2/ first, then v1/. Note that we use a search path for root.usda.
        # Since v2/root.usda doesn't exist yet, we should find v1/root.usda
        # as our root layer.
        pcpCache = CreatePcpCache("root.usda",
            Ar.DefaultResolverContext(
                searchPaths=[os.path.abspath('v2'), os.path.abspath('v1')]))

        # All relative asset paths in references/sublayers/etc. should be
        # anchored to v1/root.usda initially.
        pi, err = pcpCache.ComputePrimIndex('/AbsoluteReference')
        self.assertFalse(err)
        self.assertReferenceNode(pi.rootNode.children[0], ['absRef.usda'])

        pi, err = pcpCache.ComputePrimIndex('/RelativeReference')
        self.assertFalse(err)
        self.assertReferenceNode(pi.rootNode.children[0], ['v1/ref.usda'])

        self.assertLayerStack(
            pcpCache.layerStack, ['v1/root.usda', 'v1/sublayer.usda'])

        # Copy v1/ to v2/ and reload. This should cause the resolved path of
        # root.usda to change to v2/root.usda. Because this layer had a sublayer
        # with a relative asset path, the entire layer stack needs to be
        # recomputed. Since this recomputation could add/remove opinions
        # arbitrarily, any prims that depend on the layer stack need to be
        # resynced. Since this layer stack is the cache's root layer stack,
        # all prims need to be resynced.
        shutil.copytree('v1', 'v2')

        with Pcp._TestChangeProcessor(pcpCache) as changes:
            pcpCache.Reload()
            self.assertEqual(changes.GetSignificantChanges(), ['/'])

        self.assertFalse(pcpCache.FindPrimIndex('/AbsoluteReference'))
        self.assertFalse(pcpCache.FindPrimIndex('/RelativeReference'))

        pi, err = pcpCache.ComputePrimIndex('/AbsoluteReference')
        self.assertFalse(err)
        self.assertReferenceNode(pi.rootNode.children[0], ['absRef.usda'])

        pi, err = pcpCache.ComputePrimIndex('/RelativeReference')
        self.assertFalse(err)
        self.assertReferenceNode(pi.rootNode.children[0], ['v2/ref.usda'])

        self.assertLayerStack(
            pcpCache.layerStack, ['v2/root.usda', 'v2/sublayer.usda'])

    @cwd('referencedWithRelSublayers')
    def test_ReferencedWithRelativeSublayerPaths(self):
        '''Tests behavior when the resolved path of a referenced layer
        changes and that layer contains relative sublayer paths.'''

        # Create test directory structure and assets.
        CreateLayer('root.usda',
            '''\
            #usda 1.0

            def "SearchPathRef" (
                references = @ref.usda@</Ref>
            )
            {
            }

            def "InnocentBystander"
            {
            }
            '''
        )
        
        CreateLayer('v1/ref.usda',
            '''\
            #usda 1.0
            (
                subLayers = [
                    @./sublayer.usda@
                ]
            )
            '''
        )

        CreateLayer('v1/sublayer.usda',
            '''\
            #usda 1.0
            def "Ref" { }
            '''
        )

        # Create PcpCache using a resolver context that searches
        # v2/ first, then v1/. The search path reference to ref.usda should
        # resolve to v1/ref.usda since v2/ doesn't exist yet.
        pcpCache = CreatePcpCache("root.usda",
            Ar.DefaultResolverContext(
                searchPaths=[os.path.abspath('v2'), os.path.abspath('v1')]))

        pi, err = pcpCache.ComputePrimIndex('/InnocentBystander')
        self.assertFalse(err)

        pi, err = pcpCache.ComputePrimIndex('/SearchPathRef')
        self.assertFalse(err)
        self.assertReferenceNode(
            pi.rootNode.children[0], ['v1/ref.usda', 'v1/sublayer.usda'])

        # Copy v1/ to v2/ and reload. This should cause the resolved path of
        # ref.usda to change to v2/ref.usda, but no other scene description
        # is changed because v1/ and v2/ are exactly the same.
        # 
        # Because we had a sublayer with a relative path, we need to recompute
        # the layer stack since its anchor has changed. This means that any prim
        # index that uses that layer stack must be resynced.
        shutil.copytree('v1', 'v2')

        with Pcp._TestChangeProcessor(pcpCache) as changes:
            pcpCache.Reload()
            self.assertEqual(
                changes.GetSignificantChanges(), ['/SearchPathRef'])

        self.assertTrue(pcpCache.FindPrimIndex('/InnocentBystander'))
        self.assertFalse(pcpCache.FindPrimIndex('/SearchPathRef'))

        pi, err = pcpCache.ComputePrimIndex('/SearchPathRef')
        self.assertFalse(err)
        self.assertReferenceNode(
            pi.rootNode.children[0], ['v2/ref.usda', 'v2/sublayer.usda'])

    @cwd('referencedWithAbsSublayers')
    def test_ReferencedWithAbsoluteSublayerPaths(self):
        '''Tests behavior when the resolved path of a referenced layer
        changes and that layer contains absolute sublayer paths.'''

        # Create test directory structure and assets.
        CreateLayer('root.usda',
            '''\
            #usda 1.0

            def "SearchPathRef" (
                references = @ref.usda@</Ref>
            )
            {
            }

            def "InnocentBystander"
            {
            }
            '''
        )

        CreateLayer('absSublayer.usda',
            '''\
            #usda 1.0
            def "Ref" {}
            '''
        )
        
        CreateLayer('v1/ref.usda',
            '''\
            #usda 1.0
            (
                subLayers = [
                    @{absSublayerPath}@
                ]
            )
            '''.format(absSublayerPath=os.path.abspath('absSublayer.usda'))
        )

        # Create PcpCache using a resolver context that searches
        # v2/ first, then v1/. The search path reference to ref.usda should
        # resolve to v1/ref.usda since v2/ doesn't exist yet.
        pcpCache = CreatePcpCache("root.usda",
            Ar.DefaultResolverContext(
                searchPaths=[os.path.abspath('v2'), os.path.abspath('v1')]))

        pi, err = pcpCache.ComputePrimIndex('/InnocentBystander')
        self.assertFalse(err)

        pi, err = pcpCache.ComputePrimIndex('/SearchPathRef')
        self.assertFalse(err)
        self.assertReferenceNode(
            pi.rootNode.children[0], ['v1/ref.usda', 'absSublayer.usda'])

        # Copy v1/ to v2/ and reload. This should cause the resolved path of
        # ref.usda to change to v2/ref.usda, but no other scene description is
        # changed because v1/ and v2/ are exactly the same.
        #
        # Even though the root layer of the referenced layer stack has changed
        # resolved paths, there's no actual change to the layer stack itself
        # because the sublayer asset path was an absolute path -- it contains
        # exactly the same layers that it did prior to the reload. So no prims
        # actually needs to be resynced in this case.
        shutil.copytree('v1', 'v2')

        with Pcp._TestChangeProcessor(pcpCache) as changes:
            pcpCache.Reload()
            self.assertEqual(changes.GetSignificantChanges(), [])

if __name__ == "__main__":
    unittest.main()

