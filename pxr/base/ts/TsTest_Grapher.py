#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from . import TsTest_SplineData as SData

from pxr import Ts

import sys


class TsTest_Grapher(object):

    class Spline(object):
        def __init__(self, name, data, samples, baked, colorIndex):
            self.data = data
            self.name = name
            self.baked = baked
            self.samples = samples
            self.colorIndex = colorIndex

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
            # XXX: incorrect if the earliest knot is from looping
            self._regions.append(self._Region(
                knots[0].time,
                openStart = False, isDim = False))

            # Looping regions
            if lp.enabled:

                protoLen = lp.protoEnd - lp.protoStart

                # Prepeats: dim
                if lp.numPreLoops:
                    self._regions.append(self._Region(
                        lp.protoStart - protoLen * lp.numPreLoops,
                        openStart = False, isDim = True))

                # Prototype: normal
                self._regions.append(self._Region(
                    lp.protoStart,
                    openStart = False, isDim = False))

                # Repeats: dim
                if lp.numPostLoops:
                    self._regions.append(self._Region(
                        lp.protoEnd,
                        openStart = False, isDim = True))
                    loopEnd = lp.protoEnd + protoLen * lp.numPostLoops
                else:
                    loopEnd = lp.protoEnd

                # After: normal.  A knot exactly on the boundary belongs to the
                # prior region (openStart).
                self._regions.append(self._Region(
                    loopEnd,
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

    class _KnotData(object):

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
            self, title = None,
            widthPx = 1000, heightPx = 750, dataAspect = None,
            includeScales = True, includeBox = True):
        """
        Set up a Grapher.

        'title', if supplied, is rendered as the graph title.

        'widthPx' and 'heightPx' set the size of the overall graph image.

        'dataAspect' sets the data aspect ratio.  This scales the horizontal or
        vertical extent of the data to squash or stretch the spline curves.
        This can be useful if, for example, a graph is stretched too tall to see
        well.  If unspecified, the horizontal and vertical scales will both be
        determined automatically, so that the curves occupy most of the
        available space.  If a float value, specifies how many pixels one unit
        in the Y-axis (the value axis) will occupy, as a multiple of how many
        pixels one unit in the X-axis (the time axis) occupies; the overall
        curves will be scaled so that the larger of the data extents of the two
        axes occupies most of the available space on its drawing axis.  Higher
        values stretch the curve height; lower values squash it.  For test
        splines that use similarly scaled coordinates for time and value, a data
        apsect of 1 is a reasonable starting point.  Currently, dataAspect only
        affects the main graph, not the diff graph, if present.

        'includeScales' controls whether ticks and labels are drawn along the
        axes.

        'includeBox' controls whether an overall box appears around the graph.
        """
        self._title = title
        self._widthPx = widthPx
        self._heightPx = heightPx
        self._dataAspect = dataAspect
        self._includeScales = includeScales
        self._includeBox = includeBox

        self._splines = []
        self._diffs = None
        self._figure = None

    def AddSpline(self, name, splineData, samples, baked = None,
                  colorIndex = None):

        self._splines.append(
            TsTest_Grapher.Spline(
                name, splineData, samples,
                baked or splineData, colorIndex))

        # Reset the graph in case we're working incrementally.
        self._ClearGraph()

    def AddDiffData(self, diffs):
        self._diffs = diffs

    def Display(self):
        self._MakeGraph()
        from matplotlib import pyplot
        pyplot.show()
        self._ClearGraph()

    def Write(self, filePath):
        self._MakeGraph()
        self._figure.savefig(filePath)
        self._ClearGraph()

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
            # Turn off all ticks and labels.
            axes.get_xaxis().set_major_locator(ticker.NullLocator())
            axes.get_xaxis().set_minor_locator(ticker.NullLocator())
            axes.get_yaxis().set_major_locator(ticker.NullLocator())
            axes.get_yaxis().set_major_locator(ticker.NullLocator())
        else:
            # Turn on X-axis labels.  Axis sharing causes these to be off by
            # default for the top graph if there are two graphs.
            axes.get_xaxis().set_tick_params(labelbottom = True)

        if not self._includeBox:
            axes.set_frame_on(False)

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

        # Figure, with one or two graphs.  The 'sharex' flag says that if there
        # are two graphs, they should share an X-axis, so that they scale
        # identically, both to the bounds of the unioned data.  This is helpful
        # because sometimes the diff graph has a lesser data extent than the
        # main graph, since the main graph displays tangents while the diff
        # graph does not.
        numGraphs = 2 if self._diffs else 1
        self._figure, axSet = pyplot.subplots(
            nrows = numGraphs, squeeze = False, sharex = True)
        self._figure.set(
            dpi = 100.0,
            figwidth = self._widthPx / 100.0,
            figheight = self._heightPx / 100.0)

        # Main graph
        axMain = axSet[0][0]
        if self._title:
            axMain.set_title(self._title)
        if self._dataAspect is not None:
            axMain.set_aspect(self._dataAspect)
        self._ConfigureAxes(axMain)

        legendNames = []
        legendLines = []

        # Individual splines
        for splineIdx in range(len(self._splines)):

            # Determine drawing color for this spline.
            colorIndex = self._splines[splineIdx].colorIndex
            if colorIndex is None:
                colorIndex = splineIdx
            splineColor = colorCycle[colorIndex]

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

            if isinstance(samples, (Ts.SplineSamples,
                                    Ts.SplineSamplesWithSources)):
                # These samples were produced by Ts.Spline.Sample. Use it
                # directly.
                prevValue = None
                prevTime = None
                color = splineColor

                useSources = hasattr(samples, "sources")
                
                for i, polyline in enumerate(samples.polylines):

                    if (polyline[0][0] == prevTime
                        and polyline[0][1] != prevValue):
                        
                        # We have a cliff, draw it.
                        axMain.plot(
                            [prevTime, polyline[0][0]],
                            [prevValue, polyline[0][1]],
                            color = color,
                            linestyle = "dotted")

                    if useSources:
                        source = samples.sources[i]

                        color = splineColor
                        if source not in (Ts.SourceInnerLoopProto,
                                          Ts.SourceKnotInterp):
                            color = self._DimColor(color)

                    sampleTimes, sampleValues = zip(*polyline)
                    axMain.plot(sampleTimes, sampleValues, color=color)

                    # Save the last vertext for a possible "cliff"
                    prevTime, prevValue = polyline[-1]

            else:
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

        # Determine if all splines have the same knots and parameters.
        sharedData = not any(
            s for s in self._splines[1:] if s.baked != self._splines[0].baked)

        # Knot points and tangents
        knotSplines = [self._splines[0]] if sharedData else self._splines
        for splineIdx in range(len(knotSplines)):
            splineData = knotSplines[splineIdx].baked

            # Build style region table.
            styleTable = self._StyleTable(
                knotSplines[splineIdx].data,
                forKnots = True)

            normalKnotData = self._KnotData(splineData)
            dimKnotData = self._KnotData(splineData)

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

            # Determine drawing color for this spline's knots.
            if sharedData:
                knotColor = 'black'
            else:
                colorIndex = knotSplines[splineIdx].colorIndex
                if colorIndex is None:
                    colorIndex = splineIdx
                knotColor = colorCycle[colorIndex]

            # Draw knots and tangents.
            normalKnotData.Draw(axMain, knotColor)
            dimKnotData.Draw(axMain, self._DimColor(knotColor))

        # Diff graph
        if self._diffs:
            axDiffs = axSet[1][0]
            axDiffs.set_title("Diffs")
            self._ConfigureAxes(axDiffs)
            diffTimes = [d.time for d in self._diffs]
            diffValues = [d.value for d in self._diffs]
            axDiffs.plot(diffTimes, diffValues)

        self._figure.tight_layout()

    def _ClearGraph(self):

        if self._figure:
            from matplotlib import pyplot
            pyplot.close(self._figure)
            pyplot.clf()
            self._figure = None
