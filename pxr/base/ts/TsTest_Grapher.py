#
# Copyright 2023 Pixar
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

from . import TsTest_SplineData as SData

import sys


class TsTest_Grapher(object):

    class Spline(object):
        def __init__(self, name, data, samples, baked):
            self.data = data
            self.name = name
            self.baked = baked
            self.samples = samples

    class Diff(object):
        def __init__(self, time, value):
            self.time = time
            self.value = value

    class _StyleTable(object):

        class _Region(object):
            def __init__(self, start, openStart, isDim):
                self.start = start
                self.openStart = openStart
                self.isDim = isDim

        def __init__(self, data, forKnots):

            self._regions = []

            knots = list(data.GetKnots())
            lp = data.GetInnerLoopParams()

            # Before first knot: dim
            self._regions.append(self._Region(
                float('-inf'),
                openStart = True, isDim = True))

            # No knots: all dim
            if not knots:
                return

            # After first knot: normal
            self._regions.append(self._Region(
                knots[0].time,
                openStart = False, isDim = False))

            # Looping regions
            if lp.enabled:

                # Prepeats: dim
                if lp.preLoopStart < lp.protoStart:
                    self._regions.append(self._Region(
                        lp.preLoopStart,
                        openStart = False, isDim = True))

                # Prototype: normal
                self._regions.append(self._Region(
                    lp.protoStart,
                    openStart = False, isDim = False))

                # Repeats: dim
                if lp.postLoopEnd > lp.protoEnd:
                    self._regions.append(self._Region(
                        lp.protoEnd,
                        openStart = False, isDim = True))

                # After: normal.  A knot exactly on the boundary belongs to the
                # prior region (openStart).
                self._regions.append(self._Region(
                    lp.postLoopEnd,
                    openStart = forKnots, isDim = False))

            # After last knot: dim.  A knot exactly on the boundary belongs to
            # the prior region (openStart).
            self._regions.append(self._Region(
                knots[-1].time, openStart = forKnots, isDim = True))

        def IsDim(self, time):

            numRegs = len(self._regions)
            for i in range(numRegs):

                reg = self._regions[i]
                nextReg = self._regions[i + 1] if i < numRegs - 1 else None

                # Search regions in order until we either run out, or find one
                # whose start time exceeds the specified time.  There can be
                # consecutive regions with identical start times; in those
                # cases, we will return the later region, which is what we want
                # as far as drawing goes.
                if ((not nextReg)
                        or nextReg.start > time
                        or (nextReg.openStart and nextReg.start == time)):
                    return reg.isDim

            assert False, "Can't find region"

    class _KeyframeData(object):

        def __init__(self, splineData):

            self.splineData = splineData

            # Points: 1D arrays
            self.knotTimes, self.knotValues = [], []
            self.tanPtTimes, self.tanPtValues = [], []

            # Lines: 2D arrays
            self.tanLineTimes, self.tanLineValues = [[], []], [[], []]

        def Draw(self, ax, color):

            if self.knotTimes:
                ax.scatter(
                    self.knotTimes, self.knotValues,
                    color = color, marker = "o")

            if self.tanPtTimes:
                if not self.splineData.GetIsHermite():
                    ax.scatter(
                        self.tanPtTimes, self.tanPtValues,
                        color = color, marker = "s")
                ax.plot(
                    self.tanLineTimes, self.tanLineValues,
                    color = color, linestyle = "dashed")

    @classmethod
    def Init(cls):

        if hasattr(cls, "_initialized"):
            return cls._initialized

        try:
            import matplotlib
            cls._initialized = True
            return True
        except ImportError:
            cls._initialized = False
            print("Could not import matplotlib.  "
                  "Graphical output is disabled.  "
                  "To enable it, install the Python matplotlib module.",
                  file = sys.stderr)
            return False

    def __init__(
            self, title,
            widthPx = 1000, heightPx = 750,
            includeScales = True):

        self._title = title
        self._widthPx = widthPx
        self._heightPx = heightPx
        self._includeScales = includeScales

        self._splines = []
        self._diffs = None
        self._figure = None

    def AddSpline(self, name, splineData, samples, baked = None):

        self._splines.append(
            TsTest_Grapher.Spline(
                name, splineData, samples,
                baked or splineData))

        # Reset the graph in case we're working incrementally.
        self._ClearGraph()

    def AddDiffData(self, diffs):
        self._diffs = diffs

    def Display(self):
        self._MakeGraph()
        from matplotlib import pyplot
        pyplot.show()

    def Write(self, filePath):
        self._MakeGraph()
        self._figure.savefig(filePath)

    @staticmethod
    def _DimColor(colorStr):
        """Transform a matplotlib color string to reduced opacity."""
        from matplotlib import colors
        return colors.to_rgba(colorStr, 0.35)

    def _ConfigureAxes(self, axes):
        """
        Set whole-axes properties.
        """
        from matplotlib import ticker

        if not self._includeScales:
            axes.get_xaxis().set_major_locator(ticker.NullLocator())
            axes.get_xaxis().set_minor_locator(ticker.NullLocator())
            axes.get_yaxis().set_major_locator(ticker.NullLocator())
            axes.get_yaxis().set_major_locator(ticker.NullLocator())

    def _MakeGraph(self):
        """
        Set up self._figure, a matplotlib Figure.
        """
        if self._figure:
            return

        if not TsTest_Grapher.Init():
            raise Exception("matplotlib initialization failed")

        from matplotlib import pyplot, lines

        # Grab the color cycle.  Default colors are fine.  These are '#rrggbb'
        # strings.
        colorCycle = pyplot.rcParams['axes.prop_cycle'].by_key()['color']

        # Figure, with one or two graphs
        numGraphs = 2 if self._diffs else 1
        self._figure, axSet = pyplot.subplots(
            nrows = numGraphs, squeeze = False)
        self._figure.set(
            dpi = 100.0,
            figwidth = self._widthPx / 100.0,
            figheight = self._heightPx / 100.0)

        # Main graph
        axMain = axSet[0][0]
        axMain.set_title(self._title)
        self._ConfigureAxes(axMain)

        legendNames = []
        legendLines = []

        # Individual splines
        for splineIdx in range(len(self._splines)):

            # Determine drawing color for this spline.
            splineColor = colorCycle[splineIdx]

            # Collect legend data.  Fake up a Line2D artist.
            legendNames.append(self._splines[splineIdx].name)
            legendLines.append(lines.Line2D([0], [0], color = splineColor))

            sampleTimes = []
            sampleValues = []

            # Build style region tables.
            styleTable = self._StyleTable(
                self._splines[splineIdx].data,
                forKnots = False)
            knotStyleTable = self._StyleTable(
                self._splines[splineIdx].data,
                forKnots = True)

            # Find regions to draw.  A region is a time extent in which the
            # spline is drawn with the same styling.  Things that require new
            # regions include vertical discontinuities, extrapolation, and
            # looping.
            samples = self._splines[splineIdx].samples
            for sampleIdx in range(len(samples)):

                # Determine whether we have a vertical discontinuity.  That is
                # signaled by two consecutive samples with identical times.
                # Allow some fuzz in the detection of "identical", since
                # left-side evaluation in Maya is emulated with a small delta.
                sample = samples[sampleIdx]
                nextSample = samples[sampleIdx + 1] \
                    if sampleIdx < len(samples) - 1 else None
                isCliff = (nextSample
                           and abs(nextSample.time - sample.time) < 1e-4)

                # Determine whether we have crossed into a different style
                # region.
                isRegionEnd = (
                    nextSample
                    and styleTable.IsDim(nextSample.time) !=
                    styleTable.IsDim(sample.time))

                # Append this sample for drawing.
                sampleTimes.append(sample.time)
                sampleValues.append(sample.value)

                # If this is the end of a region, and sample times have been set
                # up correctly, then the next sample falls exactly on a region
                # boundary.  Include that sample to end this region... unless
                # this is also a cliff, in which case the vertical will span the
                # gap instead.
                if isRegionEnd and not isCliff:
                    sampleTimes.append(nextSample.time)
                    sampleValues.append(nextSample.value)

                # At the end of each region, draw.
                if (not nextSample) or isCliff or isRegionEnd:

                    # Use this spline's color, possibly dimmed.
                    color = splineColor
                    if styleTable.IsDim(sample.time):
                        color = self._DimColor(color)

                    # Draw.
                    axMain.plot(sampleTimes, sampleValues, color = color)

                    # Reset data for next region.
                    sampleTimes = []
                    sampleValues = []

                # At discontinuities, draw a dashed vertical.
                if isCliff:

                    # Verticals span no time, so the region rules are the same
                    # as for knots.
                    color = splineColor
                    if knotStyleTable.IsDim(sample.time):
                        color = self._DimColor(color)

                    axMain.plot(
                        [sample.time, nextSample.time],
                        [sample.value, nextSample.value],
                        color = color,
                        linestyle = "dashed")

        # Legend, if multiple splines
        if len(legendNames) > 1:
            axMain.legend(legendLines, legendNames)

        # Determine if all splines have the same keyframes and parameters.
        sharedData = not any(
            s for s in self._splines[1:] if s.baked != self._splines[0].baked)

        # Keyframe points and tangents
        knotSplines = [self._splines[0]] if sharedData else self._splines
        for splineIdx in range(len(knotSplines)):
            splineData = knotSplines[splineIdx].baked

            # Build style region table.
            styleTable = self._StyleTable(
                knotSplines[splineIdx].data,
                forKnots = True)

            normalKnotData = self._KeyframeData(splineData)
            dimKnotData = self._KeyframeData(splineData)

            knots = list(splineData.GetKnots())
            for knotIdx in range(len(knots)):
                knot = knots[knotIdx]
                prevKnot = knots[knotIdx - 1] if knotIdx > 0 else None
                nextKnot = \
                    knots[knotIdx + 1] if knotIdx < len(knots) - 1 else None

                # Decide whether to draw dim or not
                if styleTable.IsDim(knot.time):
                    knotData = dimKnotData
                else:
                    knotData = normalKnotData

                # Pre-value
                if knotIdx > 0 \
                        and prevKnot.nextSegInterpMethod == SData.InterpHeld:
                    knotData.knotTimes.append(knot.time)
                    knotData.knotValues.append(prevKnot.value)
                elif knot.isDualValued:
                    knotData.knotTimes.append(knot.time)
                    knotData.knotValues.append(knot.preValue)

                # Knot
                knotData.knotTimes.append(knot.time)
                knotData.knotValues.append(knot.value)

                # In-tangent
                if prevKnot \
                        and prevKnot.nextSegInterpMethod == SData.InterpCurve \
                        and not knot.preAuto:

                    if splineData.GetIsHermite():
                        preLen = (knot.time - prevKnot.time) / 3.0
                    else:
                        preLen = knot.preLen

                    if preLen > 0:
                        value = \
                            knot.preValue if knot.isDualValued else knot.value
                        knotData.tanPtTimes.append(knot.time - preLen)
                        knotData.tanPtValues.append(
                            value - knot.preSlope * preLen)
                        knotData.tanLineTimes[0].append(knotData.tanPtTimes[-1])
                        knotData.tanLineTimes[1].append(knot.time)
                        knotData.tanLineValues[0].append(
                            knotData.tanPtValues[-1])
                        knotData.tanLineValues[1].append(value)

                # Out-tangent
                if nextKnot \
                        and knot.nextSegInterpMethod == SData.InterpCurve \
                        and not knot.postAuto:

                    if splineData.GetIsHermite():
                        postLen = (nextKnot.time - knot.time) / 3.0
                    else:
                        postLen = knot.postLen

                    if postLen > 0:
                        knotData.tanPtTimes.append(knot.time + postLen)
                        knotData.tanPtValues.append(
                            knot.value + knot.postSlope * postLen)
                        knotData.tanLineTimes[0].append(knot.time)
                        knotData.tanLineTimes[1].append(
                            knotData.tanPtTimes[-1])
                        knotData.tanLineValues[0].append(knot.value)
                        knotData.tanLineValues[1].append(
                            knotData.tanPtValues[-1])

            # Draw keyframes and tangents.
            color = 'black' if sharedData else colorCycle[splineIdx]
            normalKnotData.Draw(axMain, color)
            dimKnotData.Draw(axMain, self._DimColor(color))

        # Diff graph
        if self._diffs:
            axDiffs = axSet[1][0]
            axDiffs.set_title("Diffs")
            self._ConfigureAxes(axDiffs)
            diffTimes = [d.time for d in self._diffs]
            diffValues = [d.value for d in self._diffs]
            axDiffs.plot(diffTimes, diffValues)

    def _ClearGraph(self):

        if self._figure:
            self._figure = None
            from matplotlib import pyplot
            pyplot.clf()
