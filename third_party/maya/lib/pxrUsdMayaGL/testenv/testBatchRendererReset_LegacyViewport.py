#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

from pxr import UsdMaya

from pxr import Tf

from maya import cmds

import os
import sys
import unittest


class testBatchRendererReset_LegacyViewport(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # The test USD data is authored Z-up, so make sure Maya is configured
        # that way too.
        cmds.upAxis(axis='z')

        cmds.loadPlugin('pxrUsd')

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSaveSceneAs(self):
        """
        Tests performing a "Save Scene As..." after having drawn a USD proxy
        shape.

        Previously, the batch renderer listened for kSceneUpdate Maya scene
        messages so that it could reset itself when the scene was changed. This
        was intended to fire for new empty scenes or when opening a different
        scene file. It turns out Maya also emits that message during
        "Save Scene As..." operations, in which case we do *not* want to reset
        the batch renderer, as doing so would lead to a crash, since all of the
        proxy shapes are still in the DAG.

        This test ensures that the batch renderer does not reset and Maya does
        not crash when doing a "Save Scene As..." operation.
        """
        mayaSceneFile = 'AssemblySaveAsTest_LegacyViewport.ma'
        mayaSceneFullPath = os.path.abspath(mayaSceneFile)
        Tf.Status('Opening Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        animStartTime = cmds.playbackOptions(query=True,
            animationStartTime=True)

        UsdMaya.LoadReferenceAssemblies()

        # Force a draw to complete by switching frames.
        cmds.currentTime(animStartTime + 1.0, edit=True)

        saveAsMayaSceneFile = 'SavedScene.ma'
        saveAsMayaSceneFullPath = os.path.abspath(saveAsMayaSceneFile)
        Tf.Status('Saving Maya Scene File As: %s' % saveAsMayaSceneFullPath)
        cmds.file(rename=saveAsMayaSceneFullPath)
        cmds.file(save=True)

        # Try to select the USD assembly/proxy. This will cause the proxy's
        # shape adapter's Sync() method to be called, which would fail if the
        # batch renderer had been reset out from under the shape adapter.
        cmds.select('CubeAssembly')

        # Force a draw to complete by switching frames.
        cmds.currentTime(animStartTime + 1.0, edit=True)

    def testSceneOpenAndReopen(self):
        """
        Tests opening and then re-opening scenes after having drawn a USD proxy
        shape.

        The batch renderer needs to be reset when opening a new scene, but
        kBeforeOpen and kAfterOpen scene messages are not always delivered at
        the right time. With the former, the current scene may still be active
        when the message is received in which case another draw may occur
        before the scene is finally closed. With the latter, the scene may have
        been fully read and an initial draw may have happened by the time the
        message is received. Either case results in the batch renderer being
        reset in the middle of an active scene and a possible crash.

        This test depends on the proxy shape node directly and not the assembly
        node. The proxy shape will be drawn immediately when the scene is
        opened, as opposed to an assembly that must first be loaded to create
        the proxy shape node underneath it.
        """
        mayaSceneFile = 'ProxyShapeBatchRendererResetTest_LegacyViewport.ma'
        mayaSceneFullPath = os.path.abspath(mayaSceneFile)
        Tf.Status('Opening Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        # Force a draw to complete by switching frames.
        animStartTime = cmds.playbackOptions(query=True,
            animationStartTime=True)
        cmds.currentTime(animStartTime + 1.0, edit=True)

        # Re-open the same scene.
        Tf.Status('Re-opening Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        # Force a draw to complete by switching frames.
        animStartTime = cmds.playbackOptions(query=True,
            animationStartTime=True)
        cmds.currentTime(animStartTime + 1.0, edit=True)

        # Try to select the proxy shape.
        cmds.select('CubeProxy')

        # Force a draw to complete by switching frames.
        cmds.currentTime(animStartTime + 1.0, edit=True)

    def testSceneImportAndReference(self):
        """
        Tests that importing or referencing a Maya scene does not reset the
        batch renderer.

        The batch renderer listens for kBeforeFileRead scene messages, but
        those are also emitted when a scene is imported or referenced into the
        current scene. In that case, we want to make sure that the batch
        renderer does not get reset.
        """
        mayaSceneFile = 'ProxyShapeBatchRendererResetTest_LegacyViewport.ma'
        mayaSceneFullPath = os.path.abspath(mayaSceneFile)
        Tf.Status('Opening Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, open=True, force=True)

        # Force a draw to complete by switching frames.
        animStartTime = cmds.playbackOptions(query=True,
            animationStartTime=True)
        cmds.currentTime(animStartTime + 1.0, edit=True)

        # Import another scene file into the current scene.
        mayaSceneFile = 'EmptyScene.ma'
        mayaSceneFullPath = os.path.abspath(mayaSceneFile)
        Tf.Status('Importing Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, i=True)

        # Force a draw to complete by switching frames.
        cmds.currentTime(animStartTime + 1.0, edit=True)

        # Try to select the proxy shape.
        cmds.select('CubeProxy')

        # Force a draw to complete by switching frames.
        cmds.currentTime(animStartTime + 1.0, edit=True)

        # Reference another scene file into the current scene.
        Tf.Status('Referencing Maya Scene File: %s' % mayaSceneFullPath)
        cmds.file(mayaSceneFullPath, reference=True)

        # Force a draw to complete by switching frames.
        cmds.currentTime(animStartTime + 1.0, edit=True)

        # Try to select the proxy shape.
        cmds.select('CubeProxy')


if __name__ == '__main__':
    suite = unittest.TestLoader().loadTestsFromTestCase(testBatchRendererReset_LegacyViewport)

    results = unittest.TextTestRunner(stream=sys.stdout).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1
    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
