#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Plug

# This plugin depends on an undefined external function
# and so will be unloadable.
something = TestPlugDoSomethingUndefined()

class TestPlugPythonUnloadable(Plug._TestPlugBase1):
    def GetTypeName(self):
        return 'TestPlugPythonUnloadable'

