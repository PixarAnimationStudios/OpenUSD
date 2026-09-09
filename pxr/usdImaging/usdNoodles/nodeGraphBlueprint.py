#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from pxr.UsdNoodles.core import LinkData
from pxr import Gf, Tf

from .models import NodeModel
from .nodeGraph import createNodeRenderer, NodeGraph
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


class NodeGraphBlueprint(NodeGraph):
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

        # Enable selection syncing for Blueprint graphs (they have valid USD prim paths)
        self.syncSelectionToPrimTree = True

        # Get the Blueprint prim
        blueprintPrim = stage.GetPrimAtPath(primPath)
        if not blueprintPrim or not blueprintPrim.IsValid():
            Tf.Warn(f"Could not find Blueprint prim at {primPath}")
            return

        if blueprintPrim.GetTypeName() != "Blueprint":
            Tf.Warn(
                f"Prim at {primPath} is not a Blueprint (type: {blueprintPrim.GetTypeName()})"
            )
            return

        blueprintPathStr = str(primPath)

        # Track bounds of ExecNodes to position virtual nodes
        minX = float("inf")
        maxX = float("-inf")
        minY = float("inf")
        maxY = float("-inf")

        # First pass: Collect Blueprint inputs and outputs for virtual nodes
        blueprintInputs = []
        blueprintOutputs = []

        for attr in blueprintPrim.GetAttributes():
            attrName = attr.GetName()
            side, pinName = split_direction_hint(attrName)
            if side == "input":
                blueprintInputs.append(pinName)
            elif side == "output":
                blueprintOutputs.append(pinName)
            elif ":" not in attrName:
                # Bare attributes appear as both input and output
                if attrName not in blueprintInputs:
                    blueprintInputs.append(attrName)
                if attrName not in blueprintOutputs:
                    blueprintOutputs.append(attrName)

        # Collect deferred connections to Blueprint I/O (will be processed after we know bounds)
        # Also collect unique fieldnames when useFieldnames is True
        deferredBlueprintConnections = []
        fieldnameInputs = set()  # Unique fieldnames for Blueprint inputs
        fieldnameOutputs = set()  # Unique fieldnames for Blueprint outputs

        # Second pass: Process ExecNode, Container, and Shader children
        for child in blueprintPrim.GetChildren():
            childType = child.GetTypeName()

            # Skip prims that are clearly not nodes (e.g. relationship targets,
            # property specs).  Accept any typed or typeless prim so that the
            # node factory can attempt to create a node from it.
            if childType in ("", None) and not child.GetAttributes():
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
            # Note: Pins are read from USD automatically via node properties
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
                    # Try customData first, then fall back to metadata
                    assetInfo = attr.GetCustomDataByKey("assetInfo")
                    if assetInfo and isinstance(assetInfo, dict):
                        fieldname = assetInfo.get("fieldname")
                    else:
                        # Try getting assetInfo from metadata
                        metadata = attr.GetAllMetadata()
                        if metadata and "assetInfo" in metadata:
                            assetInfo = metadata["assetInfo"]
                            if isinstance(assetInfo, dict):
                                fieldname = assetInfo.get("fieldname")

                for connPath in connections:
                    # Parse the target path to get node and pin
                    connPathStr = str(connPath)

                    # Split into prim path and property name
                    if "." in connPathStr:
                        targetPrimPath, targetPropName = connPathStr.rsplit(".", 1)
                    else:
                        continue

                    # Extract target pin name
                    side, targetPinName = split_direction_hint(targetPropName)
                    if side == "input":
                        targetIsInput = True
                    elif side == "output":
                        targetIsInput = False
                    else:
                        targetIsInput = True  # default for bare/namespaced targets

                    if isBare:
                        isInput = infer_bare_link_direction(targetPropName)

                    # Check if this connection is to the Blueprint itself (virtual node)
                    if targetPrimPath == blueprintPathStr:
                        # Defer Blueprint I/O connections until we know the bounds
                        deferredBlueprintConnections.append(
                            {
                                "sourceNodeId": node.id,
                                "sourcePinName": pinName,
                                "sourceIsInput": isInput,
                                "targetPinName": targetPinName,
                                "targetIsInput": targetIsInput,
                                "fieldname": fieldname,
                            }
                        )

                        # Collect unique fieldnames for virtual node generation
                        if useFieldnames and fieldname:
                            if targetIsInput:
                                fieldnameInputs.add((targetPinName, fieldname))
                            else:
                                fieldnameOutputs.add((targetPinName, fieldname))
                    else:
                        # Regular connection to another ExecNode
                        link = LinkData()

                        # Handle both authoring conventions:
                        # - If connection is on INPUT: input connects TO output (swap needed)
                        # - If connection is on OUTPUT: output connects TO input (no swap)
                        if isInput:
                            # Connection on INPUT side: input connects TO output
                            # Current node is target, connection points to source
                            link.sourceNodeId = targetPrimPath
                            link.sourcePort = targetPinName
                            link.targetNodeId = node.id
                            link.targetPort = pinName
                        else:
                            # Connection on OUTPUT side: output connects TO input
                            # Current node is source, connection points to target
                            link.sourceNodeId = node.id
                            link.sourcePort = pinName
                            link.targetNodeId = targetPrimPath
                            link.targetPort = targetPinName

                        if isInput:
                            node.inputLinks.append(link)
                        else:
                            node.outputLinks.append(link)

            _append_relationship_links(node, child, blueprintPathStr)

            # Calculate node size
            self._calculateNodeSize(node, calculateTextWidth, fontMetrics)

            # Update maxX with node width
            maxX = max(maxX, node.position[0] + node.size[0])

            self.nodes[node.id] = node

        # Load Backdrop prims as group stickers
        for child in blueprintPrim.GetChildren():
            if child.GetTypeName() == "Backdrop":
                from .widgets.groupSticker import GroupSticker

                sticker = GroupSticker.from_prim(child)
                self.stickers.append(sticker)

        # Create virtual nodes for Blueprint inputs (positioned to the left)
        inputNodeSpacing = 150.0
        inputStartY = minY if minY != float("inf") else 0.0
        inputX = (minX - 400.0) if minX != float("inf") else -400.0

        if useFieldnames and fieldnameInputs:
            # Create virtual nodes for each unique fieldname
            sortedFieldnameInputs = sorted(fieldnameInputs, key=lambda x: (x[0], x[1]))
            for i, (bpPinName, fieldname) in enumerate(sortedFieldnameInputs):
                virtualNodeId = f"{blueprintPathStr}#input:{bpPinName}:{fieldname}"

                node = NodeModel()
                node.id = virtualNodeId
                node.name = fieldname
                node.type = "BlueprintInput"
                node.position = Gf.Vec2d(inputX, inputStartY + i * inputNodeSpacing)

                # Blueprint input nodes have one output pin (the input value flows out to ExecNodes)
                node.outputPins = [fieldname]

                # Calculate node size
                self._calculateNodeSize(node, calculateTextWidth, fontMetrics)

                self.nodes[virtualNodeId] = node
        else:
            # Original mode: single virtual node per Blueprint input
            for i, inputPinName in enumerate(blueprintInputs):
                virtualNodeId = f"{blueprintPathStr}#input:{inputPinName}"

                node = NodeModel()
                node.id = virtualNodeId
                node.name = inputPinName
                node.type = "BlueprintInput"
                node.position = Gf.Vec2d(inputX, inputStartY + i * inputNodeSpacing)

                # Blueprint input nodes have one output pin (the input value flows out to ExecNodes)
                node.outputPins = [inputPinName]

                # Calculate node size
                self._calculateNodeSize(node, calculateTextWidth, fontMetrics)

                self.nodes[virtualNodeId] = node

        # Create virtual nodes for Blueprint outputs (positioned to the right)
        outputStartY = minY if minY != float("inf") else 0.0
        outputX = (maxX + 200.0) if maxX != float("-inf") else 400.0

        if useFieldnames and fieldnameOutputs:
            # Create virtual nodes for each unique fieldname
            sortedFieldnameOutputs = sorted(
                fieldnameOutputs, key=lambda x: (x[0], x[1])
            )
            for i, (bpPinName, fieldname) in enumerate(sortedFieldnameOutputs):
                virtualNodeId = f"{blueprintPathStr}#output:{bpPinName}:{fieldname}"

                node = NodeModel()
                node.id = virtualNodeId
                node.name = fieldname
                node.type = "BlueprintOutput"
                node.position = Gf.Vec2d(outputX, outputStartY + i * inputNodeSpacing)

                # Blueprint output nodes have one input pin (values flow in from ExecNodes)
                node.inputPins = [fieldname]

                # Calculate node size
                self._calculateNodeSize(node, calculateTextWidth, fontMetrics)

                self.nodes[virtualNodeId] = node
        else:
            # Original mode: single virtual node per Blueprint output
            for i, outputPinName in enumerate(blueprintOutputs):
                virtualNodeId = f"{blueprintPathStr}#output:{outputPinName}"

                node = NodeModel()
                node.id = virtualNodeId
                node.name = outputPinName
                node.type = "BlueprintOutput"
                node.position = Gf.Vec2d(outputX, outputStartY + i * inputNodeSpacing)

                # Blueprint output nodes have one input pin (values flow in from ExecNodes)
                node.inputPins = [outputPinName]

                # Calculate node size
                self._calculateNodeSize(node, calculateTextWidth, fontMetrics)

                self.nodes[virtualNodeId] = node

        # Now process deferred Blueprint I/O connections
        for conn in deferredBlueprintConnections:
            sourceNodeId = conn["sourceNodeId"]
            sourcePinName = conn["sourcePinName"]
            sourceIsInput = conn["sourceIsInput"]
            targetPinName = conn["targetPinName"]
            targetIsInput = conn["targetIsInput"]
            fieldname = conn["fieldname"]

            if sourceNodeId not in self.nodes:
                continue

            sourceNode = self.nodes[sourceNodeId]

            # Determine the virtual node ID based on target type and mode
            if useFieldnames and fieldname:
                # Fieldname mode: use fieldname-specific virtual node
                if targetIsInput:
                    virtualNodeId = (
                        f"{blueprintPathStr}#input:{targetPinName}:{fieldname}"
                    )
                    linkTargetPinName = fieldname
                else:
                    virtualNodeId = (
                        f"{blueprintPathStr}#output:{targetPinName}:{fieldname}"
                    )
                    linkTargetPinName = fieldname
            else:
                # Original mode: use Blueprint pin-level virtual node
                if targetIsInput:
                    virtualNodeId = f"{blueprintPathStr}#input:{targetPinName}"
                    linkTargetPinName = targetPinName
                else:
                    virtualNodeId = f"{blueprintPathStr}#output:{targetPinName}"
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
            else len(blueprintInputs)
        )
        outputCount = (
            len(fieldnameOutputs)
            if useFieldnames and fieldnameOutputs
            else len(blueprintOutputs)
        )
        modeStr = "fieldnames" if useFieldnames else "pins"

        Tf.Status(
            f"Loaded {len(self.nodes)} nodes from Blueprint {primPath} "
            f"({inputCount} inputs, {outputCount} outputs, mode={modeStr})"
        )
