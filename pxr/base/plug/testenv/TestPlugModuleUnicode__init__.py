#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Plug

# This plugin is coded correctly, but will have a pluginfo.json in a
# unicode path
class TestPlugPythonUnicode(Plug._TestPlugBase1):
    def GetTypeName(self):
        return 'TestPlugPythonUnicode'

