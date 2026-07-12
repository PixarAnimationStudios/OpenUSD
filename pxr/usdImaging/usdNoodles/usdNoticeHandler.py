#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""
USD Notice handler for the usdNoodles graph editor.
"""

from pxr import Tf, Usd


class UsdNoticeHandler:
    """
    Handles USD change notifications for the graph editor.

    The handler categorizes changes into:
    - Resynced paths: Structural changes (prims added/removed, connections changed)
    - Info-only changes: Property value changes (positions, metadata)
    """

    def __init__(self, graphView):
        self._graphView = graphView
        self._listener = None
        self._stage = None
        # Flag to temporarily disable processing (e.g., during our own edits)
        self._enabled = True

    def register(self, stage):
        self.unregister()

        if not stage:
            return

        self._stage = stage
        self._listener = Tf.Notice.Register(
            Usd.Notice.ObjectsChanged, self._onObjectsChanged, stage
        )

    def unregister(self):
        if self._listener:
            self._listener.Revoke()
            self._listener = None
        self._stage = None

    def setEnabled(self, enabled):
        """
        Use this to temporarily disable processing during batch edits
        from the graph editor itself to avoid recursive updates.
        """
        self._enabled = enabled

    def isEnabled(self):
        return self._enabled

    def _onObjectsChanged(self, notice, sender):
        if not self._enabled:
            return

        if not self._graphView:
            return

        # Collect resynced paths (structural changes)
        # These require full invalidation of affected nodes/links
        resyncedPaths = set()
        for path in notice.GetResyncedPaths():
            pathStr = str(path)
            # Skip absolute root - we only care about specific prims
            if pathStr == "/":
                continue
            resyncedPaths.add(pathStr)

        # Collect info-only changes (property value changes)
        # These allow targeted cache invalidation
        infoChangedPaths = set()
        for path in notice.GetChangedInfoOnlyPaths():
            pathStr = str(path)
            if pathStr == "/":
                continue
            infoChangedPaths.add(pathStr)

        if resyncedPaths or infoChangedPaths:
            self._graphView._handleUsdChanges(resyncedPaths, infoChangedPaths)

    def getStage(self):
        return self._stage
