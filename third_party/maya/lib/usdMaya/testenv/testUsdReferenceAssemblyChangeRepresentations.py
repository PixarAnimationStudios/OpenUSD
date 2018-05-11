#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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
#


import os
import unittest

# XXX: The try/except here is temporary until we change the Pixar-internal
# package name to match the external package name.
try:
    from pxr import UsdMaya
except ImportError:
    from pixar import UsdMaya

from pxr import UsdGeom

from maya import cmds
from maya import standalone


class testUsdReferenceAssemblyChangeRepresentations(unittest.TestCase):

    ASSEMBLY_TYPE_NAME = 'pxrUsdReferenceAssembly'
    PROXY_TYPE_NAME = 'pxrUsdProxyShape'
    TRANSFORM_TYPE_NAME = 'transform'
    CAMERA_TYPE_NAME = 'camera'
    MESH_TYPE_NAME = 'mesh'

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _GetChildren(self, nodeName):
        """
        Returns the immediate descendents of the node 'nodeName' as a list.
        Ironically enough, the 'recursive' flag does NOT traverse the
        descendents but is instead used to capture immediate descendents in
        any/all namespaces.
        """
        return cmds.ls('%s|*' % nodeName, recursive=True)

    def _ValidateUnloaded(self, nodeName):
        """
        Tests that the assembly node 'nodeName' is unloaded (no active
        representation).
        """
        self.assertEqual(cmds.nodeType(nodeName), self.ASSEMBLY_TYPE_NAME)
        childNodes = self._GetChildren(nodeName)
        self.assertEqual(childNodes, [])

    def _ValidateNodeWithSingleChild(self, nodeName, nodeTypeName,
            childNodeSuffix, childTypeName):
        """
        Tests that the given node is of type nodeTypeName with exactly one
        child node underneath it with the string in childNodeSuffix at the end
        of its name and of type childTypeName.
        The name of the child node is returned.
        """
        self.assertEqual(cmds.nodeType(nodeName), nodeTypeName)
        childNodes = self._GetChildren(nodeName)
        self.assertEqual(len(childNodes), 1)
        childNode = childNodes[0]
        self.assertTrue(childNode.endswith(childNodeSuffix))
        self.assertEqual(cmds.nodeType(childNode), childTypeName)

        return childNode

    def _ValidateCollapsed(self, nodeName):
        """
        Test that the assembly hierarchy is correct in Collapsed.
        There should be one child which is the proxy node. Note that this is
        the same for both the nested and non-nested cases.
        """
        proxyNode = self._ValidateNodeWithSingleChild(nodeName,
            self.ASSEMBLY_TYPE_NAME, 'CollapsedProxy', self.PROXY_TYPE_NAME)

        # Validate the attribute connections between the assembly node and its
        # proxy node.
        connectedAttrNames = [
            'filePath',
            'primPath',
            'complexity',
            'tint',
            'tintColor',
            'outStageData']

        for attr in connectedAttrNames:
            # Get the destination side of the attribute connection only, since
            # in the nested assembly case there may be an upstream and a
            # downstream connection.
            connections = cmds.listConnections('%s.%s' % (nodeName, attr),
                source=False, destination=True, plugs=True)
            self.assertEqual(len(connections), 1)

            destAttr = '%s.%s' % (proxyNode, attr)
            if attr == 'outStageData':
                destAttr = '%s.%s' % (proxyNode, 'inStageData')
            self.assertEqual(connections[0], destAttr)

    def _ValidateCards(self, nodeName):
        """
        Checks that the root prim on the stage has its drawmode properly set.
        """
        drawModeAttr = cmds.getAttr('%s.drawMode' % nodeName)
        self.assertEqual(drawModeAttr, 'cards')

        prim = UsdMaya.GetPrim(nodeName)
        primModelAPI = UsdGeom.ModelAPI(prim)
        self.assertEqual(primModelAPI.ComputeModelDrawMode(), 'cards')

    def _ValidateModelExpanded(self, nodeName):
        """
        Test that the model assembly hierarchy is correct in Expanded.
        There should be one immediate child under the assembly which is a
        transform, and then one immediate child under that which is a proxy.
        """
        transformNode = self._ValidateNodeWithSingleChild(nodeName,
            self.ASSEMBLY_TYPE_NAME, 'Geom', self.TRANSFORM_TYPE_NAME)

        proxyNode = self._ValidateNodeWithSingleChild(transformNode,
            self.TRANSFORM_TYPE_NAME, 'GeomProxy', self.PROXY_TYPE_NAME)

    def _ValidateModelFull(self, nodeName, nodeType=None):
        """
        Test that the model assembly hierarchy is correct in Full.
        There should be one immediate child under the assembly which is a
        transform, one immediate child under that which is another transform
        for a shape node, and then one immediate child under that which is a
        Maya mesh node.
        nodeType can also be specified as a different type. This is useful
        in the complex set case in a Full representation, where the Cube model
        gets fully unrolled.
        """
        if nodeType is None:
            nodeType = self.ASSEMBLY_TYPE_NAME

        transformNode = self._ValidateNodeWithSingleChild(nodeName,
            nodeType, 'Geom', self.TRANSFORM_TYPE_NAME)

        transformNode = self._ValidateNodeWithSingleChild(transformNode,
            self.TRANSFORM_TYPE_NAME, 'Cube', self.TRANSFORM_TYPE_NAME)

        meshNode = self._ValidateNodeWithSingleChild(transformNode,
            self.TRANSFORM_TYPE_NAME, 'CubeShape', self.MESH_TYPE_NAME)

    def _ValidateAllModelRepresentations(self, nodeName):
        """
        Tests all representations of an assembly node that references a
        model (i.e. has no sub-assemblies). This should apply for both a
        standalone model reference node as well as a nested assembly reference
        node that references a model.
        """
        # No representation has been activated yet, so ensure the assembly node
        # has no children.
        self._ValidateUnloaded(nodeName)

        # Change representations to 'Collapsed' and validate.
        cmds.assembly(nodeName, edit=True, active='Collapsed')
        self._ValidateCollapsed(nodeName)

        # Change representations to 'Cards' and validate.
        cmds.assembly(nodeName, edit=True, active='Cards')
        self._ValidateCards(nodeName)

        # Change representations to 'Expanded' and validate.
        cmds.assembly(nodeName, edit=True, active='Expanded')
        self._ValidateModelExpanded(nodeName)

        # Change representations to 'Full' and validate.
        cmds.assembly(nodeName, edit=True, active='Full')
        self._ValidateModelFull(nodeName)

        # Undo and the node should be back to Expanded.
        cmds.undo()
        self._ValidateModelExpanded(nodeName)

        # Undo and the node should be back to Cards.
        cmds.undo()
        self._ValidateCards(nodeName)

        # Undo and the node should be back to Collapsed.
        cmds.undo()
        self._ValidateCollapsed(nodeName)

        # Undo once more and no representation should be active.
        cmds.undo()
        self._ValidateUnloaded(nodeName)

        # Redo and it's back to Collapsed.
        cmds.redo()
        self._ValidateCollapsed(nodeName)

        # Redo and it's back to Cards.
        cmds.redo()
        self._ValidateCards(nodeName)

        # Redo again and it's back to Expanded.
        cmds.redo()
        self._ValidateModelExpanded(nodeName)

        # Redo once more and it's back to Full.
        cmds.redo()
        self._ValidateModelFull(nodeName)

    def _ValidateNestedExpandedTopLevel(self, nodeName):
        """
        Test that the nested assembly hierarchy is correct when the top-level
        assembly node is in Expanded. There should only be another unloaded
        assembly node under the top-level assembly node.
        """
        nestedAssemblyNode = self._ValidateNodeWithSingleChild(nodeName,
            self.ASSEMBLY_TYPE_NAME, 'Cube_1', self.ASSEMBLY_TYPE_NAME)
        self._ValidateUnloaded(nestedAssemblyNode)

    def _ValidateNestedFullTopLevel(self, nodeName):
        """
        Test that the nested assembly hierarchy is correct in Full.
        There should be one nested assembly node in the Collapsed
        representation underneath the top-level assembly node.
        """
        nestedAssemblyNode = self._ValidateNodeWithSingleChild(nodeName,
            self.ASSEMBLY_TYPE_NAME, 'Cube_1', self.ASSEMBLY_TYPE_NAME)
        self._ValidateCollapsed(nestedAssemblyNode)

    def _SetupScene(self, usdFilePath, primPath=None):
        """
        Sets up the test scene with an assembly node for the given usdFilePath
        and primPath and returns the name of the assembly node.
        """
        ASSEMBLY_NODE_NAME = 'TestAssemblyNode'

        cmds.file(new=True, force=True)

        usdFile = os.path.abspath(usdFilePath)

        assemblyNode = cmds.assembly(name=ASSEMBLY_NODE_NAME,
            type=self.ASSEMBLY_TYPE_NAME)
        cmds.setAttr("%s.filePath" % assemblyNode, usdFile, type='string')
        if primPath:
            cmds.setAttr("%s.primPath" % assemblyNode, primPath, type='string')

        self.assemNamespace = 'NS_%s' % assemblyNode

        return assemblyNode

    def testModelChangeReps(self):
        """
        This tests that changing representations of a USD reference assembly
        node in Maya that references a model works as expected, including
        undo'ing and redo'ing representation changes.
        """
        assemblyNode = self._SetupScene('CubeModel.usda', '/CubeModel')
        self._ValidateAllModelRepresentations(assemblyNode)

    def testModelNoDefaultPrimChangeReps(self):
        """
        This validates the behavior of changing representation of a USD
        reference assembly node in Maya that references a model when the USD
        file being referenced does NOT specify a default prim. In this case,
        switching to Expanded or Full representations will ONLY work when the
        assembly node provides the prim path.
        """
        assemblyNode = self._SetupScene('CubeModel_NoDefaultPrim.usda', '/CubeModel')
        self._ValidateAllModelRepresentations(assemblyNode)

        assemblyNodeNoPrimPath = self._SetupScene('CubeModel_NoDefaultPrim.usda')
        self._ValidateUnloaded(assemblyNodeNoPrimPath)

        # Change representations to 'Collapsed' and validate. This should work.
        cmds.assembly(assemblyNodeNoPrimPath, edit=True, active='Collapsed')
        self._ValidateCollapsed(assemblyNodeNoPrimPath)

        # Change representations to 'Expanded'. This should fail and we should
        # end up with nothing below the assembly (same as unloaded).
        cmds.assembly(assemblyNodeNoPrimPath, edit=True, active='Expanded')
        self._ValidateUnloaded(assemblyNodeNoPrimPath)

        # Change representations to 'Full'. Should be the same as 'Expanded'.
        cmds.assembly(assemblyNodeNoPrimPath, edit=True, active='Full')
        self._ValidateUnloaded(assemblyNodeNoPrimPath)

    def testNestedAssemblyChangeReps(self):
        """
        This tests that changing representations of a USD reference assembly
        node in Maya that references a hierarchy of assemblies works as
        expected, including undo'ing and redo'ing representation changes.
        """
        assemblyNode = self._SetupScene('OneCube_set.usda', '/set')

        # No representation has been activated yet, so ensure the assembly node
        # has no children.
        self._ValidateUnloaded(assemblyNode)

        # Change representations to 'Collapsed' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Collapsed')
        self._ValidateCollapsed(assemblyNode)

        # Change representations to 'Expanded' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Expanded')
        self._ValidateNestedExpandedTopLevel(assemblyNode)

        # Change representations to 'Full' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Full')
        self._ValidateNestedFullTopLevel(assemblyNode)

        # Undo and the node should be back to Expanded.
        cmds.undo()
        self._ValidateNestedExpandedTopLevel(assemblyNode)

        # Undo and the node should be back to Collapsed.
        cmds.undo()
        self._ValidateCollapsed(assemblyNode)

        # Undo once more and no representation should be active.
        cmds.undo()
        self._ValidateUnloaded(assemblyNode)

        # Redo and it's back to Collapsed.
        cmds.redo()
        self._ValidateCollapsed(assemblyNode)

        # Redo again and it's back to Expanded.
        cmds.redo()
        self._ValidateNestedExpandedTopLevel(assemblyNode)

        # Redo once more and it's back to Full.
        cmds.redo()
        self._ValidateNestedFullTopLevel(assemblyNode)


        # Now test changing representations of the sub-assembly.
        # Start by unloading the sub-assembly.
        childNodes = self._GetChildren(assemblyNode)
        nestedAssemblyNode = childNodes[0]
        cmds.assembly(nestedAssemblyNode, edit=True, active='')

        # From here, testing the nested assembly node should be the same as
        # testing a standalone model reference node.
        self._ValidateAllModelRepresentations(nestedAssemblyNode)

    def testNestedAssembliesWithVariantSetsChangeReps(self):
        """
        This tests that changing representations of a hierarchy of USD
        reference assembly nodes in Maya works as expected when the referenced
        prims have variant sets.
        """
        assemblyNode = self._SetupScene('SetWithModelingVariants_set.usda',
            '/SetWithModelingVariants_set')

        # Set the modelingVariant on the top-level assembly to the one that has
        # child prims.
        varSetAttrName = 'usdVariantSet_modelingVariant'
        cmds.addAttr(assemblyNode, ln=varSetAttrName, dt='string',
            internalSet=True)
        cmds.setAttr('%s.%s' % (assemblyNode, varSetAttrName), 'Cubes',
            type='string')

        # No representation has been activated yet, so ensure the assembly node
        # has no children.
        self._ValidateUnloaded(assemblyNode)

        # Change representations to 'Collapsed' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Collapsed')
        self._ValidateCollapsed(assemblyNode)

        # Change representations to 'Cards' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Cards')
        self._ValidateCards(assemblyNode)

        # Change representations to 'Expanded' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Expanded')

        childNodes = self._GetChildren(assemblyNode)
        cubesGroupNode = childNodes[0]
        childNodes = self._GetChildren(cubesGroupNode)
        cubeOneAssemblyNode = childNodes[0]

        # XXX: This test should be expanded to cover more combinations of 
        # representations like the other tests do, but more work is needed to
        # ensure that variant set selections work correctly with hierarchies of
        # assemblies.
        # For example, this model reference assembly does not currently behave
        # correctly when it appears because it is in a variant selection of a
        # parent assembly. For now, we just make sure that it doesn't crash
        # like it used to.
        # self._ValidateAllModelRepresentations(cubeOneAssemblyNode)
        cmds.assembly(cubeOneAssemblyNode, edit=True, active='Expanded')


    def _ValidateComplexSetExpandedTopLevel(self, nodeName):
        """
        Test that a complex set assembly hierarchy is correct when the top-level
        assembly node is in Expanded.
        """

        # The top-level assembly should have only a 'Geom' transform below it.
        geomTransformNode = self._ValidateNodeWithSingleChild(nodeName,
            self.ASSEMBLY_TYPE_NAME, 'Geom', self.TRANSFORM_TYPE_NAME)

        # The Geom transform should have six children, one proxy shape, four
        # transform nodes, and one camera.
        geomChildNodes = self._GetChildren(geomTransformNode)
        self.assertEqual(len(geomChildNodes), 6)

        # Validate the proxy.
        proxyShapeNode = '%s:GeomProxy' % self.assemNamespace
        self.assertIn(proxyShapeNode, geomChildNodes)
        self.assertEqual(cmds.nodeType(proxyShapeNode), self.PROXY_TYPE_NAME)

        # Validate CubesHero.
        cubesHeroTransformNode = '%s:CubesHero' % self.assemNamespace
        self.assertIn(cubesHeroTransformNode, geomChildNodes)
        self.assertEqual(cmds.nodeType(cubesHeroTransformNode), self.TRANSFORM_TYPE_NAME)

        # There should be three hero cubes and one camera under CubesHero.
        cubesHeroChildNodes = self._GetChildren(cubesHeroTransformNode)
        self.assertEqual(len(cubesHeroChildNodes), 4)

        for cubeNum in ['101', '102', '103']:
            cubeHeroNode = '%s:CubesHeroGeom%s' % (self.assemNamespace, cubeNum)
            self.assertIn(cubeHeroNode, cubesHeroChildNodes)
            cubeHeroProxyNode = self._ValidateNodeWithSingleChild(cubeHeroNode,
                self.TRANSFORM_TYPE_NAME, 'CubesHeroGeom%sProxy' % cubeNum, self.PROXY_TYPE_NAME)

        cubesHeroCameraNode = '%s:PerspCamUnderCubesHero' % self.assemNamespace
        self.assertIn(cubesHeroCameraNode, cubesHeroChildNodes)
        cameraShapeNode = self._ValidateNodeWithSingleChild(cubesHeroCameraNode,
            self.TRANSFORM_TYPE_NAME, 'PerspCamUnderCubesHeroShape', self.CAMERA_TYPE_NAME)

        # Validate the CubesFill and Ref proxies.
        cubesFillTransformNode = '%s:CubesFill' % self.assemNamespace
        self.assertIn(cubesFillTransformNode, geomChildNodes)
        cubesFillProxyNode = self._ValidateNodeWithSingleChild(cubesFillTransformNode,
            self.TRANSFORM_TYPE_NAME, 'CubesFillProxy', self.PROXY_TYPE_NAME)

        refTransformNode = '%s:Ref' % self.assemNamespace
        self.assertIn(refTransformNode, geomChildNodes)
        refProxyNode = self._ValidateNodeWithSingleChild(refTransformNode,
            self.TRANSFORM_TYPE_NAME, 'RefProxy', self.PROXY_TYPE_NAME)

        # Validate the ReferencedModels transform and the Cube_1 assembly.
        refModelsTransformNode = '%s:ReferencedModels' % self.assemNamespace
        self.assertIn(refModelsTransformNode, geomChildNodes)
        cubeAssemblyNode = self._ValidateNodeWithSingleChild(refModelsTransformNode,
            self.TRANSFORM_TYPE_NAME, 'Cube_1', self.ASSEMBLY_TYPE_NAME)

        # Validate the camera under Geom.
        geomCameraNode = '%s:PerspCamUnderGeom' % self.assemNamespace
        self.assertIn(geomCameraNode, geomChildNodes)
        cameraShapeNode = self._ValidateNodeWithSingleChild(geomCameraNode,
            self.TRANSFORM_TYPE_NAME, 'PerspCamUnderGeomShape', self.CAMERA_TYPE_NAME)

    def _ValidateComplexSetFullTopLevel(self, nodeName):
        """
        Test that a complex set assembly hierarchy is correct when the top-level
        assembly node is in Full.
        """

        # The top-level assembly should have only a 'Geom' transform below it.
        geomTransformNode = self._ValidateNodeWithSingleChild(nodeName,
            self.ASSEMBLY_TYPE_NAME, 'Geom', self.TRANSFORM_TYPE_NAME)

        # The Geom transform should have six transform node children.
        geomChildNodes = self._GetChildren(geomTransformNode)
        self.assertEqual(len(geomChildNodes), 6)

        # Validate the camera under Geom.
        geomCameraNode = '%s:PerspCamUnderGeom' % self.assemNamespace
        self.assertIn(geomCameraNode, geomChildNodes)
        cameraShapeNode = self._ValidateNodeWithSingleChild(geomCameraNode,
            self.TRANSFORM_TYPE_NAME, 'PerspCamUnderGeomShape', self.CAMERA_TYPE_NAME)

        # Validate CubesHero and CubesFill. They should be structured the same.
        for cubeType in ['CubesHero', 'CubesFill']:
            cubesTransformNode = '%s:%s' % (self.assemNamespace, cubeType)
            self.assertIn(cubesTransformNode, geomChildNodes)
            self.assertEqual(cmds.nodeType(cubesTransformNode), self.TRANSFORM_TYPE_NAME)

            # There should be three cubes and one camera under each cube scope.
            cubesChildNodes = self._GetChildren(cubesTransformNode)
            self.assertEqual(len(cubesChildNodes), 4)

            for cubeNum in ['101', '102', '103']:
                cubeNode = '%s:%sGeom%s' % (self.assemNamespace, cubeType, cubeNum)
                self.assertIn(cubeNode, cubesChildNodes)
                cubeShapeNode = self._ValidateNodeWithSingleChild(cubeNode,
                    self.TRANSFORM_TYPE_NAME, '%sGeom%sShape' % (cubeType, cubeNum), self.MESH_TYPE_NAME)

            cubesCameraNode = '%s:PerspCamUnder%s' % (self.assemNamespace, cubeType)
            self.assertIn(cubesCameraNode, cubesChildNodes)
            cameraShapeNode = self._ValidateNodeWithSingleChild(cubesCameraNode,
                self.TRANSFORM_TYPE_NAME, 'PerspCamUnder%sShape' % cubeType, self.CAMERA_TYPE_NAME)

        # Validate the ReferencedModels transform and the fully unrolled Cube_1
        # model reference assembly.
        refModelsTransformNode = '%s:ReferencedModels' % self.assemNamespace
        self.assertIn(refModelsTransformNode, geomChildNodes)
        cubeModelTransformNode = self._ValidateNodeWithSingleChild(refModelsTransformNode,
            self.TRANSFORM_TYPE_NAME, 'Cube_1', self.TRANSFORM_TYPE_NAME)
        self._ValidateModelFull(cubeModelTransformNode, nodeType=self.TRANSFORM_TYPE_NAME)

        # Validate the Ref node.
        refTransformNode = '%s:Ref' % self.assemNamespace
        self.assertIn(refTransformNode, geomChildNodes)
        self.assertEqual(cmds.nodeType(refTransformNode), self.TRANSFORM_TYPE_NAME)

        # There should be three reference cube meshes.
        refChildNodes = self._GetChildren(refTransformNode)
        self.assertEqual(len(refChildNodes), 3)

        for cubeName in ['RedCube', 'GreenCube', 'BlueCube']:
            cubeTransformNode = '%s:%s' % (self.assemNamespace, cubeName)
            self.assertIn(cubeTransformNode, refChildNodes)
            cubeShapeNode = self._ValidateNodeWithSingleChild(cubeTransformNode,
                self.TRANSFORM_TYPE_NAME, '%sShape' % cubeName, self.MESH_TYPE_NAME)

        # Validate the terrain mesh.
        terrainTransformNode = '%s:terrain' % self.assemNamespace
        self.assertIn(terrainTransformNode, geomChildNodes)
        terrainShapeNode = self._ValidateNodeWithSingleChild(terrainTransformNode,
            self.TRANSFORM_TYPE_NAME, 'terrainShape', self.MESH_TYPE_NAME)

    def testComplexSetAssemblyChangeReps(self):
        """
        This tests that changing representations of a USD reference assembly
        node in Maya that references a complex hierarchy of different types of
        prims works as expected, including undo'ing and redo'ing representation
        changes.
        """
        assemblyNode = self._SetupScene('ComplexSet.usda', '/ComplexSet')

        # No representation has been activated yet, so ensure the assembly node
        # has no children.
        self._ValidateUnloaded(assemblyNode)

        # Change representations to 'Collapsed' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Collapsed')
        self._ValidateCollapsed(assemblyNode)

        # Change representations to 'Cards' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Cards')
        self._ValidateCards(assemblyNode)

        # Change representations to 'Expanded' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Expanded')
        self._ValidateComplexSetExpandedTopLevel(assemblyNode)

        # Change representations to 'Full' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Full')
        self._ValidateComplexSetFullTopLevel(assemblyNode)

        # Undo and the node should be back to Expanded.
        cmds.undo()
        self._ValidateComplexSetExpandedTopLevel(assemblyNode)

        # Undo and the node should be back to Cards.
        cmds.undo()
        self._ValidateCards(assemblyNode)

        # Undo and the node should be back to Collapsed.
        cmds.undo()
        self._ValidateCollapsed(assemblyNode)

        # Undo once more and no representation should be active.
        cmds.undo()
        self._ValidateUnloaded(assemblyNode)

        # Redo and it's back to Collapsed.
        cmds.redo()
        self._ValidateCollapsed(assemblyNode)

        # Redo and it's back to Cards.
        cmds.redo()
        self._ValidateCards(assemblyNode)

        # Redo again and it's back to Expanded.
        cmds.redo()
        self._ValidateComplexSetExpandedTopLevel(assemblyNode)

        # Redo once more and it's back to Full.
        cmds.redo()
        self._ValidateComplexSetFullTopLevel(assemblyNode)

    def _AssertTimeIsConnected(self, assemblyNode):
        """
        Tests that the given assembly node's time attribute IS connected to
        Maya's global time as the destination.
        """
        assemblyTimePlug = '%s.time' % assemblyNode
        connections = cmds.listConnections(assemblyTimePlug,
            destination=False, source=True, plugs=True)
        self.assertEqual(connections, [u'time1.outTime'])

    def _AssertTimeIsNotConnected(self, assemblyNode):
        """
        Tests that the given assembly node's time attribute IS NOT connected to
        Maya's global time as the destination.
        """
        assemblyTimePlug = '%s.time' % assemblyNode
        connections = cmds.listConnections(assemblyTimePlug,
            destination=False, source=True, plugs=True)
        self.assertEqual(connections, None)

    def testAssemblyConnectedToTime(self):
        """
        This tests that when the Playback representation of a USD reference
        assembly node is activated that its input time is connected to Maya's
        global time. Other representations should NOT have this connection.
        """
        assemblyNode = self._SetupScene('CubeModel.usda', '/CubeModel')

        # Time should not be connected when no representation is active.
        self._AssertTimeIsNotConnected(assemblyNode)

        # Change representations to 'Collapsed' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Collapsed')
        self._AssertTimeIsNotConnected(assemblyNode)

        # Change representations to "Cards' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Cards')
        self._AssertTimeIsNotConnected(assemblyNode)

        # Change representations to 'Expanded' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Expanded')
        self._AssertTimeIsNotConnected(assemblyNode)

        # Change representations to 'Full' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Full')
        self._AssertTimeIsNotConnected(assemblyNode)

        # Change representations to 'Playback' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Playback')
        self._AssertTimeIsConnected(assemblyNode)

        # Change representations to 'Collapsed' once more and validate.
        cmds.assembly(assemblyNode, edit=True, active='Collapsed')
        self._AssertTimeIsNotConnected(assemblyNode)

    def testDisjointAssembliesVariantSetsChange(self):
        """
        Tests that changing a variant set in a nested assembly |A1|B does not
        affect the variant sets on a different nested assembly |A2|B where
        A1 and A2 reference the same model.
        """
        cmds.file(new=True, force=True)

        usdFile = os.path.abspath("OneCube_set.usda")
        primPath = "/set"

        assemblyNodes = []
        for name in ["assembly1", "assembly2"]:
            assemblyNode = cmds.assembly(
                    name=name, type=self.ASSEMBLY_TYPE_NAME)
            cmds.setAttr("%s.filePath" % assemblyNode, usdFile, type='string')
            cmds.setAttr("%s.primPath" % assemblyNode, primPath, type='string')
            cmds.assembly(assemblyNode, edit=True, active='Expanded')
            assemblyNodes.append(assemblyNode)

        cube1 = 'NS_%s:Cube_1' % assemblyNodes[0]
        self.assertTrue(cmds.objExists(cube1))

        cube2 = 'NS_%s:Cube_1' % assemblyNodes[1]
        self.assertTrue(cmds.objExists(cube2))

        cmds.setAttr('%s.usdVariantSet_shadingVariant' % cube1,
                'Blue', type='string')

        prim1 = UsdMaya.GetPrim(cube1)
        self.assertEqual(
                prim1.GetVariantSet('shadingVariant').GetVariantSelection(),
                'Blue')

        prim2 = UsdMaya.GetPrim(cube2)
        self.assertEqual(
                prim2.GetVariantSet('shadingVariant').GetVariantSelection(),
                'Default')


if __name__ == '__main__':
    unittest.main(verbosity=2)
