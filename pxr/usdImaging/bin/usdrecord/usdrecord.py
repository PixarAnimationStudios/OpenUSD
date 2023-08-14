#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

from pxr import Usd
from pxr import UsdAppUtils
from pxr import Tf

import argparse
import os
import sys


def _Msg(msg):
    sys.stdout.write(msg + '\n')

def _Err(msg):
    sys.stderr.write(msg + '\n')

def _SetupOpenGLContext(width=100, height=100):
    try:
        from PySide6.QtOpenGLWidgets import QOpenGLWidget
        from PySide6.QtOpenGL import QOpenGLFramebufferObject
        from PySide6.QtOpenGL import QOpenGLFramebufferObjectFormat
        from PySide6.QtCore import QSize
        from PySide6.QtGui import QOffscreenSurface
        from PySide6.QtGui import QOpenGLContext
        from PySide6.QtGui import QSurfaceFormat
        from PySide6.QtWidgets import QApplication
        PySideModule = 'PySide6'
    except ImportError:
        from PySide2 import QtOpenGL
        from PySide2.QtWidgets import QApplication
        PySideModule = 'PySide2'

    application = QApplication(sys.argv)

    if PySideModule == 'PySide6':
        glFormat = QSurfaceFormat()
        glFormat.setSamples(4)

        # Create an off-screen surface and bind a gl context to it.
        glWidget = QOffscreenSurface()
        glWidget.setFormat(glFormat)
        glWidget.create()

        glWidget._offscreenContext = QOpenGLContext()
        glWidget._offscreenContext.setFormat(glFormat)
        glWidget._offscreenContext.create()

        glWidget._offscreenContext.makeCurrent(glWidget)

        # Create and bind a framebuffer for the frameRecorder's present task.
        # Since the frameRecorder uses AOVs directly, this is just
        # a 1x1 default format FBO.
        glFBOFormat = QOpenGLFramebufferObjectFormat()
        glWidget._fbo = QOpenGLFramebufferObject(QSize(1, 1), glFBOFormat)
        glWidget._fbo.bind()

    else:
        glFormat = QtOpenGL.QGLFormat()
        glFormat.setSampleBuffers(True)
        glFormat.setSamples(4)
        glWidget = QtOpenGL.QGLWidget(glFormat)

        glWidget.setFixedSize(width, height)

        # note that we need to bind the gl context here, instead of explicitly
        # showing the glWidget. Binding the gl context will make sure
        # framebuffer is ready for gl operations.
        glWidget.makeCurrent()

    return glWidget

def main():
    programName = os.path.basename(sys.argv[0])
    parser = argparse.ArgumentParser(prog=programName,
        description='Generates images from a USD file')

    # Positional (required) arguments.
    parser.add_argument('usdFilePath', action='store', type=str,
        help='USD file to record')
    parser.add_argument('outputImagePath', action='store', type=str,
        help=(
            'Output image path. For frame ranges, the path must contain '
            'exactly one frame number placeholder of the form "###" or '
            '"###.###". Note that the number of hash marks is variable in '
            'each group.'))

    # Optional arguments.
    parser.add_argument('--mask', action='store', type=str,
        dest='populationMask', metavar='PRIMPATH[,PRIMPATH...]',
        help=(
            'Limit stage population to these prims, their descendants and '
            'ancestors. To specify multiple paths, either use commas with no '
            'spaces or quote the argument and separate paths by commas and/or '
            'spaces.'))

    parser.add_argument('--purposes', action='store', type=str,
        dest='purposes', metavar='PURPOSE[,PURPOSE...]', default='proxy',
        help=(
            'Specify which UsdGeomImageable purposes should be included '
            'in the renders.  The "default" purpose is automatically included, '
            'so you need specify only the *additional* purposes.  If you want '
            'more than one extra purpose, either use commas with no spaces or '
            'quote the argument and separate purposes by commas and/or spaces.'))

    # Note: The argument passed via the command line (disableGpu) is inverted
    # from the variable in which it is stored (gpuEnabled).
    parser.add_argument('--disableGpu', action='store_false',
        dest='gpuEnabled',
        help=(
            'Indicates if the GPU should not be used for rendering. If set '
            'this not only restricts renderers to those which only run on '
            'the CPU, but additionally it will prevent any tasks that require '
            'the GPU from being invoked.'))

    UsdAppUtils.cameraArgs.AddCmdlineArgs(parser)
    UsdAppUtils.framesArgs.AddCmdlineArgs(parser)
    UsdAppUtils.complexityArgs.AddCmdlineArgs(parser)
    UsdAppUtils.colorArgs.AddCmdlineArgs(parser)
    UsdAppUtils.rendererArgs.AddCmdlineArgs(parser)

    parser.add_argument('--imageWidth', '-w', action='store', type=int,
        default=960,
        help=(
            'Width of the output image. The height will be computed from this '
            'value and the camera\'s aspect ratio (default=%(default)s)'))

    args = parser.parse_args()

    UsdAppUtils.framesArgs.ValidateCmdlineArgs(parser, args,
        frameFormatArgName='outputImagePath')

    args.imageWidth = max(args.imageWidth, 1)

    purposes = args.purposes.replace(',', ' ').split()

    # Open the USD stage, using a population mask if paths were given.
    if args.populationMask:
        populationMaskPaths = args.populationMask.replace(',', ' ').split()

        populationMask = Usd.StagePopulationMask()
        for maskPath in populationMaskPaths:
            populationMask.Add(maskPath)

        usdStage = Usd.Stage.OpenMasked(args.usdFilePath, populationMask)
    else:
        usdStage = Usd.Stage.Open(args.usdFilePath)

    if not usdStage:
        _Err('Could not open USD stage: %s' % args.usdFilePath)
        return 1

    # Get the camera at the given path (or with the given name).
    usdCamera = UsdAppUtils.GetCameraAtPath(usdStage, args.camera)

    if args.gpuEnabled:
        # UsdAppUtils.FrameRecorder will expect that an OpenGL context has
        # been created and made current if the GPU is enabled.
        #
        # Frame-independent initialization.
        # Note that the size of the widget doesn't actually affect the size of
        # the output image. We just pass it along for cleanliness.
        glWidget = _SetupOpenGLContext(args.imageWidth, args.imageWidth)

    rendererPluginId = UsdAppUtils.rendererArgs.GetPluginIdFromArgument(
        args.rendererPlugin) or ''
    frameRecorder = UsdAppUtils.FrameRecorder(rendererPluginId, args.gpuEnabled)
    frameRecorder.SetImageWidth(args.imageWidth)
    frameRecorder.SetComplexity(args.complexity.value)
    frameRecorder.SetColorCorrectionMode(args.colorCorrectionMode)
    frameRecorder.SetIncludedPurposes(purposes)

    _Msg('Camera: %s' % usdCamera.GetPath().pathString)
    _Msg('Renderer plugin: %s' % frameRecorder.GetCurrentRendererId())

    for timeCode in args.frames:
        _Msg('Recording time code: %s' % timeCode)
        outputImagePath = args.outputImagePath.format(frame=timeCode.GetValue())
        try:
            frameRecorder.Record(usdStage, usdCamera, timeCode, outputImagePath)
        except Tf.ErrorException as e:

            _Err("Recording aborted due to the following failure at time code "
                 "{0}: {1}".format(timeCode, str(e)))
            break

    # Release our reference to the frame recorder so it can be deleted before
    # the Qt stuff.
    frameRecorder = None


if __name__ == '__main__':
    sys.exit(main())
