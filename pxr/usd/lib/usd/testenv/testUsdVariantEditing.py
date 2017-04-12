#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

import os, sys
from pxr import Gf, Tf, Sdf, Usd

def OpenLayer(name):
    layerFile = '%s.usda' % name
    layer = Sdf.Layer.FindOrOpen(layerFile)
    assert layer, 'failed to open layer @%s@' % layerFile
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
assert (stage.HasLocalLayer(stage.GetEditTarget().GetLayer()) and
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

def TestNewPayloadAutoLoading():
    print 'TestNewPayloadAutoLoading'
    
    # Test that switching a variant that introduces a payload causes the payload
    # to be included if the parent is loaded, and vice versa.
    rootLayer = Sdf.Layer.CreateAnonymous()
    payloadLayer = Sdf.Layer.CreateAnonymous()

    Sdf.CreatePrimInLayer(rootLayer, '/main')
    Sdf.CreatePrimInLayer(payloadLayer, '/parent/child')

    stage = Usd.Stage.Open(rootLayer)
    main = stage.GetPrimAtPath('/main')
    pvs = main.GetVariantSets().AppendVariantSet('payload_vset')

    withPayload = pvs.AppendVariant('with_payload')
    withoutPayload = pvs.AppendVariant('without_payload')

    pvs.SetVariantSelection('with_payload')
    with pvs.GetVariantEditContext():
        main.SetPayload(payloadLayer, '/parent')

    pvs.SetVariantSelection('without_payload')

    # Now open the stage load all, we shouldn't have /main/child.
    stage = Usd.Stage.Open(rootLayer, load=Usd.Stage.LoadAll)
    assert stage.GetPrimAtPath('/main')
    assert not stage.GetPrimAtPath('/main/child')

    # Switching the selection should cause the payload to auto-load.
    stage.GetPrimAtPath('/main').GetVariantSet(
        'payload_vset').SetVariantSelection('with_payload')
    main = stage.GetPrimAtPath('/main')
    assert main and main.IsLoaded()
    assert stage.GetPrimAtPath('/main/child')

    # Open the stage again, but with load-none.
    stage = Usd.Stage.Open(rootLayer, load=Usd.Stage.LoadNone)
    assert stage.GetPrimAtPath('/main')
    assert not stage.GetPrimAtPath('/main/child')

    # Switching the selection should NOT cause the payload to auto-load.
    stage.GetPrimAtPath('/main').GetVariantSet(
        'payload_vset').SetVariantSelection('with_payload')
    main = stage.GetPrimAtPath('/main')
    assert main and not main.IsLoaded()
    assert not stage.GetPrimAtPath('/main/child')

TestNewPayloadAutoLoading()

print 'OK'

