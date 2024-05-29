#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

from pxr import Sdf, Tf, Usd

def TestBug119633():

    layerFile = 'root.usda'
    try:
        stage = Usd.Stage.Open(layerFile)
        assert stage, 'failed to create stage for %s' % layerFile
        prim = stage.GetPrimAtPath('/SardineGroup_OceanA')
        assert prim, 'failed to find prim /World'
    except Tf.ErrorException:
        pass


if __name__ == "__main__":
    TestBug119633()
    print('OK')
