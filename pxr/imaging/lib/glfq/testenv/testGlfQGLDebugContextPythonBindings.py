#!/pxrpythonsubst
from PySide import QtGui, QtCore, QtOpenGL
from OpenGL import GL
from pxr import Glf
from pxr import Glfq
from pxr import Tf
import argparse
import sys

def TestDebugOutput():
    sys.stderr.write("Expected Error Begin\n")
    try:
        GL.glEnable(GL.GL_TRUE) # raise error
    except:
        pass
    try:
        GL.glLineWidth(-1.0) # raise error
    except:
        pass
    sys.stderr.write("Expected Error End\n")

    # clear errors we just raised
    while GL.glGetError() != GL.GL_NONE: pass

def _GetGLFormat():
    fmt = QtOpenGL.QGLFormat();
    fmt.setDoubleBuffer(True);
    fmt.setDepth(True);
    fmt.setAlpha(True);
    fmt.setStencil(True);
    return fmt;

class TestQGLWidget(QtOpenGL.QGLWidget):
    def __init__(self, glFormat, parent=None):
        super(TestQGLWidget, self).__init__(
            Glfq.CreateGLDebugContext(glFormat), parent)

    def initializeGL(self):
        Glf.GlewInit()
        Glf.RegisterDefaultDebugOutputMessageCallback()

        print GL.glGetString(GL.GL_VENDOR)
        print GL.glGetString(GL.GL_RENDERER)
        print GL.glGetString(GL.GL_VERSION)

    def paintGL(self):
        GL.glClearColor(1.0, 0.1, 0.1, 1.0)
        GL.glClear(GL.GL_COLOR_BUFFER_BIT)

        TestDebugOutput()

    def DrawOffscreen(self):
        self.glInit();
        self.glDraw();

parser = argparse.ArgumentParser()
parser.add_argument('--offscreen', action='store_true',
                    help = 'display offscreen, e.g. for use with tinderbox')
args = parser.parse_args()

app = QtGui.QApplication(sys.argv)
widget = TestQGLWidget(_GetGLFormat())

errorMark = Tf.Error.Mark()

if args.offscreen:
    widget.hide();
    widget.makeCurrent();
    widget.DrawOffscreen();
    widget.doneCurrent();
else:
    widget.show()
    app.exec_()

if errorMark.IsClean():
    print 'FAILED'
    sys.exit(1)

print 'OK'
sys.exit(0)
