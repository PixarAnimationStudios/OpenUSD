#!/pxrpythonsubst

import os, sys
from Mentor.Runtime import (SetAssertMode, MTR_EXIT_TEST, FindDataFile,
                            Assert, AssertTrue, AssertFalse, AssertEqual,
                            AssertNotEqual, AssertException,
                            ExpectedErrorBegin, ExpectedErrorEnd, ExitTest)

from pxr import Gf, Tf, Sdf, Pcp, Usd

def OpenLayer(name):
    fullName = 'testUsdVariantEditing.testenv/%s.usda' % name
    layerFile = FindDataFile(fullName)
    assert layerFile, 'failed to find @%s@' % fullName
    layer = Sdf.Layer.FindOrOpen(layerFile)
    assert layer, 'failed to open layer @%s@' % fullName
    return layer

# Open stage.
layer = OpenLayer('testAPI_var')
stage = Usd.Stage.Open(layer.identifier)
assert stage, 'failed to create stage for @%s@' % layer.identifier

# Check GetLayerStack behavior.
assert stage.GetLayerStack()[0] == stage.GetSessionLayer()

# Get LayerStack without session layer.
rootLayer = stage.GetLayerStack(includeSessionLayers=False)[0]
assert rootLayer == stage.GetRootLayer()

# Get Sarah prim.
sarah = stage.GetPrimAtPath('/Sarah')
assert sarah, 'failed to find prim /Sarah'

# Sanity check simple composition.
assert sarah.GetAttribute('color').Get() == Gf.Vec3d(1,0,0)

# Verify that the base prim does not have the custom attribute that we will
# create later in the variant.
emptyPrim = stage.OverridePrim('/Sarah/EmptyPrim')
assert emptyPrim
assert not emptyPrim.GetAttribute('newAttr')

# Should start out with EditTarget being local & root layer.
assert (stage.GetEditTarget().IsLocalLayer() and
        stage.GetEditTarget().GetLayer() == stage.GetRootLayer())

# Try editing a local variant.
displayColor = sarah.GetVariantSet('displayColor')
assert displayColor.GetVariantSelection() == 'red'
assert sarah.GetVariantSets().GetVariantSelection('displayColor') == 'red'
with displayColor.GetVariantEditContext():
    sarah.GetAttribute('color').Set(Gf.Vec3d(1,1,1))
    stage.DefinePrim(sarah.GetPath().AppendChild('Child'), 'Scope')

    # Bug 90706 - verify that within a VariantSet, a new attribute that does
    # not exist on the base prim returns True for IsDefined()
    over = stage.OverridePrim('/Sarah/EmptyPrim')
    assert over
    over.CreateAttribute('newAttr', Sdf.ValueTypeNames.Int)
    assert over.GetAttribute('newAttr').IsDefined()

# Test existence of the newly created attribute again, outside of the edit
# context, while we are still set to the variant selection from which we created
# the attribute.
emptyPrim = stage.OverridePrim('/Sarah/EmptyPrim')
assert emptyPrim
assert emptyPrim.GetAttribute('newAttr').IsDefined()

assert sarah.GetAttribute('color').Get() == Gf.Vec3d(1,1,1)
assert stage.GetPrimAtPath(sarah.GetPath().AppendChild('Child'))

# Switch to 'green' variant.
displayColor.SetVariantSelection('green')
assert displayColor.GetVariantSelection() == 'green'

# Should not be picking up variant opinions authored above.
assert sarah.GetAttribute('color').Get() == Gf.Vec3d(0,1,0)
assert not stage.GetPrimAtPath(sarah.GetPath().AppendChild('Scope'))
emptyPrim = stage.OverridePrim('/Sarah/EmptyPrim')
assert emptyPrim
assert not emptyPrim.GetAttribute('newAttr').IsDefined()

displayColor.ClearVariantSelection()
assert displayColor.GetVariantSelection() == ''

# Test editing a variant that doesn't yet have opinions.
sarah_ref = stage.GetPrimAtPath('/Sarah_ref')
displayColor = sarah_ref.GetVariantSet('displayColor')
displayColor.SetVariantSelection('red')
assert displayColor.GetVariantSelection() == 'red'
with displayColor.GetVariantEditContext():
    sarah_ref.GetAttribute('color').Set(Gf.Vec3d(2,2,2))
assert sarah_ref.GetAttribute('color').Get() == Gf.Vec3d(2,2,2)

print 'OK'

