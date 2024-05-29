#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

import sys
from pxr import Sdf,Usd,Tf

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

def Main(argv):
    RenamingSpec()
    ChangeInsignificantSublayer()

if __name__ == "__main__":
    Main(sys.argv)
    print('OK')

