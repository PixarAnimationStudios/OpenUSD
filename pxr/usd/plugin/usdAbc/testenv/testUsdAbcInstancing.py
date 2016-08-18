#!/pxrpythonsubst

# Additional suffix for files to distinguish between runs with different
# settings.
suffix = ""

# Parse command line early.  We need to set the env vars before the
# TfEnvSetting stuff looks at them for the first time.  Usage is:
# <prog> <suffix> [<name>=<value>]...
if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        import os
        suffix = '%s.' % (sys.argv[1], )
        for arg in sys.argv[2:]:
            (name,_,value) = arg.partition('=')
            os.environ[name] = value

from pxr import Sdf, Usd, UsdAbc
from Mentor.Runtime import (Assert,
                            FindDataFile, Fixture, Runner, SetAssertMode)

class TestUsdAbcInstancing(Fixture):
    def _Export(self, layer, name):
        layer.Export('%s.%susda' % (name, suffix))

    def _Open(self, name):
        layer = Sdf.Layer.FindOrOpen(FindDataFile('%s.abc' % (name, )))
        Assert(layer)
        return layer

    def _RunTest(self, name):
        self._Export(self._Open(name), name)

    # ========================================================================
    # Nested instance conversion
    #
    # Verify that nested instances convert correctly.
    # ========================================================================
    def TestNestedInstanceConversion(self):
        self._RunTest("nestedInstancing")

if __name__ == "__main__":
    Runner().Main()
