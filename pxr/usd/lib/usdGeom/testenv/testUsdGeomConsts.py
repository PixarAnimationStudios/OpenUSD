#!/pxrpythonsubst

import sys, os
from pxr import Sdf, Usd, UsdGeom

import Mentor.Runtime
from Mentor.Runtime import (AssertEqual, FindDataFile)


def WriteConst(prim, attrName, typeStr, constVal):
    """Add a const to the prim"""
    attr = prim.CreateAttribute(attrName, typeStr)
    assert attr.IsDefined()
    attr.Set(constVal)


def CompareAuthoredToConst(prim, attrName, constVal):
    """Ensure the const that we previously serialized compares equal to the
    source of the value we authored (i.e. the actual schema const)
    """
    attr = prim.GetAttribute(attrName)
    assert attr.IsDefined()
    val = attr.Get()
    AssertEqual(constVal, val, "Round-tripped constant '%s' did not compare "
                               "equal to schema constant. "
                               "(schema: %s, roundtripped: %s)" 
                               % (attrName, repr(constVal), repr(val)))


def CompareAuthoredToArchived(prim, attrName, archivePrim):
    """To catch ourselves if we inadvertently change the value of a const in a
    schema (which would be really bad to do unwittingly), we compare the value
    we wrote and read back in to a value from a baseline usda file of the same
    layout.
    """
    attr = prim.GetAttribute(attrName)
    assert attr.IsDefined()
    newVal = attr.Get()
    archAttr = archivePrim.GetAttribute(attrName)
    assert archAttr.IsDefined()
    archVal = archAttr.Get()
    AssertEqual(archVal, newVal, "Baseline archived constant '%s' did not "
                                 "compare equal to schema constant "
                                 "(baseline: %s, schema: %s)." 
                                 % (attrName, repr(archVal), repr(newVal)))


def Main():
    lyr = Sdf.Layer.CreateNew("testConsts.usda")
    assert lyr

    # To update this file, just grab the result of a run of this test (disabling
    # the archive comparison, so it will succeed) from the test-run directory
    archiveFn = FindDataFile('testUsdGeomConsts/testConsts.usda')
    archiveLyr = Sdf.Layer.FindOrOpen(archiveFn)
    assert archiveLyr

    stage = Usd.Stage.Open("testConsts.usda")
    assert stage

    archiveStage = Usd.Stage.Open(archiveFn)
    assert archiveStage

    prim = UsdGeom.Scope.Define(stage, "/ConstRoot").GetPrim()
    assert prim

    archivePrim = archiveStage.GetPrimAtPath("/ConstRoot")
    assert archivePrim


    testConsts = (('SHARPNESS_INFINITE', 
                   Sdf.ValueTypeNames.Float, 
                   UsdGeom.Mesh.SHARPNESS_INFINITE), )

    #
    # First write out all the consts
    #
    for ( name, cType, cVal ) in testConsts:
        WriteConst( prim, name, cType, cVal )

    lyr.Save()

    #
    # Now, Rebuild the stage from a fresh read of the file
    #
    stage = None
    lyr = None

    lyr = Sdf.Layer.FindOrOpen("testConsts.usda")
    assert lyr

    stage = Usd.Stage.Open("testConsts.usda")
    assert stage

    prim = stage.GetPrimAtPath("/ConstRoot")
    assert prim

    #
    # Finally, compare the (from code) const values with both
    # the just-serialized values (reread), and the archived values
    #
    for ( name, cType, cVal ) in testConsts:
        CompareAuthoredToConst(prim, name, cVal )
        CompareAuthoredToArchived(prim, name, archivePrim)


if __name__ == "__main__":
    Mentor.Runtime.SetAssertMode(Mentor.Runtime.MTR_EXIT_TEST)
    Main()
