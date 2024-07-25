#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Ts
import sys, math, functools

try:
    from PySide6 import QtCore
    from PySide6 import QtGui
    from PySide6 import QtWidgets
except ImportError:
    try:
        from PySide2 import QtCore
        from PySide2 import QtGui
        from PySide2 import QtWidgets
    except ImportError:
        sys.exit("Can't import PySide")


kColors = {
    Ts.AntiRegressionNone: '#3ab2ab',
    Ts.AntiRegressionContain: '#ff7f0e',
    Ts.AntiRegressionKeepRatio: '#2ca02c',
    Ts.AntiRegressionKeepStart: '#d62728',
    Ts.RegressionPreventer.ModeLimitActive: '#9467bd',
    Ts.RegressionPreventer.ModeLimitOpposite: '#5566ff',
    "Bezier": '#e377c2'
}


class KeyMonitor(QtCore.QObject):

    def __init__(self):

        super().__init__()

    def eventFilter(self, obj, event):

        if event.type() == QtCore.QEvent.KeyPress \
                and event.modifiers() == QtCore.Qt.ControlModifier \
                and event.key() == QtCore.Qt.Key_Q:

            g_app.exit()
            return True

        return False


class Controller:

    def __init__(self):

        self._CreateWidgets()
        self._ConnectWidgets()
        self._PopulateMuseum()
        self._SetDefaults()
        self._ShowMain()

    def _CreateWidgets(self):

        museumBox = QtWidgets.QGroupBox("Museum Cases")
        museumBoxLayout = QtWidgets.QVBoxLayout(museumBox)
        self._museumList = QtWidgets.QListWidget()
        self._museumList.setSelectionMode(
            QtWidgets.QAbstractItemView.SingleSelection)
        museumBoxLayout.addWidget(self._museumList)

        self._modeRadios = []
        self._AddModeRadio(
            "None", Ts.AntiRegressionNone)
        self._AddModeRadio(
            "Contain", Ts.AntiRegressionContain)
        self._AddModeRadio(
            "Keep Ratio", Ts.AntiRegressionKeepRatio)
        self._AddModeRadio(
            "Keep Start", Ts.AntiRegressionKeepStart)
        self._AddModeRadio(
            "Limit Active", Ts.RegressionPreventer.ModeLimitActive)
        self._AddModeRadio(
            "Limit Opposite", Ts.RegressionPreventer.ModeLimitOpposite)

        modeBox = QtWidgets.QGroupBox("Anti-Regression Modes")
        modeBoxLayout = QtWidgets.QVBoxLayout(modeBox)
        for rb in self._modeRadios:
            modeBoxLayout.addWidget(rb)

        self._bezCheckbox = QtWidgets.QCheckBox("Raw Bezier")
        self._bezCheckbox.setIcon(self._CreateColorIcon("Bezier"))

        self._adjustButton = QtWidgets.QPushButton("Adjust")
        self._adjustButton.setEnabled(False)

        controlBoxLayout = QtWidgets.QVBoxLayout()
        controlBoxLayout.addWidget(museumBox)
        controlBoxLayout.addWidget(modeBox)
        controlBoxLayout.addWidget(self._bezCheckbox)
        controlBoxLayout.addWidget(self._adjustButton)

        self._canvasWidget = CanvasWidget()

        self._graphWidget = GraphWidget()
        graphLayout = QtWidgets.QHBoxLayout()
        graphLayout.addStretch()
        graphLayout.addWidget(self._graphWidget)
        graphLayout.addStretch()

        drawingLayout = QtWidgets.QVBoxLayout()
        drawingLayout.addWidget(self._canvasWidget)
        drawingLayout.addLayout(graphLayout)
        drawingLayout.setStretch(0, 1)
        drawingLayout.setStretch(1, 0)

        self._mainWidget = QtWidgets.QWidget()
        mainLayout = QtWidgets.QHBoxLayout(self._mainWidget)
        mainLayout.addLayout(controlBoxLayout)
        mainLayout.addLayout(drawingLayout)

        self._window = QtWidgets.QMainWindow()
        self._window.setCentralWidget(self._mainWidget)

    def _AddModeRadio(self, label, mode):

        rb = QtWidgets.QRadioButton(label)
        rb.setIcon(self._CreateColorIcon(mode))
        rb.setProperty("arMode", mode)
        self._modeRadios.append(rb)

    def _CreateColorIcon(self, colorKey):

        pixmap = QtGui.QPixmap(100, 100)
        pixmap.fill(QtGui.QColor(kColors[colorKey]))
        return QtGui.QIcon(pixmap)

    def _ConnectWidgets(self):

        self._museumList.currentItemChanged.connect(
            lambda newItem, prevItem:
                newItem
                and self._canvasWidget.LoadMuseumCase(newItem.text()))

        for rb in self._modeRadios:
            def func(mode, checked):
                if checked:
                    self._canvasWidget.SetMode(mode)
                    self._graphWidget.SetMode(mode)
            rb.toggled.connect(
                functools.partial(func, rb.property("arMode")))

        self._bezCheckbox.stateChanged.connect(
            lambda state:
                self._canvasWidget.SetBezierEnabled(bool(state)))

        self._adjustButton.clicked.connect(
            self._canvasWidget.PerformBatchAntiRegression)

        self._canvasWidget.WidthsChanged.connect(
            self._graphWidget.SetPoints)

        self._canvasWidget.CanBatchChanged.connect(
            self._adjustButton.setEnabled)

        self._monitor = KeyMonitor()
        g_app.installEventFilter(self._monitor)

    def _PopulateMuseum(self):

        for name in Ts.TsTest_Museum.GetAllNames():
            self._museumList.addItem(name)

    def _SetDefaults(self):

        self._museumList.setCurrentRow(0)
        self._modeRadios[2].setChecked(True)
        self._bezCheckbox.setChecked(True)

    def _ShowMain(self):

        self._window.show()
        self._window.setFocus()
        self._window.activateWindow()


def _ConfigurePainter(
        painter,
        lineColor = None, lineWidth = 1,
        fillColor = None,
        intensity = 1.0):

    def _LightenedColor(name, intensity):
        """
        Create the named or stringified color, then modulate its lightness:
        unmodified at intensity 1, full white at intensity 0.
        """
        color = QtGui.QColor(name)
        hsla = color.getHslF()
        lightness = 1.0 - intensity * (1.0 - hsla[2])
        color.setHslF(hsla[0], hsla[1], lightness, hsla[3])
        return color

    if lineColor:
        color = _LightenedColor(lineColor, intensity)
        painter.setPen(QtGui.QPen(color, lineWidth))
    else:
        painter.setPen(QtGui.QPen(QtGui.QBrush(), 0))

    if fillColor:
        color = _LightenedColor(fillColor, intensity)
        painter.setBrush(color)
    else:
        painter.setBrush(QtGui.QBrush())


class CanvasWidget(QtWidgets.QWidget):

    CanBatchChanged = QtCore.Signal(bool)
    WidthsChanged = QtCore.Signal(float, float, float, float)

    kMarginPix = 200
    kLineWidthPix = 2
    kMarkerRadiusPix = 8

    HandleKnot = 0
    HandlePre = 1
    HandlePost = 2

    NoPick = 255

    class _Pickable:

        def __init__(self, knotTime, whichHandle):

            self.knotTime = knotTime
            self.whichHandle = whichHandle

        def AffectsPreSegment(self):

            return self.whichHandle in [
                CanvasWidget.HandleKnot, CanvasWidget.HandlePre]

        def AffectsPostSegment(self):

            return self.whichHandle in [
                CanvasWidget.HandleKnot, CanvasWidget.HandlePost]

    def __init__(self):

        super().__init__()

        # Established by setters.
        self._mode = Ts.AntiRegressionNone
        self._drawBezier = False

        # Established by LoadMuseumCase.
        self._spline = Ts.Spline()
        self._timeMin = 0.0
        self._timeRange = 1.0
        self._valueMin = 0.0
        self._valueRange = 1.0

        # Picking lookup.  Established by painting.  The image is an 8-bit,
        # 1-channel ID image.  Each pixel is either NoPick or an index into
        # _pickables.  Each entry in _pickables is a _Pickable object.
        self._pickImage = None
        self._pickables = []

        self._ResetDrag()

    def _ResetDrag(self):

        self._dragging = False
        self._pick = None
        self._origKnot = None
        self._activeSegmentStartKnotIndex = None
        self._activeSegmentEndKnotIndex = None
        self._preventer = None
        self._unlimitedSpline = None
        self._unlimitedPreventer = None

    def LoadMuseumCase(self, caseName):

        # Get museum data.
        splineData = Ts.TsTest_Museum.GetDataByName(caseName)

        # Only handle Beziers.
        if splineData.GetIsHermite():
            return

        # Build spline.
        self._spline = Ts.TsTest_TsEvaluator().SplineDataToSpline(splineData)

        # If we have a batch ghost, remove it.
        self._RemoveBatchGhost()

        # Find time and value range of knots and tangent endpoints.
        # Thanks to the convex hull property, this always includes the curve.
        # XXX: should use baked knots when those become available.
        knotMaps = [self._spline.GetKnots()]
        if self._unlimitedSpline:
            knotMaps.append(self._unlimitedSpline.GetKnots())
        keys = knotMaps[0].keys()
        times = []
        vals = []
        for i in range(len(keys)):
            for knotMap in knotMaps:
                knot = knotMap[keys[i]]
                times.append(knot.GetTime())
                vals.append(knot.GetValue())
                vals.append(knot.GetPreValue())
                if i > 0:
                    times.append(knot.GetTime() - knot.GetPreTanWidth())
                    vals.append(
                        knot.GetValue()
                        - knot.GetPreTanWidth() * knot.GetPreTanSlope())
                if i < len(keys) - 1:
                    times.append(knot.GetTime() + knot.GetPostTanWidth())
                    vals.append(
                        knot.GetValue()
                        + knot.GetPostTanWidth() * knot.GetPostTanSlope())
        minTime = min(*times)
        maxTime = max(*times)
        minVal = min(*vals)
        maxVal = max(*vals)

        # Store stats for scaled drawing.
        self._timeMin = minTime
        self._timeRange = maxTime - minTime
        self._valueMin = minVal
        self._valueRange = maxVal - minVal

        # Indicate whether PerformBatchAntiRegression will do anything.
        self._BroadcastCanBatch()

        # Redraw.
        self.update()

    def SetMode(self, mode):

        self._mode = mode
        self._BroadcastCanBatch()

    def SetBezierEnabled(self, enabled):

        self._drawBezier = enabled
        self.update()

    def PerformBatchAntiRegression(self):

        # Anything to do?
        if not self._CanBatch():
            return

        # Copy original and de-regress.
        self._unlimitedSpline = Ts.Spline(self._spline)
        with Ts.AntiRegressionAuthoringSelector(self._mode):
            self._spline.AdjustRegressiveTangents()

        # Indicate that there's nothing for batch anti-regression to do.
        self.CanBatchChanged.emit(False)

        # If the spline has only one segment, broadcast its tangent widths.
        if len(self._spline.GetKnots()) == 2:
            self._BroadcastTanWidths(0, 1)

        # Redraw.
        self.update()

    def _CanBatch(self):

        if self._mode not in [
                Ts.AntiRegressionContain,
                Ts.AntiRegressionKeepRatio,
                Ts.AntiRegressionKeepStart]:
            return False

        with Ts.AntiRegressionAuthoringSelector(self._mode):
            return self._spline.HasRegressiveTangents()

    def _BroadcastCanBatch(self):

        self.CanBatchChanged.emit(self._CanBatch())

    def _RemoveBatchGhost(self):

        self._unlimitedSpline = None
        self.WidthsChanged.emit(-1, -1, -1, -1)

    def sizeHint(self):

        return QtCore.QSize(1000, 750)

    def paintEvent(self, event):

        if not self._spline:
            return

        # Set up for drawing and init background.
        self._painter = QtGui.QPainter(self)
        _ConfigurePainter(self._painter, fillColor = "white")
        self._painter.drawRect(self.rect())

        # Set up for pick rendering unless we're already dragging.
        if not self._dragging:
            self._pickImage = QtGui.QImage(
                self.size(), QtGui.QImage.Format_Grayscale8)
            self._pickImage.fill(self.NoPick)
            self._pickables = []
            self._pickPainter = QtGui.QPainter(self._pickImage)
        else:
            self._pickImage = None
            self._pickables = []
            self._pickPainter = None

        # Paint the pieces.
        self._PaintBezier()
        self._PaintKnotsAndTangents()
        self._PaintCurve()

        # Complete drawing.
        self._painter.end()
        if self._pickPainter:
            self._pickPainter.end()

    def _PaintKnotsAndTangents(self):

        # XXX: should use baked knots when those become available.
        knotMap = self._spline.GetKnots()
        keys = knotMap.keys()
        lp = (self._spline.GetInnerLoopParams()
              if self._spline.HasInnerLoops()
              else None)

        for i in range(len(keys)):

            knot = knotMap[keys[i]]
            knotPt = self._ToPix(knot.GetTime(), knot.GetValue())

            # Choose handle colors.
            knotColor = preColor = postColor = "black"
            if self._dragging:
                start = self._activeSegmentStartKnotIndex
                end = self._activeSegmentEndKnotIndex
                if i >= start and i <= end:
                    knotColor = kColors[self._mode]
                if i > start and i <= end:
                    preColor = kColors[self._mode]
                if i >= start and i < end:
                    postColor = kColors[self._mode]

            # Determine whether this is an echoed knot.
            echoed = (
                lp
                and lp.GetLoopedInterval().Contains(knot.GetTime())
                and not lp.GetPrototypeInterval().Contains(knot.GetTime()))

            # Draw echoed knots fainter.
            intensity = 0.2 if echoed else 1.0

            # Draw circle at knot.
            # Don't allow echoed knots to be moved.
            _ConfigurePainter(
                self._painter, fillColor = knotColor, intensity = intensity)
            pickable = self._Pickable(knot.GetTime(), self.HandleKnot)
            self._DrawWithPickable(
                None if echoed else pickable,
                QtGui.QPainter.drawEllipse,
                knotPt, self.kMarkerRadiusPix, self.kMarkerRadiusPix)

            # XXX: we don't currently support dual-valued knots.

            # Draw square and line for pre-tangent.
            # Don't allow echoed tangents to be moved.
            if i > 0:
                prevKnot = knotMap[keys[i - 1]]
                pickable = self._Pickable(knot.GetTime(), self.HandlePre)
                self._PaintTangent(
                    knot, prevKnot, self.HandlePre, preColor, intensity,
                    None if echoed else pickable)

            # Draw square and line for post-tangent.
            # Don't allow echoed tangents to be moved.
            if i < len(keys) - 1:
                nextKnot = knotMap[keys[i + 1]]
                pickable = self._Pickable(knot.GetTime(), self.HandlePost)
                self._PaintTangent(
                    knot, nextKnot, self.HandlePost, postColor, intensity,
                    None if echoed else pickable)

    def _PaintBezier(self):

        if not (self._drawBezier and self._unlimitedSpline):
            return

        # Knots never differ between _spline and _unlimitedSpline, so don't
        # bother with them; they would always be overpainted by _spline knots.

        # XXX: should use baked knots when those become available.
        knotMap = self._unlimitedSpline.GetKnots()
        keys = knotMap.keys()
        numSegments = len(keys) - 1

        # Paint tangent handles.
        for startIdx in range(numSegments):
            startKnot = knotMap[keys[startIdx]]
            endKnot = knotMap[keys[startIdx + 1]]
            self._PaintTangent(
                startKnot, endKnot, self.HandlePost, kColors["Bezier"])
            self._PaintTangent(
                endKnot, startKnot, self.HandlePre, kColors["Bezier"])

        # Convert spline to splineData.
        splineData = Ts.TsTest_TsEvaluator().SplineToSplineData(
            self._unlimitedSpline)

        # Sample with TsTest.
        samples = Ts.TsTest_SampleBezier(
            splineData, numSamples = 200 * numSegments)

        # Translate to QPainterPath.
        path = QtGui.QPainterPath()
        path.moveTo(self._ToPix(samples[0].time, samples[0].value))
        for sample in samples[1:]:
            path.lineTo(self._ToPix(sample.time, sample.value))

        # Paint curve.
        _ConfigurePainter(
            self._painter,
            lineColor = kColors["Bezier"], lineWidth = self.kLineWidthPix)
        self._painter.drawPath(path)

    def _PaintTangent(
            self, knot, oppositeKnot, whichHandle, color,
            intensity = 1.0, pickable = None):

        # Only draw tangents for Beziers.
        startKnot = knot if whichHandle == self.HandlePost else oppositeKnot
        if startKnot.GetNextInterpolation() != Ts.InterpCurve:
            return

        # Find coordinates of knot and tangent endpoint.
        knotPt = self._ToPix(knot.GetTime(), knot.GetValue())
        if whichHandle == self.HandlePre:
            tanPt = self._ToPix(
                knot.GetTime() - knot.GetPreTanWidth(),
                knot.GetValue()
                    - knot.GetPreTanWidth() * knot.GetPreTanSlope())
        else:
            tanPt = self._ToPix(
                knot.GetTime() + knot.GetPostTanWidth(),
                knot.GetValue()
                    + knot.GetPostTanWidth() * knot.GetPostTanSlope())

        _ConfigurePainter(
            self._painter,
            lineColor = color, lineWidth = self.kLineWidthPix,
            fillColor = color,
            intensity = intensity)

        # Draw marker, optionally with pickable.
        drawArgs = (
            tanPt.x() - self.kMarkerRadiusPix,
            tanPt.y() - self.kMarkerRadiusPix,
            self.kMarkerRadiusPix * 2,
            self.kMarkerRadiusPix * 2)
        if pickable:
            self._DrawWithPickable(pickable, QtGui.QPainter.drawRect, *drawArgs)
        else:
            self._painter.drawRect(*drawArgs)

        # Draw line from endpoint to knot.
        self._painter.drawLine(
            tanPt.x(), tanPt.y(),
            knotPt.x(), knotPt.y())

    def _PaintCurve(self):

        # Draw regions.
        for region in self._MakeRegions():

            # Sample with TsTest.
            # XXX: should use TsSpline.Sample() API when that becomes available.
            evaluator = Ts.TsTest_TsEvaluator()
            splineData = evaluator.SplineToSplineData(self._spline)
            times = Ts.TsTest_SampleTimes(splineData)
            times.AddStandardTimes()
            samples = evaluator.Eval(splineData, times)

            # Translate to QPainterPath.
            path = QtGui.QPainterPath()
            path.moveTo(self._ToPix(samples[0].time, samples[0].value))
            for sample in samples[1:]:
                path.lineTo(self._ToPix(sample.time, sample.value))

            # Paint.
            _ConfigurePainter(
                self._painter,
                lineColor = region.color, lineWidth = self.kLineWidthPix,
                intensity = region.intensity)
            self._painter.drawPath(path)

    def _MakeRegions(self):

        class _Region:
            def __init__(self, startTime, endTime, color, intensity):
                self.startTime = startTime
                self.endTime = endTime
                self.color = color
                self.intensity = intensity

        # Gather times of first and last knots.
        keys = self._spline.GetKnots().keys()
        timeSet = set([keys[0], keys[-1]])

        # Gather times where looping status changes.
        lp = (self._spline.GetInnerLoopParams()
              if self._spline.HasInnerLoops()
              else None)
        if lp:
            timeSet |= set([
                lp.GetLoopedInterval().min,
                lp.GetLoopedInterval().max,
                lp.GetPrototypeInterval().min,
                lp.GetPrototypeInterval().max])

        # Gather times where active-drag status changes.
        if self._dragging:
            actStart = keys[self._activeSegmentStartKnotIndex]
            actEnd = keys[self._activeSegmentEndKnotIndex]
            timeSet |= set([actStart, actEnd])

        regions = []

        # Go through transition times in order.
        timeList = sorted(timeSet)
        for i in range(len(timeList) - 1):

            time = timeList[i]
            nextTime = timeList[i + 1]

            # Determine color, based on whether active.
            if self._dragging and time >= actStart and time < actEnd:
                color = kColors[self._mode]
            else:
                color = "black"

            # Determine intensity, based on whether loop-echoed.
            if (lp
                    and lp.GetLoopedInterval().Contains(time)
                    and not lp.GetPrototypeInterval().Contains(time)):
                intensity = 0.2
            else:
                intensity = 1.0

            regions.append(_Region(time, nextTime, color, intensity))

        return regions

    def _ToPix(self, time, value):

        xSpanPix = self.width() - 2 * self.kMarginPix
        xFrac = (time - self._timeMin) / self._timeRange
        xPix = xFrac * xSpanPix + self.kMarginPix

        ySpanPix = self.height() - 2 * self.kMarginPix
        yFrac = (value - self._valueMin) / self._valueRange
        yPix = yFrac * ySpanPix + self.kMarginPix

        # Qt is Y-down, we want Y-up; invert Y.
        invYPix = self.height() - yPix

        return QtCore.QPoint(xPix, invYPix)

    def _FromPix(self, point):

        xSpanPix = self.width() - 2 * self.kMarginPix
        xFrac = (point.x() - self.kMarginPix) / xSpanPix
        time = xFrac * self._timeRange + self._timeMin

        # Qt is Y-down, we want Y-up; invert Y.
        invYPix = self.height() - point.y()

        ySpanPix = self.height() - 2 * self.kMarginPix
        yFrac = (invYPix - self.kMarginPix) / ySpanPix
        value = yFrac * self._valueRange + self._valueMin

        return (time, value)

    def _DrawWithPickable(self, pickable, func, *args):

        # Call ordinary paint method.
        func(self._painter, *args)

        # Are we drawing for picking?
        if not self._pickPainter:
            return

        # Did we get a pickable?
        if not pickable:
            return

        # Store the pickable and find its index, which will be the pick ID.
        pickId = len(self._pickables)
        self._pickables.append(pickable)

        # Tell the pick painter to paint the pick ID, in grayscale.
        _ConfigurePainter(
            self._pickPainter, fillColor = QtGui.QColor(pickId, pickId, pickId))

        # Call identical method on pick painter as on ordinary painter.
        func(self._pickPainter, *args)

    def mousePressEvent(self, event):

        # See what we picked, if anything.
        pickId = self._pickImage.pixelColor(event.pos()).value()
        if pickId == self.NoPick:
            return
        if pickId >= len(self._pickables):
            print(f"Error: pick id {pickId} out of range", file = sys.stderr)
            return
        self._pick = self._pickables[pickId]

        # Record that we're in a drag.
        self._dragging = True

        # Copy the original knot for modification.
        self._origKnot = self._spline.GetKnot(self._pick.knotTime)

        # Determine which segments are affected.
        self._DetermineActive(self._pick.knotTime)

        # Set up a RegressionPreventer.
        if self._mode in Ts.AntiRegressionMode.allValues:
            with Ts.AntiRegressionAuthoringSelector(self._mode):
                self._preventer = Ts.RegressionPreventer(
                    self._spline, self._pick.knotTime)
        else:
            self._preventer = Ts.RegressionPreventer(
                    self._spline, self._pick.knotTime, self._mode)

        # Make a copy of the spline for non-anti-regressed drawing (the raw
        # Bezier).
        self._unlimitedSpline = Ts.Spline(self._spline)

        # Pre-process the non-anti-regressed spline so that it experiences the
        # same initial "snap" anti-regression as the main spline will.  We want
        # the difference between the two splines to illustrate dynamic
        # anti-regression, not initial anti-regression.  This reproduces the
        # logic of RegressionPreventer: initial anti-regression uses Contain if
        # that is the main mode, Limit Opposite otherwise.
        if self._mode in [Ts.AntiRegressionNone, Ts.AntiRegressionContain]:
            with Ts.AntiRegressionAuthoringSelector(self._mode):
                snapPreventer = Ts.RegressionPreventer(
                    self._unlimitedSpline, self._pick.knotTime)
        else:
            snapPreventer = Ts.RegressionPreventer(
                self._unlimitedSpline, self._pick.knotTime,
                Ts.RegressionPreventer.ModeLimitOpposite)
        snapPreventer.Set(self._origKnot)

        # Now create a do-nothing Preventer that we will use for the
        # non-anti-regressed spline for the rest of the drag.  We use a
        # Preventer just so we can use the same API as for the main spline.
        with Ts.AntiRegressionAuthoringSelector(Ts.AntiRegressionNone):
            self._unlimitedPreventer = Ts.RegressionPreventer(
                self._unlimitedSpline, self._pick.knotTime)

        # Let the main Preventer process the initial position as a no-op drag.
        # This will cause initial anti-regression and then pull the handle back
        # to its starting position.
        self.mouseMoveEvent(event)

    def mouseMoveEvent(self, event):

        if not self._dragging:
            return

        # Map from pixel coordinates to (time, value) coordinates.
        time, value = self._FromPix(event.pos())

        # Copy the original knot.
        knot = self._origKnot

        # Modify the knot.
        if self._pick.whichHandle == self.HandleKnot:
            knot.SetTime(time)
            knot.SetValue(value)
        elif self._pick.whichHandle == self.HandlePre:
            knot.SetPreTanWidth(
                max(knot.GetTime() - time, 1e-4))
            knot.SetPreTanSlope(
                (knot.GetValue() - value) / knot.GetPreTanWidth())
        else:
            knot.SetPostTanWidth(
                max(time - knot.GetTime(), 1e-4))
            knot.SetPostTanSlope(
                (value - knot.GetValue()) / knot.GetPostTanWidth())

        # Set with RegressionPreventer.
        self._preventer.Set(knot)
        self._unlimitedPreventer.Set(knot)

        # If we're moving the knot, the pick time needs to be updated, and the
        # segments may reorder.
        if self._pick.whichHandle == self.HandleKnot:
            self._pick.knotTime = time
            self._DetermineActive(knot.GetTime())

        # Redraw.
        self._BroadcastTanWidthsForDrag()
        self.update()

    def mouseReleaseEvent(self, event):

        # Reset to non-dragging state and redraw.
        self._ResetDrag()
        self.update()

        # Indicate that we're no longer editing a segment.
        self.WidthsChanged.emit(-1, -1, -1, -1)

        # Indicate whether PerformBatchAntiRegression will do anything.
        self._BroadcastCanBatch()

    def _DetermineActive(self, time):

        # Find our key in the spline.
        keys = self._spline.GetKnots().keys()
        knotIndex = None
        for i in range(len(keys)):
            if keys[i] == time:
                knotIndex = i
                break

        if knotIndex is None:
            print(f"Error: can't find knot at time {time}")
            print(keys)
            return

        # Determine start knot of first affected segment.
        if knotIndex > 0 and self._pick.AffectsPreSegment():
            self._activeSegmentStartKnotIndex = knotIndex - 1
        else:
            self._activeSegmentStartKnotIndex = knotIndex

        # Determine end knot of last affected segment.
        if knotIndex < len(keys) - 1 and self._pick.AffectsPostSegment():
            self._activeSegmentEndKnotIndex = knotIndex + 1
        else:
            self._activeSegmentEndKnotIndex = knotIndex

    def _BroadcastTanWidthsForDrag(self):

        # If multiple segments are affected, skip.
        if self._activeSegmentEndKnotIndex \
                - self._activeSegmentStartKnotIndex > 1:
            return

        self._BroadcastTanWidths(
            self._activeSegmentStartKnotIndex,
            self._activeSegmentEndKnotIndex)

    def _BroadcastTanWidths(self, startKnotIndex, endKnotIndex):

        knotMap = self._spline.GetKnots()
        keys = knotMap.keys()
        time1 = keys[startKnotIndex]
        time2 = keys[endKnotIndex]

        knot1 = knotMap[time1]
        knot2 = knotMap[time2]
        segWidth = knot2.GetTime() - knot1.GetTime()

        knotMap = self._unlimitedSpline.GetKnots()
        bezKnot1 = knotMap[time1]
        bezKnot2 = knotMap[time2]

        self.WidthsChanged.emit(
            knot1.GetPostTanWidth() / segWidth,
            knot2.GetPreTanWidth() / segWidth,
            bezKnot1.GetPostTanWidth() / segWidth,
            bezKnot2.GetPreTanWidth() / segWidth)


class GraphWidget(QtWidgets.QWidget):

    kCoordMax = 1.5

    kMarginPix = 20
    kAxisLineWidthPix = 1
    kGridLineWidthPix = 1
    kPointRadiusPix = 5

    kAxisColor = '#000000'
    kGridColor = '#999999'
    kSquareColor = '#5000ff00'
    kEllipseColor = '#50ffa500'

    kEllipseSamples = 100

    def __init__(self):

        super().__init__()
        self._w1, self._w2 = -1, -1
        self._uw1, self._uw2 = -1, -1
        self._ComputeEllipsePoints()

    def SetMode(self, mode):

        self._pointColor = kColors[mode]
        self.update()

    def SetPoints(self, w1, w2, uw1, uw2):

        self._w1, self._w2 = w1, w2
        self._uw1, self._uw2 = uw1, uw2
        self.update()

    def _ComputeEllipsePoints(self):

        # See math documentation in regressionPreventer.cpp.
        #
        # This computes the points for the top ellipse section; we transpose
        # them to get the right ellipse section.
        #
        # Solve for y, given x in [0, 1]:
        #   y^2 + (x - 2) y + (x - 1)^2 = 0
        #
        # Quadratic formula (take longer solution):
        #   a = 1
        #   b = x - 2
        #   c = (x - 1)^2
        #   b^2 = x^2 - 4x + 4
        #   4ac = 4x^2 - 8x + 4
        #   b^2 - 4ac = -3x^2 + 4x
        #             = x (4 - 3x)

        self._ellipsePoints = []
        for i in range(self.kEllipseSamples + 1):
            x = i / float(self.kEllipseSamples)
            discr = math.sqrt(x * (4 - 3*x))
            y = (discr - x + 2) / 2
            self._ellipsePoints.append([x, y])

    def hasHeightForWidth(self):

        return True

    def heightForWidth(self, width):

        return width

    def sizeHint(self):

        return QtCore.QSize(300, 300)

    def paintEvent(self, event):

        self._painter = QtGui.QPainter(self)
        _ConfigurePainter(self._painter, fillColor = "white")
        self._painter.drawRect(self.rect())

        self._DrawAreas()
        if self._w1 >= 0:
            self._DrawPoints()

        self._painter.end()

    def _DrawAreas(self):

        # Axes.
        _ConfigurePainter(
            self._painter,
            lineColor = self.kAxisColor, lineWidth = self.kAxisLineWidthPix)
        self._painter.drawLine(
            self._ToPix(0, 0), self._ToPix(0, self.kCoordMax))
        self._painter.drawLine(
            self._ToPix(0, 0), self._ToPix(self.kCoordMax, 0))

        # Grid.
        _ConfigurePainter(
            self._painter,
            lineColor = self.kGridColor, lineWidth = self.kGridLineWidthPix)
        for i in range(1, 4):
            self._painter.drawLine(
                self._ToPix(0, i / 3.0), self._ToPix(4.0 / 3.0, i / 3.0))
            self._painter.drawLine(
                self._ToPix(i / 3.0, 0), self._ToPix(i / 3.0, 4.0 / 3.0))

        # Square.
        _ConfigurePainter(self._painter, fillColor = self.kSquareColor)
        self._painter.drawRect(
            QtCore.QRect(
                self._ToPix(0, 1),
                self._ToPix(1, 0) - QtCore.QPoint(1, 1)))

        # Top ellipse section.
        path1 = QtGui.QPainterPath()
        path1.moveTo(self._ToPix(0, 1))
        for point in self._ellipsePoints:
            path1.lineTo(self._ToPix(point[0], point[1]))
        path1.lineTo(self._ToPix(0, 1))

        # Right ellipse section.
        path2 = QtGui.QPainterPath()
        path2.moveTo(self._ToPix(1, 0))
        for point in self._ellipsePoints:
            path2.lineTo(self._ToPix(point[1], point[0]))
        path2.lineTo(self._ToPix(1, 0))

        # Draw ellipse sections.
        _ConfigurePainter(self._painter, fillColor = self.kEllipseColor)
        self._painter.drawPath(path1)
        self._painter.drawPath(path2)

    def _DrawPoints(self):

        _ConfigurePainter(self._painter, fillColor = kColors["Bezier"])
        self._painter.drawEllipse(
            self._ToPix(self._uw1, self._uw2),
            self.kPointRadiusPix, self.kPointRadiusPix)

        _ConfigurePainter(self._painter, fillColor = self._pointColor)
        self._painter.drawEllipse(
            self._ToPix(self._w1, self._w2),
            self.kPointRadiusPix, self.kPointRadiusPix)

    def _ToPix(self, w1, w2):

        spanPix = min(self.width(), self.height()) - 2 * self.kMarginPix

        xOffset = self.width() - spanPix - self.kMarginPix
        xFrac = w1 / self.kCoordMax
        xPix = xFrac * spanPix + xOffset

        yOffset = self.height() - spanPix - self.kMarginPix
        yFrac = w2 / self.kCoordMax
        yPix = yFrac * spanPix + yOffset

        # Qt is Y-down, we want Y-up; invert Y.
        invYPix = self.height() - yPix

        return QtCore.QPoint(xPix, invYPix)


if __name__ == "__main__":

    g_app = QtWidgets.QApplication(sys.argv)
    g_controller = Controller()
    g_app.exec_()
