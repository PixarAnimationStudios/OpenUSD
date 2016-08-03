#!/pxrpythonsubst

import sys, os
from pxr import Sdf,Usd,Vt,Tf, Gf
from Mentor.Runtime import (AssertEqual, AssertNotEqual, RequiredException, 
                            SetAssertMode, MTR_EXIT_TEST)

def BasicTest():
    """Sanity tests for Usd.TimeCode API"""
    default1 = Usd.TimeCode.Default()
    default2 = Usd.TimeCode.Default()
    AssertEqual(default1, default2)
    earliestTime1 = Usd.TimeCode.EarliestTime()
    earliestTime2 = Usd.TimeCode.EarliestTime()
    AssertEqual(earliestTime1, earliestTime2)

    nonSpecial = Usd.TimeCode(24.0)
    AssertNotEqual(default1, nonSpecial)
    AssertNotEqual(earliestTime1, nonSpecial)
    print default1, default2, nonSpecial, earliestTime1, earliestTime2

    # test relational operators and hash.
    time1 = Usd.TimeCode(1.0)
    time2 = Usd.TimeCode(2.0)
    assert time1 == Usd.TimeCode(1.0)
    assert time2 == Usd.TimeCode(2.0)
    assert time1 != Usd.TimeCode(2.0)
    assert time2 != Usd.TimeCode(1.0)
    assert time1 != time2
    assert time1 < time2
    assert time1 <= time2
    assert time2 > time1
    assert time2 >= time1
    assert not (time1 < time1)
    assert time1 <= time1
    assert not (time1 > time1)
    assert time1 >= time1
    assert default1 < time1
    assert default1 <= time1
    assert time1 > default1
    assert time1 >= default1
    assert default1 < earliestTime1
    assert default1 <= earliestTime1
    assert time1 > earliestTime1
    assert time1 >= earliestTime1
    assert hash(default1) == hash(default2)
    assert hash(earliestTime1) == hash(earliestTime2)
    assert hash(default1) != hash(earliestTime1)
    assert hash(Usd.TimeCode(1.234)) == hash(Usd.TimeCode(1.234))
    assert hash(time1) != hash(time2)

    # Assert that invoking GetValue() on Default time raises.
    # with RequiredException(RuntimeError):
    #     Usd.TimeCode.Default().GetValue()

    allFormats = ['usd' + x for x in 'abc']

    for fmt in allFormats:

        layerName = "testUsdTimeSamples." + fmt

        if os.path.exists(layerName):
            os.unlink(layerName)

        stage = Usd.Stage.CreateNew(layerName)
        l = stage.GetRootLayer()
        prim = stage.OverridePrim("/Test")
        attr = prim.CreateAttribute("varying", Sdf.ValueTypeNames.Int)
        attr.Set(0)
        attr.Set(1, 1)
        attr.Set(2, 2)
        sdVaryingAttr = l.GetAttributeAtPath(attr.GetPath())
        AssertEqual(l.ListTimeSamplesForPath(attr.GetPath()), [1.0,2.0])
        AssertEqual(attr.GetTimeSamples(), [1.0,2.0])
        AssertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0,1)), [1.0]) 
        AssertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0,6)), [1.0, 2.0])
        AssertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0,0)), []) 
        AssertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(1.0, 2.0)), [1.0, 2.0])  

        # ensure that open, finite endpoints are disallowed
        with RequiredException(Tf.ErrorException):
            # IsClosed() will be True by default in the Interval ctor
            bothClosed = Gf.Interval(1.0, 2.0, False, False)
            attr.GetTimeSamplesInInterval(bothClosed)
         
        with RequiredException(Tf.ErrorException):
            finiteMinClosed = Gf.Interval(1.0, 2.0, True, False)
            attr.GetTimeSamplesInInterval(finiteMinClosed)
        
        with RequiredException(Tf.ErrorException):
            finiteMaxClosed = Gf.Interval(1.0, 2.0, False, True)
            attr.GetTimeSamplesInInterval(finiteMaxClosed)

        AssertEqual(attr.GetBracketingTimeSamples(1.5), (1.0,2.0))
        AssertEqual(attr.GetBracketingTimeSamples(1.0), (1.0,1.0))
        AssertEqual(attr.GetBracketingTimeSamples(2.0), (2.0,2.0))
        AssertEqual(attr.GetBracketingTimeSamples(.9), (1.0,1.0))
        AssertEqual(attr.GetBracketingTimeSamples(earliestTime1.GetValue()), 
                    (1.0,1.0))
        AssertEqual(attr.GetBracketingTimeSamples(2.1), (2.0,2.0))
        # XXX: I would like to verify timeSamples here using the Sd API
        #      but GetInfo fails to convert the SdTimeSampleMap back to
        #      python correctly, and SetInfo does not convert a python
        #      dictionary back to C++ correctly.
        #d = sdVaryingAttr.GetInfo("timeSamples")
        #d[1.0] = 99
        #d[2.0] = 42 
        #sdVaryingAttr.SetInfo("timeSamples", d)
        #AssertEqual(l.ListTimeSamplesForPath(attr.GetPath()), [99.0,42.0])

        attr = prim.CreateAttribute("unvarying", Sdf.ValueTypeNames.Int)
        attr.Set(0)
        sdUnvaryingAttr = l.GetAttributeAtPath(attr.GetPath())
        AssertEqual(l.ListTimeSamplesForPath(attr.GetPath()), [])
        AssertEqual(attr.GetTimeSamples(), [])
        AssertEqual(attr.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [])
        AssertEqual(attr.GetBracketingTimeSamples(1.5), ())

        # Test for bug/81006 . Could break this out into a separate test, but
        # given the ratio of setup to test, figured I'd stick it in here.  This
        # will segfault if the fix is really not working.
        empty = Vt.DoubleArray()
        emptyAttr = prim.CreateAttribute(
            "empty", Sdf.ValueTypeNames.DoubleArray)
        emptyAttr.Set(empty)
        roundEmpty = emptyAttr.Get(Usd.TimeCode.Default())
        # See bug/81998 why we cannot test for equality here
        AssertEqual(len(roundEmpty), len(empty))

        # print the layer contents for debugging
        print l.ExportToString()

        AssertEqual(sdUnvaryingAttr.HasInfo("timeSamples"), False)
        AssertEqual(sdVaryingAttr.HasInfo("timeSamples"), True)


def Main(argv):
    BasicTest()


if __name__ == "__main__":
    SetAssertMode(MTR_EXIT_TEST)
    Main(sys.argv)


