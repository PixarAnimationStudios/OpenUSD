#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function

# section 1
from pxr import Usd, UsdGeom, Vt

stage = Usd.Stage.Open('HelloWorld.usda')
xform = stage.GetPrimAtPath('/hello')
sphere = stage.GetPrimAtPath('/hello/world')

# section 2 
expected = ['proxyPrim', 'purpose', 'visibility', 'xformOpOrder']
assert xform.GetPropertyNames() == expected
        
expected = ['doubleSided', 'extent', 'orientation',
            'primvars:displayColor', 'primvars:displayOpacity', 
            'proxyPrim', 'purpose', 'radius', 'visibility', 'xformOpOrder']
assert sphere.GetPropertyNames() == expected


# section 3
extentAttr = sphere.GetAttribute('extent')
expected = Vt.Vec3fArray([(-1, -1, -1), (1, 1, 1)])
assert extentAttr.Get() == expected

# section 4
radiusAttr = sphere.GetAttribute('radius')
radiusAttr.Set(2)
extentAttr.Set(extentAttr.Get() * 2)
print(stage.GetRootLayer().ExportToString())

# section 5
from pxr import UsdGeom
sphereSchema = UsdGeom.Sphere(sphere)
color = sphereSchema.GetDisplayColorAttr()
color.Set([(0,0,1)])
print(stage.GetRootLayer().ExportToString())
stage.GetRootLayer().Save()
