#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import sys
from pxr import Sdf,Usd,Pcp

def RenamingSpec():
    '''Test renaming a SdfPrimSpec.'''
    stage = Usd.Stage.CreateInMemory()
    layer = stage.GetRootLayer()

    parent = stage.DefinePrim("/parent")
    child = stage.DefinePrim("/parent/child")

    assert stage.GetPrimAtPath('/parent')
    assert stage.GetPrimAtPath('/parent/child')
    layer.GetPrimAtPath(parent.GetPath()).name = "parent_renamed"
    assert stage.GetPrimAtPath('/parent_renamed')
    assert stage.GetPrimAtPath('/parent_renamed/child')

def ChangeInsignificantSublayer():
    '''Test making changes after adding an insignificant sublayer.'''
    stage = Usd.Stage.CreateInMemory()
    layer = stage.GetRootLayer()

    insignificantSublayer = Sdf.Layer.CreateAnonymous(".usda")
    assert insignificantSublayer.empty

    layer.subLayerPaths.append(insignificantSublayer.identifier)
    assert insignificantSublayer in stage.GetUsedLayers()

    with Usd.EditContext(stage, insignificantSublayer):
        prim = stage.DefinePrim("/Foo")
        assert prim

def _TestStageErrors(stageAndErrorCounts):
    for stage, errorCount in stageAndErrorCounts:
        errors = stage.GetCompositionErrors()
        assert len(errors) == errorCount

        for i in range(errorCount):
            assert isinstance(errors[i], Pcp.ErrorSublayerCycle)

def AddSublayerWithCycle():
    '''Tests that adding a sublayer resulting in a cycle produces warnings'''

    root = Usd.Stage.CreateNew("root.usda")
    a = Usd.Stage.CreateNew("a.usda")
    b = Usd.Stage.CreateNew("b.usda")

    root.GetRootLayer().subLayerPaths.append("b.usda")
    b.GetRootLayer().subLayerPaths.append("a.usda")

    # Initial sanity test, there should be no errors on any stage
    _TestStageErrors([(root, 0), (a, 0), (b, 0)])

    # Add the sublayer which creates a cycle in all stages
    a.GetRootLayer().subLayerPaths.append("b.usda")
    _TestStageErrors([(root, 1), (a, 1), (b, 1)])

def UnmuteWithCycle():
    '''Tests that unmuting a sublayer resulting in a cycle produces warnings'''

    root = Usd.Stage.CreateNew("root.usda")
    a = Usd.Stage.CreateNew("a.usda")
    b = Usd.Stage.CreateNew("b.usda")

    # Mute layer b on the root stage, this will prevent a cycle from being
    # created while the sublayers are assembled
    root.MuteLayer(b.GetRootLayer().identifier)

    root.GetRootLayer().subLayerPaths.append("b.usda")
    b.GetRootLayer().subLayerPaths.append("a.usda")
    a.GetRootLayer().subLayerPaths.append("b.usda")

    # Initial sanity test, there should be no errors on root
    _TestStageErrors([(root, 0)])

    # Unmute layer b creating a cycle on root
    root.UnmuteLayer(b.GetRootLayer().identifier)
    _TestStageErrors([(root, 1)])

    # Mute the layer ensuring that the cycle has been removed
    root.MuteLayer(b.GetRootLayer().identifier)
    _TestStageErrors([(root, 0)])

def Main(argv):
    RenamingSpec()
    ChangeInsignificantSublayer()
    AddSublayerWithCycle()
    UnmuteWithCycle()

if __name__ == "__main__":
    Main(sys.argv)
    print('OK')

