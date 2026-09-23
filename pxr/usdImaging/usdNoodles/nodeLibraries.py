#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""
Node Library System for Dynamic Node Creation.

Provides a flexible system for managing different types of node creation sources
(UsdShade shaders, Exec nodes, etc.) that can be toggled on/off and queried
for available node types.

This module defines the abstract base class for node libraries. Concrete
implementations are provided in the nodeLibs subpackage.

Node libraries now also handle creating UI node objects from existing USD prims,
providing port/attribute descriptions and UI style information.
"""

from abc import ABC, abstractmethod
from typing import Dict, List, Optional

from pxr import Gf, Sdf, Tf, Usd


class NodeDescriptor:
    """
    Describes how to create a NodeModel from USD data.

    This is an intermediate representation that captures all the information
    needed to construct a NodeModel and its connections.
    """

    def __init__(self):
        self.primPath: Optional[Sdf.Path] = None
        self.inputPins: List[str] = []
        self.outputPins: List[str] = []
        self.inputPinTypes: Dict[str, str] = {}
        self.outputPinTypes: Dict[str, str] = {}
        self.uiStyle: Optional[str] = None
        self.renderer = None  # Optional GraphNodeRenderer


class NodeLibrary(ABC):
    """
    Abstract base class for node libraries.

    A node library represents a source of node types that can be created in the
    node graph. Each library provides:
    - A name for display in the UI
    - A list of families (categories) for organizing nodes
    - Node types within each family
    - Logic to create nodes of its types

    Attributes:
        enabled: Whether this library is currently active and should contribute
                 nodes to the hotbox
    """

    def __init__(self, enabled=True):
        """
        Initialize the node library.

        Args:
            enabled: Whether the library starts enabled (default: True)
        """
        self.enabled = enabled

    @abstractmethod
    def get_name(self) -> str:
        """
        Get the display name of this library.

        Returns:
            str: Library name (e.g., "Shaders", "Exec Nodes")
        """
        pass

    @abstractmethod
    def get_families(self) -> List[str]:
        """
        Get the list of family names for grouping node types.

        Families are used to organize nodes in the hotbox UI. For example,
        shaders might be grouped by "UsdPreviewSurface", "MaterialX", etc.,
        while exec nodes might have "Execution" and "Containers".

        Returns:
            List[str]: List of family/category names
        """
        pass

    @abstractmethod
    def get_node_types(self, family: str) -> List[Dict]:
        """
        Get the list of node types for a given family.

        Args:
            family: The family name to query

        Returns:
            List[Dict]: List of node type dictionaries, each containing:
                - 'name': Display name for the node type
                - 'identifier': Unique identifier for creation
                - 'family': Family/group name (same as input parameter)
        """
        pass

    @abstractmethod
    def create_node(
        self,
        stage: Usd.Stage,
        parent_path: Sdf.Path,
        identifier: str,
        position: Gf.Vec2d,
    ) -> Optional[Sdf.Path]:
        """
        Create a new node prim of the specified type.

        Args:
            stage: The USD stage
            parent_path: Parent container/blueprint path
            identifier: Node type identifier (from get_node_types)
            position: World-space position for the node

        Returns:
            Sdf.Path: Path to the created prim, or None on failure
        """
        pass

    @abstractmethod
    def can_handle_prim(self, prim: Usd.Prim) -> bool:
        """
        Check if this library can handle creating a node from the given prim.

        This is used to determine which library should be used when creating
        NodeModel objects from existing USD prims.

        Args:
            prim: USD prim to check

        Returns:
            bool: True if this library can create a node for this prim type
        """
        pass

    @abstractmethod
    def create_node_descriptor_from_prim(
        self, prim: Usd.Prim, stage: Usd.Stage
    ) -> Optional[Dict]:
        """
        Create a node descriptor from an existing USD prim.

        This returns all information needed to construct a NodeModel without
        actually creating the NodeModel. This allows the caller to handle
        node instantiation and link creation consistently.

        The descriptor provides port information (names and types) which may
        come from a registry (e.g., Sdr for shaders) or from the prim's
        existing attributes.

        Args:
            prim: USD prim to create descriptor from
            stage: USD stage

        Returns:
            Dict containing:
                - 'primPath': Sdf.Path to the prim
                - 'inputPins': List[str] of input pin names
                - 'outputPins': List[str] of output pin names
                - 'inputPinTypes': Dict[str, str] mapping pin name to type
                - 'outputPinTypes': Dict[str, str] mapping pin name to type
                - 'uiStyle': Optional[str] UI style identifier for rendering
                - 'renderer': Optional GraphNodeRenderer instance
            None if the prim cannot be handled
        """
        pass

    @abstractmethod
    def get_prim_ui_style(self, prim: Usd.Prim) -> Optional[str]:
        """
        Get the UI style identifier for a prim.

        The UI style determines which renderer class is used to draw the node.
        Common styles include "default", "graffi", etc.

        Args:
            prim: USD prim

        Returns:
            str: UI style identifier (e.g., "default", "graffi")
            None if no specific style or prim cannot be handled
        """
        pass

    @abstractmethod
    def create_connection(
        self,
        stage: Usd.Stage,
        source_prim: Usd.Prim,
        source_port: str,
        target_prim: Usd.Prim,
        target_port: str,
        *,
        source_property_name: str = "",
        target_property_name: str = "",
    ) -> bool:
        """
        Create a connection in USD between two ports.

        This method handles the USD-specific authoring for creating connections.
        Different node types (ExecNode, Shader) have different connection conventions.

        Args:
            stage: USD stage
            source_prim: Source node prim (output side)
            source_port: Source port/pin name (output)
            target_prim: Target node prim (input side)
            target_port: Target port/pin name (input)
            source_property_name: Exact USD property name for the source endpoint
            target_property_name: Exact USD property name for the target endpoint

        Returns:
            bool: True if connection was created successfully
        """
        pass

    @abstractmethod
    def delete_connection(
        self,
        stage: Usd.Stage,
        source_prim: Usd.Prim,
        source_port: str,
        target_prim: Usd.Prim,
        target_port: str,
        *,
        source_property_name: str = "",
        target_property_name: str = "",
    ) -> bool:
        """
        Delete a connection in USD between two ports.

        Args:
            stage: USD stage
            source_prim: Source node prim (output side)
            source_port: Source port/pin name (output)
            target_prim: Target node prim (input side)
            target_port: Target port/pin name (input)
            source_property_name: Exact USD property name for the source endpoint
            target_property_name: Exact USD property name for the target endpoint

        Returns:
            bool: True if connection was deleted successfully
        """
        pass

    @abstractmethod
    def can_connect(
        self,
        source_prim: Usd.Prim,
        source_port: str,
        source_is_output: bool,
        target_prim: Usd.Prim,
        target_port: str,
        target_is_output: bool,
    ) -> bool:
        """
        Validate if a connection is allowed between two ports.

        This can check:
        - Direction rules (output->input only)
        - Type compatibility
        - Node-specific connection rules

        Args:
            source_prim: Source node prim
            source_port: Source port/pin name
            source_is_output: True if source is output port
            target_prim: Target node prim
            target_port: Target port/pin name
            target_is_output: True if target is output port

        Returns:
            bool: True if connection is valid
        """
        pass

    @abstractmethod
    def get_links_for_prim(self, prim: Usd.Prim) -> List[Dict]:
        """
        Get all connections for a prim as link data.

        This method reads USD connections from both INPUT and OUTPUT attributes
        and returns normalized link data. Different node types may have different
        connection authoring conventions (e.g., connections authored on outputs vs inputs),
        so this method ensures all connections are properly discovered.

        The returned links use a consistent format regardless of how connections
        are authored in USD:
        - sourceNodeId: Path to the node with the output port
        - sourcePinName: Name of the output port (without 'outputs:' prefix)
        - targetNodeId: Path to the node with the input port
        - targetPinName: Name of the input port (without 'inputs:' prefix)
        - is_input_link: True if discovered from an input attribute

        Args:
            prim: USD prim to get connections for

        Returns:
            List[Dict]: List of link dictionaries with connection data
        """
        pass


# Register NodeLibrary as a discoverable TfType base so that external plugins
# can be found at runtime via Plug.Registry.GetAllDerivedTypes().
# This is the same pattern usdview uses for PluginContainer discovery.
Tf.Type.Define(NodeLibrary)
