#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from pxr.UsdNoodles.core import LinkData
from pxr import Gf, Tf

from .models import NodeModel
from .nodeGraph import createNodeRenderer
from .nodeGraphBlueprint import NodeGraphBlueprint
from .pinUtils import (
    collect_links_for_prim,
    infer_bare_link_direction,
    split_direction_hint,
)


def _append_relationship_links(node, prim, excluded_node_id):
    for link_data in collect_links_for_prim(prim):
        if not bool(link_data.get("is_relationship_link", False)):
            continue
        if excluded_node_id in (
            link_data["sourceNodeId"],
            link_data["targetNodeId"],
        ):
            continue
        link = LinkData()
        link.sourceNodeId = link_data["sourceNodeId"]
        link.sourcePort = link_data["sourcePinName"]
        link.targetNodeId = link_data["targetNodeId"]
        link.targetPort = link_data["targetPinName"]
        link.is_input_link = bool(link_data["is_input_link"])
        link.propertyOwnerNodeId = str(link_data.get("propertyOwnerNodeId", ""))
        link.propertyName = str(link_data.get("propertyName", ""))
        link.is_relationship_link = True
        link.sourcePropertyName = str(link_data.get("sourcePropertyName", ""))
        link.targetPropertyName = str(link_data.get("targetPropertyName", ""))
        if link.is_input_link:
            node.inputLinks.append(link)
        else:
            node.outputLinks.append(link)


class NodeGraphBpContainer(NodeGraphBlueprint):
    """
    NodeGraphBpContainer is a subclass of NodeGraphBlueprint that loads
    and displays the internal node graph of a Container prim.

    Containers are USD prims that represent subnetworks/subnets of nodes
    within a Blueprint. They can be opened and edited just like Blueprints.
    """

    def load(
        self,
        stage,
        primPath,
        calculateTextWidth,
        fontMetrics,
        useFieldnames=True,
        nodeFactory=None,
    ):
        # Clear existing data
        self.clear()

        # Store stage reference for USD-aware nodes
        self._stage = stage

        # Enable selection syncing for Container graphs (they have valid USD prim paths)
        self.syncSelectionToPrimTree = True

        # Get the Container prim
        containerPrim = stage.GetPrimAtPath(primPath)
        if not containerPrim or not containerPrim.IsValid():
            Tf.Warn(f"Could not find Container prim at {primPath}")
            return

        if containerPrim.GetTypeName() != "Container":
            Tf.Warn(
                f"Prim at {primPath} is not a Container (type: {containerPrim.GetTypeName()})"
            )
            return

        containerPathStr = str(primPath)

        # Track bounds of nodes to position virtual nodes
        minX = float("inf")
        maxX = float("-inf")
        minY = float("inf")
        maxY = float("-inf")

        # First pass: Collect Container inputs and outputs for virtual nodes
        containerInputs = []
        containerOutputs = []

        for attr in containerPrim.GetAttributes():
            attrName = attr.GetName()
            side, pinName = split_direction_hint(attrName)
            if side == "input":
                containerInputs.append(pinName)
            elif side == "output":
                containerOutputs.append(pinName)
            elif ":" not in attrName:
                # Bare attributes appear as both input and output
                if attrName not in containerInputs:
                    containerInputs.append(attrName)
                if attrName not in containerOutputs:
                    containerOutputs.append(attrName)

        # Collect deferred connections to Container I/O
        deferredContainerConnections = []
        fieldnameInputs = set()  # Unique fieldnames for Container inputs
        fieldnameOutputs = set()  # Unique fieldnames for Container outputs

        # Second pass: Process ExecNode, Container, and Shader children
        for child in containerPrim.GetChildren():
            childType = child.GetTypeName()

            # Process ExecNodes, nested Containers, and Shaders
            if childType not in ["ExecNode", "Container", "Shader"]:
                continue

            # Create USD-aware node that reads from the prim directly
            childPath = child.GetPath()

            # Use NodeFactory if available, otherwise use legacy method
            if nodeFactory:
                node = nodeFactory.create_node_from_prim(child, stage)
                if not node:
                    # No library can handle this prim - skip it
                    Tf.Warn(
                        f"No library can handle prim type '{childType}' at {childPath}"
                    )
                    continue
            else:
                # Legacy: Create USD-aware node directly
                node = NodeModel(
                    stage=stage, primPath=childPath, renderer=createNodeRenderer()
                )

            # Node will read name, type, position, and pins from USD on demand
            # We just need to track the bounds for virtual node positioning
            nodePos = node.position

            # Force lazy-load of name, type, and pins so that the underlying
            # C++ NodeData fields are populated before calculateNodeSize and
            # generateVertices read them directly from the struct.
            _ = node.name
            _ = node.type
            _ = node.schemaTypeName
            _ = node.inputPins
            _ = node.outputPins
            _ = node.inputPinTypes
            _ = node.outputPinTypes

            # Track bounds
            minX = min(minX, nodePos[0])
            maxX = max(maxX, nodePos[0])
            minY = min(minY, nodePos[1])
            maxY = max(maxY, nodePos[1])

            # Process connections on USD attributes
            for attr in child.GetAttributes():
                attrName = attr.GetName()

                # Extract pin name and determine if input or output
                isBare = False
                side, pinName = split_direction_hint(attrName)
                if side == "input":
                    isInput = True
                elif side == "output":
                    isInput = False
                elif ":" not in attrName:
                    isBare = True
                    isInput = True  # placeholder
                else:
                    continue

                # Check for connections on this attribute
                connections = attr.GetConnections()
                if not connections:
                    continue

                # Get fieldname from assetInfo metadata if useFieldnames mode is enabled
                fieldname = None
                if useFieldnames:
                    assetInfo = attr.GetCustomDataByKey("assetInfo")
                    if assetInfo and isinstance(assetInfo, dict):
                        fieldname = assetInfo.get("fieldname")
                    else:
                        metadata = attr.GetAllMetadata()
                        if metadata and "assetInfo" in metadata:
                            assetInfo = metadata["assetInfo"]
                            if isinstance(assetInfo, dict):
                                fieldname = assetInfo.get("fieldname")

                for connPath in connections:
                    connPathStr = str(connPath)

                    if "." in connPathStr:
                        targetPrimPath, targetPropName = connPathStr.rsplit(".", 1)
                    else:
                        continue

                    side, targetPinName = split_direction_hint(targetPropName)
                    if side == "input":
                        targetIsInput = True
                    elif side == "output":
                        targetIsInput = False
                    else:
                        targetIsInput = True  # default for bare/namespaced targets

                    if isBare:
                        isInput = infer_bare_link_direction(targetPropName)

                    # Check if this connection is to the Container itself (virtual node)
                    if targetPrimPath == containerPathStr:
                        deferredContainerConnections.append(
                            {
                                "sourceNodeId": node.id,
                                "sourcePinName": pinName,
                                "sourceIsInput": isInput,
                                "targetPinName": targetPinName,
                                "targetIsInput": targetIsInput,
                                "fieldname": fieldname,
                            }
                        )

                        if useFieldnames and fieldname:
                            if targetIsInput:
                                fieldnameInputs.add((targetPinName, fieldname))
                            else:
                                fieldnameOutputs.add((targetPinName, fieldname))
                    else:
                        link = LinkData()
                        link.sourceNodeId = node.id
                        link.sourcePort = pinName
                        link.targetNodeId = targetPrimPath
                        link.targetPort = targetPinName

                        if isInput:
                            node.inputLinks.append(link)
                        else:
                            node.outputLinks.append(link)

            _append_relationship_links(node, child, containerPathStr)

            # Calculate node size
            self._calculateNodeSize(node, calculateTextWidth, fontMetrics)

            # Update maxX with node width
            maxX = max(maxX, node.position[0] + node.size[0])

            self.nodes[node.id] = node

        # Load Backdrop prims as group stickers
        for child in containerPrim.GetChildren():
            if child.GetTypeName() == "Backdrop":
                from .widgets.groupSticker import GroupSticker

                sticker = GroupSticker.from_prim(child)
                self.stickers.append(sticker)

        # Create virtual nodes for Container inputs (positioned to the left)
        inputNodeSpacing = 150.0
        inputStartY = minY if minY != float("inf") else 0.0
        inputX = (minX - 400.0) if minX != float("inf") else -400.0

        if useFieldnames and fieldnameInputs:
            sortedFieldnameInputs = sorted(fieldnameInputs, key=lambda x: (x[0], x[1]))
            for i, (containerPinName, fieldname) in enumerate(sortedFieldnameInputs):
                virtualNodeId = (
                    f"{containerPathStr}#input:{containerPinName}:{fieldname}"
                )

                node = NodeModel(renderer=createNodeRenderer())
                node.id = virtualNodeId
                node.name = fieldname
                node.type = "ContainerInput"
                node.position = Gf.Vec2d(inputX, inputStartY + i * inputNodeSpacing)
                node.outputPins = [fieldname]

                self._calculateNodeSize(node, calculateTextWidth, fontMetrics)
                self.nodes[virtualNodeId] = node
        else:
            for i, inputPinName in enumerate(containerInputs):
                virtualNodeId = f"{containerPathStr}#input:{inputPinName}"

                node = NodeModel(renderer=createNodeRenderer())
                node.id = virtualNodeId
                node.name = inputPinName
                node.type = "ContainerInput"
                node.position = Gf.Vec2d(inputX, inputStartY + i * inputNodeSpacing)
                node.outputPins = [inputPinName]

                self._calculateNodeSize(node, calculateTextWidth, fontMetrics)
                self.nodes[virtualNodeId] = node

        # Create virtual nodes for Container outputs (positioned to the right)
        outputStartY = minY if minY != float("inf") else 0.0
        outputX = (maxX + 200.0) if maxX != float("-inf") else 400.0

        if useFieldnames and fieldnameOutputs:
            sortedFieldnameOutputs = sorted(
                fieldnameOutputs, key=lambda x: (x[0], x[1])
            )
            for i, (containerPinName, fieldname) in enumerate(sortedFieldnameOutputs):
                virtualNodeId = (
                    f"{containerPathStr}#output:{containerPinName}:{fieldname}"
                )

                node = NodeModel(renderer=createNodeRenderer())
                node.id = virtualNodeId
                node.name = fieldname
                node.type = "ContainerOutput"
                node.position = Gf.Vec2d(outputX, outputStartY + i * inputNodeSpacing)
                node.inputPins = [fieldname]

                self._calculateNodeSize(node, calculateTextWidth, fontMetrics)
                self.nodes[virtualNodeId] = node
        else:
            for i, outputPinName in enumerate(containerOutputs):
                virtualNodeId = f"{containerPathStr}#output:{outputPinName}"

                node = NodeModel(renderer=createNodeRenderer())
                node.id = virtualNodeId
                node.name = outputPinName
                node.type = "ContainerOutput"
                node.position = Gf.Vec2d(outputX, outputStartY + i * inputNodeSpacing)
                node.inputPins = [outputPinName]

                self._calculateNodeSize(node, calculateTextWidth, fontMetrics)
                self.nodes[virtualNodeId] = node

        # Process deferred Container I/O connections
        for conn in deferredContainerConnections:
            sourceNodeId = conn["sourceNodeId"]
            sourcePinName = conn["sourcePinName"]
            sourceIsInput = conn["sourceIsInput"]
            targetPinName = conn["targetPinName"]
            targetIsInput = conn["targetIsInput"]
            fieldname = conn["fieldname"]

            if sourceNodeId not in self.nodes:
                continue

            sourceNode = self.nodes[sourceNodeId]

            if useFieldnames and fieldname:
                if targetIsInput:
                    virtualNodeId = (
                        f"{containerPathStr}#input:{targetPinName}:{fieldname}"
                    )
                    linkTargetPinName = fieldname
                else:
                    virtualNodeId = (
                        f"{containerPathStr}#output:{targetPinName}:{fieldname}"
                    )
                    linkTargetPinName = fieldname
            else:
                if targetIsInput:
                    virtualNodeId = f"{containerPathStr}#input:{targetPinName}"
                    linkTargetPinName = targetPinName
                else:
                    virtualNodeId = f"{containerPathStr}#output:{targetPinName}"
                    linkTargetPinName = targetPinName

            if virtualNodeId not in self.nodes:
                continue

            link = LinkData()
            link.sourceNodeId = sourceNodeId
            link.sourcePort = sourcePinName
            link.targetNodeId = virtualNodeId
            link.targetPort = linkTargetPinName

            if sourceIsInput:
                sourceNode.inputLinks.append(link)
            else:
                sourceNode.outputLinks.append(link)

        self.linksChanged = True

        inputCount = (
            len(fieldnameInputs)
            if useFieldnames and fieldnameInputs
            else len(containerInputs)
        )
        outputCount = (
            len(fieldnameOutputs)
            if useFieldnames and fieldnameOutputs
            else len(containerOutputs)
        )
        modeStr = "fieldnames" if useFieldnames else "pins"

        Tf.Status(
            f"Loaded {len(self.nodes)} nodes from Container {primPath} "
            f"({inputCount} inputs, {outputCount} outputs, mode={modeStr})"
        )
