from __future__ import print_function

from pxr import Tf
from pxr.Usdviewq.plugin import PluginContainer


class ExtraContainerDuplicateCommand(PluginContainer):

    def registerPlugins(self, plugRegistry, plugCtx):

        self._extra1 = plugRegistry.registerCommandPlugin(
            "ExtraContainerDuplicateCommand.extraCommand1",
            "Extra Command 1", lambda plugCtx: print("Extra Command 1 Invoked"))
        self._extra2 = plugRegistry.registerCommandPlugin(
            "ExtraContainerDuplicateCommand.extraCommand1",
            "Extra Command 1 again!", lambda plugCtx: print("Extra Command 1 Invoked"))

    def configureView(self, plugRegistry, plugUIBuilder):

        extraMenu = plugUIBuilder.findOrCreateMenu("Extra")
        extraMenu.addItem(self._extra1)
        extraMenu.addItem(self._extra2)

Tf.Type.Define(ExtraContainerDuplicateCommand)
