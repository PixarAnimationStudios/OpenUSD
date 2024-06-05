#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Usd

stage = Usd.Stage.CreateNew('HelloWorldRedux.usda')
xform = stage.DefinePrim('/hello', 'Xform')
sphere = stage.DefinePrim('/hello/world', 'Sphere')
stage.GetRootLayer().Save()
