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

# section 1
from pxr import Usd, UsdGeom, Vt

stage = Usd.Stage.Open('HelloWorld.usda')
xform = stage.GetPrimAtPath('/hello')
sphere = stage.GetPrimAtPath('/hello/world')

# section 2 
expected = ['purpose', 'visibility', 'xformOpOrder']
assert xform.GetPropertyNames() == expected
        
expected = ['doubleSided', 'extent', 'orientation',
            'primvars:displayColor', 'primvars:displayOpacity', 
            'purpose', 'radius', 'visibility', 'xformOpOrder']
assert sphere.GetPropertyNames() == expected


# section 3
extentAttr = sphere.GetAttribute('extent')
expected = Vt.Vec3fArray([(-1, -1, -1), (1, 1, 1)])
assert extentAttr.Get() == expected

# section 4
radiusAttr = sphere.GetAttribute('radius')
radiusAttr.Set(2)
extentAttr.Set(extentAttr.Get() * 2)
print stage.GetRootLayer().ExportToString()

# section 5
from pxr import UsdGeom
sphereSchema = UsdGeom.Sphere(sphere)
color = sphereSchema.GetDisplayColorAttr()
color.Set([(0,0,1)])
print stage.GetRootLayer().ExportToString()
stage.GetRootLayer().Save()
