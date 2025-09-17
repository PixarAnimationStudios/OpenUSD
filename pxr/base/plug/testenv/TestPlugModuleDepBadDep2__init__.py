#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Plug

class TestPlugPythonDepBadDep2(Plug._TestPlugBase3):
    def GetTypeName(self):
        return 'TestPlugPythonDepBadDep2'

