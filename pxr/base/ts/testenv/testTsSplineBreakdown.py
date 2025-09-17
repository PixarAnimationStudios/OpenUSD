#!/pxrpythonsubst

#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Gf, Ts

import dataclasses
from typing import Union
import unittest

# The default knot constructor is unfortunately long winded to use. You must
# either use keyword arguments for everything or start with the data typeName and
# curveType arguments followed by time, nextInterp, value, prevalue, and customData
# before you can get to tangents.
#
# Make a dataclass that simplifies this.

interpAbbrev = dict(b=Ts.InterpValueBlock,
                    h=Ts.InterpHeld,
                    l=Ts.InterpLinear,
                    c=Ts.InterpCurve)

algAbbrev = dict(n=Ts.TangentAlgorithmNone,
                 c=Ts.TangentAlgorithmCustom,
                 a=Ts.TangentAlgorithmAutoEase)

curveAbbrev = dict(b=Ts.CurveTypeBezier,
                   h=Ts.CurveTypeHermite)

extrapAbbrev = dict(b=Ts.ExtrapValueBlock,
                    h=Ts.ExtrapHeld,
                    l=Ts.ExtrapLinear,
                    s=Ts.ExtrapSloped,
                    ll=Ts.ExtrapLoopRepeat,
                    lx=Ts.ExtrapLoopReset,
                    lo=Ts.ExtrapLoopOscillate)
                    
@dataclasses.dataclass
class Knot:
    """
    Knot holds all the parameters of a Ts.Knot but is much simpler
    and more compact to type.
    """
    time: float
    value: float
    interp: Union[Ts.InterpMode, str] = Ts.InterpHeld
    preTan: tuple[float] = (0.0, 0.0)
    postTan: tuple[float] = (0.0, 0.0)
    preAlg: Union[Ts.TangentAlgorithm, str] = Ts.TangentAlgorithmNone
    postAlg: Union[Ts.TangentAlgorithm, str] = Ts.TangentAlgorithmNone

    def __post_init__(self):
        global interpAbbrev, algAbbrev
        if self.interp in interpAbbrev:
            self.interp = interpAbbrev[self.interp]

        if self.preAlg in algAbbrev:
            self.preAlg = algAbbrev[self.preAlg]

        if self.postAlg in algAbbrev:
            self.postAlg = algAbbrev[self.postAlg]

    def replace(self, **kw):
        # Return a new tuple with some fields replaced.
        return dataclasses.replace(self, **kw)

    def GetTsKnot(self):
        return Ts.Knot(time=self.time,
                       value=self.value,
                       nextInterp=self.interp,
                       preTanWidth=self.preTan[0],
                       preTanSlope=self.preTan[1],
                       postTanWidth=self.postTan[0],
                       postTanSlope=self.postTan[1],
                       preTanAlgorithm=self.preAlg,
                       postTanAlgorithm=self.postAlg)

@dataclasses.dataclass
class Loop:
    """
    Loop holds all the parameters of a Ts.LoopParams but is much simpler
    and more compatct to type.
    """
    start: float
    end: float
    numPre: int = 0
    numPost: int = 1
    offset: float = 0.0

    def replace(self, **kw):
        # Return a new tuple with some fields replaced.
        return dataclasses.replace(self, **kw)

    def GetTsLoopParams(self):
        params = Ts.LoopParams()
        params.protoStart = self.start
        params.protoEnd = self.end
        params.numPreLoops = self.numPre
        params.numPostLoops = self.numPost
        params.valueOffset = self.offset
        return params

class TsTest_Breakdown(unittest.TestCase):

    ############################################################################
    # HELPERS

    def GenSpline(self,
                  knots=None,
                  curveType=None,
                  loopParams=None,
                  preExtrap=None,
                  postExtrap=None):
        """
        Create a spline with the given knots, curveType, etc. Anything
        not specified gets the spline default value.
        """
        def _GetExtrap(extrap):
            if isinstance(extrap, Ts.Extrapolation):
                return extrap
            if isinstance(extrap, Ts.ExtrapMode):
                return Ts.Extrapolation(extrap)
            if isinstance(extrap, (float, int)):
                extrapolation = Ts.Extrapolation(Ts.ExtrapSloped)
                extrapolation.slope = extrap
                return extrapolation

        def _GetLoopParams(loop):
            if isinstance(loop, Ts.LoopParams):
                return loop
            if isinstance(loop, Loop):
                return loop.GetTsLoopParams()
            if isinstance(loop, tuple):
                return Loop(*loop).GetTsLoopParams()

        spline = Ts.Spline()
        
        if curveType is not None:
            spline.SetCurveType(curveType)

        if knots is not None:
            for knot in knots:
                if isinstance(knot, Ts.Knot):
                    spline.SetKnot(knot)
                elif isinstance(knot, Knot):
                    spline.SetKnot(knot.GetTsKnot())
            

        if loopParams is not None:
            spline.SetInnerLoopParams(_GetLoopParams(loopParams))

        if preExtrap is not None:
            spline.SetPreExtrapolation(_GetExtrap(preExtrap))

        if postExtrap is not None:
            spline.SetPostExtrapolation(_GetExtrap(postExtrap))

        return spline
    

    ############################################################################
    # TEST ROUTINES

    def test_Interpolation(self):
        """
        Test Breakdown between existing knots with different interpolations.
        """
        knots = [Knot(1.0, 1.0, 'b'),
                 Knot(2.0, 2.0, 'h'),
                 Knot(3.0, 3.0, 'l'),
                 Knot(4.0, 4.0, 'c', (.5, 0), (.5, 0)),
                 Knot(5.0, 5.0, 'h', (.5, 0))]

        for algorithm in Ts.TangentAlgorithm.allValues:
            algoKnots = [knot.replace(preAlg=algorithm, postAlg=algorithm)
                         for knot in knots]

            for curveType in Ts.CurveType.allValues:
                spline1 = self.GenSpline(algoKnots,
                                         curveType=curveType)

                spline2 = self.GenSpline(algoKnots,
                                         curveType=curveType)
                self.assertEqual(Gf.Interval(1, 2), spline2.Breakdown(1.25))
                self.assertEqual(Gf.Interval(2, 3), spline2.Breakdown(2.25))
                self.assertEqual(Gf.Interval(3, 4), spline2.Breakdown(3.25))
                self.assertEqual(Gf.Interval(4, 5), spline2.Breakdown(4.25))

                before = [spline1.Eval(i/10) for i in range(60)]
                after  = [spline2.Eval(i/10) for i in range(60)]

                for i, (b, a) in enumerate(zip(before, after)):
                    self.assertAlmostEqual(
                        b, a, places=6,
                        msg=(f"Failed at time={i/10} for {curveType=}"
                             f" and {algorithm=}"))
                

    def test_Extrapolation(self):
        """
        Test Breakdown between in extrapolation regions
        """
        knots = [Knot(1.0, 1.0, 'c'),
                 Knot(3.0, 2.0, 'c'),
                 Knot(5.0, 1.0, 'c')]
        extrapolations = {mode: Ts.Extrapolation(mode)
                          for mode in Ts.ExtrapMode.allValues}

        for extrap in extrapolations.values():
            spline1 = self.GenSpline(knots, preExtrap=extrap, postExtrap=extrap)
            spline2 = self.GenSpline(knots, preExtrap=extrap, postExtrap=extrap)

            print(f'================ f{extrap.mode} ================')
            if extrap.IsLooping():
                self.assertIsNone(spline2.Breakdown(0.5))
                self.assertIsNone(spline2.Breakdown(5.5))
            else:
                self.assertEqual(Gf.Interval(0.5, 1.0), spline2.Breakdown(0.5))
                self.assertEqual(Gf.Interval(5.0, 5.5), spline2.Breakdown(5.5))

            before = [spline1.Eval(i/10) for i in range(60)]
            after  = [spline2.Eval(i/10) for i in range(60)]

            for i, (b, a) in enumerate(zip(before, after)):
                self.assertAlmostEqual(
                    b, a, places=6,
                    msg=(f"Failed at time={i/10} for {extrap.mode=})"))


    def test_InnerLooping(self):
        """
        Test Breakdown in the presence of inner looping
        """
        # This spline copied from the Museum's "SimpleInnerLoop" spline
        simpleKnots = [Knot(112, 8.8, 'c', (0, 0), (0.9, 15.0)),
                       Knot(137, 0.0, 'c', (1.3, -5.3), (1.8, -5.3)),
                       Knot(145, 8.5, 'c', (1.0, 12.5), (1.2, 12.5)),
                       Knot(155, 20.2, 'c', (0.7, -15.7), (0.8, -15.7)),
                       Knot(181, 38.2, 'c', (2.0, -9.0))]
        simpleLoop = Loop(137, 155, 1, 1, 20.2)

        spline1 = self.GenSpline(simpleKnots, loopParams=simpleLoop)
        spline2 = self.GenSpline(simpleKnots, loopParams=simpleLoop)

        for t in (115, 125, 145, 150, 165, 175):
            if result := spline2.CanBreakdown(t):
                self.assertIsNotNone(spline2.Breakdown(t))
            else:
                print(f"CanBreakdown reports: {result.reason!r}", flush=True)
                self.assertIsNone(spline2.Breakdown(t))
                
        # print(f'{spline2.Breakdown(115) = }')
        # print(f'{spline2.Breakdown(125) = }')
        # print(f'{spline2.Breakdown(145) = }')
        # print(f'{spline2.Breakdown(150) = }')
        # print(f'{spline2.Breakdown(165) = }')
        # print(f'{spline2.Breakdown(175) = }')

        print(spline2)

        # So Breakdown in the region immediately before or after the loop echoes
        # will change the curve's shape. Breakdown in the prototype region should
        # leave it unchanged. Breakdown in a region masked by an echo is not
        # allowed.

        for t in (112, 115, 137, 145, 150, 175, 181):
            self.assertAlmostEqual(spline1.Eval(t), spline2.Eval(t), 8,
                                   msg=f'Failure at knot at time {t}')

        # Breakdown immediately before or after a loop echo will change the
        # shape of the spline because the echoed start knot cannot be updated.
        #
        # Breakdown inside a loop prototype in the first segment of the
        # prototype will change the shape because it will change the start
        # knot's tangents.
        #
        # Breakdown inside a loop prototype in the last segment of the prototype will
        # change the shale of the last segment because it cannot change the echoed
        # start knot.
        
        # These regions of the spline should be unchanged:
        unchanged = [(105, 115),  # pre-extrap to first Breakdown
                     (119, 132),  # pre-echo to pre-echo of Breakdown(150)
                     (137, 150),  # proto region to Breakdown(150)
                     (175, 185)]
        # These are the converse regions.
        changed = [(115, 119), (132, 137), (150, 175)]
        
        # This region of the spline should be unchanged.
        for span in unchanged:
            for i in range(span[0]*10, span[1]*10 + 1):
                t = i/10
                self.assertAlmostEqual(spline1.Eval(t), spline2.Eval(t), 8,
                                       msg=f'Failure at time {t}')

        for span in changed:
            self.assertFalse(all(Gf.IsClose(spline1.Eval(i/10),
                                            spline2.Eval(i/10),
                                            1.0e-8)
                                 for i in range(span[0]*10, span[1]*10 + 1)),
                             msg=(f'Span [{span[0]}..{span[1]}] should have'
                                  'changes but does not.'))
        
if __name__ == "__main__":

    # 'buffer' means that all stdout will be captured and swallowed, unless
    # there is an error, in which case the stdout of the erroring case will be
    # printed on stderr along with the test results.  Suppressing the output of
    # passing cases makes it easier to find the output of failing ones.
    unittest.main(testRunner = unittest.TextTestRunner(buffer = True))
