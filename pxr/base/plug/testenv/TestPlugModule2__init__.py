#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Plug, Tf

class TestPlugPythonDerived2(Plug._TestPlugBase2):
    def GetTypeName(self):
        return 'TestPlugModule2.TestPlugPythonDerived2'

Tf.Type.Define(TestPlugPythonDerived2)
