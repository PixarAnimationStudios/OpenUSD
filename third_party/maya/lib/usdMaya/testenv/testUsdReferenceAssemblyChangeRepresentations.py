#!/bedrock_subst/bin/pypixb

# Initialize maya.
import pixrun
pixrun.setupEnv()
from pixar import pxMayaVersion
pxMayaVersion.setup(version='mayaversion_subst')
import maya.standalone
maya.standalone.initialize('pypix')

from maya import cmds

from Mentor.Runtime import AssertEqual
from Mentor.Runtime import AssertIn
from Mentor.Runtime import AssertTrue
from Mentor.Runtime import FindDataFile
from Mentor.Runtime import Fixture
from Mentor.Runtime import Runner


class testUsdReferenceAssemblyChangeRepresentations(Fixture):

    ASSEMBLY_TYPE_NAME = 'pxrUsdReferenceAssembly'
    PROXY_TYPE_NAME = 'pxrUsdProxyShape'
    TRANSFORM_TYPE_NAME = 'transform'
    CAMERA_TYPE_NAME = 'camera'
    MESH_TYPE_NAME = 'mesh'

    def ClassSetup(self):
        cmds.loadPlugin('pxrUsd')

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
        AssertEqual(cmds.nodeType(nodeName), self.ASSEMBLY_TYPE_NAME)
        childNodes = self._GetChildren(nodeName)
        AssertEqual(childNodes, [])

    def _ValidateNodeWithSingleChild(self, nodeName, nodeTypeName,
            childNodeSuffix, childTypeName):
        """
        Tests that the given node is of type nodeTypeName with exactly one
        child node underneath it with the string in childNodeSuffix at the end
        of its name and of type childTypeName.
        The name of the child node is returned.
        """
        AssertEqual(cmds.nodeType(nodeName), nodeTypeName)
        childNodes = self._GetChildren(nodeName)
        AssertEqual(len(childNodes), 1)
        childNode = childNodes[0]
        AssertTrue(childNode.endswith(childNodeSuffix))
        AssertEqual(cmds.nodeType(childNode), childTypeName)

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
            AssertEqual(len(connections), 1)

            destAttr = '%s.%s' % (proxyNode, attr)
            if attr == 'outStageData':
                destAttr = '%s.%s' % (proxyNode, 'inStageData')
            AssertEqual(connections[0], destAttr)

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

        # Change representations to 'Expanded' and validate.
        cmds.assembly(nodeName, edit=True, active='Expanded')
        self._ValidateModelExpanded(nodeName)

        # Change representations to 'Full' and validate.
        cmds.assembly(nodeName, edit=True, active='Full')
        self._ValidateModelFull(nodeName)

        # Undo and the node should be back to Expanded.
        cmds.undo()
        self._ValidateModelExpanded(nodeName)

        # Undo and the node should be back to Collapsed.
        cmds.undo()
        self._ValidateCollapsed(nodeName)

        # Undo once more and no representation should be active.
        cmds.undo()
        self._ValidateUnloaded(nodeName)

        # Redo and it's back to Collapsed.
        cmds.redo()
        self._ValidateCollapsed(nodeName)

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

        usdFile = FindDataFile(usdFilePath)

        assemblyNode = cmds.assembly(name=ASSEMBLY_NODE_NAME,
            type=self.ASSEMBLY_TYPE_NAME)
        cmds.setAttr("%s.filePath" % assemblyNode, usdFile, type='string')
        if primPath:
            cmds.setAttr("%s.primPath" % assemblyNode, primPath, type='string')

        self.assemNamespace = 'NS_%s' % assemblyNode

        return assemblyNode

    def TestModelChangeReps(self):
        """
        This tests that changing representations of a USD reference assembly
        node in Maya that references a model works as expected, including
        undo'ing and redo'ing representation changes.
        """
        assemblyNode = self._SetupScene('CubeModel.usda', '/CubeModel')
        self._ValidateAllModelRepresentations(assemblyNode)

    def TestModelNoDefaultPrimChangeReps(self):
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

    def TestNestedAssemblyChangeReps(self):
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
        AssertEqual(len(geomChildNodes), 6)

        # Validate the proxy.
        proxyShapeNode = '%s:GeomProxy' % self.assemNamespace
        AssertIn(proxyShapeNode, geomChildNodes)
        AssertEqual(cmds.nodeType(proxyShapeNode), self.PROXY_TYPE_NAME)

        # Validate RocksHero.
        rocksHeroTransformNode = '%s:RocksHero' % self.assemNamespace
        AssertIn(rocksHeroTransformNode, geomChildNodes)
        AssertEqual(cmds.nodeType(rocksHeroTransformNode), self.TRANSFORM_TYPE_NAME)

        # There should be three hero rocks and one camera under RocksHero.
        rocksHeroChildNodes = self._GetChildren(rocksHeroTransformNode)
        AssertEqual(len(rocksHeroChildNodes), 4)

        for rockNum in ['101', '102', '103']:
            rockHeroNode = '%s:RocksHeroGeom%s' % (self.assemNamespace, rockNum)
            AssertIn(rockHeroNode, rocksHeroChildNodes)
            rockHeroProxyNode = self._ValidateNodeWithSingleChild(rockHeroNode,
                self.TRANSFORM_TYPE_NAME, 'RocksHeroGeom%sProxy' % rockNum, self.PROXY_TYPE_NAME)

        rocksHeroCameraNode = '%s:PerspCamUnderRocksHero' % self.assemNamespace
        AssertIn(rocksHeroCameraNode, rocksHeroChildNodes)
        cameraShapeNode = self._ValidateNodeWithSingleChild(rocksHeroCameraNode,
            self.TRANSFORM_TYPE_NAME, 'PerspCamUnderRocksHeroShape', self.CAMERA_TYPE_NAME)

        # Validate the RocksFill and Ref proxies.
        rocksFillTransformNode = '%s:RocksFill' % self.assemNamespace
        AssertIn(rocksFillTransformNode, geomChildNodes)
        rocksFillProxyNode = self._ValidateNodeWithSingleChild(rocksFillTransformNode,
            self.TRANSFORM_TYPE_NAME, 'RocksFillProxy', self.PROXY_TYPE_NAME)

        refTransformNode = '%s:Ref' % self.assemNamespace
        AssertIn(refTransformNode, geomChildNodes)
        refProxyNode = self._ValidateNodeWithSingleChild(refTransformNode,
            self.TRANSFORM_TYPE_NAME, 'RefProxy', self.PROXY_TYPE_NAME)

        # Validate the ReferencedModels transform and the Cube_1 assembly.
        refModelsTransformNode = '%s:ReferencedModels' % self.assemNamespace
        AssertIn(refModelsTransformNode, geomChildNodes)
        cubeAssemblyNode = self._ValidateNodeWithSingleChild(refModelsTransformNode,
            self.TRANSFORM_TYPE_NAME, 'Cube_1', self.ASSEMBLY_TYPE_NAME)

        # Validate the camera under Geom.
        geomCameraNode = '%s:PerspCamUnderGeom' % self.assemNamespace
        AssertIn(geomCameraNode, geomChildNodes)
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
        AssertEqual(len(geomChildNodes), 6)

        # Validate the camera under Geom.
        geomCameraNode = '%s:PerspCamUnderGeom' % self.assemNamespace
        AssertIn(geomCameraNode, geomChildNodes)
        cameraShapeNode = self._ValidateNodeWithSingleChild(geomCameraNode,
            self.TRANSFORM_TYPE_NAME, 'PerspCamUnderGeomShape', self.CAMERA_TYPE_NAME)

        # Validate RocksHero and RocksFill. They should be structured the same.
        for rockType in ['RocksHero', 'RocksFill']:
            rocksTransformNode = '%s:%s' % (self.assemNamespace, rockType)
            AssertIn(rocksTransformNode, geomChildNodes)
            AssertEqual(cmds.nodeType(rocksTransformNode), self.TRANSFORM_TYPE_NAME)

            # There should be three rocks and one camera under each rock scope.
            rocksChildNodes = self._GetChildren(rocksTransformNode)
            AssertEqual(len(rocksChildNodes), 4)

            for rockNum in ['101', '102', '103']:
                rockNode = '%s:%sGeom%s' % (self.assemNamespace, rockType, rockNum)
                AssertIn(rockNode, rocksChildNodes)
                rockShapeNode = self._ValidateNodeWithSingleChild(rockNode,
                    self.TRANSFORM_TYPE_NAME, '%sGeom%sShape' % (rockType, rockNum), self.MESH_TYPE_NAME)

            rocksCameraNode = '%s:PerspCamUnder%s' % (self.assemNamespace, rockType)
            AssertIn(rocksCameraNode, rocksChildNodes)
            cameraShapeNode = self._ValidateNodeWithSingleChild(rocksCameraNode,
                self.TRANSFORM_TYPE_NAME, 'PerspCamUnder%sShape' % rockType, self.CAMERA_TYPE_NAME)

        # Validate the ReferencedModels transform and the fully unrolled Cube_1
        # model reference assembly.
        refModelsTransformNode = '%s:ReferencedModels' % self.assemNamespace
        AssertIn(refModelsTransformNode, geomChildNodes)
        cubeModelTransformNode = self._ValidateNodeWithSingleChild(refModelsTransformNode,
            self.TRANSFORM_TYPE_NAME, 'Cube_1', self.TRANSFORM_TYPE_NAME)
        self._ValidateModelFull(cubeModelTransformNode, nodeType=self.TRANSFORM_TYPE_NAME)

        # Validate the Ref node.
        refTransformNode = '%s:Ref' % self.assemNamespace
        AssertIn(refTransformNode, geomChildNodes)
        AssertEqual(cmds.nodeType(refTransformNode), self.TRANSFORM_TYPE_NAME)

        # There should be three reference character meshes.
        refChildNodes = self._GetChildren(refTransformNode)
        AssertEqual(len(refChildNodes), 3)

        for character in ['Dory', 'Nemo', 'Marlin']:
            characterTransformNode = '%s:%s' % (self.assemNamespace, character)
            AssertIn(characterTransformNode, refChildNodes)
            characterShapeNode = self._ValidateNodeWithSingleChild(characterTransformNode,
                self.TRANSFORM_TYPE_NAME, '%sShape' % character, self.MESH_TYPE_NAME)

        # Validate the terrain mesh.
        terrainTransformNode = '%s:terrainAlgaeGeom' % self.assemNamespace
        AssertIn(terrainTransformNode, geomChildNodes)
        terrainShapeNode = self._ValidateNodeWithSingleChild(terrainTransformNode,
            self.TRANSFORM_TYPE_NAME, 'terrainAlgaeGeomShape', self.MESH_TYPE_NAME)

    def TestComplexSetAssemblyChangeReps(self):
        """
        This tests that changing representations of a USD reference assembly
        node in Maya that references a complex hierarchy of different types of
        prims works as expected, including undo'ing and redo'ing representation
        changes.
        """
        assemblyNode = self._SetupScene('TerrainGlowingAlgae.usda',
            '/TerrainGlowingAlgae')

        # No representation has been activated yet, so ensure the assembly node
        # has no children.
        self._ValidateUnloaded(assemblyNode)

        # Change representations to 'Collapsed' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Collapsed')
        self._ValidateCollapsed(assemblyNode)

        # Change representations to 'Expanded' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Expanded')
        self._ValidateComplexSetExpandedTopLevel(assemblyNode)

        # Change representations to 'Full' and validate.
        cmds.assembly(assemblyNode, edit=True, active='Full')
        self._ValidateComplexSetFullTopLevel(assemblyNode)

        # Undo and the node should be back to Expanded.
        cmds.undo()
        self._ValidateComplexSetExpandedTopLevel(assemblyNode)

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
        self._ValidateComplexSetExpandedTopLevel(assemblyNode)

        # Redo once more and it's back to Full.
        cmds.redo()
        self._ValidateComplexSetFullTopLevel(assemblyNode)


if __name__ == '__main__':
    Runner().Main()
