#!/pxrpythonsubst
#
# Copyright 2025 Apple
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd, UsdGeom, Sdf, Vt, Gf
import unittest
import os
import subprocess
import zipfile
import math
import tempfile


class TestUsdPmc(unittest.TestCase):

    def _make_sphere_stage(self, path):
        """Create a USD stage with a procedurally generated polygon sphere."""
        stage = Usd.Stage.CreateNew(path)
        mesh = UsdGeom.Mesh.Define(stage, '/Sphere')

        u_segments = 16
        v_segments = 8
        radius = 1.0

        points = []
        normals = []
        face_vertex_counts = []
        face_vertex_indices = []

        for v in range(v_segments + 1):
            theta = math.pi * v / v_segments
            for u in range(u_segments):
                phi = 2.0 * math.pi * u / u_segments
                x = radius * math.sin(theta) * math.cos(phi)
                y = radius * math.cos(theta)
                z = radius * math.sin(theta) * math.sin(phi)
                points.append(Gf.Vec3f(x, y, z))
                normals.append(Gf.Vec3f(x, y, z).GetNormalized())

        for v in range(v_segments):
            for u in range(u_segments):
                i0 = v * u_segments + u
                i1 = v * u_segments + (u + 1) % u_segments
                i2 = (v + 1) * u_segments + (u + 1) % u_segments
                i3 = (v + 1) * u_segments + u
                face_vertex_counts.append(4)
                face_vertex_indices.extend([i0, i1, i2, i3])

        mesh.CreatePointsAttr().Set(Vt.Vec3fArray(points))
        mesh.CreateNormalsAttr().Set(Vt.Vec3fArray(normals))
        mesh.CreateFaceVertexCountsAttr().Set(Vt.IntArray(face_vertex_counts))
        mesh.CreateFaceVertexIndicesAttr().Set(Vt.IntArray(face_vertex_indices))
        stage.Save()
        return stage

    def test_CrushUsdz(self):
        """
        Test that usdcrush can process a usdz file containing a
        procedurally generated polygon sphere and verify the output file
        structure.
        """
        sphere_usdc = tempfile.NamedTemporaryFile(suffix='.usdc', delete=False).name
        sphere_usdz = tempfile.NamedTemporaryFile(suffix='.usdz', delete=False).name
        sphere_crushed_usdz = tempfile.NamedTemporaryFile(suffix='.usdz', delete=False).name

        self._make_sphere_stage(sphere_usdc)

        # Package it into a usdz file using UsdUtils API
        from pxr.UsdUtils import CreateNewUsdzPackage
        success = CreateNewUsdzPackage(sphere_usdc, sphere_usdz)
        self.assertTrue(success, "Failed to create usdz package")
        self.assertTrue(os.path.exists(sphere_usdz))

        # Run usdcrush on the usdz file
        # usdcrush will be found via PATH since PRE_PATH adds CMAKE_INSTALL_PREFIX/bin
        usdcrush_cmd = [
            'usdcrush',
            sphere_usdz,
            '-o', sphere_crushed_usdz
        ]
        result = subprocess.run(usdcrush_cmd, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0,
                        f"usdcrush failed: {result.stderr}")
        self.assertTrue(os.path.exists(sphere_crushed_usdz))

        # Verify the crushed usdz file structure
        with zipfile.ZipFile(sphere_crushed_usdz, 'r') as zf:
            file_list = sorted(zf.namelist())

            # Expected files
            expected_files = sorted([
                os.path.basename(sphere_usdc),
                'pmcCodec/0.pmc'
            ])

            # For now, just verify we have some files in the archive
            self.assertGreater(len(file_list), 0,
                             "Crushed usdz should contain files")

            self.assertEqual(file_list, expected_files,
                            f"File list mismatch. Got: {file_list}")

        # Check that sphere_usdz is smaller than sphere_usdc
        usdc_size = os.path.getsize(sphere_usdc)
        usdz_size = os.path.getsize(sphere_crushed_usdz)
        self.assertLess(usdz_size, usdc_size,
                       f"usdz file ({usdz_size} bytes) should be smaller than "
                       f"usdc file ({usdc_size} bytes)")


    def test_CrushUsdc(self):
        """
        Test that usdcrush can process a plain .usdc file and write the
        output alongside sibling pmcCodec files.
        """
        sphere_usdc = tempfile.NamedTemporaryFile(suffix='.usdc', delete=False).name
        out_dir = tempfile.mkdtemp()
        out_usdc = os.path.join(out_dir, 'sphere_crushed.usdc')

        self._make_sphere_stage(sphere_usdc)

        usdcrush_cmd = ['usdcrush', sphere_usdc, '-o', out_usdc]
        result = subprocess.run(usdcrush_cmd, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0,
                         f"usdcrush failed: {result.stderr}")
        self.assertTrue(os.path.exists(out_usdc),
                        "Output .usdc file should exist")

        pmc_file = os.path.join(out_dir, 'pmcCodec', '0.pmc')
        self.assertTrue(os.path.exists(pmc_file),
                        f"PMC sibling file should exist at {pmc_file}")

        # Verify the output stage references the PMC file
        stage = Usd.Stage.Open(out_usdc)
        self.assertIsNotNone(stage, "Output stage should be openable")
        sphere_prim = stage.GetPrimAtPath('/Sphere')
        self.assertTrue(sphere_prim.IsValid(), "/Sphere prim should exist")
        refs = sphere_prim.GetMetadata('references')
        self.assertIsNotNone(refs, "/Sphere should have references metadata")

    def test_CrushUsdaToUsdz(self):
        """
        Test that usdcrush can read a .usda input and produce a .usdz output
        containing the entry layer and PMC files.
        """
        sphere_usda = tempfile.NamedTemporaryFile(suffix='.usda', delete=False).name
        out_usdz = tempfile.NamedTemporaryFile(suffix='.usdz', delete=False).name

        self._make_sphere_stage(sphere_usda)

        usdcrush_cmd = ['usdcrush', sphere_usda, '-o', out_usdz]
        result = subprocess.run(usdcrush_cmd, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0,
                         f"usdcrush failed: {result.stderr}")
        self.assertTrue(os.path.exists(out_usdz),
                        "Output .usdz file should exist")

        with zipfile.ZipFile(out_usdz, 'r') as zf:
            file_list = zf.namelist()
            self.assertGreater(len(file_list), 0,
                               "Output usdz should contain files")
            pmc_files = [f for f in file_list if f.endswith('.pmc')]
            self.assertGreater(len(pmc_files), 0,
                               "Output usdz should contain at least one .pmc file")


    def test_CrushUsdzToUsdc(self):
        """
        Test that usdcrush can read a .usdz input and write a plain .usdc
        output with PMC files as siblings.
        """
        sphere_usdc = tempfile.NamedTemporaryFile(suffix='.usdc', delete=False).name
        sphere_usdz = tempfile.NamedTemporaryFile(suffix='.usdz', delete=False).name
        out_dir = tempfile.mkdtemp()
        out_usdc = os.path.join(out_dir, 'sphere_crushed.usdc')

        self._make_sphere_stage(sphere_usdc)

        from pxr.UsdUtils import CreateNewUsdzPackage
        success = CreateNewUsdzPackage(sphere_usdc, sphere_usdz)
        self.assertTrue(success, "Failed to create input usdz package")

        usdcrush_cmd = ['usdcrush', sphere_usdz, '-o', out_usdc]
        result = subprocess.run(usdcrush_cmd, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0,
                         f"usdcrush failed: {result.stderr}")
        self.assertTrue(os.path.exists(out_usdc),
                        "Output .usdc file should exist")

        pmc_file = os.path.join(out_dir, 'pmcCodec', '0.pmc')
        self.assertTrue(os.path.exists(pmc_file),
                        f"PMC sibling file should exist at {pmc_file}")

        # Verify the output stage is loadable and references the PMC file
        stage = Usd.Stage.Open(out_usdc)
        self.assertIsNotNone(stage, "Output stage should be openable")
        sphere_prim = stage.GetPrimAtPath('/Sphere')
        self.assertTrue(sphere_prim.IsValid(), "/Sphere prim should exist")
        refs = sphere_prim.GetMetadata('references')
        self.assertIsNotNone(refs, "/Sphere should have references metadata")


if __name__ == '__main__':
    unittest.main()
