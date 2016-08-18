#!/pxrpythonsubst

import os, sys
from Mentor.Runtime import (SetAssertMode, MTR_EXIT_TEST, FindDataFile,
                            Assert, AssertTrue, AssertFalse, AssertEqual,
                            AssertNotEqual, AssertException,
                            ExpectedErrorBegin, ExpectedErrorEnd, ExitTest)

from pxr import Gf, Tf, Sdf, Pcp, Usd, Plug

# Register test plugin
testenvDir = FindDataFile("testUsdVariantFallbacks.testenv")
assert testenvDir
Plug.Registry().RegisterPlugins(testenvDir)

# We should pick up the plugInfo.json fallbacks now.
assert Usd.Stage.GetGlobalVariantFallbacks()['displayColor'] == ['green']

def OpenLayer(name):
    fullName = 'testUsdVariantFallbacks.testenv/%s.usda' % name
    layerFile = FindDataFile(fullName)
    assert layerFile, 'failed to find @%s@' % fullName
    layer = Sdf.Layer.FindOrOpen(layerFile)
    assert layer, 'failed to open layer @%s@' % fullName
    return layer

# Open stage.
layer = OpenLayer('testAPI_var')
stage = Usd.Stage.Open(layer.identifier)
assert stage, 'failed to create stage for @%s@' % layer.identifier
sarah = stage.GetPrimAtPath('/Sarah')
displayColor = sarah.GetVariantSet('displayColor')
assert sarah, 'failed to find prim /Sarah'

# Because our test has plugInfo.json that specifies a fallback of green,
# we should see green.
assert sarah.GetAttribute('color').Get() == Gf.Vec3d(0,1,0)

# Now override our process global variant policy and open a new stage.
Usd.Stage.SetGlobalVariantFallbacks({'displayColor':['red']})
assert Usd.Stage.GetGlobalVariantFallbacks() == {'displayColor':['red']}
stage = Usd.Stage.Open(layer.identifier)
sarah = stage.GetPrimAtPath('/Sarah')
displayColor = sarah.GetVariantSet('displayColor')

# We should now get an attribute that resolves to red
assert sarah.GetAttribute('color').Get() == Gf.Vec3d(1,0,0)

# Author a variant selection.
displayColor .SetVariantSelection('blue')
assert sarah.GetAttribute('color').Get() == Gf.Vec3d(0,0,1)

print 'OK'
