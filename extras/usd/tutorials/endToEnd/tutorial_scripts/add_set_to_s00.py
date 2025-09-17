#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function

import os
ASSET_BASE = os.path.join('../../', 'models')

def main():
    sequenceFilePath = 'shots/s00/s00.usd'
    setsLayoutLayerFilePath = 'shots/s00/s00_sets.usd'

    from pxr import Kind, Usd, UsdGeom, Sdf
    stage = Usd.Stage.Open(sequenceFilePath)

    # we use Sdf, a lower level library, to obtain the 'sets' layer.
    workingLayer = Sdf.Layer.FindOrOpen(setsLayoutLayerFilePath)
    assert stage.HasLocalLayer(workingLayer)

    # this makes the workingLayer the target for authoring operations by the
    # stage.
    stage.SetEditTarget(workingLayer)

    # Make sure the model-parents we need are well-specified
    Usd.ModelAPI(UsdGeom.Xform.Define(stage, '/World')).SetKind(Kind.Tokens.group)
    Usd.ModelAPI(UsdGeom.Xform.Define(stage, '/World/sets')).SetKind(Kind.Tokens.group)
    
    # in previous examples, we've been using GetReferences().AddReference(...).  The
    # following uses .SetItems() instead which lets us explicitly set (replace)
    # the references at once instead of adding.
    stage.DefinePrim('/World/sets/Room_set').GetReferences().SetReferences([
        Sdf.Reference(os.path.join(ASSET_BASE, 'Room_set/Room_set.usd'))])

    stage.GetEditTarget().GetLayer().Save()

    print('===')
    print('usdview %s' % sequenceFilePath)
    print('usdcat %s' % setsLayoutLayerFilePath)

if __name__ == '__main__':
    main()

