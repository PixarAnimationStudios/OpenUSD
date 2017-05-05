#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
from pxr import Tf

# On Windows if this script is run with stdout redirected then the C++
# stdout buffer is different from Python's stdout buffer.  As a result
# output from Python stdout and C++ stdout will not interleave as
# expected unless we flush both after each write.  We force flushing
# in Python here.
import platform
if platform.system() == 'Windows':
    import os, sys
    sys.stdout = os.fdopen(sys.stdout.fileno(), 'a+', 0)

sml = Tf.ScriptModuleLoader()

# Preload all currently known libraries to avoid their output in the test
# output.
sml._LoadModulesForLibrary('')

# Turn on script module loader debug output.
Tf.Debug.SetDebugSymbolsByName('TF_SCRIPT_MODULE_LOADER', True)

prefix = Tf.__package__ + '.testenv.testTfScriptModuleLoader_'

def Import(name):
    exec "import " + prefix + name

# Declare libraries.
def Register(libs):
    for name, deps in libs:
        sml._RegisterLibrary(name, prefix + name, deps)

print ("# Test case: loading one library generates a request to load all\n"
       "# libraries, one of which attemps to import the library we're\n"
       "# currently loading.")
Register([
        ('LoadsAll', []),
        ('DepLoadsAll', ['LoadsAll']),
        ('Other', ['LoadsAll'])
        ])
print ("# This should attempt to (forwardly) load Other, which in turn tries\n"
       "# to import LoadsAll, which would fail, but we defer loading Other\n"
       "# until after LoadsAll is finished loading.")
sml._LoadModulesForLibrary('DepLoadsAll')

print ("# Registering a library that is totally independent, and raises an\n"
       "# error when loaded, but whose name comes first in dependency order.\n"
       "# Since there is no real dependency, the SML should not try to load\n"
       "# this module, which would cause an exception.")
Register([
        ('AAA_RaisesError', [])
        ])

print ("# Test case: loading one library dynamically imports a new,\n"
       "# previously unknown dependent library, which registers further\n"
       "# dependencies, and expects them to load.")
Register([
        ('LoadsUnknown', []),
        ('Test', ['LoadsUnknown'])
        # New dynamic dependencies discovered when loading LoadsUnknown.
        # ('Unknown', ['NewDynamicDependency']),
        # ('NewDynamicDependency', [])
        ])
print ("# This should load LoadsUnknown, which loads Unknown dynamically,\n"
       "# which should request for Unknown's dependencies (NewDependency) to\n"
       "# load, which should work.")
sml._LoadModulesForLibrary('Test')

