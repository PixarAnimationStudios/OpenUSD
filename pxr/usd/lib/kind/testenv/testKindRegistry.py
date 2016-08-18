#!/pxrpythonsubst

from pxr import Gf, Tf, Kind
from Mentor.Runtime import *
from pxr import Plug

# Assert failures end test
SetAssertMode(MTR_EXIT_TEST)

# Test plugins are installed relative this script
import os
testRoot = os.path.join(os.path.dirname(__file__), 'KindPlugins')
testPluginsPython = testRoot + '/lib/python'

# Register python module plugins
Plug.Registry().RegisterPlugins(testPluginsPython + "/**/")

reg = Kind.Registry()
assert reg

# Test factory default kinds + config file contributions
expectedDefaultKinds = [
    'group',
    'model',
    'test_model_kind',
    'test_root_kind',
    ]
actualDefaultKinds = Kind.Registry.GetAllKinds()

# We cannot expect actual to be equal to expected, because there is no
# way to prune the site's extension plugins from actual.
#AssertEqual(sorted(expectedDefaultKinds), sorted(actualDefaultKinds))

for expected in expectedDefaultKinds:
    AssertTrue( Kind.Registry.HasKind(expected) )
    AssertTrue( expected in actualDefaultKinds )

# Check the 'test_model_kind' kind from the TestKindModule_plugInfo.json
Assert(Kind.Registry.HasKind('test_root_kind'))
AssertEqual(Kind.Registry.GetBaseKind('test_root_kind'), '')

# Check the 'test_model_kind' kind from the TestKindModule_plugInfo.json
Assert(Kind.Registry.HasKind('test_model_kind'))
AssertEqual(Kind.Registry.GetBaseKind('test_model_kind'), 'model')


ExitTest()

