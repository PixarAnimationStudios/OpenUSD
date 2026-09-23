#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#

"""
Library Toggle Menu Widget.

Provides a Qt menu with checkboxes to enable/disable node libraries in the
node creation hotbox.
"""

from pxr.Usdviewq.qt import QtCore, QtGui, QtWidgets

# Handle PySide6 vs PySide2 differences
try:
    QAction = QtGui.QAction  # PySide6
except AttributeError:
    QAction = QtWidgets.QAction  # PySide2


class LibraryToggleMenu(QtWidgets.QMenu):
    """
    A menu widget for toggling node libraries on and off.

    Displays a list of available node libraries with checkboxes. When a library
    is toggled, it emits a signal and updates the library's enabled state.

    Signals:
        libraryToggled: Emitted when any library is toggled (passes the library object)
    """

    # Custom signal for library state changes
    libraryToggled = QtCore.Signal(object)  # Passes the NodeLibrary object

    def __init__(self, parent=None):
        """
        Initialize the library toggle menu.

        Args:
            parent: Parent Qt widget
        """
        super().__init__(parent)
        self.setTitle("Node Libraries")

        self._libraries = []
        self._actions = {}  # Map from library to QAction

        # Style the menu
        self.setStyleSheet("""
            QMenu {
                background-color: #1e1e1e;
                color: #cccccc;
                border: 1px solid #3c3c3c;
                padding: 4px;
            }
            QMenu::item {
                padding: 6px 24px 6px 8px;
                border-radius: 2px;
            }
            QMenu::item:selected {
                background-color: #2d2d30;
            }
            QMenu::indicator {
                width: 16px;
                height: 16px;
                margin-left: 4px;
            }
            QMenu::indicator:checked {
                image: none;
                background-color: #007acc;
                border: 1px solid #007acc;
                border-radius: 2px;
            }
            QMenu::indicator:unchecked {
                image: none;
                background-color: #3c3c3c;
                border: 1px solid #5a5a5a;
                border-radius: 2px;
            }
        """)

    def set_libraries(self, libraries):
        """
        Configure the menu with a list of node libraries.

        Args:
            libraries: List of NodeLibrary instances
        """
        # Clear existing actions
        self.clear()
        self._libraries = libraries
        self._actions = {}

        # Create a checkbox action for each library
        for library in libraries:
            action = QAction(library.get_name(), self)
            action.setCheckable(True)
            action.setChecked(library.enabled)

            # Connect the action to toggle the library
            action.triggered.connect(
                lambda checked, lib=library: self._on_library_toggled(lib, checked)
            )

            self.addAction(action)
            self._actions[library] = action

        # Add separator and info text if there are libraries
        if libraries:
            self.addSeparator()
            info_action = QAction("Toggle libraries to show/hide node types", self)
            info_action.setEnabled(False)
            self.addAction(info_action)

    def _on_library_toggled(self, library, checked):
        """
        Handle library toggle action.

        Args:
            library: The NodeLibrary instance that was toggled
            checked: New checked state (True/False)
        """
        # Update the library's enabled state
        library.enabled = checked

        # Emit signal to notify listeners
        self.libraryToggled.emit(library)

    def update_library_states(self):
        """
        Update the checkbox states to match the current library enabled states.

        Call this if library states are changed externally.
        """
        for library, action in self._actions.items():
            action.setChecked(library.enabled)


class LibraryToggleButton(QtWidgets.QPushButton):
    """
    A button that opens the LibraryToggleMenu when clicked.

    This is a convenience widget that combines a button with the menu for
    easy integration into toolbars or other UIs.
    """

    def __init__(self, parent=None):
        """
        Initialize the library toggle button.

        Args:
            parent: Parent Qt widget
        """
        super().__init__("Node Libraries", parent)

        # Create the menu
        self._menu = LibraryToggleMenu(self)

        # Set button properties
        self.setToolTip("Toggle which node libraries are available in the hotbox")
        self.setFocusPolicy(QtCore.Qt.NoFocus)

        # Connect button click to show menu
        self.clicked.connect(self._show_menu)

        # Style the button
        self.setStyleSheet("""
            QPushButton {
                background-color: #2d2d30;
                color: #cccccc;
                border: 1px solid #3c3c3c;
                padding: 4px 12px;
                border-radius: 2px;
            }
            QPushButton:hover {
                background-color: #3e3e42;
                border: 1px solid #007acc;
            }
            QPushButton:pressed {
                background-color: #007acc;
            }
        """)

    def _show_menu(self):
        """Show the library toggle menu below the button."""
        # Position the menu below the button
        pos = self.mapToGlobal(QtCore.QPoint(0, self.height()))
        self._menu.exec_(pos)

    def set_libraries(self, libraries):
        """
        Configure the menu with a list of node libraries.

        Args:
            libraries: List of NodeLibrary instances
        """
        self._menu.set_libraries(libraries)

    def library_toggled_signal(self):
        """
        Get the libraryToggled signal from the menu.

        Returns:
            QtCore.Signal: The signal emitted when a library is toggled
        """
        return self._menu.libraryToggled

    def update_library_states(self):
        """Update the menu's checkbox states to match current library states."""
        self._menu.update_library_states()
