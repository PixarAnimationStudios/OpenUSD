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

from pxr import Sdf, Tf, Usd
import unittest

class TestUsdInstanceProxy(unittest.TestCase):
    def _ValidateInstanceProxy(self, expectedPath, prim, 
                               expectedPathInMaster=False):
        expectedPath = Sdf.Path(expectedPath)

        # Verify that the instance proxy has the expected path.
        self.assertEqual(prim.GetPath(), expectedPath)

        # Instance proxies should be identified as such.
        self.assertTrue(prim)
        self.assertTrue(prim.IsInstanceProxy())

        # An instance proxy is not in a master unless it came from
        # explicitly traversing within a master. For instance, calling
        # GetPrimAtPath('/__Master_1/Instance/Child') will return
        # an instance proxy at that path, which is inside a master.
        if expectedPathInMaster:
            self.assertTrue(prim.IsInMaster())
        else:
            self.assertFalse(prim.IsInMaster())            

    def test_GetPrimAtPath(self):
        s = Usd.Stage.Open('nested/root.usda')

        # Test that getting a prim for a non-nested instance
        # does not return an instance proxy.
        prim = s.GetPrimAtPath('/World/sets/Set_1')
        self.assertTrue(prim)
        self.assertFalse(prim.IsInstanceProxy())
        self.assertTrue(prim.IsInstance())

        # Test getting prims at paths beneath instances and
        # nested instances
        for path in ['/World/sets/Set_1/Prop_1',
                     '/World/sets/Set_1/Prop_1/geom',
                     '/World/sets/Set_1/Prop_2',
                     '/World/sets/Set_1/Prop_2/geom',
                     '/World/sets/Set_2/Prop_1',
                     '/World/sets/Set_2/Prop_1/geom',
                     '/World/sets/Set_2/Prop_2',
                     '/World/sets/Set_2/Prop_2/geom']:
            self._ValidateInstanceProxy(path, s.GetPrimAtPath(path))

        # Test getting prims at paths beneath instances nested
        # in masters.
        master = s.GetPrimAtPath('/World/sets/Set_1').GetMaster()
        for path in [master.GetPath().AppendPath('Prop_1/geom'),
                     master.GetPath().AppendPath('Prop_2/geom')]:
            self._ValidateInstanceProxy(path, s.GetPrimAtPath(path),
                                        expectedPathInMaster=True)

    def test_GetParent(self):
        s = Usd.Stage.Open('nested/root.usda')

        # Test getting parents of instance proxies
        for path in ['/World/sets/Set_1/Prop_1/geom',
                     '/World/sets/Set_1/Prop_2/geom',
                     '/World/sets/Set_2/Prop_1/geom',
                     '/World/sets/Set_2/Prop_2/geom']:
            prim = s.GetPrimAtPath(path)
            self._ValidateInstanceProxy(path, prim)

            prim = prim.GetParent()
            expectedPath = Sdf.Path(path).GetParentPath()
            self._ValidateInstanceProxy(expectedPath, prim)
            self.assertEqual(prim, s.GetPrimAtPath(expectedPath))

            prim = prim.GetParent()
            expectedPath = Sdf.Path(expectedPath).GetParentPath()
            self.assertEqual(prim.GetPath(), expectedPath)
            self.assertFalse(prim.IsInstanceProxy())

        # Test getting parents of instance proxies inside masters
        master = s.GetPrimAtPath('/World/sets/Set_1').GetMaster()
        for path in [master.GetPath().AppendPath('Prop_1/geom'),
                     master.GetPath().AppendPath('Prop_2/geom')]:
            prim = s.GetPrimAtPath(path)
            self._ValidateInstanceProxy(path, prim, 
                                        expectedPathInMaster=True)

            prim = prim.GetParent()
            path = path.GetParentPath()
            self.assertEqual(prim.GetPath(), path)
            self.assertEqual(prim, s.GetPrimAtPath(path))
            self.assertTrue(prim.IsInstance())
            self.assertFalse(prim.IsInstanceProxy())

            prim = prim.GetParent()
            path = path.GetParentPath()
            self.assertEqual(prim.GetPath(), path)
            self.assertEqual(prim, s.GetPrimAtPath(path))
            self.assertTrue(prim.IsMaster())
            self.assertFalse(prim.IsInstanceProxy())
            
    def test_GetChildren(self):
        s = Usd.Stage.Open('nested/root.usda')

        def _ValidateChildren(parentPath, expectedChildrenPaths):
            expectedPrims = [s.GetPrimAtPath(p) for p in expectedChildrenPaths]

            parentPrim = s.GetPrimAtPath(parentPath)

            # Children queries on instance proxies will always return
            # instance proxies.
            if parentPrim.IsInstanceProxy():
                children = parentPrim.GetChildren()
            else:
                children = parentPrim \
                    .GetFilteredChildren(Usd.TraverseInstanceProxies())
            self.assertEqual(expectedPrims, children)

            for (expectedPath, child) in zip(expectedChildrenPaths, children):
                self._ValidateInstanceProxy(expectedPath, child)

        _ValidateChildren(
            parentPath = '/World/sets/Set_1',
            expectedChildrenPaths = [
                '/World/sets/Set_1/Prop_1',
                '/World/sets/Set_1/Prop_2'])

        _ValidateChildren(
            parentPath = '/World/sets/Set_2',
            expectedChildrenPaths = [
                '/World/sets/Set_2/Prop_1',
                '/World/sets/Set_2/Prop_2'])

        _ValidateChildren(
            parentPath = '/World/sets/Set_1/Prop_1',
            expectedChildrenPaths = [
                '/World/sets/Set_1/Prop_1/geom',
                '/World/sets/Set_1/Prop_1/anim'])

        _ValidateChildren(
            parentPath = '/World/sets/Set_1/Prop_2',
            expectedChildrenPaths = [
                '/World/sets/Set_1/Prop_2/geom',
                '/World/sets/Set_1/Prop_2/anim'])

        _ValidateChildren(
            parentPath = '/World/sets/Set_2/Prop_1',
            expectedChildrenPaths = [
                '/World/sets/Set_2/Prop_1/geom',
                '/World/sets/Set_2/Prop_1/anim'])

        _ValidateChildren(
            parentPath = '/World/sets/Set_2/Prop_2',
            expectedChildrenPaths = [
                '/World/sets/Set_2/Prop_2/geom',
                '/World/sets/Set_2/Prop_2/anim'])

    def test_GetPrimInMaster(self):
        s = Usd.Stage.Open('nested/root.usda')

        setMaster = s.GetPrimAtPath('/World/sets/Set_1').GetMaster()
        propMaster = setMaster.GetChild('Prop_1').GetMaster()

        self.assertEqual(
            s.GetPrimAtPath('/World/sets/Set_1/Prop_1').GetPrimInMaster(),
            setMaster.GetChild('Prop_1'))
        self.assertEqual(
            s.GetPrimAtPath('/World/sets/Set_1/Prop_1/geom').GetPrimInMaster(),
            propMaster.GetChild('geom'))

        self.assertEqual(
            s.GetPrimAtPath('/World/sets/Set_1/Prop_2').GetPrimInMaster(),
            setMaster.GetChild('Prop_2'))
        self.assertEqual(
            s.GetPrimAtPath('/World/sets/Set_1/Prop_2/geom').GetPrimInMaster(),
            propMaster.GetChild('geom'))

        self.assertEqual(
            s.GetPrimAtPath('/World/sets/Set_2/Prop_1').GetPrimInMaster(),
            setMaster.GetChild('Prop_1'))
        self.assertEqual(
            s.GetPrimAtPath('/World/sets/Set_2/Prop_1/geom').GetPrimInMaster(),
            propMaster.GetChild('geom'))

        self.assertEqual(
            s.GetPrimAtPath('/World/sets/Set_2/Prop_2').GetPrimInMaster(),
            setMaster.GetChild('Prop_2'))
        self.assertEqual(
            s.GetPrimAtPath('/World/sets/Set_2/Prop_2/geom').GetPrimInMaster(),
            propMaster.GetChild('geom'))

    def test_LoadSet(self):
        s = Usd.Stage.Open('nested_payloads/root.usda',
                      Usd.Stage.LoadNone)

        master = s.GetPrimAtPath('/World/sets/Set_1').GetMaster()

        # Note that FindLoadable never returns paths to prims in masters.
        self.assertEqual(
            s.FindLoadable(),
            ['/World/sets/Set_1/props/Prop_1',
             '/World/sets/Set_1/props/Prop_2',
             '/World/sets/Set_2/props/Prop_1',
             '/World/sets/Set_2/props/Prop_2'])
        
        self.assertEqual(
            s.FindLoadable('/World/sets/Set_1/props'),
            ['/World/sets/Set_1/props/Prop_1',
             '/World/sets/Set_1/props/Prop_2'])

        self.assertEqual(
            s.FindLoadable('/World/sets/Set_2/props'),
            ['/World/sets/Set_2/props/Prop_1',
             '/World/sets/Set_2/props/Prop_2'])

        # Load and unload models through instance proxies.
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_1/props/Prop_1').IsLoaded())
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_1/props/Prop_2').IsLoaded())
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_2/props/Prop_1').IsLoaded())
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_2/props/Prop_2').IsLoaded())

        s.GetPrimAtPath('/World/sets/Set_1/props/Prop_1').Load()
        self.assertTrue(
            s.GetPrimAtPath('/World/sets/Set_1/props/Prop_1').IsLoaded())
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_1/props/Prop_2').IsLoaded())
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_2/props/Prop_1').IsLoaded())
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_2/props/Prop_2').IsLoaded())

        s.GetPrimAtPath('/World/sets/Set_2/props/Prop_2').Load()
        self.assertTrue(
            s.GetPrimAtPath('/World/sets/Set_1/props/Prop_1').IsLoaded())
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_1/props/Prop_2').IsLoaded())
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_2/props/Prop_1').IsLoaded())
        self.assertTrue(
            s.GetPrimAtPath('/World/sets/Set_2/props/Prop_2').IsLoaded())

        s.LoadAndUnload([], ['/World/sets/Set_1/props'])
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_1/props/Prop_1').IsLoaded())
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_1/props/Prop_2').IsLoaded())
        self.assertFalse(
            s.GetPrimAtPath('/World/sets/Set_2/props/Prop_1').IsLoaded())
        self.assertTrue(
            s.GetPrimAtPath('/World/sets/Set_2/props/Prop_2').IsLoaded())

    def test_PrimRange(self):
        s = Usd.Stage.Open('nested/root.usda')

        # Test iterating over all prims on the stage
        expectedPrims = [s.GetPrimAtPath(p) for p in
            ['/World',
             '/World/sets',
             '/World/sets/Set_1',
             '/World/sets/Set_1/Prop_1',
             '/World/sets/Set_1/Prop_1/geom',
             '/World/sets/Set_1/Prop_1/anim',
             '/World/sets/Set_1/Prop_2',
             '/World/sets/Set_1/Prop_2/geom',
             '/World/sets/Set_1/Prop_2/anim',
             '/World/sets/Set_2',
             '/World/sets/Set_2/Prop_1',
             '/World/sets/Set_2/Prop_1/geom',
             '/World/sets/Set_2/Prop_1/anim',
             '/World/sets/Set_2/Prop_2',
             '/World/sets/Set_2/Prop_2/geom',
             '/World/sets/Set_2/Prop_2/anim']
        ]

        gotPrims = list(Usd.PrimRange.Stage(s, Usd.TraverseInstanceProxies()))
        self.assertEqual(expectedPrims, gotPrims)

        # Test getting instance proxy descendants from each instance.
        def _ValidateInstanceDescendants(parentPath, expectedDescendantPaths):
            parent = s.GetPrimAtPath(parentPath)
            descendants = \
                list(Usd.PrimRange(parent, Usd.TraverseInstanceProxies()))
            expectedDescendants = \
                [s.GetPrimAtPath(p) for p in expectedDescendantPaths]
            self.assertEqual(expectedDescendants, descendants)
            
        _ValidateInstanceDescendants(
            parentPath = '/World/sets/Set_1',
            expectedDescendantPaths = [
                '/World/sets/Set_1',
                '/World/sets/Set_1/Prop_1',
                '/World/sets/Set_1/Prop_1/geom',
                '/World/sets/Set_1/Prop_1/anim',
                '/World/sets/Set_1/Prop_2',
                '/World/sets/Set_1/Prop_2/geom',
                '/World/sets/Set_1/Prop_2/anim'])

        _ValidateInstanceDescendants(
            parentPath = '/World/sets/Set_1/Prop_1',
            expectedDescendantPaths = [
                '/World/sets/Set_1/Prop_1',
                '/World/sets/Set_1/Prop_1/geom',
                '/World/sets/Set_1/Prop_1/anim'])

        _ValidateInstanceDescendants(
            parentPath = '/World/sets/Set_1/Prop_2',
            expectedDescendantPaths = [
                '/World/sets/Set_1/Prop_2',
                '/World/sets/Set_1/Prop_2/geom',
                '/World/sets/Set_1/Prop_2/anim'])

        _ValidateInstanceDescendants(
            parentPath = '/World/sets/Set_2',
            expectedDescendantPaths = [
                '/World/sets/Set_2',
                '/World/sets/Set_2/Prop_1',
                '/World/sets/Set_2/Prop_1/geom',
                '/World/sets/Set_2/Prop_1/anim',
                '/World/sets/Set_2/Prop_2',
                '/World/sets/Set_2/Prop_2/geom',
                '/World/sets/Set_2/Prop_2/anim'])

        _ValidateInstanceDescendants(
            parentPath = '/World/sets/Set_2/Prop_1',
            expectedDescendantPaths = [
                '/World/sets/Set_2/Prop_1',
                '/World/sets/Set_2/Prop_1/geom',
                '/World/sets/Set_2/Prop_1/anim'])

        _ValidateInstanceDescendants(
            parentPath = '/World/sets/Set_2/Prop_2',
            expectedDescendantPaths = [
                '/World/sets/Set_2/Prop_2',
                '/World/sets/Set_2/Prop_2/geom',
                '/World/sets/Set_2/Prop_2/anim'])
        
        # Test iterating starting from a master prim
        master = s.GetPrimAtPath('/World/sets/Set_1').GetMaster()
        _ValidateInstanceDescendants(
            parentPath = master.GetPath(),
            expectedDescendantPaths = [
                master.GetPath(),
                master.GetPath().AppendPath('Prop_1'),
                master.GetPath().AppendPath('Prop_1/geom'),
                master.GetPath().AppendPath('Prop_1/anim'),
                master.GetPath().AppendPath('Prop_2'),
                master.GetPath().AppendPath('Prop_2/geom'),
                master.GetPath().AppendPath('Prop_2/anim')])

        master = s.GetPrimAtPath('/World/sets/Set_1/Prop_1').GetMaster()
        _ValidateInstanceDescendants(
            parentPath = master.GetPath(),
            expectedDescendantPaths = [
                master.GetPath(),
                master.GetPath().AppendPath('geom'),
                master.GetPath().AppendPath('anim')])

    def test_GetAttributeValue(self):
        s = Usd.Stage.Open('nested/root.usda')

        def _ValidateAttributeValue(attrPath, expectedValue):
            attrPath = Sdf.Path(attrPath)
            prim = s.GetPrimAtPath(attrPath.GetPrimPath())
            self._ValidateInstanceProxy(attrPath.GetPrimPath(), prim)

            attr = prim.GetAttribute(attrPath.name)
            self.assertTrue(attr)
            self.assertEqual(attr.GetPath(), attrPath)
            self.assertEqual(attr.Get(), expectedValue)

        _ValidateAttributeValue(
            attrPath = '/World/sets/Set_1/Prop_1/geom.x', expectedValue = 1.0)

        _ValidateAttributeValue(
            attrPath = '/World/sets/Set_1/Prop_2/geom.x', expectedValue = 1.0)

        _ValidateAttributeValue(
            attrPath = '/World/sets/Set_2/Prop_1/geom.x', expectedValue = 1.0)

        _ValidateAttributeValue(
            attrPath = '/World/sets/Set_2/Prop_2/geom.x', expectedValue = 1.0)

    def test_GetRelationshipTargets(self):
        s = Usd.Stage.Open('rels/root.usda')

        def _ValidateRelationshipTargets(relPath, expectedTargets):
            relPath = Sdf.Path(relPath)
            prim = s.GetPrimAtPath(relPath.GetPrimPath())
            self._ValidateInstanceProxy(relPath.GetPrimPath(), prim)

            rel = prim.GetRelationship(relPath.name)
            self.assertTrue(rel)
            self.assertEqual(rel.GetPath(), relPath)
            self.assertEqual(
                rel.GetTargets(), [Sdf.Path(p) for p in expectedTargets])

        _ValidateRelationshipTargets(
            relPath = '/Root/Instance_1/A.rel',
            expectedTargets = [
                '/Root/Instance_1',
                '/Root/Instance_1.attr',
                '/Root/Instance_1/A',
                '/Root/Instance_1/A.attr',
                '/Root/Instance_1/NestedInstance_1',
                '/Root/Instance_1/NestedInstance_1.attr',
                '/Root/Instance_1/NestedInstance_1/B',
                '/Root/Instance_1/NestedInstance_1/B.attr',
                '/Root/Instance_1/NestedInstance_2',
                '/Root/Instance_1/NestedInstance_2.attr',
                '/Root/Instance_1/NestedInstance_2/B',
                '/Root/Instance_1/NestedInstance_2/B.attr'])

        _ValidateRelationshipTargets(
            relPath = '/Root/Instance_2/A.rel',
            expectedTargets = [
                '/Root/Instance_2',
                '/Root/Instance_2.attr',
                '/Root/Instance_2/A',
                '/Root/Instance_2/A.attr',
                '/Root/Instance_2/NestedInstance_1',
                '/Root/Instance_2/NestedInstance_1.attr',
                '/Root/Instance_2/NestedInstance_1/B',
                '/Root/Instance_2/NestedInstance_1/B.attr',
                '/Root/Instance_2/NestedInstance_2',
                '/Root/Instance_2/NestedInstance_2.attr',
                '/Root/Instance_2/NestedInstance_2/B',
                '/Root/Instance_2/NestedInstance_2/B.attr'])

        _ValidateRelationshipTargets(
            relPath = '/Root/Instance_1/NestedInstance_1/B.rel',
            expectedTargets = [
                '/Root/Instance_1/NestedInstance_1/B',
                '/Root/Instance_1/NestedInstance_1/B.attr'])

        _ValidateRelationshipTargets(
            relPath = '/Root/Instance_1/NestedInstance_2/B.rel',
            expectedTargets = [
                '/Root/Instance_1/NestedInstance_2/B',
                '/Root/Instance_1/NestedInstance_2/B.attr'])

        _ValidateRelationshipTargets(
            relPath = '/Root/Instance_2/NestedInstance_1/B.rel',
            expectedTargets = [
                '/Root/Instance_2/NestedInstance_1/B',
                '/Root/Instance_2/NestedInstance_1/B.attr'])

        _ValidateRelationshipTargets(
            relPath = '/Root/Instance_2/NestedInstance_2/B.rel',
            expectedTargets = [
                '/Root/Instance_2/NestedInstance_2/B',
                '/Root/Instance_2/NestedInstance_2/B.attr'])

    def test_GetConnections(self):
        s = Usd.Stage.Open('attrs/root.usda')

        def _ValidateConnections(attrPath, expectedSrcs):
            attrPath = Sdf.Path(attrPath)
            prim = s.GetPrimAtPath(attrPath.GetPrimPath())
            self._ValidateInstanceProxy(attrPath.GetPrimPath(), prim)

            attr = prim.GetAttribute(attrPath.name)
            self.assertTrue(attr)
            self.assertEqual(attr.GetPath(), attrPath)
            self.assertEqual(
                attr.GetConnections(), [Sdf.Path(p) for p in expectedSrcs])

        _ValidateConnections(
            attrPath = '/Root/Instance_1/A.con',
            expectedSrcs = [
                '/Root/Instance_1',
                '/Root/Instance_1.attr',
                '/Root/Instance_1/A',
                '/Root/Instance_1/A.attr',
                '/Root/Instance_1/NestedInstance_1',
                '/Root/Instance_1/NestedInstance_1.attr',
                '/Root/Instance_1/NestedInstance_1/B',
                '/Root/Instance_1/NestedInstance_1/B.attr',
                '/Root/Instance_1/NestedInstance_2',
                '/Root/Instance_1/NestedInstance_2.attr',
                '/Root/Instance_1/NestedInstance_2/B',
                '/Root/Instance_1/NestedInstance_2/B.attr'])

        _ValidateConnections(
            attrPath = '/Root/Instance_2/A.con',
            expectedSrcs = [
                '/Root/Instance_2',
                '/Root/Instance_2.attr',
                '/Root/Instance_2/A',
                '/Root/Instance_2/A.attr',
                '/Root/Instance_2/NestedInstance_1',
                '/Root/Instance_2/NestedInstance_1.attr',
                '/Root/Instance_2/NestedInstance_1/B',
                '/Root/Instance_2/NestedInstance_1/B.attr',
                '/Root/Instance_2/NestedInstance_2',
                '/Root/Instance_2/NestedInstance_2.attr',
                '/Root/Instance_2/NestedInstance_2/B',
                '/Root/Instance_2/NestedInstance_2/B.attr'])

        _ValidateConnections(
            attrPath = '/Root/Instance_1/NestedInstance_1/B.con',
            expectedSrcs = [
                '/Root/Instance_1/NestedInstance_1/B',
                '/Root/Instance_1/NestedInstance_1/B.attr'])

        _ValidateConnections(
            attrPath = '/Root/Instance_1/NestedInstance_2/B.con',
            expectedSrcs = [
                '/Root/Instance_1/NestedInstance_2/B',
                '/Root/Instance_1/NestedInstance_2/B.attr'])

        _ValidateConnections(
            attrPath = '/Root/Instance_2/NestedInstance_1/B.con',
            expectedSrcs = [
                '/Root/Instance_2/NestedInstance_1/B',
                '/Root/Instance_2/NestedInstance_1/B.attr'])

        _ValidateConnections(
            attrPath = '/Root/Instance_2/NestedInstance_2/B.con',
            expectedSrcs = [
                '/Root/Instance_2/NestedInstance_2/B',
                '/Root/Instance_2/NestedInstance_2/B.attr'])

    def test_Editing(self):
        """Test that edits cannot be made on instance proxies"""

        # Verify that edits cannot be made via instance proxies
        # in the local layer stack, since they represent prims 
        # beneath instances and those opinions would be ignored.
        s = Usd.Stage.Open('nested/root.usda')

        proxy = s.GetPrimAtPath('/World/sets/Set_1/Prop_1')
        assert proxy
        assert proxy.IsInstanceProxy()

        with self.assertRaises(Tf.ErrorException):
            s.DefinePrim(proxy.GetPath())
        with self.assertRaises(Tf.ErrorException):
            s.OverridePrim(proxy.GetPath())
        with self.assertRaises(Tf.ErrorException):
            s.CreateClassPrim(proxy.GetPath())

        with self.assertRaises(Tf.ErrorException):
            proxy.SetDocumentation('test')
        with self.assertRaises(Tf.ErrorException):
            proxy.ClearDocumentation()
        with self.assertRaises(Tf.ErrorException):
            proxy.CreateRelationship('testRel')
        with self.assertRaises(Tf.ErrorException):
            proxy.CreateAttribute('testAttr', Sdf.ValueTypeNames.Int)

        attrInProxy = proxy.GetChild('geom').GetAttribute('x')
        assert attrInProxy

        with self.assertRaises(Tf.ErrorException):
            attrInProxy.SetDocumentation('test')
        with self.assertRaises(Tf.ErrorException):
            attrInProxy.ClearDocumentation()
        with self.assertRaises(Tf.ErrorException):
            attrInProxy.Set(2.0, time=1.0)
        with self.assertRaises(Tf.ErrorException):
            attrInProxy.Set(2.0, time=Usd.TimeCode.Default())
        with self.assertRaises(Tf.ErrorException):
            attrInProxy.Clear()
        with self.assertRaises(Tf.ErrorException):
            attrInProxy.ClearAtTime(time=1.0)
        with self.assertRaises(Tf.ErrorException):
            attrInProxy.SetConnections(['/Some/Connection'])
            
        relInProxy = proxy.GetChild('geom').GetRelationship('y')
        assert relInProxy

        with self.assertRaises(Tf.ErrorException):
            relInProxy.SetDocumentation('test')
        with self.assertRaises(Tf.ErrorException):
            relInProxy.ClearDocumentation()
        with self.assertRaises(Tf.ErrorException):
            relInProxy.SetTargets(['/Some/Target'])

if __name__ == '__main__':
    unittest.main()
