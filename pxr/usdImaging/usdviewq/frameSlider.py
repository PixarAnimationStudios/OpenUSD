#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from .qt import QtCore, QtWidgets, QtGui


class FrameSlider(QtWidgets.QSlider):
    """Custom QSlider class to allow scrubbing on left-click."""

    PAUSE_TIMER_INTERVAL = 500

    def __init__(self, parent=None):
        super(FrameSlider, self).__init__(parent=parent)
        # Create a mouse pause timer to trigger an update if the slider
        # scrubbing pauses.
        self._mousePauseTimer = QtCore.QTimer(self)
        self._mousePauseTimer.setInterval(self.PAUSE_TIMER_INTERVAL)
        self._mousePauseTimer.timeout.connect(self.mousePaused)

    def mousePaused(self):
        """Slot called when the slider scrubbing is paused."""
        self._mousePauseTimer.stop()
        self.valueChanged.emit(self.sliderPosition())

    def mousePressEvent(self, event):
        # This is a temporary solution that should revisited in future.
        #
        # The issue is that the current QStyle on the application
        # only allows the slider position to be set on a MiddleButton
        # MouseEvent.
        #
        # The correct solution is for us to create a QProxyStyle for the
        # application so we can edit the value of the
        # QStyle.SH_Slider_AbsoluteSetButtons property (to include
        # LeftButton). Unfortunately QProxyStyle is not yet available
        # in this version of PySide.
        #
        # Instead, we are forced to duplicate the MouseEvent as a
        # MiddleButton event. This creates the exact behavior we
        # want to see from the QSlider.
        styleHint = QtWidgets.QStyle.SH_Slider_AbsoluteSetButtons
        if self.style().styleHint(styleHint) == QtCore.Qt.MiddleButton:
            if event.button() == QtCore.Qt.LeftButton:
                event = QtGui.QMouseEvent(
                    event.type(),
                    event.pos(),
                    QtCore.Qt.MiddleButton,
                    QtCore.Qt.MiddleButton,
                    event.modifiers()
                )

        super(FrameSlider, self).mousePressEvent(event)

    def mouseMoveEvent(self, event):
        super(FrameSlider, self).mouseMoveEvent(event)

        # Start the pause timer if tracking is disabled.
        if not self.hasTracking():
            self._mousePauseTimer.start()

    def mouseReleaseEvent(self, event):
        # Stop the pause timer.
        self._mousePauseTimer.stop()

        super(FrameSlider, self).mouseReleaseEvent(event)
