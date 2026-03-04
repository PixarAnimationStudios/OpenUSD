#!/usr/bin/env python3

#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

"""
Convert SPZ (compressed Gaussian splat) files to USD format.

SPZ is a compressed file format for 3D Gaussian splats developed by
Niantic Labs.
It is typically ~10x smaller than PLY files with minimal visual quality loss.
See: https://github.com/nianticlabs/spz

This script includes a pure Python SPZ reader implementation, so no external
dependencies are required beyond the standard library and USD Python bindings.
"""

import argparse
import gzip
import math
import os
import struct
from dataclasses import dataclass
from typing import List, Optional, Tuple

from pxr import Gf, Usd, UsdGeom, UsdVol, Vt


# ============================================================================
# SPZ FILE FORMAT READER (Pure Python Implementation)
# ============================================================================

@dataclass
class GaussianCloud:
    """Container for Gaussian splat data."""
    num_points: int
    sh_degree: int
    antialiased: bool
    positions: List[float]
    scales: List[float]
    rotations: List[float]
    alphas: List[float]
    colors: List[float]
    sh: List[float]


class SPZReader:
    """
    Pure Python reader for SPZ (compressed Gaussian splat) files.
    SPZ format specification: https://github.com/nianticlabs/spz
    """

    MAGIC = 0x5053474E
    SUPPORTED_VERSIONS = (2, 3)
    COLOR_SCALE = 0.15

    def __init__(self, filename: str):
        self.filename = filename

    def read(self) -> GaussianCloud:
        """Read and decompress an SPZ file."""
        with open(self.filename, "rb") as f:
            compressed_data = f.read()

        data = gzip.decompress(compressed_data)
        return self._parse_data(data)

    def _parse_data(self, data: bytes) -> GaussianCloud:
        """Parse decompressed SPZ data."""
        offset = 0

        header_fmt = "<IIIBBBB"
        (
            magic,
            version,
            num_points,
            sh_degree,
            fractional_bits,
            flags,
            reserved,
        ) = struct.unpack_from(header_fmt, data, offset)
        # '<IIIBBBB' = uint32 magic, uint32 version, uint32 numPoints,
        #              uint8 shDegree, uint8 fractionalBits, uint8 flags,
        #              uint8 reserved.
        del reserved
        offset += struct.calcsize(header_fmt)

        if magic != self.MAGIC:
            raise ValueError(f"Invalid SPZ magic number: {hex(magic)}")

        if version not in self.SUPPORTED_VERSIONS:
            raise ValueError(f"Unsupported SPZ version: {version}")

        if sh_degree > 3:
            raise ValueError(f"Invalid SH degree: {sh_degree}")

        antialiased = bool(flags & 0x1)

        # SPZ payload order (after header):
        # positions, alphas, colors, scales, rotations, SH.
        positions, offset = self._read_positions(
            data,
            offset,
            num_points,
            fractional_bits,
        )
        alphas, offset = self._read_alphas(data, offset, num_points)
        colors, offset = self._read_colors(data, offset, num_points)
        scales, offset = self._read_scales(data, offset, num_points)
        rotations, offset = self._read_rotations(
            data,
            offset,
            num_points,
            version,
        )
        sh, offset = self._read_sh(data, offset, num_points, sh_degree)

        return GaussianCloud(
            num_points=num_points,
            sh_degree=sh_degree,
            antialiased=antialiased,
            positions=positions,
            scales=scales,
            rotations=rotations,
            alphas=alphas,
            colors=colors,
            sh=sh,
        )

    def _read_positions(
        self,
        data: bytes,
        offset: int,
        num_points: int,
        fractional_bits: int,
    ) -> Tuple[List[float], int]:
        """Read positions as 24-bit fixed point values."""
        positions = []
        scale = 1.0 / (1 << fractional_bits)

        for _ in range(num_points):
            for _ in range(3):
                b0, b1, b2 = struct.unpack_from("BBB", data, offset)
                offset += 3

                value = b0 | (b1 << 8) | (b2 << 16)
                if value & 0x800000:
                    value -= 0x1000000

                positions.append(value * scale)

        return positions, offset

    def _read_scales(
        self,
        data: bytes,
        offset: int,
        num_points: int,
    ) -> Tuple[List[float], int]:
        """Read scales as 8-bit log-encoded values."""
        scales = []

        for _ in range(num_points):
            for _ in range(3):
                encoded = struct.unpack_from("B", data, offset)[0]
                offset += 1

                log_scale = (encoded / 16.0) - 10.0
                scales.append(log_scale)

        return scales, offset

    def _read_rotations(
        self,
        data: bytes,
        offset: int,
        num_points: int,
        version: int,
    ) -> Tuple[List[float], int]:
        """Read rotations as quaternions."""
        rotations = []

        if version == 3:
            for _ in range(num_points):
                packed = struct.unpack_from("<I", data, offset)[0]
                offset += 4

                c_mask = (1 << 9) - 1
                i_largest = packed >> 30
                comps = [0.0, 0.0, 0.0, 0.0]
                sum_sq = 0.0

                # Niantic "smallest three" unpacking.
                for i in (3, 2, 1, 0):
                    if i == i_largest:
                        continue
                    mag = packed & c_mask
                    negbit = (packed >> 9) & 0x1
                    packed >>= 10
                    v = 0.7071067811865476 * (float(mag) / float(c_mask))
                    if negbit == 1:
                        v = -v
                    comps[i] = v
                    sum_sq += v * v

                comps[i_largest] = math.sqrt(max(0.0, 1.0 - sum_sq))
                # SPZ order is xyzw.
                rotations.extend([comps[0], comps[1], comps[2], comps[3]])
        else:
            for _ in range(num_points):
                # Legacy v2: first three components are uint8 in [-1, 1].
                qx = (struct.unpack_from("B", data, offset)[0] / 127.5) - 1.0
                offset += 1
                qy = (struct.unpack_from("B", data, offset)[0] / 127.5) - 1.0
                offset += 1
                qz = (struct.unpack_from("B", data, offset)[0] / 127.5) - 1.0
                offset += 1

                sum_sq = qx * qx + qy * qy + qz * qz
                qw = math.sqrt(max(0.0, 1.0 - sum_sq))

                rotations.extend([qx, qy, qz, qw])

        return rotations, offset

    def _read_alphas(
        self,
        data: bytes,
        offset: int,
        num_points: int,
    ) -> Tuple[List[float], int]:
        """Read alphas as 8-bit unsigned values (before sigmoid)."""
        alphas = []

        for _ in range(num_points):
            encoded = struct.unpack_from("B", data, offset)[0]
            offset += 1

            # SPZ stores sigmoid(alpha) in [0, 255], so apply inverse sigmoid.
            s = encoded / 255.0
            s = min(1.0 - 1e-6, max(1e-6, s))
            alpha = math.log(s / (1.0 - s))
            alphas.append(alpha)

        return alphas, offset

    def _read_colors(
        self,
        data: bytes,
        offset: int,
        num_points: int,
    ) -> Tuple[List[float], int]:
        """Read SH DC color coefficients from 8-bit values."""
        colors = []

        for _ in range(num_points):
            r_encoded = struct.unpack_from("B", data, offset)[0]
            offset += 1
            g_encoded = struct.unpack_from("B", data, offset)[0]
            offset += 1
            b_encoded = struct.unpack_from("B", data, offset)[0]
            offset += 1

            r = ((r_encoded / 255.0) - 0.5) / self.COLOR_SCALE
            g = ((g_encoded / 255.0) - 0.5) / self.COLOR_SCALE
            b = ((b_encoded / 255.0) - 0.5) / self.COLOR_SCALE
            colors.extend([r, g, b])

        return colors, offset

    def _read_sh(
        self,
        data: bytes,
        offset: int,
        num_points: int,
        sh_degree: int,
    ) -> Tuple[List[float], int]:
        """Read spherical harmonics coefficients."""
        if sh_degree == 0:
            return [], offset

        num_coeffs_per_point = ((sh_degree + 1) ** 2 - 1) * 3
        total = num_points * num_coeffs_per_point
        sh = [0.0] * total
        # SPZ stores SH in point-major layout with RGB as fastest-varying.
        for i in range(total):
            encoded = struct.unpack_from("B", data, offset)[0]
            offset += 1
            sh[i] = (encoded - 128.0) / 128.0

        return sh, offset


def read_spz(filename: str) -> GaussianCloud:
    """
    Read an SPZ file and return Gaussian cloud data.

    TODO: We have an `spzlib`-based implementation, but keep this converter on
    the pure-Python parser path until `spzlib` is available on pip as a wheel.
    """
    reader = SPZReader(filename)
    return reader.read()


def parse_args() -> argparse.Namespace:
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Convert SPZ Gaussian Splat files to USD format"
    )
    parser.add_argument(
        "-i",
        "--input",
        required=True,
        help="Input SPZ file path",
    )
    parser.add_argument(
        "-o",
        "--output",
        required=True,
        help="Output USD file path",
    )
    parser.add_argument(
        "-n",
        "--name",
        default=None,
        help=(
            "Name of the Gaussian Splat USD prim "
            "(default: input filename without extension)"
        ),
    )
    parser.add_argument(
        "--shDegree",
        type=int,
        default=None,
        choices=[0, 1, 2, 3],
        help=(
            "Override spherical harmonics degree (0-3). "
            "If not specified, uses the degree from the file."
        ),
    )
    return parser.parse_args()


def load_gaussian_cloud(input_file: str) -> GaussianCloud:
    """
    Load a Gaussian cloud from an SPZ file.

    Parameters
    ----------
    input_file : str
        Path to the input SPZ file.

    Returns
    -------
    GaussianCloud
        The loaded Gaussian cloud data.
    """
    ext = os.path.splitext(input_file)[1].lower()

    if ext == ".spz":
        return read_spz(input_file)

    raise ValueError(f"Unsupported file format: {ext}. Expected .spz")


def convert_spz_to_usd(
    input_file: str,
    output_file: str,
    prim_name: Optional[str] = None,
    sh_degree_override: Optional[int] = None,
) -> None:
    """
    Convert an SPZ file to USD format.

    Parameters
    ----------
    input_file : str
        Path to the input SPZ file.
    output_file : str
        Path to the output USD file.
    prim_name : str, optional
        Name for the USD prim. If None, uses the input filename.
    sh_degree_override : int, optional
        Override the spherical harmonics degree (0-3).
    """
    if prim_name is None:
        prim_name = os.path.splitext(os.path.basename(input_file))[0]

    print(f"Input file: {input_file}")
    print(f"Output USD file: {output_file}")
    print(f"Prim name: /{prim_name}")

    print(f"\nLoading file: {input_file}")
    cloud = load_gaussian_cloud(input_file)

    vertex_count = cloud.num_points
    print("\nGaussian Cloud Information:")
    print(f"  Number of points: {vertex_count}")
    print(f"  SH degree: {cloud.sh_degree}")
    print(f"  Antialiased: {cloud.antialiased}")

    if vertex_count == 0:
        raise ValueError("Input file contains no Gaussian splats")

    print(f"\nCreating USD stage: {output_file}")
    stage = Usd.Stage.CreateNew(output_file)

    UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.y)
    UsdGeom.SetStageMetersPerUnit(stage, 1.0)

    stage.GetRootLayer().comment = (
        f"Converted from SPZ to USD using py3dgsSpzToUsd\n"
        f"Source file: {input_file}"
    )

    prim_path = f"/{prim_name}"
    gs_prim = UsdVol.ParticleField3DGaussianSplat.Define(stage, prim_path)
    stage.SetDefaultPrim(gs_prim.GetPrim())

    # Process positions (flat array: [x1, y1, z1, x2, y2, z2, ...])
    print("\nProcessing positions...")
    positions_flat = cloud.positions
    if len(positions_flat) == 0:
        raise ValueError("No position data in input file")

    positions_vt = Vt.Vec3fArray([
        Gf.Vec3f(
            float(positions_flat[i * 3]),
            float(positions_flat[i * 3 + 1]),
            float(positions_flat[i * 3 + 2])
        )
        for i in range(vertex_count)
    ])
    gs_prim.CreatePositionsAttr(positions_vt)

    # Calculate extents
    x_coords = [positions_flat[i * 3] for i in range(vertex_count)]
    y_coords = [positions_flat[i * 3 + 1] for i in range(vertex_count)]
    z_coords = [positions_flat[i * 3 + 2] for i in range(vertex_count)]

    extent_limit = 50000.0
    extent_min = Gf.Vec3f(
        float(max(-extent_limit, min(x_coords))),
        float(max(-extent_limit, min(y_coords))),
        float(max(-extent_limit, min(z_coords)))
    )
    extent_max = Gf.Vec3f(
        float(min(extent_limit, max(x_coords))),
        float(min(extent_limit, max(y_coords))),
        float(min(extent_limit, max(z_coords)))
    )

    extent = Vt.Vec3fArray([extent_min, extent_max])
    boundable_prim = UsdGeom.Boundable(gs_prim.GetPrim())
    boundable_prim.CreateExtentAttr(extent)
    print(f"  Set positions for {vertex_count} vertices")
    print(f"  Extent: {extent_min} to {extent_max}")

    # Process scales (flat array: [sx1, sy1, sz1, ...], values are log-scale)
    print("\nProcessing scales...")
    scales_flat = cloud.scales
    if len(scales_flat) > 0:
        scales_vt = Vt.Vec3fArray([
            Gf.Vec3f(
                math.exp(float(scales_flat[i * 3])),
                math.exp(float(scales_flat[i * 3 + 1])),
                math.exp(float(scales_flat[i * 3 + 2]))
            )
            for i in range(vertex_count)
        ])
        gs_prim.CreateScalesAttr(scales_vt)
        print(
            f"  Set scales for {vertex_count} vertices "
            "(converted from log-scale)"
        )
    else:
        print("  Warning: No scale data in input file")

    # Process rotations (flat array: [x1, y1, z1, w1, x2, y2, z2, w2, ...])
    # SPZ quaternion order is (x, y, z, w)
    # GfQuatf constructor is (real, imaginary_x, imaginary_y, imaginary_z)
    print("\nProcessing rotations...")
    rotations_flat = cloud.rotations
    if len(rotations_flat) > 0:
        orientations_list = []
        for i in range(vertex_count):
            qx = float(rotations_flat[i * 4])
            qy = float(rotations_flat[i * 4 + 1])
            qz = float(rotations_flat[i * 4 + 2])
            qw = float(rotations_flat[i * 4 + 3])
            quat = Gf.Quatf(qw, qx, qy, qz)
            quat.Normalize()
            orientations_list.append(quat)

        orientations_vt = Vt.QuatfArray(orientations_list)
        gs_prim.CreateOrientationsAttr(orientations_vt)
        print(f"  Set orientations for {vertex_count} vertices")
    else:
        print("  Warning: No rotation data in input file")

    # Process opacities (flat array: [a1, a2, ...], values are before sigmoid)
    print("\nProcessing opacities...")
    alphas_flat = cloud.alphas
    if len(alphas_flat) > 0:
        opacities_vt = Vt.FloatArray([
            1.0 / (1.0 + math.exp(-float(a))) for a in alphas_flat
        ])
        gs_prim.CreateOpacitiesAttr(opacities_vt)
        print(
            f"  Set opacities for {vertex_count} vertices "
            "(converted via sigmoid)"
        )
    else:
        print("  Warning: No alpha/opacity data in input file")

    # Process spherical harmonics and colors
    print("\nProcessing spherical harmonics...")
    sh_degree = (
        sh_degree_override
        if sh_degree_override is not None
        else cloud.sh_degree
    )
    colors_flat = cloud.colors
    sh_flat = cloud.sh

    # Number of SH coefficients per point (excluding DC from colors).
    # degree 1: 9 coeffs, degree 2: 24 coeffs, degree 3: 45 coeffs
    # Formula: ((sh_degree + 1)^2 - 1) * 3
    num_sh_coeffs = ((sh_degree + 1) ** 2 - 1) * 3 if sh_degree > 0 else 0

    if sh_degree > 0 and len(sh_flat) > 0:
        sh_vec_stride = 1 + (num_sh_coeffs // 3)
        gs_prim.CreateRadianceSphericalHarmonicsDegreeAttr(sh_degree)

        sh_data = []
        for i in range(vertex_count):
            vertex_sh = []

            # SPZ colors contain SH DC coefficients.
            if len(colors_flat) >= (i + 1) * 3:
                dc = [
                    float(colors_flat[i * 3]),
                    float(colors_flat[i * 3 + 1]),
                    float(colors_flat[i * 3 + 2]),
                ]
                vertex_sh.append(dc)
            else:
                vertex_sh.append([0.0, 0.0, 0.0])

            # Higher-order SH coefficients
            sh_offset = i * num_sh_coeffs
            num_sh_vec3 = num_sh_coeffs // 3

            for j in range(num_sh_vec3):
                idx = sh_offset + j * 3
                if idx + 2 < len(sh_flat):
                    vertex_sh.append([
                        float(sh_flat[idx]),
                        float(sh_flat[idx + 1]),
                        float(sh_flat[idx + 2])
                    ])
                else:
                    vertex_sh.append([0.0, 0.0, 0.0])

            sh_data.extend(vertex_sh)

        sh_vt = Vt.Vec3fArray([
            Gf.Vec3f(float(v[0]), float(v[1]), float(v[2]))
            for v in sh_data
        ])

        sh_attr = (
            gs_prim.CreateRadianceSphericalHarmonicsCoefficientsAttr(sh_vt)
        )
        sh_primvar = UsdGeom.Primvar(sh_attr)
        sh_primvar.SetElementSize(sh_vec_stride)
        sh_primvar.SetInterpolation(UsdGeom.Tokens.vertex)

        print(
            f"  Set spherical harmonics degree {sh_degree} "
            f"with {sh_vec_stride} Vec3f per vertex"
        )

    elif len(colors_flat) > 0:
        gs_prim.CreateRadianceSphericalHarmonicsDegreeAttr(0)

        sh_dc = Vt.Vec3fArray([
            Gf.Vec3f(
                float(colors_flat[i * 3]),
                float(colors_flat[i * 3 + 1]),
                float(colors_flat[i * 3 + 2]),
            )
            for i in range(vertex_count)
        ])

        sh_attr = (
            gs_prim.CreateRadianceSphericalHarmonicsCoefficientsAttr(sh_dc)
        )

        sh_primvar = UsdGeom.Primvar(sh_attr)
        sh_primvar.SetElementSize(1)
        sh_primvar.SetInterpolation(UsdGeom.Tokens.vertex)

        print(f"  Set spherical harmonics degree 0 (DC only from colors)")
    else:
        print("  Warning: No color or spherical harmonics data in input file")

    stage.Save()
    print(f"\nSuccessfully saved USD file: {output_file}")


if __name__ == "__main__":
    args = parse_args()
    convert_spz_to_usd(args.input, args.output, args.name, args.shDegree)
