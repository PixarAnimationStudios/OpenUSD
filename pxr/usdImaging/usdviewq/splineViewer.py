#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from .qt import QtWidgets, QtGui, QtCore
from pxr import Usd, Ts, Gf

"""
SplineViewer is a widget that visualizes a spline value of a UsdAttribute.
It samples the spline and draws it in a 2D coordinate system, with time on the
x-axis and value on the y-axis. It also shows a playhead for the current frame
and the current value at that frame.
"""
class SplineViewer(QtWidgets.QWidget):

    ### Some constants for the widget
    MIN_WIDTH = 400
    MIN_HEIGHT = 300
    MIN_TIME_RANGE = 20
    DEFAULT_SAMPLING_VALUESCALE = 1
    DEFAULT_SAMPLING_TOLERANCE = 1
    MIN_DELTA = 1e-5
    MARGIN_BUFFER = 10
    BACKGROUND_COLOR = QtGui.QColor(30, 30, 30)
    GRID_COLOR = QtGui.QColor(80, 80, 80)
    AXIS_COLOR = QtGui.QColor(200, 200, 200)
    SPLINE_COLOR = QtGui.QColor(0, 200, 255)
    KNOT_COLOR = QtGui.QColor(255, 255, 255)
    PLAYHEAD_COLOR = QtGui.QColor(255, 255, 0)
    KNOT_SIZE = 5
    TANGENT_SIZE = 3
    AXIS_TICK_LENGTH = 5
    AXIS_TICK_X_LABEL_WIDTH_OFFSET = 10
    AXIS_TICK_X_LABEL_HEIGHT_OFFSET = 15
    AXIS_TICK_Y_LABEL_WIDTH_OFFSET = 5
    AXIS_TICK_Y_LABEL_HEIGHT_OFFSET = 5

    def __init__(self, attr=None, parent=None, currentFrame=None):
        super().__init__(parent)
        self.setMinimumSize(self.MIN_WIDTH, self.MIN_HEIGHT)
        self.polylinesToPlot = None
        self.knots = None
        self.margin = 20
        self.numTicks = 5
        self.fontMetrics = QtGui.QFontMetrics(self.font())
        self.startTime = None
        self.endTime = None
        self.attr = None
        if attr:
            self.setWindowTitle(f"Spline Viewer: {attr.GetPath()}")
            self.setWindowFlags(QtCore.Qt.WindowType.Window)
            self.SetAttribute(attr, currentFrame)
    
    def SetAttribute(self, attr, frame):
        """
        Used by AttributeValueEditor to set the attribute and current frame.
        """
        if not attr or not isinstance(attr, Usd.Attribute):
            raise ValueError("Invalid attribute provided to SplineViewer.")

        if self.attr == attr:
            # If the attribute is the same, just update the current frame.
            self.currentFrame = frame
            self.update()
            return
        self.attr = attr
        self.currentFrame = frame

        # Set the start and end time based on the stage's time code range
        # to begin with.
        if self.startTime is None:
            self.startTime = attr.GetPrim().GetStage().GetStartTimeCode()
        if self.endTime is None:
            self.endTime = attr.GetPrim().GetStage().GetEndTimeCode()
        if self.endTime <= self.startTime:
            self.endTime = self.startTime + self.MIN_TIME_RANGE

        self._updateKnots()
        self._updateSamples()
        self._transformToDevice()
        self.update()

    def SetCurrentFrame(self, frame):
        """
        Used by AttributeValueEditor and the floating SplineViewer instances to 
        set the current frame when dataModel's currentFrame changes.
        """
        self.currentFrame = frame
        self.update()

    def SetStartAndEndTime(self, startTime, endTime):
        """
        Used by AttributeValueEditor and the floating SplineViewer instances to
        set the start and end time when dataModel's frame range changes
        explicitly.
        """
        if self.startTime != startTime or self.endTime != endTime:
            # The times are changing. Update our knots and samples.
            self.startTime = startTime
            self.endTime = endTime
            if self.endTime <= self.startTime:
                self.endTime = self.startTime + self.MIN_TIME_RANGE

            self._updateKnots()
            self._updateSamples()

        self._transformToDevice()
        self.update()

    def CanView(self, attr):
        """
        Used by AttributeValueEditor to determine if the attribute can be viewed
        by SplineViewer.
        """
        # Check if the attribute is a spline
        if not isinstance(attr, Usd.Attribute):
            return False

        if not attr.HasAuthoredValue():
            return False

        # Check if the value source is a spline
        if attr.GetResolveInfo().GetSource() != Usd.ResolveInfoSourceSpline:
            return False

        return True

    def Clear(self):
        """
        Used by AttributeValueEditor to clear the SplineViewer, when prim
        selection changes.
        """
        # Clear the current attribute and reset the view
        self.attr = None
        self.currentFrame = None
        self.polylinesToPlot = None
        self.update()

    def _updateKnots(self):
        self.knots = None
        if not self.attr:
            return
        spline = self.attr.GetSpline()
        if not spline or spline.IsEmpty():
            return
        timeInterval = Gf.Interval(self.startTime, self.endTime)
        self.knots = spline.GetKnotsWithLoopsBaked(timeInterval).values()

    def _getEstimatedMinMaxFromSpline(self, spline):
        # TODO: Use knot tangents to estimate min/max more accurately
        values = []
        prevCurve = False
        for knot in self.knots:
            t = knot.GetTime()

            if self.startTime <= t:
                pv = knot.GetPreValue()
                values.append(pv)
                # Only include tangents if we would draw them
                if prevCurve:
                    values.append(
                        pv - knot.GetPreTanWidth() * knot.GetPreTanSlope())

            if t <= self.endTime:
                v = knot.GetValue()
                values.append(v)
                prevCurve = (knot.GetNextInterpolation() == Ts.InterpCurve)
                # Only include tangents if we would draw them
                if prevCurve and knot != self.knots[-1]:
                    values.append(
                        v + knot.GetPostTanWidth() * knot.GetPostTanSlope())

        if not values:
            return None
        minValue = min(values)
        maxValue = max(values)
        return None if minValue == maxValue else (minValue, maxValue)

    def _updateSamples(self):
        """
        Sample the spline, should be called when attr/spline/range changes,
        since these determine sampling of the spline. This is responsible to
        populate self.polylinesToPlot with the sampled polylines.
        """
        if not self.attr:
            return
        spline = self.attr.GetSpline()
        if not spline or spline.IsEmpty():
            return
        # Make sure to 
        width = self.width() - 2 * self.margin
        height = self.height() - 2 * self.margin
        timeInterval = Gf.Interval(self.startTime, self.endTime)
        timeScale = width / (self.endTime - self.startTime)
        self.estimatedKnotMinMax = self._getEstimatedMinMaxFromSpline(spline)

        valueScale = self.DEFAULT_SAMPLING_VALUESCALE
        if self.estimatedKnotMinMax:
            valueRange = self.estimatedKnotMinMax[1] - self.estimatedKnotMinMax[0]
            valueScale = height / valueRange
        tolerance = self.DEFAULT_SAMPLING_TOLERANCE
        # TODO: use withSources and draw polyline from each source with 
        # different color?
        samples = spline.Sample(
            timeInterval, timeScale, valueScale, tolerance, withSources=False)
        if not samples:
            return
        self.polylinesToPlot = samples.polylines
        self.splinePath = QtGui.QPainterPath()
        for polyline in self.polylinesToPlot:
            # Using polyline[1:] makes a copy of the list, so use an iterator
            # to visit each item once without copying
            polyIter = iter(polyline)
            p = next(polyIter)
            self.splinePath.moveTo(p[0], p[1])
            for p in polyIter:
                self.splinePath.lineTo(p[0], p[1])

    def _transformToDevice(self):
        """
        Compute the plotting data which uses the sampled spline data, widget
        width, height, and current frame and is used for drawing.
        """
        if not self.polylinesToPlot:
            return

        ys = [pt[1] for polyline in self.polylinesToPlot for pt in polyline]
        # X is time and Y is value
        self.minX, self.maxX = self.startTime, self.endTime
        self.minY, self.maxY = min(ys), max(ys)
        if self.estimatedKnotMinMax:
            self.minY = min(self.estimatedKnotMinMax[0], self.minY)
            self.maxY = max(self.estimatedKnotMinMax[1], self.maxY)

        self.dx = max(self.maxX - self.minX, self.MIN_DELTA)
        self.dy = max(self.maxY - self.minY, self.MIN_DELTA)

        # Estimate left margin based on Y axis labels
        yLabels = [f"{self.minY + (i / self.numTicks) * self.dy:.3f}" 
            for i in range(self.numTicks + 1)]
        maxLabelWidth = max(self.fontMetrics.boundingRect(label).width() 
            for label in yLabels)
        leftMargin = maxLabelWidth + self.MARGIN_BUFFER
        self.leftMargin = leftMargin

        # Define world coordinates including all data
        worldRect = QtCore.QRectF(self.minX, self.minY, self.dx, self.dy)

        # Define view rectangle with margins
        viewRect = QtCore.QRectF(leftMargin, self.margin,
                          self.width() - leftMargin - self.margin,
                          self.height() - self.margin * 2)

        # Construct transform from world to view
        self.xform = QtGui.QTransform()
        self.xform.translate(viewRect.left(), viewRect.bottom())
        self.xform.scale(viewRect.width() / worldRect.width(),
                         -viewRect.height() / worldRect.height())
        self.xform.translate(-worldRect.left(), -worldRect.top())

    def _drawGrid(self, painter):
        gridPen = QtGui.QPen(self.GRID_COLOR)
        gridPen.setStyle(QtCore.Qt.PenStyle.DotLine)
        painter.setPen(gridPen)

        dx = self.dx / self.numTicks
        dy = self.dy / self.numTicks
        for i in range(self.numTicks):
            x = self.minX + i * dx
            p1 = self.xform.map(QtCore.QPointF(x, self.minY))
            p2 = self.xform.map(QtCore.QPointF(x, self.maxY))
            painter.drawLine(p1, p2)
            y = self.minY + i * dy
            p1 = self.xform.map(QtCore.QPointF(self.minX, y))
            p2 = self.xform.map(QtCore.QPointF(self.maxX, y))
            painter.drawLine(p1, p2)

    def _drawAxis(self, painter):
        axisPen = QtGui.QPen(self.AXIS_COLOR, 1)
        painter.setPen(axisPen)

        width = self.width()
        height = self.height()

        # X-axis
        painter.drawLine(self.leftMargin, height - self.margin, 
                         width - self.margin, height - self.margin)

        # Y-axis
        painter.drawLine(self.leftMargin, self.margin, 
                         self.leftMargin, height - self.margin)

        # --- Draw X-axis ticks and labels ---
        for i in range(self.numTicks + 1):
            t = i / self.numTicks
            xVal = self.minX + t * self.dx
            px = self.xform.map(QtCore.QPointF(xVal, 0)).x()
            painter.drawLine(px, height - self.margin, 
                             px, height - self.margin + self.AXIS_TICK_LENGTH)
            painter.drawText(
                px - self.AXIS_TICK_X_LABEL_WIDTH_OFFSET, 
                height - self.margin + self.AXIS_TICK_X_LABEL_HEIGHT_OFFSET, 
                f"{xVal:.3f}")

        # --- Draw Y-axis ticks and labels ---
        # if difference between minY and maxY is too small, only draw one tick
        # which is the minY value/maxY value itself.
        numTicksY = self.numTicks if self.dy > 1e-5 else 0
        for i in range(numTicksY + 1):
            t = i / numTicksY if numTicksY > 0 else 0
            yVal = self.minY + t * self.dy
            py = self.xform.map(QtCore.QPointF(0, yVal)).y()
            painter.drawLine(self.leftMargin - self.AXIS_TICK_LENGTH, py, 
                             self.leftMargin, py)
            painter.drawText(
                self.AXIS_TICK_Y_LABEL_WIDTH_OFFSET,
                py + self.AXIS_TICK_Y_LABEL_HEIGHT_OFFSET,
                f"{yVal:.3f}")

    def _drawKnots(self, painter):
        if not self.knots:
            return

        knotBrush = QtGui.QBrush(self.KNOT_COLOR)
        knotPen = QtGui.QPen(knotBrush, 1)

        # Offsets from the knot or tangent points to the corner of the
        # rect. kHalf is used in dual valued knots to just draw 1/2 the knot.
        kCorner = QtCore.QPointF(self.KNOT_SIZE / 2, self.KNOT_SIZE / 2)
        kHalf = QtCore.QPointF(0.0, self.KNOT_SIZE / 2)
        tCorner = QtCore.QPointF(self.TANGENT_SIZE / 2, self.TANGENT_SIZE / 2)

        painter.setPen(knotPen)
        painter.setBrush(knotBrush)

        # Enable clipping while drawing knots.  The set of knots may include
        # knots (and tangents) that are outside the bounds of the graph, but the
        # drawn knot rect and/or the knot's tangent may extend into the viewed
        # region and we want to draw the portion that's visible.
        try:
            painter.save()

            worldRect = QtCore.QRectF(self.minX, self.minY, self.dx, self.dy)
            clipRect = self.xform.mapRect(worldRect)
            painter.setClipRect(clipRect)

            # Was the previous segment a curve?
            prevCurve = False

            for knot in self.knots:
                t = knot.GetTime()
                v = knot.GetValue()
                valuePoint = self.xform.map(QtCore.QPointF(t, v))

                dualValued = knot.IsDualValued()
                if dualValued:
                    pv = knot.GetPreValue()
                    preValuePoint = self.xform.map(QtCore.QPointF(t, pv))
                else:
                    pv = v
                    preValuePoint = valuePoint

                preTanWidth = knot.GetPreTanWidth()
                preTanSlope = knot.GetPreTanSlope()

                postTanWidth = knot.GetPostTanWidth()
                postTanSlope = knot.GetPostTanSlope()

                # preTanPoint is relative to preValuePoint
                preTanPoint = self.xform.map(
                    QtCore.QPointF(t - preTanWidth,
                                   pv - (preTanWidth * preTanSlope)))

                # postTanPoint is relative to valuePoint
                postTanPoint = self.xform.map(
                    QtCore.QPointF(t + postTanWidth,
                                   v + (postTanWidth * postTanSlope)))

                # Draw tangents. Pre-tangents only draw if the previous segment
                # was a cure.
                if prevCurve:
                    painter.drawLine(preValuePoint, preTanPoint)
                    painter.fillRect(QtCore.QRectF(preTanPoint - tCorner,
                                                   preTanPoint + tCorner),
                                     knotBrush)


                # Set prevCurve for the next loop iteration, conveniently we
                # can also use it for the current iteration to draw the
                # post-tangents.
                prevCurve = (knot.GetNextInterpolation() == Ts.InterpCurve)

                # Never draw tangents past the last knot.
                if prevCurve and knot is not self.knots[-1]:
                    painter.drawLine(valuePoint, postTanPoint)
                    painter.fillRect(QtCore.QRectF(postTanPoint - tCorner,
                                                   postTanPoint + tCorner),
                                     knotBrush)
                # draw knot

                if dualValued:
                    painter.fillRect(QtCore.QRectF(preValuePoint - kCorner,
                                                   preValuePoint + kHalf),
                                     knotBrush)
                    painter.fillRect(QtCore.QRectF(valuePoint - kHalf,
                                                   valuePoint + kCorner),
                                     knotBrush)
                else:
                    painter.fillRect(QtCore.QRectF(valuePoint - kCorner,
                                                   valuePoint + kCorner),
                                     knotBrush)

        finally:
            painter.restore()

    def _drawSpline(self, painter):
        splinePen = QtGui.QPen(self.SPLINE_COLOR, 2)
        splineBrush = QtGui.QBrush(QtCore.Qt.BrushStyle.NoBrush)

        painter.setPen(splinePen)
        painter.setBrush(splineBrush)

        # Transform and draw the entire spline path at once.
        painter.drawPath(self.xform.map(self.splinePath))

    def _drawPlayheadAndCurrentValue(self, painter):
        if not self.currentFrame:
            return
        height = self.height()
        cv = self.attr.Get(self.currentFrame)
        cf = self.currentFrame.GetValue()
        px = self.xform.map(QtCore.QPointF(cf, self.minY)).x()
        pen = QtGui.QPen(self.PLAYHEAD_COLOR, 2)
        painter.setPen(pen)
        painter.drawLine(px, 0, px, height)
        if cv:
            point = self.xform.map(QtCore.QPointF(cf, cv))
            radius = 4
            painter.drawEllipse(point, radius, radius)
            # draw a light hairline from this point to the y-axis
            dashedLinePen = QtGui.QPen(self.PLAYHEAD_COLOR, 1)
            dashedLinePen.setStyle(QtCore.Qt.PenStyle.DashLine)
            painter.setPen(dashedLinePen)
            painter.drawLine(point.x(), point.y(), self.leftMargin, point.y())
            # Do I need to reset the pen for the text?
            painter.setPen(pen)
            # Draw the text just above it
            text = f"{cv:.3f}"  # format the value
            font = painter.font()
            font.setPointSize(8)
            painter.setFont(font)
            textRect = painter.boundingRect(QtCore.QRectF(), text)
            textOffsetY = 6
            painter.drawText(QtCore.QPointF(
                point.x() - textRect.width() * 2, 
                point.y() - radius - textOffsetY), text)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        if not self.attr:
            return
        if not self.polylinesToPlot:
            return
        self._updateSamples()
        self._transformToDevice()
        self.update()

    def paintEvent(self, event):
        if not self.attr:
            return
        if not self.polylinesToPlot:
            return

        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.RenderHint.Antialiasing)

        # Draw background
        painter.fillRect(self.rect(), self.BACKGROUND_COLOR)

        # Draw grid
        self._drawGrid(painter)

        # Draw axes
        self._drawAxis(painter)

        # Draw knots
        self._drawKnots(painter)
    
        # Draw spline
        self._drawSpline(painter)

        # Draw playhead and current value
        self._drawPlayheadAndCurrentValue(painter)
