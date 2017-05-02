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

# Assumes that $USD_INSTALL_ROOT/lib/python is in sys.path

from pxr import Usd, UsdGeom

stage = Usd.Stage.Open('HelloWorld.usda')
# XXX: save the usd file to the local directory where the test
# is being administered so RefExample will have access to it
stage.Export('HelloWorld.usda')
hello = stage.GetPrimAtPath('/hello')
stage.SetDefaultPrim(hello)
UsdGeom.XformCommonAPI(hello).SetTranslate((4, 5, 6))
print stage.GetRootLayer().ExportToString()
stage.GetRootLayer().Save()

# Create an over on which to create our reference.
refStage = Usd.Stage.CreateNew('RefExample.usda')
refSphere = refStage.OverridePrim('/refSphere')
print refStage.GetRootLayer().ExportToString()

# Reference in the layer.
refSphere.GetReferences().AppendReference('./HelloWorld.usda')
print refStage.GetRootLayer().ExportToString()
refStage.GetRootLayer().Save()

# Clear out any authored transformation over the reference.
refXform = UsdGeom.Xformable(refSphere)
refXform.SetXformOpOrder([])
print refStage.GetRootLayer().ExportToString()

# Reference in the layer again, on another over.
refSphere2 = refStage.OverridePrim('/refSphere2')
refSphere2.GetReferences().AppendReference('./HelloWorld.usda')
print refStage.GetRootLayer().ExportToString()
refStage.GetRootLayer().Save()

# Author displayColor on a namespace descendant of the referenced prim.
overSphere = UsdGeom.Sphere.Get(refStage, '/refSphere2/world' )
overSphere.GetDisplayColorAttr().Set( [(1, 0, 0)] )
print refStage.GetRootLayer().ExportToString()
refStage.GetRootLayer().Save()

# Print the final composed results.
print refStage.ExportToString()
