#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

import json

from pxr import Gf, Sdf, Tf, Usd

from .nodeGraphBlueprint import NodeGraphBlueprint


class NodeGraphJson(NodeGraphBlueprint):
    """
    NodeGraphJson loads a JSON node graph file and converts it to USD format
    at the /jsonGraph path, then uses the Blueprint loader to display it.
    """

    def __init__(self):
        super().__init__()
        self._jsonStage = None

    def load(self, jsonPath, calculateTextWidth, fontMetrics):
        """Load a JSON graph file, convert it to USD, and display it."""
        try:
            with open(jsonPath, "r") as f:
                data = json.load(f)
        except Exception as e:
            Tf.Warn(f"Failed to load graph from JSON: {e}")
            return

        # Create an in-memory USD stage to hold the graph
        self._jsonStage = Usd.Stage.CreateInMemory()

        # Create the root graph container at /jsonGraph
        graphPath = Sdf.Path("/jsonGraph")
        graphPrim = self._jsonStage.DefinePrim(graphPath, "Blueprint")

        if not graphPrim:
            Tf.Warn("Failed to create /jsonGraph prim")
            return

        # Scale factor: JSON uses larger coordinates, USD uses small units
        # Reverse the scale from Blueprint (which multiplies by 1000)
        positionScale = 1.0 / 1000.0

        # Process all nodes from JSON
        nodesArray = data.get("nodes", [])
        nodeIdMap = {}  # Map JSON node IDs to USD prim paths

        for nodeObj in nodesArray:
            if not nodeObj:
                continue

            nodeId = list(nodeObj.keys())[0]
            nodeData = nodeObj[nodeId]

            # Create a valid prim name from the node ID
            # Replace invalid characters with underscores
            nodeName = nodeId.replace(":", "_").replace(".", "_").replace("/", "_")
            nodePath = graphPath.AppendChild(nodeName)

            # Create ExecNode prim
            nodePrim = self._jsonStage.DefinePrim(nodePath, "ExecNode")
            nodeIdMap[nodeId] = nodePath

            # Set node type as info:implementationSource
            nodeTypeName = nodeData.get("type_", "")
            if nodeTypeName:
                nodePrim.CreateAttribute(
                    "info:implementationSource", Sdf.ValueTypeNames.String
                ).Set(nodeTypeName)

            # Set position (scaled down for USD)
            pos = nodeData.get("pos", [0, 0])
            posAttr = nodePrim.CreateAttribute(
                "ui:nodegraph:node:pos", Sdf.ValueTypeNames.Float2
            )
            posAttr.Set(Gf.Vec2f(pos[0] * positionScale, pos[1] * positionScale))

            # Create input pins
            for inputPort in nodeData.get("input_ports", []):
                portName = inputPort.get("name", "")
                if not portName:
                    continue

                # Create input attribute
                attrName = f"inputs:{portName}"
                # Use token type as a placeholder; type info could be stored in metadata
                inputAttr = nodePrim.CreateAttribute(attrName, Sdf.ValueTypeNames.Token)

                # Store port type in metadata if available
                portType = inputPort.get("type", inputPort.get("type_", ""))
                if portType:
                    inputAttr.SetCustomDataByKey("portType", portType)

            # Create output pins
            for outputPort in nodeData.get("output_ports", []):
                portName = outputPort.get("name", "")
                if not portName:
                    continue

                # Create output attribute
                attrName = f"outputs:{portName}"
                outputAttr = nodePrim.CreateAttribute(
                    attrName, Sdf.ValueTypeNames.Token
                )

                # Store port type in metadata if available
                portType = outputPort.get("type", outputPort.get("type_", ""))
                if portType:
                    outputAttr.SetCustomDataByKey("portType", portType)

        # Second pass: Create connections
        for nodeObj in nodesArray:
            if not nodeObj:
                continue

            nodeId = list(nodeObj.keys())[0]
            nodeData = nodeObj[nodeId]

            if nodeId not in nodeIdMap:
                continue

            sourcePrimPath = nodeIdMap[nodeId]
            sourcePrim = self._jsonStage.GetPrimAtPath(sourcePrimPath)

            # Process output connections
            outputs = nodeData.get("outputs", {})
            for outputName, outputObj in outputs.items():
                sourceAttrName = f"outputs:{outputName}"
                sourceAttr = sourcePrim.GetAttribute(sourceAttrName)

                if not sourceAttr:
                    continue

                for connectedNodeId, targetPins in outputObj.items():
                    if connectedNodeId not in nodeIdMap:
                        continue

                    targetPrimPath = nodeIdMap[connectedNodeId]

                    for targetPin in targetPins:
                        # Create connection from source output to target input
                        targetAttrPath = targetPrimPath.AppendProperty(
                            f"inputs:{targetPin}"
                        )
                        sourceAttr.AddConnection(targetAttrPath)

            # Process input connections (if they exist in the JSON)
            inputs = nodeData.get("inputs", {})
            for inputName, inputObj in inputs.items():
                targetAttrName = f"inputs:{inputName}"
                targetAttr = sourcePrim.GetAttribute(targetAttrName)

                if not targetAttr:
                    continue

                for connectedNodeId, sourcePins in inputObj.items():
                    if connectedNodeId not in nodeIdMap:
                        continue

                    sourcePrimPath = nodeIdMap[connectedNodeId]

                    for sourcePin in sourcePins:
                        # Create connection from connected node's output to this input
                        sourceAttrPath = sourcePrimPath.AppendProperty(
                            f"outputs:{sourcePin}"
                        )
                        targetAttr.AddConnection(sourceAttrPath)

        # Now load the USD graph using the parent Blueprint loader
        # Note: We don't sync selection to prim tree for JSON graphs
        self.syncSelectionToPrimTree = False

        super().load(
            self._jsonStage,
            graphPath,
            calculateTextWidth,
            fontMetrics,
            useFieldnames=False,
        )

        Tf.Status(f"Loaded JSON graph from {jsonPath}, converted to USD at {graphPath}")
