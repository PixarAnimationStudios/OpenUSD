#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
import os

from pxr import Tf
from pxr.Usdviewq.plugin import PluginContainer

class OpenExecContainer(PluginContainer):
    """Usdview plugin container for OpenExec plugin."""

    def __init__(self):
        self._openExecPluginEnabled = (
            os.getenv('USDVIEW_ENABLE_OPENEXEC_DEMO_PLUGIN') == '1')
        
        if self._openExecPluginEnabled and \
            os.getenv('USDIMAGINGGL_ENGINE_ENABLE_EXEC_SCENE_INDEX') != '1':
            Tf.Warn(
                'Environment variable '
                'USDIMAGINGGL_ENGINE_ENABLE_EXEC_SCENE_INDEX not set to 1. '
                'OpenExec related imaging features may not function properly.')

    def registerPlugins(self, plugRegistry, plugCtx):
        if self._openExecPluginEnabled:
            deferredImport_plugin = self.deferredImport(".plugin")
            self._openExecInvertibleRigDemo = \
                plugRegistry.registerCommandPlugin(
                    "OpenExecContainer.openExecInvertibleRigDemo",
                    "Invertible Rig Demo", 
                    deferredImport_plugin.invertibleRigDemo,
                    description="Open controls for posing an invertible rig.")

    def configureView(self, plugRegistry, plugUIBuilder):
        if self._openExecPluginEnabled:
            openExecMenu = plugUIBuilder.findOrCreateMenu("OpenExec")
            openExecMenu.addItem(self._openExecInvertibleRigDemo)

# Define OpenExecContainer as a Tf.Type so libplug can pick it up.
Tf.Type.Define(OpenExecContainer)
