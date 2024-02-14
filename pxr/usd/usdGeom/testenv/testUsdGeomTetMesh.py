#!/pxrpythonsubst
#
# Copyright 2023 Pixar
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

import sys, unittest
from pxr import Usd, UsdGeom, Vt, Gf

class TestUsdGeomTetMesh(unittest.TestCase):
    # Tests time varying topology and surface computation for a rightHanded
    # orientation tetmesh
    def test_ComputeSurfaceExtractionFromUsdGeomTetMeshRightHanded(self):
        stage = Usd.Stage.CreateInMemory()
        myTetMesh = UsdGeom.TetMesh.Define(stage,"/tetMesh")
        pointsAttr = myTetMesh.GetPointsAttr()

        pointsTime0 = Vt.Vec3fArray(5, (Gf.Vec3f(0.0, 0.0, 0.0),
                                        Gf.Vec3f(2.0, 0.0, 0.0),
                                        Gf.Vec3f(0.0, 2.0, 0.0),
                                        Gf.Vec3f(0.0, 0.0, 2.0),
                                        Gf.Vec3f(0.0, 0.0, -2.0)))

        pointsAttr.Set(pointsTime0, 0.0)

        pointsTime10 = Vt.Vec3fArray(8, (Gf.Vec3f(0.0, 0.0, 3.0),
                                        Gf.Vec3f(2.0, 0.0, 3.0),
                                        Gf.Vec3f(0.0, 2.0, 3.0),
                                        Gf.Vec3f(0.0, 0.0, 5.0),
                                        Gf.Vec3f(0.0, 0.0, -3.0),
                                        Gf.Vec3f(2.0, 0.0, -3.0),
                                        Gf.Vec3f(0.0, 2.0, -3.0),
                                        Gf.Vec3f(0.0, 0.0, -5.0)))

        pointsAttr.Set(pointsTime10, 10.0)

        tetVertexIndicesAttr = myTetMesh.GetTetVertexIndicesAttr();
        tetIndicesTime0 = Vt.Vec4iArray(2, (Gf.Vec4i(0,1,2,3),
                                            Gf.Vec4i(0,2,1,4)))

        tetVertexIndicesAttr.Set(tetIndicesTime0, 0.0)

        tetIndicesTime10 = Vt.Vec4iArray(2, (Gf.Vec4i(0,1,2,3),
                                             Gf.Vec4i(4,6,5,7)))

        tetVertexIndicesAttr.Set(tetIndicesTime10, 10.0)

        # Check for inverted elements at frame 0 
        invertedElementsTime0 = UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 10.0)
        self.assertEqual(len(invertedElementsTime0), 0)  

        # Check for inverted elements at frame 10 
        invertedElementsTime10 = UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 10.0)
        self.assertEqual(len(invertedElementsTime10), 0) 

        surfaceFacesTime0 = UsdGeom.TetMesh.ComputeSurfaceFaces(myTetMesh, 0.0)

        # When the tets are joined we have 6 faces
        self.assertEqual(len(surfaceFacesTime0), 6)

        surfaceFacesTime10 = UsdGeom.TetMesh.ComputeSurfaceFaces(myTetMesh, 10.0)
       
        # When they separate we have 8 faces
        self.assertEqual(len(surfaceFacesTime10), 8)

        triMesh = UsdGeom.Mesh.Define(stage,"/triMesh")
        triMeshPointsAttr = triMesh.GetPointsAttr()
        triMeshPointsAttr.Set(pointsTime0, 0.0)
        triMeshPointsAttr.Set(pointsTime10, 10.0)
        triMeshFaceVertexCountsAttr = triMesh.GetFaceVertexCountsAttr()

        faceVertexCountsTime0 = Vt.IntArray(6, (3,3,3,3,3,3))
        triMeshFaceVertexCountsAttr.Set(faceVertexCountsTime0, 0.0)
        faceVertexCountsTime10 = Vt.IntArray(8, (3,3,3,3,3,3,3,3))
        triMeshFaceVertexCountsAttr.Set(faceVertexCountsTime10, 10.0)

        # Need to convert surfaceFaceIndices from VtVec3iArray to VtIntArray
        faceVertexIndicesTime0 = Vt.IntArray(18)
        for i in range(0,6):
            for j in range(0,3):
                faceVertexIndicesTime0[i * 3 + j] = surfaceFacesTime0[i][j]

        faceVertexIndicesTime10 = Vt.IntArray(24)
        for i in range(0,8):
            for j in range(0,3):
                faceVertexIndicesTime10[i * 3 + j] = surfaceFacesTime10[i][j]

        triMeshFaceVertexIndicesAttr = triMesh.GetFaceVertexIndicesAttr()
        triMeshFaceVertexIndicesAttr.Set(faceVertexIndicesTime0, 0.0)
        triMeshFaceVertexIndicesAttr.Set(faceVertexIndicesTime10, 10.0)
        stage.SetStartTimeCode(0.0)
        stage.SetEndTimeCode(15.0)
        stage.Export('tetMeshRH.usda')

    # Tests time varying topology and surface computation for a leftHanded
    # orientation tetmesh
    def test_ComputeSurfaceExtractionFromUsdGeomTetMeshLeftHanded(self):
        stage = Usd.Stage.CreateInMemory()
        myTetMesh = UsdGeom.TetMesh.Define(stage,"/tetMesh")
        orientationAttr = myTetMesh.GetOrientationAttr();   
        orientationAttr.Set(UsdGeom.Tokens.leftHanded)        
        pointsAttr = myTetMesh.GetPointsAttr()

        pointsTime0 = Vt.Vec3fArray(5, (Gf.Vec3f(0.0, 0.0, 0.0),
                                        Gf.Vec3f(-2.0, 0.0, 0.0),
                                        Gf.Vec3f(0.0, 2.0, 0.0),
                                        Gf.Vec3f(0.0, 0.0, 2.0),
                                        Gf.Vec3f(0.0, 0.0, -2.0)))

        pointsAttr.Set(pointsTime0, 0.0)

        pointsTime10 = Vt.Vec3fArray(8, (Gf.Vec3f(0.0, 0.0, 3.0),
                                        Gf.Vec3f(-2.0, 0.0, 3.0),
                                        Gf.Vec3f(0.0, 2.0, 3.0),
                                        Gf.Vec3f(0.0, 0.0, 5.0),
                                        Gf.Vec3f(0.0, 0.0, -3.0),
                                        Gf.Vec3f(-2.0, 0.0, -3.0),
                                        Gf.Vec3f(0.0, 2.0, -3.0),
                                        Gf.Vec3f(0.0, 0.0, -5.0)))

        pointsAttr.Set(pointsTime10, 10.0)

        tetVertexIndicesAttr = myTetMesh.GetTetVertexIndicesAttr();
        tetIndicesTime0 = Vt.Vec4iArray(2, (Gf.Vec4i(0,1,2,3),
                                            Gf.Vec4i(0,2,1,4)))

        tetVertexIndicesAttr.Set(tetIndicesTime0, 0.0)

        tetIndicesTime10 = Vt.Vec4iArray(2, (Gf.Vec4i(0,1,2,3),
                                             Gf.Vec4i(4,6,5,7)))

        tetVertexIndicesAttr.Set(tetIndicesTime10, 10.0)


        # Check for inverted elements at frame 0 
        invertedElementsTime0 = UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 10.0)
        self.assertEqual(len(invertedElementsTime0), 0)  
        
        # Check for inverted elements at frame 10 
        invertedElementsTime10 = UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 10.0)
        self.assertEqual(len(invertedElementsTime10), 0) 

        surfaceFacesTime0 = UsdGeom.TetMesh.ComputeSurfaceFaces(myTetMesh, 0.0)

        # When the tets are joined we have 6 faces
        self.assertEqual(len(surfaceFacesTime0), 6)

        surfaceFacesTime10 = UsdGeom.TetMesh.ComputeSurfaceFaces(myTetMesh, 10.0)
       
        # When they separate we have 8 faces
        self.assertEqual(len(surfaceFacesTime10), 8)

        triMesh = UsdGeom.Mesh.Define(stage,"/triMesh")
        triMeshOrientationAttr = triMesh.GetOrientationAttr();   
        triMeshOrientationAttr.Set(UsdGeom.Tokens.leftHanded)           
        triMeshPointsAttr = triMesh.GetPointsAttr()
        triMeshPointsAttr.Set(pointsTime0, 0.0)
        triMeshPointsAttr.Set(pointsTime10, 10.0)
        triMeshFaceVertexCountsAttr = triMesh.GetFaceVertexCountsAttr()

        faceVertexCountsTime0 = Vt.IntArray(6, (3,3,3,3,3,3))
        triMeshFaceVertexCountsAttr.Set(faceVertexCountsTime0, 0.0)
        faceVertexCountsTime10 = Vt.IntArray(8, (3,3,3,3,3,3,3,3))
        triMeshFaceVertexCountsAttr.Set(faceVertexCountsTime10, 10.0)

        # Need to convert surfaceFaceIndices from VtVec3iArray to VtIntArray
        faceVertexIndicesTime0 = Vt.IntArray(18)
        for i in range(0,6):
            for j in range(0,3):
                faceVertexIndicesTime0[i * 3 + j] = surfaceFacesTime0[i][j]

        faceVertexIndicesTime10 = Vt.IntArray(24)
        for i in range(0,8):
            for j in range(0,3):
                faceVertexIndicesTime10[i * 3 + j] = surfaceFacesTime10[i][j]

        triMeshFaceVertexIndicesAttr = triMesh.GetFaceVertexIndicesAttr()
        triMeshFaceVertexIndicesAttr.Set(faceVertexIndicesTime0, 0.0)
        triMeshFaceVertexIndicesAttr.Set(faceVertexIndicesTime10, 10.0)
        stage.SetStartTimeCode(0.0)
        stage.SetEndTimeCode(15.0)
        stage.Export('tetMeshLH.usda')

    def test_UsdGeomTetMeshFindInvertedElements(self):
        stage = Usd.Stage.CreateInMemory()
        myTetMesh = UsdGeom.TetMesh.Define(stage,"/tetMesh")
        pointsAttr = myTetMesh.GetPointsAttr()

        pointsTime10 = Vt.Vec3fArray(8, (Gf.Vec3f(0.0, 0.0, 3.0),
                                        Gf.Vec3f(-2.0, 0.0, 3.0),
                                        Gf.Vec3f(0.0, 2.0, 3.0),
                                        Gf.Vec3f(0.0, 0.0, 5.0),
                                        Gf.Vec3f(0.0, 0.0, -3.0),
                                        Gf.Vec3f(2.0, 0.0, -3.0),
                                        Gf.Vec3f(0.0, 2.0, -3.0),
                                        Gf.Vec3f(0.0, 0.0, -5.0)))
        # Test default rightHanded orientation
        pointsAttr.Set(pointsTime10, 10.0)
        tetVertexIndicesAttr = myTetMesh.GetTetVertexIndicesAttr();        

        tetIndicesTime10 = Vt.Vec4iArray(2, (Gf.Vec4i(0,1,2,3),
                                             Gf.Vec4i(4,6,5,7)))

        tetVertexIndicesAttr.Set(tetIndicesTime10, 10.0)
        
        invertedElementsTime10 =  UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 10.0)
        self.assertEqual(len(invertedElementsTime10), 1)
        self.assertEqual(invertedElementsTime10[0], 0)
        # Test leftHanded orientation
        orientationAttr = myTetMesh.GetOrientationAttr();   
        orientationAttr.Set(UsdGeom.Tokens.leftHanded)
        invertedElementsTime10 =  UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 10.0)
        self.assertEqual(len(invertedElementsTime10), 1)
        self.assertEqual(invertedElementsTime10[0], 1)        

if __name__ == '__main__':
    unittest.main()
