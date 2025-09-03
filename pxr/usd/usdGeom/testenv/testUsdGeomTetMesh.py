#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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

        surfaceFaceVertexIndicesAttr = myTetMesh.GetSurfaceFaceVertexIndicesAttr();

        surfaceFacesTime0 = UsdGeom.TetMesh.ComputeSurfaceFaces(myTetMesh, 0.0)
         
        surfaceFaceVertexIndicesAttr.Set(surfaceFacesTime0, 0.0) 
        # When the tets are joined we have 6 faces
        self.assertEqual(len(surfaceFacesTime0), 6)

        surfaceFacesTime10 = UsdGeom.TetMesh.ComputeSurfaceFaces(myTetMesh, 10.0)
        surfaceFaceVertexIndicesAttr.Set(surfaceFacesTime10, 10.0) 
        # When they separate we have 8 faces
        self.assertEqual(len(surfaceFacesTime10), 8)

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

        surfaceFaceVertexIndicesAttr = myTetMesh.GetSurfaceFaceVertexIndicesAttr();

        surfaceFacesTime0 = UsdGeom.TetMesh.ComputeSurfaceFaces(myTetMesh, 0.0)
        
        surfaceFaceVertexIndicesAttr.Set(surfaceFacesTime0, 0.0)
        # When the tets are joined we have 6 faces
        self.assertEqual(len(surfaceFacesTime0), 6)

        surfaceFacesTime10 = UsdGeom.TetMesh.ComputeSurfaceFaces(myTetMesh, 10.0)
        
        surfaceFaceVertexIndicesAttr.Set(surfaceFacesTime10, 10.0)
        # When they separate we have 8 faces
        self.assertEqual(len(surfaceFacesTime10), 8)
        
        stage.SetStartTimeCode(0.0)
        stage.SetEndTimeCode(15.0)
        stage.Export('tetMeshLH.usda')

    def test_UsdGeomTetMeshFindInvertedElements(self):
        stage = Usd.Stage.CreateInMemory()
        myTetMesh = UsdGeom.TetMesh.Define(stage,"/tetMesh")
        pointsAttr = myTetMesh.GetPointsAttr()

        pointsTime0 = Vt.Vec3fArray(4, (Gf.Vec3f(0.0, 0.0, 0.0),
                                        Gf.Vec3f(0.0, 0.0, 1.0),
                                        Gf.Vec3f(-1.0, 0.0, 0.0),
                                        Gf.Vec3f(0.0, -1.0, 0.0)))
                                        
        # Test default rightHanded orientation wrt. rightHanded element
        pointsAttr.Set(pointsTime0, 0.0)
        tetVertexIndicesAttr = myTetMesh.GetTetVertexIndicesAttr();        

        tetIndicesTime0 = Vt.Vec4iArray(1, (Gf.Vec4i(0,1,2,3)))

        tetVertexIndicesAttr.Set(tetIndicesTime0, 0.0)                 
        invertedElementsTime0 = UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 0.0)
        self.assertEqual(len(invertedElementsTime0), 0)
        
 
        # Test default rightHanded element with leftHanded orientation
        orientationAttr = myTetMesh.GetOrientationAttr();                
        orientationAttr.Set(UsdGeom.Tokens.leftHanded)          
        invertedElementsTime0 = UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 0.0)
        self.assertEqual(len(invertedElementsTime0), 1)
        

        # Test rightHanded orientation with inverted element
        orientationAttr.Set(UsdGeom.Tokens.rightHanded)          
        pointsTime0 = Vt.Vec3fArray(4, (Gf.Vec3f(0.0, 0.0, 0.0),
                                        Gf.Vec3f(0.0, 0.0, 1.0),
                                        Gf.Vec3f(1.0, 0.0, 0.0),
                                        Gf.Vec3f(0.0, -1.0, 0.0)))
        pointsAttr.Set(pointsTime0, 0.0)
        invertedElementsTime0 = UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 0.0)        
        self.assertEqual(len(invertedElementsTime0), 1)
                                                                
        
        # Test leftHanded orientation wrt. leftHanded element
        orientationAttr = myTetMesh.GetOrientationAttr();                
        orientationAttr.Set(UsdGeom.Tokens.leftHanded)    

        pointsTime0 = Vt.Vec3fArray(4, (Gf.Vec3f(0.0, 0.0, 0.0),
                                        Gf.Vec3f(0.0, 0.0, 1.0),
                                        Gf.Vec3f(1.0, 0.0, 0.0),
                                        Gf.Vec3f(0.0, -1.0, 0.0)))
        pointsAttr.Set(pointsTime0, 0.0)
        invertedElementsTime0 =  UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 0.0)        
        self.assertEqual(len(invertedElementsTime0), 0)
                      
        
        # Test leftHanded element with rightHanded orientation attr value             
        orientationAttr.Set(UsdGeom.Tokens.rightHanded)                                          
        invertedElementsTime0 =  UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 0.0)        
        self.assertEqual(len(invertedElementsTime0), 1)        
        
        # Test inverted element with leftHanded orientation attr value  
        orientationAttr.Set(UsdGeom.Tokens.leftHanded) 
        pointsTime0 = Vt.Vec3fArray(4, (Gf.Vec3f(0.0, 0.0, 0.0),
                                        Gf.Vec3f(0.0, 0.0, 1.0),
                                        Gf.Vec3f(1.0, 0.0, 0.0),
                                        Gf.Vec3f(0.0, 1.0, 0.0)))   
        pointsAttr.Set(pointsTime0, 0.0)  
        invertedElementsTime0 =  UsdGeom.TetMesh.FindInvertedElements(myTetMesh, 0.0)        
        self.assertEqual(len(invertedElementsTime0), 1)                                                          

if __name__ == '__main__':
    unittest.main()
