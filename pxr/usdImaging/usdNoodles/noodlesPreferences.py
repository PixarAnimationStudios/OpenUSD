#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""
NoodlesPreferences - Preferences dialog for usdNoodles configuration.

Provides a comprehensive UI for editing all configuration settings defined
in NoodlesSettingsDataModel. Settings are organized by category using tabs,
with dynamic widget generation based on setting types.

Features:
- Tab-based organization by category (nodes, canvas, links, minimap, etc.)
- Dynamic widget generation from SETTINGS_SCHEMA
- Color picker for RGBA values
- OK/Cancel/Apply/RestoreDefaults buttons
- Live graph updates when settings are applied
- Persistent storage via StateSource
"""

from pxr.Usdviewq.qt import QtCore, QtGui, QtWidgets

from .noodlesConfig import NoodlesConfig
from .widgets.keyBindingDialog import KeyBindingDialog


class NoodlesPreferences(QtWidgets.QDialog):
    """
    Preferences dialog for usdNoodles configuration.

    Organizes settings by category using tabs, with dynamic widget
    generation from SETTINGS_SCHEMA. Supports OK/Cancel/Apply/Reset.
    """

    def __init__(self, parent, graphView):
        """
        Initialize the preferences dialog.

        Args:
            parent: Parent widget (typically the GraphView)
            graphView: GraphView instance for live updates
        """
        super().__init__(parent)
        self._graphView = graphView
        self._settings = NoodlesConfig.settings()

        if not self._settings:
            # Settings not initialized - shouldn't happen but handle gracefully
            QtWidgets.QMessageBox.warning(
                parent,
                "Settings Not Available",
                "Configuration system not initialized. Please restart the editor.",
            )
            self.close()
            return

        # Track widgets for batch operations
        self._widgets = {}  # {settingName: widget}

        # Prevent update cycles
        self._muteUpdates = False

        self._setupUI()
        self._loadSettings()

    def _setupUI(self):
        """Build dialog UI programmatically."""
        self.setWindowTitle("Noodles Preferences")
        self.setMinimumSize(600, 500)

        layout = QtWidgets.QVBoxLayout()

        # Create tab widget for categories
        self._tabWidget = QtWidgets.QTabWidget()

        # Generate tabs from SETTINGS_SCHEMA
        categories = self._settings.getCategories()
        for category in categories:
            # Skip empty categories
            settings = self._settings.getSettingsByCategory(category)
            if not settings:
                continue

            tab = self._createCategoryTab(category)
            # Capitalize and prettify category names
            tabName = category.replace("_", " ").title()
            self._tabWidget.addTab(tab, tabName)

        layout.addWidget(self._tabWidget)

        # Add button box
        buttonBox = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.Ok
            | QtWidgets.QDialogButtonBox.Cancel
            | QtWidgets.QDialogButtonBox.Apply
            | QtWidgets.QDialogButtonBox.RestoreDefaults
        )

        buttonBox.accepted.connect(self._onAccept)
        buttonBox.rejected.connect(self._onReject)

        applyBtn = buttonBox.button(QtWidgets.QDialogButtonBox.Apply)
        applyBtn.clicked.connect(self._onApply)

        resetBtn = buttonBox.button(QtWidgets.QDialogButtonBox.RestoreDefaults)
        resetBtn.clicked.connect(self._onRestoreDefaults)

        layout.addWidget(buttonBox)
        self.setLayout(layout)

    def _createCategoryTab(self, category):
        """Create a tab for a category with form layout."""
        if category == "shortcuts":
            return self._createShortcutsRedirectTab()

        widget = QtWidgets.QWidget()
        formLayout = QtWidgets.QFormLayout()
        formLayout.setLabelAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
        formLayout.setFormAlignment(QtCore.Qt.AlignLeft | QtCore.Qt.AlignTop)

        # Get settings for this category
        settings = self._settings.getSettingsByCategory(category)

        for name, currentValue, description in settings:
            # Get setting info for type and default
            info = self._settings.getSettingInfo(name)
            if not info:
                continue

            # Create appropriate widget based on type
            settingWidget = self._createWidgetForSetting(
                name, info["type"], info["default"], currentValue
            )

            if settingWidget:
                # Store widget reference (unless already set by color widget)
                if name not in self._widgets:
                    self._widgets[name] = settingWidget

                # Add to form with tooltip
                label = QtWidgets.QLabel(self._prettifyName(name))
                label.setToolTip(description)
                formLayout.addRow(label, settingWidget)

        # Add stretch to push widgets to top
        formLayout.setRowWrapPolicy(QtWidgets.QFormLayout.DontWrapRows)
        widget.setLayout(formLayout)

        # Wrap in scroll area
        scrollArea = QtWidgets.QScrollArea()
        scrollArea.setWidgetResizable(True)
        scrollArea.setWidget(widget)
        scrollArea.setFrameShape(QtWidgets.QFrame.NoFrame)

        return scrollArea

    def _createWidgetForSetting(self, name, settingType, default, current):
        """Create appropriate widget based on setting type."""

        # Boolean → QCheckBox
        if settingType == "bool":
            widget = QtWidgets.QCheckBox()
            widget.setChecked(current)
            widget.stateChanged.connect(lambda state, n=name: self._onSettingChanged(n))
            return widget

        # Integer → QSpinBox
        elif settingType == "int":
            widget = QtWidgets.QSpinBox()
            widget.setMinimum(-(2**31))
            widget.setMaximum(2**31 - 1)
            widget.setValue(current)
            widget.valueChanged.connect(lambda value, n=name: self._onSettingChanged(n))
            return widget

        # Float → QDoubleSpinBox
        elif settingType == "float":
            widget = QtWidgets.QDoubleSpinBox()
            widget.setDecimals(3)
            widget.setMinimum(-1e9)
            widget.setMaximum(1e9)
            widget.setSingleStep(0.1)
            widget.setValue(current)
            widget.valueChanged.connect(lambda value, n=name: self._onSettingChanged(n))
            return widget

        # List (RGBA color) → Custom color widget with button
        elif settingType == "list" and len(default) == 4:
            # Check if this looks like a color (all values 0-255 or 0-1)
            if all(isinstance(v, (int, float)) for v in default):
                return self._createColorWidget(name, current)

        # List (other) → Custom list widget
        if settingType == "list":
            widget = QtWidgets.QLineEdit()
            widget.setText(str(current))
            widget.editingFinished.connect(lambda n=name: self._onSettingChanged(n))
            return widget

        # String → QLineEdit
        elif settingType == "str":
            widget = QtWidgets.QLineEdit()
            widget.setText(current)
            widget.editingFinished.connect(lambda n=name: self._onSettingChanged(n))
            return widget

        return None

    def _createColorWidget(self, name, current):
        """Create color picker widget for RGBA colors."""
        container = QtWidgets.QWidget()
        layout = QtWidgets.QHBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)

        # Color preview button
        colorBtn = QtWidgets.QPushButton()
        colorBtn.setFixedSize(50, 25)
        self._updateColorButton(colorBtn, current)
        colorBtn.clicked.connect(lambda checked, n=name: self._onColorPicker(n))

        # RGBA value display
        valueLabel = QtWidgets.QLabel(self._formatColor(current))
        valueLabel.setMinimumWidth(200)

        layout.addWidget(colorBtn)
        layout.addWidget(valueLabel)
        layout.addStretch()

        container.setLayout(layout)

        # Store both button and label as tuple
        self._widgets[name] = (colorBtn, valueLabel, current)

        return container

    def _updateColorButton(self, button, rgba):
        """Update color button background."""
        r, g, b, a = rgba
        # Convert to 0-255 range if needed
        if all(v <= 1.0 for v in [r, g, b, a]):
            r, g, b, a = int(r * 255), int(g * 255), int(b * 255), int(a * 255)

        button.setStyleSheet(
            f"background-color: rgba({r}, {g}, {b}, {a}); border: 1px solid #555;"
        )

    def _formatColor(self, rgba):
        """Format RGBA values for display."""
        return f"[{rgba[0]:.3f}, {rgba[1]:.3f}, {rgba[2]:.3f}, {rgba[3]:.3f}]"

    def _onColorPicker(self, name):
        """Show color picker dialog."""
        current = self._widgets[name][2]

        # Convert to QColor
        r, g, b, a = current
        if all(v <= 1.0 for v in [r, g, b, a]):
            color = QtGui.QColor.fromRgbF(r, g, b, a)
        else:
            color = QtGui.QColor(int(r), int(g), int(b), int(a))

        # Show color dialog with alpha channel
        options = QtWidgets.QColorDialog.ShowAlphaChannel
        newColor = QtWidgets.QColorDialog.getColor(color, self, "Select Color", options)

        if newColor.isValid():
            # Convert back to list format (normalized 0-1 or 0-255 based on original)
            if all(v <= 1.0 for v in current):
                # Original was normalized, keep it normalized
                rgba = [
                    newColor.redF(),
                    newColor.greenF(),
                    newColor.blueF(),
                    newColor.alphaF(),
                ]
            else:
                # Original was 0-255, keep it 0-255
                rgba = [
                    newColor.red(),
                    newColor.green(),
                    newColor.blue(),
                    newColor.alpha(),
                ]

            # Update widget
            button, label, _ = self._widgets[name]
            self._updateColorButton(button, rgba)
            label.setText(self._formatColor(rgba))
            self._widgets[name] = (button, label, rgba)

            self._onSettingChanged(name)

    def _prettifyName(self, name):
        """Convert camelCase to Title Case."""
        import re

        # Insert space before capitals
        pretty = re.sub(r"([a-z])([A-Z])", r"\1 \2", name)
        return pretty.title()

    def _loadSettings(self):
        """Load current settings into widgets."""
        self._muteUpdates = True

        for name, widget in self._widgets.items():
            # Skip color widgets (already initialized)
            if isinstance(widget, tuple):
                continue

            value = getattr(self._settings, name)

            if isinstance(widget, QtWidgets.QCheckBox):
                widget.setChecked(value)
            elif isinstance(widget, QtWidgets.QSpinBox):
                widget.setValue(value)
            elif isinstance(widget, QtWidgets.QDoubleSpinBox):
                widget.setValue(value)
            elif isinstance(widget, QtWidgets.QLineEdit):
                widget.setText(str(value))

        self._muteUpdates = False

    def _onSettingChanged(self, name):
        """Called when a setting widget changes."""
        if self._muteUpdates:
            return

        # Mark as modified (for potential unsaved changes warning)
        # Could add visual indicator here

    def _applySettings(self):
        """Apply all settings from widgets to data model."""
        self._muteUpdates = True

        for name, widget in self._widgets.items():
            # Handle color widgets
            if isinstance(widget, tuple):
                button, label, rgba = widget
                setattr(self._settings, name, rgba)
                continue

            # Handle standard widgets
            if isinstance(widget, QtWidgets.QCheckBox):
                setattr(self._settings, name, widget.isChecked())
            elif isinstance(widget, QtWidgets.QSpinBox):
                setattr(self._settings, name, widget.value())
            elif isinstance(widget, QtWidgets.QDoubleSpinBox):
                setattr(self._settings, name, widget.value())
            elif isinstance(widget, QtWidgets.QLineEdit):
                # Try to parse as list if it looks like one
                text = widget.text()
                try:
                    if text.startswith("["):
                        import ast

                        value = ast.literal_eval(text)
                    else:
                        value = text
                    setattr(self._settings, name, value)
                except Exception:
                    pass  # Keep old value on parse error

        # Save to disk
        NoodlesConfig.save()

        # Update shortcuts in GraphView
        if self._graphView and hasattr(self._graphView, "updateShortcuts"):
            self._graphView.updateShortcuts()

        # Update profiler state from settings
        if self._graphView and hasattr(self._graphView, "_profiler"):
            self._graphView._profiler.enabled = NoodlesConfig.get(
                "enableRenderProfiling", False
            )
            if self._graphView._profiler.enabled:
                self._graphView._profiler.reset()  # Reset stats when enabled

        # Force GraphView to refresh (rebuild text, links, etc.)
        if self._graphView:
            self._graphView.textChanged = True
            self._graphView.linksChanged = True
            self._graphView.update()

        self._muteUpdates = False

    def _onApply(self):
        """Apply button clicked - save but keep dialog open."""
        self._applySettings()

    def _onAccept(self):
        """OK button clicked - save and close."""
        self._applySettings()
        self.accept()

    def _onReject(self):
        """Cancel button clicked - close without saving."""
        self.reject()

    def _onRestoreDefaults(self):
        """Restore Defaults button clicked."""
        reply = QtWidgets.QMessageBox.question(
            self,
            "Restore Defaults",
            "Reset all settings to default values?",
            QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
            QtWidgets.QMessageBox.No,
        )

        if reply == QtWidgets.QMessageBox.Yes:
            # Reset all widgets to defaults
            for name, widget in self._widgets.items():
                info = self._settings.getSettingInfo(name)
                if not info:
                    continue

                default = info["default"]

                # Handle color widgets
                if isinstance(widget, tuple):
                    button, label, _ = widget
                    self._updateColorButton(button, default)
                    label.setText(self._formatColor(default))
                    self._widgets[name] = (button, label, default)
                    continue

                # Handle standard widgets
                if isinstance(widget, QtWidgets.QCheckBox):
                    widget.setChecked(default)
                elif isinstance(widget, QtWidgets.QSpinBox):
                    widget.setValue(default)
                elif isinstance(widget, QtWidgets.QDoubleSpinBox):
                    widget.setValue(default)
                elif isinstance(widget, QtWidgets.QLineEdit):
                    widget.setText(str(default))

    def _createShortcutsRedirectTab(self):
        widget = QtWidgets.QWidget()
        layout = QtWidgets.QVBoxLayout()
        layout.addStretch()

        label = QtWidgets.QLabel(
            "Keyboard shortcuts are managed in a dedicated dialog."
        )
        label.setAlignment(QtCore.Qt.AlignCenter)
        layout.addWidget(label)

        btn = QtWidgets.QPushButton("Open Keyboard Shortcuts...")
        btn.clicked.connect(self._openKeyBindingDialog)
        btn.setMaximumWidth(250)
        layout.addWidget(btn, alignment=QtCore.Qt.AlignCenter)

        layout.addStretch()
        widget.setLayout(layout)
        return widget

    def _openKeyBindingDialog(self):
        dialog = KeyBindingDialog(self, self._graphView)
        dialog.exec_()
