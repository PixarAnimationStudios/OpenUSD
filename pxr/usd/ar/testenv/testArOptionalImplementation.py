#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
