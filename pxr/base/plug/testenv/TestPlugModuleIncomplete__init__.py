#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Plug

# This plugin is coded correctly, but will be incomplete
# because it won't have a plugInfo.json
class TestPlugPythonIncomplete(Plug._TestPlugBase1):
    def GetTypeName(self):
        return 'TestPlugPythonIncomplete'

