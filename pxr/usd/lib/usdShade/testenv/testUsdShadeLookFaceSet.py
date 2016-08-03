#!/pxrpythonsubst

from pxr import Usd, UsdGeom, UsdShade, Vt

from Mentor.Runtime import *

# Configure mentor so assertions terminate execution.
SetAssertMode(MTR_EXIT_TEST)

usdFilePath = FindDataFile("testUsdShadeLookFaceSet/Sphere.usda")

stage = Usd.Stage.Open(usdFilePath)
Assert(stage)

sphere = stage.GetPrimAtPath('/Sphere/Mesh')
look1 = stage.GetPrimAtPath('/Sphere/Looks/initialShadingGroup')
look2 = stage.GetPrimAtPath('/Sphere/Looks/lambert2SG')
look3 = stage.GetPrimAtPath('/Sphere/Looks/lambert3SG')

AssertTrue(sphere and look1 and look2 and look3)

# Verify that the sphere mesh does not have an existing look face-set.
AssertFalse(UsdShade.Look.HasLookFaceSet(sphere))

# GetLookFaceSet should return an invalid FaceSetAPI object.
faceSet = UsdShade.Look.GetLookFaceSet(sphere)
AssertFalse(faceSet)

# Create a new "look" face-set.
faceSet = UsdShade.Look.CreateLookFaceSet(sphere)
AssertEqual(faceSet.GetFaceSetName(), "look")
AssertTrue(faceSet)
AssertTrue(faceSet.GetPrim() and faceSet.GetIsPartition())

faceIndices1 = Vt.IntArray((0, 1, 2, 3))
faceIndices2 = Vt.IntArray((4, 5, 6, 7, 8, 9, 10, 11))
faceIndices3 = Vt.IntArray((12, 13, 14, 15))

faceSet.AppendFaceGroup(faceIndices1, look1.GetPath())
faceSet.AppendFaceGroup(faceIndices2, look2.GetPath())
faceSet.AppendFaceGroup(faceIndices3, look3.GetPath())

# Don't save the modified source stage. Export it into a new layer for baseline 
# diffing.
stage.Export("SphereWithFaceSets.usda", addSourceFileComment=False)

ExitTest()
