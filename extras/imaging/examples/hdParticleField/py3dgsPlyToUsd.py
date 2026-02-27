#!/usr/bin/env python3

#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import argparse
import math
import os
import struct
from typing import Dict, Tuple, List, Any
from pxr import Usd, UsdGeom, Vt, Gf, UsdVol


# ============================================================================
# PLY FILE READER
# ============================================================================

class PLYReader:
    """Minimal PLY file reader for Gaussian splat data."""

    # Mapping from PLY data types to (struct_format, size_bytes, python_type)
    PLY_TYPE_MAP = {
        'char': ('b', 1, int),
        'uchar': ('B', 1, int),
        'short': ('h', 2, int),
        'ushort': ('H', 2, int),
        'int': ('i', 4, int),
        'uint': ('I', 4, int),
        'float': ('f', 4, float),
        'double': ('d', 8, float),
        'int8': ('b', 1, int),
        'uint8': ('B', 1, int),
        'int16': ('h', 2, int),
        'uint16': ('H', 2, int),
        'int32': ('i', 4, int),
        'uint32': ('I', 4, int),
        'float32': ('f', 4, float),
        'float64': ('d', 8, float),
    }

    def __init__(self, filename: str):
        """
        Initialize the PLY reader.

        Parameters
        ----------
        filename : str
            Path to the PLY file to read.
        """
        self.filename = filename
        self.format = None
        self.vertex_count = 0
        self.properties = []
        self.data = {}

    def read(self) -> Dict[str, List[Any]]:
        """
        Read the PLY file and return vertex data.

        Returns
        -------
        dict
            Dictionary mapping property names to Python lists.
        """
        with open(self.filename, 'rb') as f:
            # Parse header
            self._parse_header(f)

            # Read vertex data based on format
            if self.format == 'binary_little_endian':
                self._read_binary(f, '<')  # Little-endian
            elif self.format == 'binary_big_endian':
                self._read_binary(f, '>')  # Big-endian
            elif self.format == 'ascii':
                self._read_ascii(f)
            else:
                raise ValueError(f"Unsupported PLY format: {self.format}")

        return self.data

    def _parse_header(self, f) -> None:
        """Parse the PLY file header."""
        line = f.readline().decode('ascii').strip()
        if line != 'ply':
            raise ValueError("Not a valid PLY file - missing 'ply' magic number")

        in_vertex_element = False

        while True:
            line_bytes = f.readline()

            # Check for EOF before end_header
            if not line_bytes:
                raise ValueError("Unexpected EOF in PLY header - missing 'end_header'")

            line = line_bytes.decode('ascii').strip()

            if line.startswith('format'):
                parts = line.split()
                if len(parts) < 2:
                    raise ValueError(f"Invalid format line in PLY header: '{line}'")
                self.format = parts[1]

            elif line.startswith('element vertex'):
                parts = line.split()
                if len(parts) < 3:
                    raise ValueError(f"Invalid element vertex line in PLY header: '{line}'")
                try:
                    self.vertex_count = int(parts[2])
                except ValueError:
                    raise ValueError(f"Invalid vertex count in PLY header: '{parts[2]}'")
                in_vertex_element = True

            elif line.startswith('element'):
                # Non-vertex element, stop reading properties
                in_vertex_element = False

            elif line.startswith('property') and in_vertex_element:
                parts = line.split()
                if len(parts) < 3:
                    raise ValueError(f"Invalid property line in PLY header: '{line}'")
                prop_type = parts[1]
                prop_name = parts[2]
                self.properties.append((prop_name, prop_type))

            elif line == 'end_header':
                break

    def _read_binary(self, f, byte_order: str) -> None:
        """
        Read binary PLY data.

        Parameters
        ----------
        f : file object
            File to read from.
        byte_order : str
            Byte order prefix: '<' for little-endian, '>' for big-endian.
        """
        # Initialize lists for each property
        for prop_name, _ in self.properties:
            self.data[prop_name] = []

        # Build struct format string
        struct_format = byte_order
        for prop_name, prop_type in self.properties:
            if prop_type not in self.PLY_TYPE_MAP:
                raise ValueError(f"Unsupported property type: {prop_type}")
            struct_format += self.PLY_TYPE_MAP[prop_type][0]

        # Calculate size of one vertex
        struct_size = struct.calcsize(struct_format)

        # Read all vertex data
        bytes_to_read = struct_size * self.vertex_count
        data_bytes = f.read(bytes_to_read)

        # Validate we read the expected amount of data
        if len(data_bytes) != bytes_to_read:
            raise ValueError(
                f"Incomplete PLY data: expected {bytes_to_read} bytes, "
                f"got {len(data_bytes)} bytes"
            )

        # Unpack each vertex
        for i in range(self.vertex_count):
            offset = i * struct_size
            vertex_bytes = data_bytes[offset:offset + struct_size]
            values = struct.unpack(struct_format, vertex_bytes)

            # Store each property value
            for j, (prop_name, _) in enumerate(self.properties):
                self.data[prop_name].append(values[j])

    def _read_ascii(self, f) -> None:
        """Read ASCII PLY data."""
        # Initialize lists for each property
        for prop_name, prop_type in self.properties:
            if prop_type not in self.PLY_TYPE_MAP:
                raise ValueError(f"Unsupported property type: {prop_type}")
            self.data[prop_name] = []

        # Read vertices line by line
        for i in range(self.vertex_count):
            line = f.readline().decode('ascii').strip()
            values = line.split()

            for j, (prop_name, prop_type) in enumerate(self.properties):
                python_type = self.PLY_TYPE_MAP[prop_type][2]
                self.data[prop_name].append(python_type(values[j]))


def read_ply(filename: str) -> Tuple[Dict[str, List[Any]], int, List[str]]:
    """
    Read a PLY file and return vertex data.

    Parameters
    ----------
    filename : str
        Path to the PLY file.

    Returns
    -------
    data : dict
        Dictionary mapping property names to Python lists.
    vertex_count : int
        Number of vertices in the file.
    property_names : list
        List of property names in order.
    """
    reader = PLYReader(filename)
    data = reader.read()
    property_names = [prop[0] for prop in reader.properties]
    return data, reader.vertex_count, property_names


# ============================================================================
# PLY TO USD CONVERTER
# ============================================================================

def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description='Convert PLY Gaussian Splat files to USD format'
    )
    parser.add_argument(
        '-i', '--input',
        required=True,
        help='Input PLY file path'
    )
    parser.add_argument(
        '-o', '--output',
        required=True,
        help='Output USD file path'
    )
    parser.add_argument(
        '-n', '--name',
        default=None,
        help='Name of the Gaussian Splat USD prim (default: input filename without extension)'
    )
    parser.add_argument(
        '--generateSh',
        action='store_true',
        help='Generate DC spherical harmonics from RGB data, if spherical harmonics are not provided'
    )
    parser.add_argument(
        '--generateScales',
        action='store_true',
        help='Generate scales based on local neighborhood spacing, if scales are not provided (requires scipy)'
    )
    return parser.parse_args()


def convertPlyUSD(input_file, output_file, prim_name=None, generateSh=False, generateScales=False):
    # If no prim name provided, use input filename without extension
    if prim_name is None:
        prim_name = os.path.splitext(os.path.basename(input_file))[0]

    print(f"Input PLY file: {input_file}")
    print(f"Output USD file: {output_file}")
    print(f"Prim name: /{prim_name}")

    # Load PLY file
    print(f"\nLoading PLY file: {input_file}")
    vertex_data, vertex_count, property_names = read_ply(input_file)

    print(f"\nPLY Data Information:")
    print(f"Vertex count: {vertex_count}")
    print(f"Available properties: {tuple(property_names)}")

    # Create USD stage
    print(f"\nCreating USD stage: {output_file}")
    stage = Usd.Stage.CreateNew(output_file)

    # Set required metadata
    UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)
    UsdGeom.SetStageMetersPerUnit(stage, 1.0)

    # Add custom layer metadata to document the conversion
    stage.GetRootLayer().comment = (
        f"Converted from PLY to USD using pySplatPly2Usd\n"
        f"Source file: {input_file}"
    )

    # Create 3D Gaussian Splat prim
    prim_path = f"/{prim_name}"
    gs_prim = UsdVol.ParticleField3DGaussianSplat.Define(stage, prim_path)
    stage.SetDefaultPrim(gs_prim.GetPrim())

    # Extract and set positions
    if all(prop in vertex_data for prop in ['x', 'y', 'z']):
        print("\nProcessing positions...")
        # Combine x, y, z into list of tuples
        positions = list(zip(
            vertex_data['x'],
            vertex_data['y'],
            vertex_data['z']
        ))

        # Convert to VtVec3fArray
        positions_vt = Vt.Vec3fArray([Gf.Vec3f(float(p[0]), float(p[1]), float(p[2])) for p in positions])
        gs_prim.CreatePositionsAttr(positions_vt)

        # Calculate extents
        min_x = min(vertex_data['x'])
        min_y = min(vertex_data['y'])
        min_z = min(vertex_data['z'])
        max_x = max(vertex_data['x'])
        max_y = max(vertex_data['y'])
        max_z = max(vertex_data['z'])

        # Apply extent limit of 50000
        extent_limit = 50000.0
        extent_min = Gf.Vec3f(
            float(max(-extent_limit, min_x)),
            float(max(-extent_limit, min_y)),
            float(max(-extent_limit, min_z))
        )
        extent_max = Gf.Vec3f(
            float(min(extent_limit, max_x)),
            float(min(extent_limit, max_y)),
            float(min(extent_limit, max_z))
        )

        extent = Vt.Vec3fArray([extent_min, extent_max])
        boundable_prim = UsdGeom.Boundable(gs_prim.GetPrim())
        boundable_prim.CreateExtentAttr(extent)
        print(f"  Set positions for {vertex_count} vertices")
        print(f"  Extent: {extent_min} to {extent_max}")
    else:
        print("ERROR: PLY file missing x, y, z position data")
        return

    # Extract and set scales
    if all(prop in vertex_data for prop in ['scale_0', 'scale_1', 'scale_2']):
        print("\nProcessing scales...")
        # Combine scales and apply exp() transformation
        scales = [
            (math.exp(s0), math.exp(s1), math.exp(s2))
            for s0, s1, s2 in zip(
                vertex_data['scale_0'],
                vertex_data['scale_1'],
                vertex_data['scale_2']
            )
        ]

        # Convert to VtVec3fArray
        scales_vt = Vt.Vec3fArray([Gf.Vec3f(float(s[0]), float(s[1]), float(s[2])) for s in scales])
        gs_prim.CreateScalesAttr(scales_vt)
        print(f"  Set scales for {vertex_count} vertices")
    elif generateScales:
        from scipy.spatial import KDTree
        import numpy as np
        points = np.array(positions)
        tree = KDTree(points)
        distances, _ = tree.query(points, k=3, workers=-1)
        # For scale, take the average distance to nearby particles, and halve
        # that since we're using it to scale a radius
        scales = (distances[:, 0] + distances[:, 1] + distances[:, 2]) / (3.0 * 2.0)
        scales_vt = Vt.Vec3fArray([Gf.Vec3f(float(s), float(s), float(s)) for s in scales])
        gs_prim.CreateScalesAttr(scales_vt)
        print(f"  Set scales for {vertex_count} vertices, generated from local neighborhood spacing")
    else:
        print("  Warning: PLY file missing scale_0, scale_1, scale_2 data. Consider re-running with --generateScales")

    # Extract and set orientations/quaternions
    # Note: PLY quaternion layout is (rot_0=real, rot_1/2/3=imaginary)
    # GfQuatf layout is (imaginary, real), so we extract in order: rot_1, rot_2, rot_3, rot_0
    if all(prop in vertex_data for prop in ['rot_0', 'rot_1', 'rot_2', 'rot_3']):
        print("\nProcessing orientations...")
        # Extract quaternions in GfQuatf order: imaginary (rot_1, rot_2, rot_3), then real (rot_0)
        orientations = list(zip(
            vertex_data['rot_1'],  # imaginary x
            vertex_data['rot_2'],  # imaginary y
            vertex_data['rot_3'],  # imaginary z
            vertex_data['rot_0']   # real
        ))

        # Convert to VtQuatfArray and normalize
        orientations_list = []
        for q in orientations:
            quat = Gf.Quatf(float(q[3]), float(q[0]), float(q[1]), float(q[2]))  # real, imag_x, imag_y, imag_z
            quat.Normalize()
            orientations_list.append(quat)

        orientations_vt = Vt.QuatfArray(orientations_list)
        gs_prim.CreateOrientationsAttr(orientations_vt)
        print(f"  Set orientations for {vertex_count} vertices")
    else:
        print("  Warning: PLY file missing rot_0, rot_1, rot_2, rot_3 data")

    # Extract and set opacities
    if 'opacity' in vertex_data:
        print("\nProcessing opacities...")
        # Apply sigmoid transformation: 1.0 / (1.0 + exp(-v))
        opacities = [1.0 / (1.0 + math.exp(-v)) for v in vertex_data['opacity']]

        # Convert to VtFloatArray
        opacities_vt = Vt.FloatArray(opacities)
        gs_prim.CreateOpacitiesAttr(opacities_vt)
        print(f"  Set opacities for {vertex_count} vertices")
    else:
        print("  Warning: PLY file missing opacity data")

    # Extract and set spherical harmonics
    if all(prop in vertex_data for prop in ['f_dc_0', 'f_dc_1', 'f_dc_2']):
        print("\nProcessing spherical harmonics...")

        # Extract DC coefficients
        f_dc = list(zip(
            vertex_data['f_dc_0'],
            vertex_data['f_dc_1'],
            vertex_data['f_dc_2']
        ))

        # Find the maximum contiguous f_rest_X index
        max_sh_index = -1
        for i in range(45):
            prop_name = f'f_rest_{i}'
            if prop_name in vertex_data:
                max_sh_index = i
            else:
                break

        # Determine SH degree based on max_sh_index
        # degree 0: no f_rest (max_sh_index=-1)
        # degree 1: f_rest_0 to f_rest_8 (max_sh_index=8)
        # degree 2: f_rest_0 to f_rest_23 (max_sh_index=23)
        # degree 3: f_rest_0 to f_rest_44 (max_sh_index=44)
        if max_sh_index == -1:
            sh_degree = 0
        elif max_sh_index == 8:
            sh_degree = 1
        elif max_sh_index == 23:
            sh_degree = 2
        elif max_sh_index == 44:
            sh_degree = 3
        else:
            print(f"  Warning: Invalid number of SH coefficients found ({max_sh_index})")
            sh_degree = 0

        print(f"  Found SH degree: {sh_degree}")
        gs_prim.CreateRadianceSphericalHarmonicsDegreeAttr(sh_degree)

        if max_sh_index >= 0:
            # Extract f_rest data
            stride = max_sh_index + 1
            f_rest_data = []
            for i in range(stride):
                prop_name = f'f_rest_{i}'
                f_rest_data.append(vertex_data[prop_name])

            # Transpose f_rest_data to get it by vertex
            f_rest = list(zip(*f_rest_data))

            # Shuffle data into correct order
            # The PLY file stores SH coefficients in a different order than USD expects
            num_sh_vec3 = stride // 3
            sh_vec_stride = num_sh_vec3 + 1  # +1 for f_dc

            # Initialize sh_data as list of lists
            sh_data = []
            for i in range(vertex_count):
                vertex_sh = [[0.0, 0.0, 0.0] for _ in range(sh_vec_stride)]
                # First, write the f_dc values
                vertex_sh[0] = list(f_dc[i])

                # Then, shuffle f_rest data
                for j in range(num_sh_vec3):
                    for k in range(3):
                        src_index = k * num_sh_vec3 + j
                        vertex_sh[j + 1][k] = f_rest[i][src_index]

                sh_data.extend(vertex_sh)

            # Convert to VtVec3fArray
            sh_vt = Vt.Vec3fArray([Gf.Vec3f(float(v[0]), float(v[1]), float(v[2])) for v in sh_data])

            # Create SH attribute
            sh_attr = gs_prim.CreateRadianceSphericalHarmonicsCoefficientsAttr(sh_vt)

            # Set element size and interpolation
            sh_primvar = UsdGeom.Primvar(sh_attr)
            sh_primvar.SetElementSize(sh_vec_stride)
            sh_primvar.SetInterpolation(UsdGeom.Tokens.vertex)

            print(f"  Set spherical harmonics with {sh_vec_stride} coefficients per vertex")
        else:
            # Only DC coefficients, degree 0
            gs_prim.CreateRadianceSphericalHarmonicsDegreeAttr(0)

            sh_vt = Vt.Vec3fArray([Gf.Vec3f(float(v[0]), float(v[1]), float(v[2])) for v in f_dc])
            sh_attr = gs_prim.CreateRadianceSphericalHarmonicsCoefficientsAttr(sh_vt)

            sh_primvar = UsdGeom.Primvar(sh_attr)
            sh_primvar.SetElementSize(1)
            sh_primvar.SetInterpolation(UsdGeom.Tokens.vertex)

            print(f"  Set spherical harmonics with degree 0 (DC only)")
    elif all(prop in vertex_data for prop in ['red', 'green', 'blue']):
        if generateSh:
            f_rgb = list(zip(
                vertex_data['red'],
                vertex_data['green'],
                vertex_data['blue']
            ))
            rgb = Vt.Vec3fArray([Gf.Vec3f(float(v[0]), float(v[1]), float(v[2])) for v in f_rgb])
            SH_C0 = 0.28209479177387814
            sh_dc = Vt.Vec3fArray([(((v / 255.0) - Gf.Vec3f(0.5)) / SH_C0) for v in rgb])

            gs_prim.CreateRadianceSphericalHarmonicsDegreeAttr(0)
            sh_attr = gs_prim.CreateRadianceSphericalHarmonicsCoefficientsAttr(sh_dc)

            sh_primvar = UsdGeom.Primvar(sh_attr)
            sh_primvar.SetElementSize(1)
            sh_primvar.SetInterpolation(UsdGeom.Tokens.vertex)
            print("  Set spherical harmonics with degree 0 (DC only) generated from (red/green/blue)")
        else:
            print("  Warning: PLY file missing spherical harmonics data (f_dc_0/1/2). PLY file contains (red/green/blue); consider re-running with --generateSh")
    else:
        print("  Warning: PLY file missing spherical harmonics data (f_dc_0/1/2).")

    # Save the stage
    stage.Save()
    print(f"\nSuccessfully saved USD file: {output_file}")


if __name__ == "__main__":
    args = parse_args()
    convertPlyUSD(args.input, args.output, args.name, args.generateSh, args.generateScales)
