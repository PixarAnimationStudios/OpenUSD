try:
  from PySide import QtGui, QtCore, QtOpenGL
  QtWidgets = QtGui
except ImportError:
  from PySide2 import QtWidgets, QtGui, QtOpenGL, QtCore
