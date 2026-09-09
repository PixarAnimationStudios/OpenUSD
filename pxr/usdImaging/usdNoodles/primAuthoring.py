#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""
USD Prim Authoring Utilities.

Provides utilities for creating and manipulating USD prims in the node graph,
particularly for node creation workflows.
"""

from pxr import Gf, Sdf, Tf

try:
    from pxr import UsdUI
except ImportError:
    UsdUI = None


class PrimAuthor:
    """
    Utilities for creating prims in the USD stage.

    Provides methods for:
    - Generating unique prim names
    - Creating node prims (ExecNode, Container, Shader)
    - Setting up default attributes and positions
    """

    @staticmethod
    def generate_unique_prim_name(stage, parent_path, base_name):
        """
        Generate a unique prim name like 'ExecNode1', 'ExecNode2', etc.

        Args:
            stage: The USD stage
            parent_path: Parent container/blueprint path (as Sdf.Path or string)
            base_name: Base name for the prim (e.g., "ExecNode", "Container")

        Returns:
            str: Unique prim name (e.g., "ExecNode1", "Container2")
        """
        # Ensure parent_path is a string
        if isinstance(parent_path, Sdf.Path):
            parent_path = str(parent_path)

        parent_prim = stage.GetPrimAtPath(parent_path)
        if not parent_prim or not parent_prim.IsValid():
            # If parent doesn't exist, start with 1
            return f"{base_name}1"

        # Find the highest numbered variant of this base name
        max_number = 0
        for child in parent_prim.GetChildren():
            child_name = child.GetName()
            if child_name.startswith(base_name):
                # Try to extract the number suffix
                suffix = child_name[len(base_name) :]
                if suffix.isdigit():
                    max_number = max(max_number, int(suffix))

        # Return the next number
        return f"{base_name}{max_number + 1}"

    @staticmethod
    def create_node_prim(stage, parent_path, prim_type, position, edit_target=None):
        """
        Create a new node prim in the USD stage.

        Uses Sdf to speed things up.

        Args:
            stage: The USD stage
            parent_path: Parent container/blueprint path (as Sdf.Path or string)
            prim_type: Type of prim to create ("ExecNode", "Container", "Shader")
            position: World-space position (Gf.Vec2d)
            edit_target: Optional specific layer to author in (defaults to stage's edit target)

        Returns:
            Sdf.Path: Path to the created prim
        """
        if isinstance(parent_path, Sdf.Path):
            parent_path = str(parent_path)

        parent_prim = stage.GetPrimAtPath(parent_path)
        if not parent_prim or not parent_prim.IsValid():
            Tf.Warn(f"Cannot create prim: parent path {parent_path} does not exist")
            return None

        unique_name = PrimAuthor.generate_unique_prim_name(
            stage, parent_path, prim_type
        )
        if parent_path.endswith("/"):
            prim_path = f"{parent_path}{unique_name}"
        else:
            prim_path = f"{parent_path}/{unique_name}"

        prim_path = Sdf.Path(prim_path)

        # Get the layer to edit
        if edit_target:
            layer = edit_target.GetLayer()
        else:
            layer = stage.GetEditTarget().GetLayer()

        if not layer:
            Tf.Warn("No edit target layer found")
            return None

        try:
            primSpec = Sdf.CreatePrimInLayer(layer, prim_path)
            if not primSpec:
                Tf.Warn(f"Failed to create prim spec at {prim_path}")
                return None

            primSpec.typeName = prim_type
            primSpec.specifier = Sdf.SpecifierDef

            position_scale = 1.0 / 1000.0
            scaled_pos = Gf.Vec2f(
                position[0] * position_scale, position[1] * position_scale
            )

            attrSpec = Sdf.AttributeSpec(
                primSpec, "ui:nodegraph:node:pos", Sdf.ValueTypeNames.Float2
            )
            if attrSpec:
                attrSpec.default = scaled_pos

            Tf.Status(
                f"Created {prim_type} prim at {prim_path} with position {position}"
            )

            return prim_path

        except Exception as e:
            Tf.Warn(f"Error creating prim: {e}")
            import traceback

            traceback.print_exc()
            return None

    @staticmethod
    def create_backdrop_prim(
        stage,
        parent_path,
        position,
        size,
        description="",
        color=None,
        edit_target=None,
    ):
        """Create a new Backdrop prim in the USD stage.

        Args:
            stage: The USD stage
            parent_path: Parent container/blueprint path (as Sdf.Path or string)
            position: World-space position (Gf.Vec2d)
            size: World-space size (Gf.Vec2d)
            description: Text label for the backdrop
            color: Optional display color as Gf.Vec3f (0-1 range)
            edit_target: Optional specific layer to author in

        Returns:
            Sdf.Path: Path to the created prim, or None on failure
        """
        if isinstance(parent_path, Sdf.Path):
            parent_path = str(parent_path)

        parent_prim = stage.GetPrimAtPath(parent_path)
        if not parent_prim or not parent_prim.IsValid():
            Tf.Warn(f"Cannot create backdrop: parent path {parent_path} does not exist")
            return None

        unique_name = PrimAuthor.generate_unique_prim_name(
            stage, parent_path, "Backdrop"
        )
        if parent_path.endswith("/"):
            prim_path = f"{parent_path}{unique_name}"
        else:
            prim_path = f"{parent_path}/{unique_name}"

        prim_path = Sdf.Path(prim_path)

        if edit_target:
            layer = edit_target.GetLayer()
        else:
            layer = stage.GetEditTarget().GetLayer()

        if not layer:
            Tf.Warn("No edit target layer found")
            return None

        try:
            primSpec = Sdf.CreatePrimInLayer(layer, prim_path)
            if not primSpec:
                Tf.Warn(f"Failed to create prim spec at {prim_path}")
                return None

            primSpec.typeName = "Backdrop"
            primSpec.specifier = Sdf.SpecifierDef

            position_scale = 1.0 / 1000.0

            # Position
            scaled_pos = Gf.Vec2f(
                position[0] * position_scale, position[1] * position_scale
            )
            posAttrSpec = Sdf.AttributeSpec(
                primSpec, "ui:nodegraph:node:pos", Sdf.ValueTypeNames.Float2
            )
            if posAttrSpec:
                posAttrSpec.default = scaled_pos

            # Size
            scaled_size = Gf.Vec2f(size[0] * position_scale, size[1] * position_scale)
            sizeAttrSpec = Sdf.AttributeSpec(
                primSpec, "ui:nodegraph:node:size", Sdf.ValueTypeNames.Float2
            )
            if sizeAttrSpec:
                sizeAttrSpec.default = scaled_size

            # Description
            if description:
                descAttrSpec = Sdf.AttributeSpec(
                    primSpec, "ui:description", Sdf.ValueTypeNames.Token
                )
                if descAttrSpec:
                    descAttrSpec.default = description

            # Display color
            if color is not None:
                colorAttrSpec = Sdf.AttributeSpec(
                    primSpec,
                    "ui:nodegraph:node:displayColor",
                    Sdf.ValueTypeNames.Float3,
                )
                if colorAttrSpec:
                    colorAttrSpec.default = color

            Tf.Status(f"Created Backdrop prim at {prim_path} with position {position}")
            return prim_path

        except Exception as e:
            Tf.Warn(f"Error creating backdrop prim: {e}")
            import traceback

            traceback.print_exc()
            return None

    @staticmethod
    def get_current_container_path(stage, data_model=None):
        """
        Get the current container/blueprint path for creating nodes.

        This attempts to find the appropriate parent path for new nodes by:
        1. Checking if there's a selected container/blueprint in the data model
        2. Falling back to the root or a default blueprint

        Args:
            stage: The USD stage
            data_model: Optional UsdviewApi.dataModel for selection context

        Returns:
            Sdf.Path or None: Path to the current container/blueprint
        """
        if data_model and data_model.selection:
            # Get selected prims
            selected_prims = data_model.selection.getPrims()

            # Look for a Blueprint or Container in the selection
            for prim in selected_prims:
                prim_type = prim.GetTypeName()
                if prim_type in ["Blueprint", "Container"]:
                    return prim.GetPath()

                # Check if the prim's parent is a Blueprint/Container
                parent = prim.GetParent()
                if parent and parent.GetTypeName() in ["Blueprint", "Container"]:
                    return parent.GetPath()

        # Fall back to searching for any Blueprint in the stage
        for prim in stage.Traverse():
            if prim.GetTypeName() == "Blueprint":
                return prim.GetPath()

        # If no Blueprint found, return root
        return Sdf.Path("/")
