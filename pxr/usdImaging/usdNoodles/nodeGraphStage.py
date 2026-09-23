#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from pxr.UsdNoodles.core import LinkData
from pxr import Tf

from .models import NodeModel
from .nodeGraph import createNodeRenderer, NodeGraph
from .pinUtils import collect_links_for_prim


def _append_collected_links(node, prim):
    for link_data in collect_links_for_prim(prim):
        link = LinkData()
        link.sourceNodeId = link_data["sourceNodeId"]
        link.sourcePort = link_data["sourcePinName"]
        link.targetNodeId = link_data["targetNodeId"]
        link.targetPort = link_data["targetPinName"]
        link.is_input_link = bool(link_data["is_input_link"])
        link.propertyOwnerNodeId = str(link_data.get("propertyOwnerNodeId", ""))
        link.propertyName = str(link_data.get("propertyName", ""))
        link.is_relationship_link = bool(link_data.get("is_relationship_link", False))
        link.sourcePropertyName = str(link_data.get("sourcePropertyName", ""))
        link.targetPropertyName = str(link_data.get("targetPropertyName", ""))
        if link.is_input_link:
            node.inputLinks.append(link)
        else:
            node.outputLinks.append(link)


class NodeGraphStage(NodeGraph):
    """Loads all supported node prims from the root of a USD stage.

    Unlike NodeGraphBlueprint, this does not require a Blueprint-typed
    parent prim.  It simply traverses the stage's root children and
    creates nodes for any ExecNode, Container, or Shader prims it finds.
    """

    def load(
        self,
        stage,
        calculateTextWidth,
        fontMetrics,
        nodeFactory=None,
    ):
        self.clear()
        self._stage = stage
        self.syncSelectionToPrimTree = True

        pseudoRoot = stage.GetPseudoRoot()
        validTypes = {"ExecNode", "Container", "Shader"}

        for child in pseudoRoot.GetChildren():
            if not child.IsActive():
                continue

            childType = child.GetTypeName()
            childPath = child.GetPath()

            if nodeFactory:
                # Let the node factory (which queries all registered
                # libraries) decide whether this prim is supported.
                # Fall through to the hardcoded type check only when
                # no factory is available.
                node = nodeFactory.create_node_from_prim(child, stage)
                if not node:
                    # Not handled by any library — skip silently
                    continue
            elif childType in validTypes:
                node = NodeModel(
                    stage=stage, primPath=childPath, renderer=createNodeRenderer()
                )
            else:
                continue

            _ = node.name
            _ = node.type
            _ = node.schemaTypeName
            _ = node.position
            _ = node.inputPins
            _ = node.outputPins
            _ = node.inputPinTypes
            _ = node.outputPinTypes

            _append_collected_links(node, child)

            self._calculateNodeSize(node, calculateTextWidth, fontMetrics)
            self.nodes[node.id] = node

        # Load Backdrop prims as group stickers
        for child in pseudoRoot.GetChildren():
            if child.GetTypeName() == "Backdrop":
                from .widgets.groupSticker import GroupSticker

                sticker = GroupSticker.from_prim(child)
                self.stickers.append(sticker)

        self.linksChanged = True

        Tf.Status(
            f"Loaded {len(self.nodes)} nodes and "
            f"{len(self.stickers)} backdrops from stage root"
        )
