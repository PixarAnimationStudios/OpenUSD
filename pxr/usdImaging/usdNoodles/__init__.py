#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from pxr import Tf
Tf.PreparePythonModule()
del Tf

# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

from pathlib import Path

from pxr import Plug, Tf


TEST_GRAPH_FILES = [
    "body_graph.json",
    "body_graph2.json",
    "my_nodes.json",
    "test_single_node.json",
]


def _getAssetsPath():
    plug = Plug.Registry().GetPluginWithName("pxr.UsdNoodles")

    if plug:
        resourcePath = Path(plug.resourcePath)
        assetsInResources = resourcePath / "assets"
        if assetsInResources.exists():
            return assetsInResources
        else:
            return resourcePath
    else:
        resourcePath = Path(__file__).parent
        assetsPath = resourcePath / "assets"
        if assetsPath.exists():
            return assetsPath
        return resourcePath


try:
    from ._qt_editor import (  # noqa: F401
        GetEditorManager,
        GetLayerEditorManager,
        NoodlesEditorManager,
        NoodlesEditorWidget,
        NoodlesEditorWindow,
        NoodlesLayerEditorManager,
        NoodlesLayerEditorWidget,
        NoodlesLayerEditorWindow,
        NoodlesPluginContainer,
        ShowNoodlesEditor,
        ShowNoodlesEditorForBlueprint,
        ShowNoodlesEditorForContainer,
        ShowNoodlesLayerEditor,
    )

    _HAS_QT = True
except ImportError:
    _HAS_QT = False

if _HAS_QT:
    # Tf.Type.Define uses __module__ to build the type path that must match
    # plugInfo.json ("pxr.UsdNoodles.NoodlesPluginContainer"). Since the class
    # lives in _qt_editor, we patch __module__ so the plugin registry finds it.
    NoodlesPluginContainer.__module__ = __name__
    Tf.Type.Define(NoodlesPluginContainer)
