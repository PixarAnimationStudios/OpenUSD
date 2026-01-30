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


class TestUsdCrush(unittest.TestCase):

    def test_CrushUsdz(self):
        """
        Test that usdcrush can process a usdz file containing a
        procedurally generated polygon sphere and verify the output file
        structure.
        """
        # Create temporary paths for test files
        sphere_usdc = tempfile.NamedTemporaryFile(suffix='.usdc', delete=False).name
        sphere_usdz = tempfile.NamedTemporaryFile(suffix='.usdz', delete=False).name
        sphere_crushed_usdz = tempfile.NamedTemporaryFile(suffix='.usdz', delete=False).name
        
        # Create a stage with a procedurally generated polygon sphere mesh
        stage = Usd.Stage.CreateNew(sphere_usdc)
        mesh = UsdGeom.Mesh.Define(stage, '/Sphere')
        
        # Generate a simple UV sphere with procedural geometry
        # Parameters for sphere generation
        u_segments = 16  # longitude segments
        v_segments = 8   # latitude segments
        radius = 1.0
        
        points = []
        normals = []
        face_vertex_counts = []
        face_vertex_indices = []
        
        # Generate vertices
        for v in range(v_segments + 1):
            theta = math.pi * v / v_segments  # 0 to pi
            for u in range(u_segments):
                phi = 2.0 * math.pi * u / u_segments  # 0 to 2pi
                
                x = radius * math.sin(theta) * math.cos(phi)
                y = radius * math.cos(theta)
                z = radius * math.sin(theta) * math.sin(phi)
                
                points.append(Gf.Vec3f(x, y, z))
                # Normals point outward from center
                normals.append(Gf.Vec3f(x, y, z).GetNormalized())
        
        # Generate faces (quads, except at poles where we use triangles)
        for v in range(v_segments):
            for u in range(u_segments):
                # Current vertex indices
                i0 = v * u_segments + u
                i1 = v * u_segments + (u + 1) % u_segments
                i2 = (v + 1) * u_segments + (u + 1) % u_segments
                i3 = (v + 1) * u_segments + u
                
                # Create quad face
                face_vertex_counts.append(4)
                face_vertex_indices.extend([i0, i1, i2, i3])
        
        # Set mesh attributes
        mesh.CreatePointsAttr().Set(Vt.Vec3fArray(points))
        mesh.CreateNormalsAttr().Set(Vt.Vec3fArray(normals))
        mesh.CreateFaceVertexCountsAttr().Set(Vt.IntArray(face_vertex_counts))
        mesh.CreateFaceVertexIndicesAttr().Set(Vt.IntArray(face_vertex_indices))
        
        stage.Save()
        
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


if __name__ == '__main__':
    unittest.main()
