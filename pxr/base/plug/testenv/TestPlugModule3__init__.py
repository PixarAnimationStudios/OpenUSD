#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Plug, Tf

class TestPlugPythonDerived3_3(Plug._TestPlugBase3):
    def GetTypeName(self):
        return 'TestPlugModule3.TestPlugPythonDerived3_3'
Tf.Type.Define(TestPlugPythonDerived3_3)

class TestPlugPythonDerived3_4(Plug._TestPlugBase4):
    def GetTypeName(self):
        return 'TestPlugModule3.TestPlugPythonDerived3_4'
Tf.Type.Define(TestPlugPythonDerived3_4)
