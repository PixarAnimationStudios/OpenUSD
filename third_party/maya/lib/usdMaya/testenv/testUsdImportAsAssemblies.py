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

from maya import cmds
from maya import standalone


class testUsdImportAsAssemblies(unittest.TestCase):

    MODEL_USD_FILE = 'CubeModel.usda'
    MODEL_ROOT_PRIM_PATH = '/CubeModel'

    SET_USD_FILE = 'Cubes_set.usda'

    ASSEMBLY_TYPE_NAME = 'pxrUsdReferenceAssembly'
    PROXY_TYPE_NAME = 'pxrUsdProxyShape'
    TRANSFORM_TYPE_NAME = 'transform'
    MESH_TYPE_NAME = 'mesh'

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    @staticmethod
    def _ImportPrimPathFromSet(*args, **kwargs):
        usdFile = os.path.abspath(testUsdImportAsAssemblies.SET_USD_FILE)
        cmds.usdImport(file=usdFile, **kwargs)

    @staticmethod
    def _GetNodeHierarchy(nodeName):
        """
        Returns a dictionary mapping Maya node names to the type of that node
        for all nodes underneath and including nodeName.
        """
        dagNodeNames = cmds.ls(nodeName, dag=True, long=True)

        result = {}
        for dagNodeName in dagNodeNames:
            result[dagNodeName] = cmds.nodeType(dagNodeName)

        return result

    def _ValidateModelAssemblyNode(self, nodeName):
        nodeType = cmds.nodeType(nodeName)
        self.assertEqual(nodeType, self.ASSEMBLY_TYPE_NAME)

        filePath = cmds.getAttr('%s.filePath' % nodeName)
        self.assertEqual(filePath, self.MODEL_USD_FILE)

        primPath = cmds.getAttr('%s.primPath' % nodeName)
        self.assertEqual(primPath, self.MODEL_ROOT_PRIM_PATH)

    def testImportModelFromSet(self):
        """
        Tests that importing an individual model reference prim out of a set
        USD file results in the contents of that model being imported (i.e. NO
        reference assembly node is created).
        """
        self._ImportPrimPathFromSet(primPath='/Cubes_set/Cubes_grp/Cube_3')

        expectedHierarchy = {
            '|Cube_3': self.TRANSFORM_TYPE_NAME,
            '|Cube_3|Geom': self.TRANSFORM_TYPE_NAME,
            '|Cube_3|Geom|Cube': self.TRANSFORM_TYPE_NAME,
            '|Cube_3|Geom|Cube|CubeShape': self.MESH_TYPE_NAME
        }

        nodeName = '|Cube_3'
        nodeHierarchy = self._GetNodeHierarchy(nodeName)
        self.assertEqual(nodeHierarchy, expectedHierarchy)

    def testImportGroupFromSet(self):
        """
        Tests that importing a group prim that is the parent of multiple model
        reference prims out of a set USD file results in a Maya transform node
        with the corresponding number of model reference assembly node beneath it.
        """
        self._ImportPrimPathFromSet(primPath='/Cubes_set/Cubes_grp')

        expectedHierarchy = {
            '|Cubes_grp': self.TRANSFORM_TYPE_NAME,
            '|Cubes_grp|Cube_1': self.ASSEMBLY_TYPE_NAME,
            '|Cubes_grp|Cube_1|NS_Cube_1:CollapsedProxy': self.PROXY_TYPE_NAME,
            '|Cubes_grp|Cube_2': self.ASSEMBLY_TYPE_NAME,
            '|Cubes_grp|Cube_2|NS_Cube_2:CollapsedProxy': self.PROXY_TYPE_NAME,
            '|Cubes_grp|Cube_3': self.ASSEMBLY_TYPE_NAME,
            '|Cubes_grp|Cube_3|NS_Cube_3:CollapsedProxy': self.PROXY_TYPE_NAME
        }

        nodeName = '|Cubes_grp'
        nodeHierarchy = self._GetNodeHierarchy(nodeName)
        self.assertEqual(nodeHierarchy, expectedHierarchy)

        for i in xrange(1, 4):
            assemblyNodeName = '|Cubes_grp|Cube_%d' % i
            self._ValidateModelAssemblyNode(assemblyNodeName)

    def _ValidateFullSetImport(self):
        expectedHierarchy = {
            '|Cubes_set': self.TRANSFORM_TYPE_NAME,
            '|Cubes_set|Cubes_grp': self.TRANSFORM_TYPE_NAME,
            '|Cubes_set|Cubes_grp|Cube_1': self.ASSEMBLY_TYPE_NAME,
            '|Cubes_set|Cubes_grp|Cube_1|NS_Cube_1:CollapsedProxy': self.PROXY_TYPE_NAME,
            '|Cubes_set|Cubes_grp|Cube_2': self.ASSEMBLY_TYPE_NAME,
            '|Cubes_set|Cubes_grp|Cube_2|NS_Cube_2:CollapsedProxy': self.PROXY_TYPE_NAME,
            '|Cubes_set|Cubes_grp|Cube_3': self.ASSEMBLY_TYPE_NAME,
            '|Cubes_set|Cubes_grp|Cube_3|NS_Cube_3:CollapsedProxy': self.PROXY_TYPE_NAME
        }

        nodeName = '|Cubes_set'
        nodeHierarchy = self._GetNodeHierarchy(nodeName)
        self.assertEqual(nodeHierarchy, expectedHierarchy)

        for i in xrange(1, 4):
            assemblyNodeName = '|Cubes_set|Cubes_grp|Cube_%d' % i
            self._ValidateModelAssemblyNode(assemblyNodeName)

    def testImportRootOfSet(self):
        """
        Tests that importing the root prim out of a set USD file results in
        the correct hierarchy of Maya transforms and model reference assembly
        nodes.
        """
        self._ImportPrimPathFromSet(primPath='/Cubes_set')
        self._ValidateFullSetImport()

    def testImportSetNoPrimPath(self):
        """
        Tests that importing a set USD file without specifying a prim path
        results in the correct hierarchy of Maya transforms and model reference
        assembly nodes.
        """
        self._ImportPrimPathFromSet()
        self._ValidateFullSetImport()

    def testImportGroupAssemblyRepImport(self):
        """
        Tests that importing a group prim from a set USD file with
        assemblyRep="Import" specified to usdImport results in the correct
        hierarchy of Maya transforms and mesh nodes. No model reference
        assembly nodes should be created in this case.
        """
        self._ImportPrimPathFromSet(primPath='/Cubes_set/Cubes_grp',
            assemblyRep='Import')

        expectedHierarchy = {
            '|Cubes_grp': self.TRANSFORM_TYPE_NAME,
            '|Cubes_grp|Cube_1': self.TRANSFORM_TYPE_NAME,
            '|Cubes_grp|Cube_1|Geom': self.TRANSFORM_TYPE_NAME,
            '|Cubes_grp|Cube_1|Geom|Cube': self.TRANSFORM_TYPE_NAME,
            '|Cubes_grp|Cube_1|Geom|Cube|CubeShape': self.MESH_TYPE_NAME,
            '|Cubes_grp|Cube_2': self.TRANSFORM_TYPE_NAME,
            '|Cubes_grp|Cube_2|Geom': self.TRANSFORM_TYPE_NAME,
            '|Cubes_grp|Cube_2|Geom|Cube': self.TRANSFORM_TYPE_NAME,
            '|Cubes_grp|Cube_2|Geom|Cube|CubeShape': self.MESH_TYPE_NAME,
            '|Cubes_grp|Cube_3': self.TRANSFORM_TYPE_NAME,
            '|Cubes_grp|Cube_3|Geom': self.TRANSFORM_TYPE_NAME,
            '|Cubes_grp|Cube_3|Geom|Cube': self.TRANSFORM_TYPE_NAME,
            '|Cubes_grp|Cube_3|Geom|Cube|CubeShape': self.MESH_TYPE_NAME
        }

        nodeName = '|Cubes_grp'
        nodeHierarchy = self._GetNodeHierarchy(nodeName)
        self.assertEqual(nodeHierarchy, expectedHierarchy)

        assemblyNodes = cmds.ls(dag=True, type=self.ASSEMBLY_TYPE_NAME)
        self.assertEqual(assemblyNodes, [])


if __name__ == '__main__':
    unittest.main(verbosity=2)
