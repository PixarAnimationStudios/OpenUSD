#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Plug, Tf

class TestPlugPythonDerived1(Plug._TestPlugBase1):
    def GetTypeName(self):
        return 'TestPlugModule1.TestPlugPythonDerived1'

Tf.Type.Define(TestPlugPythonDerived1)
