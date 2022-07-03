#!/pxrpythonsubst
#
# Copyright 2021 Pixar
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
import argparse
import os

from pxr import Plug, Ar, Tf

def SetupPlugins():
    # Register test resolver plugins
    # Test plugins are installed relative to this script
    testRoot = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), 'ArPlugins')

    pr = Plug.Registry()
    
    testResolverPath = os.path.join(
        testRoot, 'lib/TestArOptionalImplementation*/Resources/')
    assert pr.RegisterPlugins(testResolverPath)
    

parser = argparse.ArgumentParser()
parser.add_argument("resolverName", type=str,
                    help=("Name of ArResolver subclass to test"))

args = parser.parse_args()

SetupPlugins()

Ar.SetPreferredResolver(args.resolverName)
r = Ar.GetResolver()

# Call each method to see whether the corresponding virtual method
# on the resolver is called.

# This context manager calls ArResolver::BindContext when entered
# and ArResolver::UnbindContext on exit.
with Ar.ResolverContextBinder(Ar.ResolverContext()):
    pass

r.CreateDefaultContext()
r.CreateDefaultContextForAsset('foo')
r.CreateContextFromString('foo')
r.RefreshContext(Ar.ResolverContext())
r.GetCurrentContext()
r.IsContextDependentPath('foo')

# This context manager calls ArResolver::BeginCacheScope when entered
# and ArResolver::EndCacheScope on exit.
with Ar.ResolverScopedCache():
    pass
