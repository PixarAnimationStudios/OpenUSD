#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Tf

Tf.ScriptModuleLoader()._LoadModulesForLibrary('LoadsAll')

# Request to load everything.  This will be deferred by the TfScriptModuleLoader
# until after the current dependency load is complete.
Tf.ScriptModuleLoader()._LoadModulesForLibrary('')
