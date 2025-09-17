#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Tf

# Register this library with a new, previously unknown dependency.  Note, we're
# just using 'sys' as the module to load here, which is a lie.  It should really
# be this module, but it doesn't matter, and it's not trivial to come up with a
# robust, correct module name for this module.
Tf.ScriptModuleLoader()._RegisterLibrary('Unknown', 'sys',
                                         ['NewDynamicDependency'])

# Register the dependency.  In this case we use 'sys' just because it saves us
# from creating a real module called NewDynamicDependency.
Tf.ScriptModuleLoader()._RegisterLibrary('NewDynamicDependency', 'sys', [])

# Load dependencies for this module, which includes the
# NewDynamicDependency. This is a reentrant load that will be handled
# immediately by TfScriptModuleLoader.
Tf.ScriptModuleLoader()._LoadModulesForLibrary('Unknown')


