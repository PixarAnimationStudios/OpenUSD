#!/pxrpythonsubst
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
#

import unittest, os

from pxr import Gf, Usd, UsdSkel

from maya import cmds
from maya import standalone

import maya.api.OpenMaya as OM


def _MMatrixToGf(mx):
    gfmx = Gf.Matrix4d()
    for i in xrange(4):
        for j in xrange(4):
            gfmx[i][j] = mx[i*4+j]
    return gfmx


def _GfMatrixToList(mx):
    return [mx[i][j] for i in xrange(4) for j in xrange(4)]


def _GetDepNode(name):
    selectionList = OM.MSelectionList()
    selectionList.add(name)
    return OM.MFnDependencyNode(selectionList.getDependNode(0))


def _ArraysAreClose(a, b, threshold=1e-5):
    if not len(a) == len(b):
        return False

    for i in xrange(len(a)):
        if not Gf.IsClose(a[i], b[i], threshold):
            return False
    return True


class testUsdImportSkeleton(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd', quiet=True)


    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()


    def _ValidateJointTransforms(self, usdSkelQuery, joints):

        for time in usdSkelQuery.GetAnimQuery().GetJointTransformTimeSamples():

            cmds.currentTime(time)

            # Check local transforms
            usdXforms = usdSkelQuery.ComputeJointLocalTransforms(time)

            for i,joint in enumerate(joints):

                jointXf = cmds.getAttr("%s.matrix"%joint.name())

                self.assertTrue(_ArraysAreClose(
                    _GfMatrixToList(usdXforms[i]), jointXf))

            # Check skel-space transforms
            usdXforms = usdSkelQuery.ComputeJointSkelTransforms(time)

            for i,joint in enumerate(joints):
                jointXf = cmds.getAttr("%s.worldMatrix"%joint.name())
                
                self.assertTrue(_ArraysAreClose(
                    _GfMatrixToList(usdXforms[i]), jointXf))


    def _ValidateJointBindPoses(self, usdSkelQuery, joints):

        restXforms = usdSkelQuery.ComputeJointSkelTransforms(atRest=True)
        for i,joint in enumerate(joints):

            bindPose = cmds.getAttr("%s.bindPose"%joint.name())

            self.assertTrue(_ArraysAreClose(
                bindPose, _GfMatrixToList(restXforms[i])))


    def _ValidateJoints(self, usdSkelQuery, joints):

        self._ValidateJointTransforms(usdSkelQuery, joints)
        self._ValidateJointBindPoses(usdSkelQuery, joints)

        for joint in joints:
            segmentScaleCompensate = cmds.getAttr(
                "%s.segmentScaleComponensate"%joint.name())
            self.assertEqual(segmentScaleCompensate, False)


    def _ValidateBindPose(self, name, usdSkelQuery, joints):

        bindPose = _GetDepNode(name)
        self.assertEqual(bindPose.typeName, "dagPose")

        for i,joint in enumerate(joints):

            parentIdx = usdSkelQuery.GetTopology().GetParentIndices()[i]
            
            connections = cmds.listConnections(
                "%s.members[%d]"%(name, i),
                destination=False, source=True, plugs=True)
            self.assertEqual(connections, [u"%s.message"%joint.name()])

            connections = cmds.listConnections(
                "%s.worldMatrix[%d]"%(name, i),
                destination=False, source=True, plugs=True)
            self.assertEqual(connections, [u"%s.bindPose"%joint.name()])

            connections = cmds.listConnections(
                "%s.parents[%d]"%(name, i),
                destination=False, source=True, plugs=True)

            if parentIdx >= 0:
                self.assertEqual(connections, [u"%s.members[%d]"%(name,parentIdx)])
            else:
                self.assertEqual(connections, [u"%s.world"%name])

        self.assertTrue(cmds.getAttr("bindPose.bindPose"))


    def _ValidateMeshTransform(self, name, usdSkinningQuery):

        mesh = _GetDepNode(name)
        self.assertEqual(mesh.typeName, "transform")

        # inheritsTransform must be disabled to prevent
        # double transform issues.
        self.assertEqual(cmds.getAttr("%s.inheritsTransform"%name), False)

        # Mesh's transform should match the geomBindTransform.
        self.assertTrue(_ArraysAreClose(
            cmds.getAttr("%s.worldMatrix"%name),
            _GfMatrixToList(usdSkinningQuery.GetGeomBindTransform())))


    def _ValidateSkinClusterRig(self, joints, skinClusterName, groupPartsName,
                                groupIdName, bindPoseName, meshName,
                                usdSkelQuery, usdSkinningQuery):
        
        skinCluster = _GetDepNode(skinClusterName)
        self.assertEqual(skinCluster.typeName, "skinCluster")

        groupParts = _GetDepNode(groupPartsName)
        self.assertEqual(groupParts.typeName, "groupParts")
        
        groupId = _GetDepNode(groupIdName)
        self.assertEqual(groupId.typeName, "groupId")

        self.assertTrue(
            _ArraysAreClose(cmds.getAttr("%s.geomMatrix"%skinClusterName),
                            _GfMatrixToList(
                                usdSkinningQuery.GetGeomBindTransform())))

        connections = cmds.listConnections(
            "%s.groupId"%groupIdName,
            destination=True, source=False, plugs=True)
        self.assertEqual(
            sorted(connections),
            sorted([u"%sShape.instObjGroups.objectGroups[0].objectGroupId"%meshName,
                    u"%s.groupId"%groupPartsName,
                    u"%s.input[0].groupId"%skinClusterName]))

        connections = cmds.listConnections(
            "%s.outputGeometry"%groupPartsName,
            destination=True, source=False, plugs=True)
        self.assertEqual(connections, [u"%s.input[0].inputGeometry"%skinClusterName])

        connections = cmds.listConnections(
            "%s.outputGeometry[0]"%skinClusterName,
            destination=True, source=False, plugs=True)
        self.assertEqual(connections, [u"%s.inMesh"%(meshName+"Shape")])

        skelRestXforms = usdSkelQuery.ComputeJointSkelTransforms(atRest=True)

        connections = cmds.listConnections(
            "%s.bindPose"%skinClusterName,
            destination=False, source=True, plugs=True)
        self.assertEqual(connections, [u"%s.message"%bindPoseName])

        for i,joint in enumerate(joints):

            connections = cmds.listConnections(
                "%s.worldMatrix[0]"%joint.name(),
                destination=True, source=False, plugs=True)
            self.assertEqual(connections, [u"%s.matrix[%d]"%(skinClusterName,i)])

            # bindPreMatrix should be the inverse of the skel-
            # rest tranfsorm.

            self.assertTrue(_ArraysAreClose(
                cmds.getAttr("%s.bindPreMatrix[%d]"%(skinClusterName,i)),
                _GfMatrixToList(skelRestXforms[i].GetInverse())))


    def test_SkelImport(self):
        cmds.file(new=True, force=True)

        path = os.path.abspath("skelCube.usda")

        cmds.usdImport(file=path, readAnimData=True, primPath="/Root",
                       assemblyRep="Import", shadingMode="none")

        stage = Usd.Stage.Open(path)
        skelCache = UsdSkel.Cache()
        
        bindingSitePrim = stage.GetPrimAtPath("/Root")
        self.assertTrue(bindingSitePrim.IsA(UsdSkel.Root))
        
        skelCache.Populate(UsdSkel.Root(bindingSitePrim))
        skelQuery = skelCache.GetSkelQuery(bindingSitePrim)
        self.assertTrue(skelQuery)

        meshPrim = stage.GetPrimAtPath("/Root/Cube")
        self.assertTrue(meshPrim)

        skinningQuery = skelCache.GetSkinningQuery(meshPrim)
        self.assertTrue(skinningQuery)

        jointNames = [name.split("/")[-1] for name in skelQuery.GetJointOrder()]

        joints = [_GetDepNode(n) for n in jointNames]
        self.assertTrue(all(joints))

        self._ValidateJointTransforms(skelQuery, joints)
        self._ValidateJointBindPoses(skelQuery, joints)

        self._ValidateBindPose("bindPose", skelQuery, joints)

        self._ValidateMeshTransform(meshPrim.GetName(), skinningQuery)

        self._ValidateSkinClusterRig(joints=joints,
                                     skinClusterName="skinCluster1",
                                     groupPartsName="skinClusterGroupParts",
                                     groupIdName="skinClusterGroupId",
                                     bindPoseName="bindPose",
                                     meshName=meshPrim.GetName(),
                                     usdSkelQuery=skelQuery,
                                     usdSkinningQuery=skinningQuery)


if __name__ == '__main__':
    unittest.main(verbosity=2)
