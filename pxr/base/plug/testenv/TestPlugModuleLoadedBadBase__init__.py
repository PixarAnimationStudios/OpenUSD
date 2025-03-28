#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Plug, Tf

class TestPlugPythonLoadedBadBase(Plug._TestPlugBase1):
    def GetTypeName(self):
        return 'TestPlugPythonLoadedBadbase'
Tf.Type.Define(TestPlugPythonLoadedBadBase)
