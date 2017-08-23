#
# USD builds against PySide before PySide2, so try to import PySide first.
# However, assume PySide2 is the way forward and adjust PySide to match PySide2
#
try:
  from PySide import QtGui, QtCore, QtOpenGL
  QtWidgets = QtGui
  assert getattr(QtWidgets.QHeaderView, 'setSectionResizeMode', None) == None
  assert getattr(QtWidgets.QApplication, 'devicePixelRatio', None) == None
  QtWidgets.QHeaderView.setSectionResizeMode = QtGui.QHeaderView.setResizeMode
  QtWidgets.QApplication.devicePixelRatio = lambda self: 1

except ImportError:
  from PySide2 import QtWidgets, QtGui, QtOpenGL, QtCore

