#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from pxr.Usdviewq.qt import QtCore, QtGui, QtWidgets

from ..noodlesConfig import NoodlesConfig


SHORTCUT_DEFINITIONS = [
    ("shortcutToggleLinks", "Toggle Links", "View", True),
    ("shortcutToggleNodes", "Toggle Nodes", "View", True),
    ("shortcutToggleText", "Toggle Text", "View", True),
    ("shortcutToggleLinkDimming", "Toggle Link Dimming", "View", True),
    ("shortcutSelectAll", "Select All", "Selection", True),
    ("shortcutClearSelection", "Clear Selection", "Selection", False),
    ("shortcutFrameSelection", "Frame Selection", "Navigation", True),
    ("shortcutFrameAll", "Frame All", "Navigation", True),
    ("shortcutFrameWithConnections", "Frame with Connections", "Navigation", True),
    ("shortcutNavigateBackward", "Navigate Backward", "Navigation", True),
    ("shortcutNavigateForward", "Navigate Forward", "Navigation", True),
    ("shortcutAddFromPrimTree", "Add from Prim Tree", "Editing", True),
    ("shortcutNewEditor", "Open New Editor", "Window", False),
    ("shortcutShowEditor", "Show Editor", "Window", False),
]

NON_REMAPPABLE_KEYS = [
    "Tab",
    "Space",
    "Ctrl+Z",
    "Ctrl+Shift+Z",
    "Ctrl+Y",
    "Delete",
    "Backspace",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "Shift+1",
    "Shift+2",
    "Shift+3",
    "Shift+4",
    "Shift+5",
    "Shift+6",
    "Shift+7",
    "Shift+8",
    "Shift+9",
]


def find_shortcut_conflicts(bindings, non_remappable_keys):
    """Find duplicate key sequences among shortcuts and non-remappable keys.

    Args:
        bindings: list of (configKey, displayName, keyString) for enabled shortcuts
        non_remappable_keys: list of key strings that are hardcoded

    Returns:
        dict mapping key_string -> [action_name, ...] for keys with 2+ bindings
    """
    key_to_actions = {}

    for _configKey, display_name, key_str in bindings:
        if not key_str:
            continue
        if key_str not in key_to_actions:
            key_to_actions[key_str] = []
        key_to_actions[key_str].append(display_name)

    for non_remap_key in non_remappable_keys:
        if non_remap_key in key_to_actions:
            key_to_actions[non_remap_key].append(f"(hardcoded: {non_remap_key})")

    return {k: v for k, v in key_to_actions.items() if len(v) > 1}


class KeyBindingDialog(QtWidgets.QDialog):
    def __init__(self, parent, graphView):
        super().__init__(parent)
        self._graphView = graphView
        self._editors = {}
        self._defaults = {}
        self._lastFocusedEditor = None
        self._focusConnected = False

        self._setupUI()
        self._loadCurrentBindings()

        QtWidgets.QApplication.instance().focusChanged.connect(self._onFocusChanged)
        self._focusConnected = True

    def closeEvent(self, event):
        app = QtWidgets.QApplication.instance()
        if app and self._focusConnected:
            app.focusChanged.disconnect(self._onFocusChanged)
            self._focusConnected = False
        super().closeEvent(event)

    def _onFocusChanged(self, _old, new):
        if new in self._editors.values():
            self._lastFocusedEditor = new

    def _setupUI(self):
        self.setWindowTitle("Keyboard Shortcuts")
        self.setMinimumSize(500, 550)

        layout = QtWidgets.QVBoxLayout()

        scrollArea = QtWidgets.QScrollArea()
        scrollArea.setWidgetResizable(True)
        scrollArea.setFrameShape(QtWidgets.QFrame.NoFrame)

        contentWidget = QtWidgets.QWidget()
        formLayout = QtWidgets.QFormLayout()
        formLayout.setLabelAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)

        currentCategory = None
        settings = NoodlesConfig.settings()

        if not settings:
            QtWidgets.QMessageBox.warning(
                self.parent(),
                "Settings Not Available",
                "Configuration system not initialized.",
            )
            self.close()
            return

        for configKey, displayName, category, implemented in SHORTCUT_DEFINITIONS:
            if category != currentCategory:
                currentCategory = category
                headerLabel = QtWidgets.QLabel(category)
                font = headerLabel.font()
                font.setBold(True)
                headerLabel.setFont(font)
                formLayout.addRow(headerLabel)

            info = settings.getSettingInfo(configKey)
            default = info["default"] if info else ""
            self._defaults[configKey] = default

            label = QtWidgets.QLabel(displayName)
            if not implemented:
                font = label.font()
                font.setItalic(True)
                label.setFont(font)
                label.setToolTip("Not yet implemented")

            editor = QtWidgets.QKeySequenceEdit()
            if not implemented:
                editor.setEnabled(False)

            self._editors[configKey] = editor
            formLayout.addRow(label, editor)

        contentWidget.setLayout(formLayout)
        scrollArea.setWidget(contentWidget)
        layout.addWidget(scrollArea)

        buttonLayout = QtWidgets.QHBoxLayout()

        resetSelectedBtn = QtWidgets.QPushButton("Reset Selected")
        resetSelectedBtn.clicked.connect(self._onResetSelected)
        buttonLayout.addWidget(resetSelectedBtn)

        resetAllBtn = QtWidgets.QPushButton("Reset All to Defaults")
        resetAllBtn.clicked.connect(self._onResetAll)
        buttonLayout.addWidget(resetAllBtn)

        buttonLayout.addStretch()

        cancelBtn = QtWidgets.QPushButton("Cancel")
        cancelBtn.clicked.connect(self.reject)
        buttonLayout.addWidget(cancelBtn)

        applyBtn = QtWidgets.QPushButton("Apply")
        applyBtn.clicked.connect(self._onApply)
        applyBtn.setDefault(True)
        buttonLayout.addWidget(applyBtn)

        layout.addLayout(buttonLayout)
        self.setLayout(layout)

    def _loadCurrentBindings(self):
        for configKey, editor in self._editors.items():
            if not editor.isEnabled():
                shortcutStr = self._defaults.get(configKey, "")
            else:
                shortcutStr = NoodlesConfig.get(configKey, "")
            if shortcutStr:
                editor.setKeySequence(QtGui.QKeySequence(shortcutStr))
            else:
                editor.clear()

    def _onResetSelected(self):
        editor = self._lastFocusedEditor
        if editor is None or not editor.isEnabled():
            return
        for configKey, ed in self._editors.items():
            if ed is editor:
                default = self._defaults.get(configKey, "")
                if default:
                    editor.setKeySequence(QtGui.QKeySequence(default))
                else:
                    editor.clear()
                return

    def _onResetAll(self):
        reply = QtWidgets.QMessageBox.question(
            self,
            "Reset All Shortcuts",
            "Reset all shortcuts to their default values?",
            QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
            QtWidgets.QMessageBox.No,
        )
        if reply == QtWidgets.QMessageBox.Yes:
            for configKey, editor in self._editors.items():
                if not editor.isEnabled():
                    continue
                default = self._defaults.get(configKey, "")
                if default:
                    editor.setKeySequence(QtGui.QKeySequence(default))
                else:
                    editor.clear()

    def _onApply(self):
        conflicts = self._findConflicts()
        if conflicts:
            msg = "The following shortcut conflicts were found:\n\n"
            for key, actions in conflicts.items():
                msg += f"  {key}: {', '.join(actions)}\n"
            msg += "\nApply anyway?"

            reply = QtWidgets.QMessageBox.warning(
                self,
                "Shortcut Conflicts",
                msg,
                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
                QtWidgets.QMessageBox.No,
            )
            if reply != QtWidgets.QMessageBox.Yes:
                return

        settings = NoodlesConfig.settings()
        if settings:
            settings.blockSignals(True)

        try:
            for configKey, editor in self._editors.items():
                if not editor.isEnabled():
                    continue
                seq = editor.keySequence()
                NoodlesConfig.set(
                    configKey, seq.toString(QtGui.QKeySequence.PortableText)
                )

            NoodlesConfig.save()
        finally:
            if settings:
                settings.blockSignals(False)

        if self._graphView and hasattr(self._graphView, "_onSettingsChanged"):
            self._graphView._onSettingsChanged()

        self.accept()

    def _findConflicts(self):
        bindings = []
        for configKey, editor in self._editors.items():
            if not editor.isEnabled():
                continue
            seq = editor.keySequence()
            if seq.isEmpty():
                continue
            key_str = seq.toString(QtGui.QKeySequence.PortableText)
            if not key_str:
                continue

            display_name = configKey
            for ck, dn, _, _ in SHORTCUT_DEFINITIONS:
                if ck == configKey:
                    display_name = dn
                    break

            bindings.append((configKey, display_name, key_str))

        return find_shortcut_conflicts(bindings, NON_REMAPPABLE_KEYS)
