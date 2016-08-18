#!/pxrpythonsubst

from pxr import Sdf, Usd, UsdAbc
from Mentor.Runtime import (Assert,
                            FindDataFile, Fixture, Runner, SetAssertMode)

class TestUsdAbcBugs(Fixture):

    # ========================================================================
    # Bug 107381
    #
    # Write primvars to top level if prim doesn't have .arbGeomParams property.
    # ========================================================================
    def TestBug107381(self):
        layer = Sdf.Layer.FindOrOpen(FindDataFile('bug107381.usd'))
        Assert(layer)
        layer.ExportToString()

if __name__ == "__main__":
    Runner().Main()
