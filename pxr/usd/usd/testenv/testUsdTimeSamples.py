#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

import sys, os, unittest
from pxr import Sdf,Usd,Vt,Tf, Gf

class TestUsdTimeSamples(unittest.TestCase):
    def test_Basic(self):
        """Sanity tests for Usd.TimeCode API"""
        default1 = Usd.TimeCode.Default()
        default2 = Usd.TimeCode.Default()
        self.assertEqual(default1, default2)
        earliestTime1 = Usd.TimeCode.EarliestTime()
        earliestTime2 = Usd.TimeCode.EarliestTime()
        self.assertEqual(earliestTime1, earliestTime2)
        self.assertEqual(default1, Usd.TimeCode(default1))

        nonSpecial = Usd.TimeCode(24.0)
        self.assertNotEqual(default1, nonSpecial)
        self.assertNotEqual(earliestTime1, nonSpecial)
        print(default1, default2, nonSpecial, earliestTime1, earliestTime2)

        # test relational operators and hash.
        time1 = Usd.TimeCode(1.0)
        time2 = Usd.TimeCode(2.0)
        self.assertTrue(time1 == Usd.TimeCode(1.0))
        self.assertTrue(time2 == Usd.TimeCode(2.0))
        self.assertTrue(time1 != Usd.TimeCode(2.0))
        self.assertTrue(time2 != Usd.TimeCode(1.0))
        self.assertTrue(time1 != time2)
        self.assertTrue(time1 < time2)
        self.assertTrue(time1 <= time2)
        self.assertTrue(time2 > time1)
        self.assertTrue(time2 >= time1)
        self.assertTrue(not (time1 < time1))
        self.assertTrue(time1 <= time1)
        self.assertTrue(not (time1 > time1))
        self.assertTrue(time1 >= time1)
        self.assertTrue(default1 < time1)
        self.assertTrue(default1 <= time1)
        self.assertTrue(time1 > default1)
        self.assertTrue(time1 >= default1)
        self.assertTrue(default1 < earliestTime1)
        self.assertTrue(default1 <= earliestTime1)
        self.assertTrue(time1 > earliestTime1)
        self.assertTrue(time1 >= earliestTime1)
        self.assertTrue(hash(default1) == hash(default2))
        self.assertTrue(hash(earliestTime1) == hash(earliestTime2))
        self.assertTrue(hash(default1) != hash(earliestTime1))
        self.assertTrue(hash(Usd.TimeCode(1.234)) == hash(Usd.TimeCode(1.234)))
        self.assertTrue(hash(time1) != hash(time2))

        # Basic tests for SafeStep.
        d = Usd.TimeCode.SafeStep()
        self.assertTrue(Usd.TimeCode(1e6+d) != 1e6 and Usd.TimeCode(1e6+d) > 1e6)
        self.assertTrue(Usd.TimeCode(1e12+d) == 1e12) # <- aliases at this scale
        d = Usd.TimeCode.SafeStep(maxValue=1e12)
        self.assertTrue(Usd.TimeCode(1e12+d) != 1e12)

        # with our factor of 2 safety margin, two values separated by delta at twice
        # the max, scaled down by the max scale factor, then shifted back out to the
        # max (not scaled) should still be distinct.
        d = Usd.TimeCode.SafeStep()
        t1, t2 = (1e6*2)/10.0, (1e6*2+d)/10.0
        self.assertTrue(t1 != t2 and t1 < t2)
        # shift them over so they're back at twice the max.
        self.assertTrue((t1 + 1800000.0) != (t2 + 1800000.0) and
                (t1 + 1800000.0) < (t2 + 1800000.0))

        # do same test but instead of twice the max, test twice the shrinkage.
        d = Usd.TimeCode.SafeStep()
        t1, t2 = (1e6)/20.0, (1e6+d)/20.0
        self.assertTrue(t1 != t2 and t1 < t2)
        # shift them over so they're back at twice the max.
        self.assertTrue((t1 + 950000.0) != (t2 + 950000.0) and
                (t1 + 950000.0) < (t2 + 950000.0))

        # Assert that invoking GetValue() on Default time raises.
        # with self.assertRaises(RuntimeError):
        #     Usd.TimeCode.Default().GetValue()

        allFormats = ['usd' + x for x in 'ac']

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

            # Verify resolve info source is time samples at numeric time.
            self.assertEqual(attr.GetResolveInfo().GetSource(),
                Usd.ResolveInfoSourceTimeSamples)
            self.assertEqual(attr.GetResolveInfo(Usd.TimeCode(1.0)).GetSource(),
                Usd.ResolveInfoSourceTimeSamples)
            # Verify resolve info source is default at default time.
            self.assertEqual(attr.GetResolveInfo(Usd.TimeCode.Default()).GetSource(),
                Usd.ResolveInfoSourceDefault)

            sdVaryingAttr = l.GetAttributeAtPath(attr.GetPath())
            self.assertEqual(l.ListTimeSamplesForPath(attr.GetPath()), [1.0,2.0])
            self.assertEqual(attr.GetTimeSamples(), [1.0,2.0])
            self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0,1)), [1.0]) 
            self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0,6)), [1.0, 2.0])
            self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0,0)), []) 
            self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(1.0, 2.0)), [1.0, 2.0])  

            bothOpen = Gf.Interval(1.0, 2.0, False, False)
            self.assertEqual([], attr.GetTimeSamplesInInterval(bothOpen))
             
            finiteMinClosed = Gf.Interval(1.0, 2.0, True, False)
            self.assertEqual([1.0], attr.GetTimeSamplesInInterval(finiteMinClosed))
            
            finiteMaxClosed = Gf.Interval(1.0, 2.0, False, True)
            self.assertEqual([2.0], attr.GetTimeSamplesInInterval(finiteMaxClosed))

            # Ensure that an empty interval returns nothing
            emptyInterval = Gf.Interval()
            self.assertEqual([], attr.GetTimeSamplesInInterval(emptyInterval))

            emptyInterval2 = Gf.Interval(50,1)
            self.assertEqual([], attr.GetTimeSamplesInInterval(emptyInterval2))

            self.assertEqual(attr.GetBracketingTimeSamples(1.5), (1.0,2.0))
            self.assertEqual(attr.GetBracketingTimeSamples(1.0), (1.0,1.0))
            self.assertEqual(attr.GetBracketingTimeSamples(2.0), (2.0,2.0))
            self.assertEqual(attr.GetBracketingTimeSamples(.9), (1.0,1.0))
            self.assertEqual(attr.GetBracketingTimeSamples(earliestTime1.GetValue()), 
                        (1.0,1.0))
            self.assertEqual(attr.GetBracketingTimeSamples(2.1), (2.0,2.0))
            # XXX: I would like to verify timeSamples here using the Sd API
            #      but GetInfo fails to convert the SdTimeSampleMap back to
            #      python correctly, and SetInfo does not convert a python
            #      dictionary back to C++ correctly.
            #d = sdVaryingAttr.GetInfo("timeSamples")
            #d[1.0] = 99
            #d[2.0] = 42 
            #sdVaryingAttr.SetInfo("timeSamples", d)
            #self.assertEqual(l.ListTimeSamplesForPath(attr.GetPath()), [99.0,42.0])

            attr = prim.CreateAttribute("unvarying", Sdf.ValueTypeNames.Int)
            attr.Set(0)

            # Verify resolve info source is default at all time.
            self.assertEqual(attr.GetResolveInfo().GetSource(),
                Usd.ResolveInfoSourceDefault)
            self.assertEqual(attr.GetResolveInfo(Usd.TimeCode(1.0)).GetSource(),
                Usd.ResolveInfoSourceDefault)
            self.assertEqual(attr.GetResolveInfo(Usd.TimeCode.Default()).GetSource(),
                Usd.ResolveInfoSourceDefault)

            sdUnvaryingAttr = l.GetAttributeAtPath(attr.GetPath())
            self.assertEqual(l.ListTimeSamplesForPath(attr.GetPath()), [])
            self.assertEqual(attr.GetTimeSamples(), [])
            self.assertEqual(attr.GetTimeSamplesInInterval(
                Gf.Interval.GetFullInterval()), [])
            self.assertEqual(attr.GetBracketingTimeSamples(1.5), ())

            # Test for bug/81006 . Could break this out into a separate test, but
            # given the ratio of setup to test, figured I'd stick it in here.  This
            # will segfault if the fix is really not working.
            empty = Vt.DoubleArray()
            emptyAttr = prim.CreateAttribute(
                "empty", Sdf.ValueTypeNames.DoubleArray)
            emptyAttr.Set(empty)
            roundEmpty = emptyAttr.Get(Usd.TimeCode.Default())
            # See bug/81998 why we cannot test for equality here
            self.assertEqual(len(roundEmpty), len(empty))

            # print the layer contents for debugging
            print(l.ExportToString())

            self.assertEqual(sdUnvaryingAttr.HasInfo("timeSamples"), False)
            self.assertEqual(sdVaryingAttr.HasInfo("timeSamples"), True)

    def test_GetUnionedTimeSamples(self):
        s = Usd.Stage.CreateInMemory()
        foo = s.DefinePrim('/foo')
        attr1 = foo.CreateAttribute('attr1', Sdf.ValueTypeNames.Bool)
        self.assertEqual([], attr1.GetTimeSamples())

        attr1.Set(True, 1.0)
        attr1.Set(False, 3.0)
        
        attr2 = foo.CreateAttribute('attr2', Sdf.ValueTypeNames.Float)
        attr2.Set(100.0, 2.0)
        attr2.Set(200.0, 4.0)

        self.assertEqual(Usd.Attribute.GetUnionedTimeSamples(
                                [attr1, attr2]), 
                         [1.0, 2.0, 3.0, 4.0])

        self.assertEqual(Usd.Attribute.GetUnionedTimeSamplesInInterval(
                                [attr1, attr2], Gf.Interval(1.5, 3.5)), 
                         [2.0, 3.0])

        attrQueries = [Usd.AttributeQuery(attr1), Usd.AttributeQuery(attr2)]
        self.assertEqual(Usd.AttributeQuery.GetUnionedTimeSamples(
                attrQueries), [1.0, 2.0, 3.0, 4.0])

        self.assertEqual(Usd.AttributeQuery.GetUnionedTimeSamplesInInterval(
                attrQueries, Gf.Interval(1.5, 3.5)), [2.0, 3.0])

    def test_EmptyTimeSamplesMap(self):
        layer = Sdf.Layer.CreateAnonymous()
        layer.ImportFromString('''#usda 1.0
def "Foo" {
    int x = 123
    int x.timeSamples = {}
}''')
        stage = Usd.Stage.Open(layer)
        x = stage.GetPrimAtPath('/Foo').GetAttribute('x')

        # Empty timeSamples should have no effect on value resolution --
        # should resolve through to the default value.
        self.assertEqual(x.Get(), 123)
        self.assertEqual(x.GetResolveInfo().GetSource(),
            Usd.ResolveInfoSourceDefault)

    def test_usdaPrecisionBug(self):
        s = Usd.Stage.CreateInMemory()
        foo = s.DefinePrim('/foo')
        test = foo.CreateAttribute('test', Sdf.ValueTypeNames.Float)
        test.Set(0.0, 1.0)
        test.Set(1.0, 1.0-Usd.TimeCode.SafeStep())
        self.assertEqual(len(test.GetTimeSamples()), 2)
        l = Sdf.Layer.CreateAnonymous('.usda')
        l.ImportFromString(s.GetRootLayer().ExportToString())
        self.assertEqual(
            len(l.GetAttributeAtPath('/foo.test').GetInfo('timeSamples')), 2)

    def test_TimeSamplesWithOffset(self):
        '''
        Test the effect of SdfLayerOffset on timesample API.
        '''
        stage = Usd.Stage.CreateInMemory()

        # Set up a simple prim </Source>
        # with an attribute 'x' with a simple linear ramp over frames 0..10.
        source = stage.DefinePrim('/Source')
        source_attr = source.CreateAttribute('x', Sdf.ValueTypeNames.Float)
        source_attr.Set(0.0, 0.0)
        source_attr.Set(10.0, 10.0)
        self.assertEqual(source_attr.GetTimeSamples(), [0.0, 10.0])
        self.assertEqual(source_attr.Get(0.0), 0)
        self.assertEqual(source_attr.Get(10.0), 10)

        # Reference that prim, with an offset of +100 frames.
        test1 = stage.DefinePrim('/Test1')
        test1.GetReferences().AddInternalReference('/Source',
            Sdf.LayerOffset(offset=100.0, scale=1.0))
        test1_attr = test1.GetAttribute('x')
        # Both samples should pass through.
        # The sample times should be offset.
        self.assertEqual(test1_attr.GetTimeSamples(), [100.0, 110.0])
        self.assertEqual(test1_attr.GetTimeSamplesInInterval(
            Gf.Interval(0, 10)), [])
        self.assertEqual(test1_attr.GetTimeSamplesInInterval(
            Gf.Interval(0, 110)), [100.0, 110.0])
        # Value resolution should respect the offset.
        # Times outside the interval hold at the value of the nearest sample.
        self.assertEqual(test1_attr.Get(0.0), 0)
        self.assertEqual(test1_attr.Get(10.0), 0)
        self.assertEqual(test1_attr.Get(100.0), 0)
        self.assertEqual(test1_attr.Get(110.0), 10)
        self.assertEqual(test1_attr.Get(120.0), 10)

        # Reference that prim, with a 2x scale.
        test2 = stage.DefinePrim('/Test2')
        test2.GetReferences().AddInternalReference('/Source',
            Sdf.LayerOffset(offset=0.0, scale=2.0))
        test2_attr = test2.GetAttribute('x')
        # Both samples should pass through.
        # The sample times should be offset.
        self.assertEqual(test2_attr.GetTimeSamples(), [0.0, 20.0])
        self.assertEqual(test2_attr.GetTimeSamplesInInterval(
            Gf.Interval(0, 10)), [0.0])
        self.assertEqual(test2_attr.GetTimeSamplesInInterval(
            Gf.Interval(0, 20)), [0.0, 20.0])
        # Value resolution should respect the offset.
        # Times outside the interval hold at the value of the nearest sample.
        self.assertEqual(test2_attr.Get(0.0), 0)
        self.assertEqual(test2_attr.Get(10.0), 5)
        self.assertEqual(test2_attr.Get(20.0), 10)
        self.assertEqual(test2_attr.Get(30.0), 10)

    def test_PreviousTimeSamples(self):

        def _CheckPreviousTimeSamples(layer):
            # No Previous sample before first sample
            self.assertFalse(
                layer.GetPreviousTimeSampleForPath("/Prim.attr", 0.0)[0])
            # No Previous sample at first sample
            self.assertFalse(
                layer.GetPreviousTimeSampleForPath("/Prim.attr", 1.0)[0])
            # Previous sample at 2.0 is 1.0
            self.assertTrue(
                layer.GetPreviousTimeSampleForPath("/Prim.attr", 2.0)[0])
            self.assertEqual(
                layer.GetPreviousTimeSampleForPath("/Prim.attr", 2.0)[1], 1.0)
            # Previous sample at outside the range is the last sample
            self.assertTrue(
                layer.GetPreviousTimeSampleForPath("/Prim.attr", 7.0)[0])
            self.assertEqual(
                layer.GetPreviousTimeSampleForPath("/Prim.attr", 7.0)[1], 3.0)

        layerContent = '''#usda 1.0
        def "Prim" {
            double attr.timeSamples = {
                1.0: 1.0,
                2.0: 2.0,
                3.0: 3.0
            }
        }
        '''.strip()
        sdfLayer = Sdf.Layer.CreateAnonymous(".usda")
        sdfLayer.ImportFromString(layerContent)
        _CheckPreviousTimeSamples(sdfLayer)
        sdfLayer = Sdf.Layer.CreateAnonymous(".usdc")
        sdfLayer.ImportFromString(layerContent)
        _CheckPreviousTimeSamples(sdfLayer)

    def test_PreTimeTimeSamples(self):
        layer = Sdf.Layer.CreateAnonymous("preValueTimeSamples.usda")

        ### Test for pure held interpolators -- types which are held
        layer.ImportFromString(
            '''#usda 1.0
            def "Foo" {
                string x.timeSamples = {
                    1: "zero",
                    2: "one",
                    3: "two"
                }
            }
            '''.strip())
        stage = Usd.Stage.Open(layer)
        x = stage.GetAttributeAtPath('/Foo.x')
        # Ordinary time samples queries.
        self.assertEqual(x.Get(Usd.TimeCode(0)), 'zero')
        self.assertEqual(x.Get(Usd.TimeCode(1)), 'zero')
        self.assertEqual(x.Get(Usd.TimeCode(2)), 'one')
        self.assertEqual(x.Get(Usd.TimeCode(3)), 'two')
        self.assertEqual(x.Get(Usd.TimeCode(4)), 'two')
        # Pre-time queries.
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(0)), "zero")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(1)), "zero")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(1.5)), "zero")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(2)), "zero")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(2.5)), "one")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(3)), "one")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(4)), "two")

        # Test for Linear types, note that prevalue doesn't have any special
        # behavior here, other than the block or mismatched array size types which
        # falls back to held interpolation as done in the subsequent tests.
        layer.ImportFromString(
            '''#usda 1.0
            def "Foo" {
                double x.timeSamples = {
                    1: 0.0,
                    2: 1.1,
                    3: 2.2
                }
            }
            '''.strip())
        stage = Usd.Stage.Open(layer)
        x = stage.GetPrimAtPath('/Foo').GetAttribute('x')
        # Ordinary time samples queries are same as prevalue queries.
        self.assertEqual(x.Get(Usd.TimeCode(0)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode(1)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode(2)), 1.1)
        self.assertEqual(x.Get(Usd.TimeCode(3)), 2.2)
        self.assertEqual(x.Get(Usd.TimeCode(4)), 2.2)
        # Pre-time queries.
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(0)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(1)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(1.5)), 
                         x.Get(Usd.TimeCode(1.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(2)), 1.1)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(2.5)), 
                         x.Get(Usd.TimeCode(2.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(3)), 2.2)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(4)), 2.2)

        # Test for Linear types, with in between value blocks, making it
        # fallback to Held interpolation.
        layer.ImportFromString(
            '''#usda 1.0
            def "Foo" {
                double x.timeSamples = {
                    1: 0.0,
                    2: None,
                    3: 2.2
                }
            }
            '''.strip())
        stage = Usd.Stage.Open(layer)
        x = stage.GetPrimAtPath('/Foo').GetAttribute('x')
        # Ordinary time samples queries are same as prevalue queries.
        self.assertEqual(x.Get(Usd.TimeCode(0)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode(1)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode(2)), None)
        self.assertEqual(x.Get(Usd.TimeCode(3)), 2.2)
        self.assertEqual(x.Get(Usd.TimeCode(4)), 2.2)
        # Pre-time queries.
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(0)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(1)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(1.5)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(1.5)), 
                         x.Get(Usd.TimeCode(1.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(2)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(2.5)), None)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(2.5)), 
                         x.Get(Usd.TimeCode(2.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(3)), None)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(4)), 2.2)

        # Test for Linear array types, with values of different sizes in
        # consecutive samples, causing fallback to held behavior.
        layer.ImportFromString(
            '''#usda 1.0
            def "Foo" {
                double[] x.timeSamples = {
                    1: [0.0, 1.0],
                    2: [1.0],
                    3: [2.0, 3.0],
                    4: [4.0, 5.0],
                    5: None,
                    6: [6.0, 7.0]
                }
            }
            '''.strip())
        stage = Usd.Stage.Open(layer)
        x = stage.GetPrimAtPath('/Foo').GetAttribute('x')
        # Ordinary time samples queries are same as prevalue queries.
        self.assertEqual(x.Get(Usd.TimeCode(0)), Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode(1)), Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode(2)), Vt.DoubleArray(1, (1,)))
        self.assertEqual(x.Get(Usd.TimeCode(3)), Vt.DoubleArray(2, (2, 3)))
        self.assertEqual(x.Get(Usd.TimeCode(4)), Vt.DoubleArray(2, (4, 5)))
        self.assertEqual(x.Get(Usd.TimeCode(5)), None)
        self.assertEqual(x.Get(Usd.TimeCode(6)), Vt.DoubleArray(2, (6, 7)))
        self.assertEqual(x.Get(Usd.TimeCode(7)), Vt.DoubleArray(2, (6, 7)))
        # Pre-time queries.
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(0)), 
                         Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(1)), 
                         Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(1.5)), 
                         Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(1.5)), 
                         x.Get(Usd.TimeCode(1.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(2)), 
                               Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(2.5)), 
                               Vt.DoubleArray(1, (1,)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(2.5)), 
                         x.Get(Usd.TimeCode(2.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(3)), 
                         Vt.DoubleArray(1, (1,)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(3.5)), 
                         Vt.DoubleArray(2, (3, 4)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(3.5)), 
                         x.Get(Usd.TimeCode(3.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(4)), 
                         Vt.DoubleArray(2, (4, 5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(4.5)), 
                         Vt.DoubleArray(2, (4, 5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(4.5)), 
                         x.Get(Usd.TimeCode(4.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(5)), 
                         Vt.DoubleArray(2, (4, 5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(6)), None)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(7)), 
                         Vt.DoubleArray(2, (6, 7)))

    def test_PreTimeTimeSamplesOffset(self):
        layer = Sdf.Layer.CreateAnonymous("preValueTimeSamples.usda")

        ### Test for pure held interpolators -- types which are held
        layer.ImportFromString(
            '''#usda 1.0
            def "Foo" {
                string x.timeSamples = {
                    1: "zero",
                    2: "one",
                    3: "two"
                }
            }
            '''.strip())
        stage = Usd.Stage.Open(layer)
        FooOffset = stage.DefinePrim('/FooOffset')
        FooOffset.GetReferences().AddInternalReference(
            '/Foo', Sdf.LayerOffset(offset=10))
        x = stage.GetAttributeAtPath('/FooOffset.x')
        # Ordinary time samples queries.
        self.assertEqual(x.Get(Usd.TimeCode(0)), 'zero')
        self.assertEqual(x.Get(Usd.TimeCode(11)), 'zero')
        self.assertEqual(x.Get(Usd.TimeCode(12)), 'one')
        self.assertEqual(x.Get(Usd.TimeCode(13)), 'two')
        self.assertEqual(x.Get(Usd.TimeCode(14)), 'two')
        # Pre-time queries.
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(2.5)), "zero")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(11)), "zero")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(11.5)), "zero")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(12)), "zero")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(12.5)), "one")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(13)), "one")
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(14)), "two")

        # Test for Linear types, note that prevalue doesn't have any special
        # behavior here, other than the block or mismatched array size types which
        # falls back to held interpolation as done in the subsequent tests.
        layer.ImportFromString(
            '''#usda 1.0
            def "Foo" {
                double x.timeSamples = {
                    1: 0.0,
                    2: 1.1,
                    3: 2.2
                }
            }
            '''.strip())
        stage = Usd.Stage.Open(layer)
        FooOffset = stage.DefinePrim('/FooOffset')
        FooOffset.GetReferences().AddInternalReference(
            '/Foo', Sdf.LayerOffset(offset=10))
        x = stage.GetAttributeAtPath('/FooOffset.x')
        # Ordinary time samples queries are same as prevalue queries.
        self.assertEqual(x.Get(Usd.TimeCode(0)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode(11)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode(12)), 1.1)
        self.assertEqual(x.Get(Usd.TimeCode(13)), 2.2)
        self.assertEqual(x.Get(Usd.TimeCode(14)), 2.2)
        # Pre-time queries.
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(0)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(11)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(11.5)), 
                         x.Get(Usd.TimeCode(11.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(12)), 1.1)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(12.5)), 
                         x.Get(Usd.TimeCode(12.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(13)), 2.2)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(14)), 2.2)

        # Test for Linear types, with in between value blocks, making it
        # fallback to Held interpolation.
        layer.ImportFromString(
            '''#usda 1.0
            def "Foo" {
                double x.timeSamples = {
                    1: 0.0,
                    2: None,
                    3: 2.2
                }
            }
            '''.strip())
        stage = Usd.Stage.Open(layer)
        FooOffset = stage.DefinePrim('/FooOffset')
        FooOffset.GetReferences().AddInternalReference(
            '/Foo', Sdf.LayerOffset(offset=10))
        x = stage.GetAttributeAtPath('/FooOffset.x')
        # Ordinary time samples queries are same as prevalue queries.
        self.assertEqual(x.Get(Usd.TimeCode(0)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode(11)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode(12)), None)
        self.assertEqual(x.Get(Usd.TimeCode(13)), 2.2)
        self.assertEqual(x.Get(Usd.TimeCode(14)), 2.2)
        # Pre-time queries.
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(0)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(11)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(11.5)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(11.5)), 
                         x.Get(Usd.TimeCode(11.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(12)), 0.0)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(12.5)), None)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(12.5)), 
                         x.Get(Usd.TimeCode(12.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(13)), None)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(14)), 2.2)

        # Test for Linear array types, with values of different sizes in
        # consecutive samples, causing fallback to held behavior.
        layer.ImportFromString(
            '''#usda 1.0
            def "Foo" {
                double[] x.timeSamples = {
                    1: [0.0, 1.0],
                    2: [1.0],
                    3: [2.0, 3.0],
                    4: [4.0, 5.0],
                    5: None,
                    6: [6.0, 7.0]
                }
            }
            '''.strip())
        stage = Usd.Stage.Open(layer)
        FooOffset = stage.DefinePrim('/FooOffset')
        FooOffset.GetReferences().AddInternalReference(
            '/Foo', Sdf.LayerOffset(offset=10))
        x = stage.GetAttributeAtPath('/FooOffset.x')
        # Ordinary time samples queries are same as prevalue queries.
        self.assertEqual(x.Get(Usd.TimeCode(0)), Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode(11)), Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode(12)), Vt.DoubleArray(1, (1,)))
        self.assertEqual(x.Get(Usd.TimeCode(13)), Vt.DoubleArray(2, (2, 3)))
        self.assertEqual(x.Get(Usd.TimeCode(14)), Vt.DoubleArray(2, (4, 5)))
        self.assertEqual(x.Get(Usd.TimeCode(15)), None)
        self.assertEqual(x.Get(Usd.TimeCode(16)), Vt.DoubleArray(2, (6, 7)))
        self.assertEqual(x.Get(Usd.TimeCode(17)), Vt.DoubleArray(2, (6, 7)))
        # Pre-time queries.
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(0)), 
                         Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(11)), 
                         Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(11.5)), 
                         Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(11.5)), 
                         x.Get(Usd.TimeCode(11.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(12)), 
                               Vt.DoubleArray(2, (0, 1)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(12.5)), 
                               Vt.DoubleArray(1, (1,)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(12.5)), 
                         x.Get(Usd.TimeCode(12.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(13)), 
                         Vt.DoubleArray(1, (1,)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(13.5)), 
                         Vt.DoubleArray(2, (3, 4)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(13.5)), 
                         x.Get(Usd.TimeCode(13.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(14)), 
                         Vt.DoubleArray(2, (4, 5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(14.5)), 
                         Vt.DoubleArray(2, (4, 5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(14.5)), 
                         x.Get(Usd.TimeCode(14.5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(15)), 
                         Vt.DoubleArray(2, (4, 5)))
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(16)), None)
        self.assertEqual(x.Get(Usd.TimeCode.PreTime(17)), 
                         Vt.DoubleArray(2, (6, 7)))

if __name__ == "__main__":
    unittest.main()
