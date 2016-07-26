#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
"""
glfq

"""

import _glfq
from pxr import Tf
Tf.PrepareModule(_glfq, locals())
del _glfq, Tf

def CreateGLDebugContext(glFormat):
    from PySide import QtOpenGL
    class GLDebugContext(QtOpenGL.QGLContext):
        def __init__(self, glFormat):
            QtOpenGL.QGLContext.__init__(self, glFormat)
            self._platformContext = None

        def create(self, shareContext):
            self._platformContext = None
            return QtOpenGL.QGLContext.create(self, shareContext)

        def makeCurrent(self):
            QtOpenGL.QGLContext.makeCurrent(self)
            if not self._platformContext:
                self._platformContext = GLPlatformDebugContext(
                    self.format().majorVersion(),
                    self.format().minorVersion(),
                    self.format().profile() == QtOpenGL.QGLFormat.CoreProfile,
                    self.format().directRendering())
            self._platformContext.makeCurrent()
    return GLDebugContext(glFormat)

try:
    from . import __DOC
    __DOC.Execute(locals())
    del __DOC
except Exception:
    pass

