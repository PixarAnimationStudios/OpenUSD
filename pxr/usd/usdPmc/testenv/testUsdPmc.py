#!/pxrpythonsubst
#
# Copyright 2025 Apple
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd, UsdGeom, UsdShade, Sdf, Vt, Gf, UsdPmc
import unittest
import os
import zipfile
import math
import base64
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
        Test that UsdPmcMeshEncoder can process a usdz file containing a
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

        # Encode using the PMC API directly
        encoder = UsdPmc.UsdPmcMeshEncoder()
        result = encoder.EncodeStage(sphere_usdz, sphere_crushed_usdz)
        self.assertTrue(result, "UsdPmcMeshEncoder.EncodeStage failed")
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
        Test that UsdPmcMeshEncoder can process a plain .usdc file and write
        the output alongside sibling pmcCodec files.
        """
        sphere_usdc = tempfile.NamedTemporaryFile(suffix='.usdc', delete=False).name
        out_dir = tempfile.mkdtemp()
        out_usdc = os.path.join(out_dir, 'sphere_crushed.usdc')

        self._make_sphere_stage(sphere_usdc)

        encoder = UsdPmc.UsdPmcMeshEncoder()
        result = encoder.EncodeStage(sphere_usdc, out_usdc)
        self.assertTrue(result, "UsdPmcMeshEncoder.EncodeStage failed")
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
        Test that UsdPmcMeshEncoder can read a .usda input and produce a
        .usdz output containing the entry layer and PMC files.
        """
        sphere_usda = tempfile.NamedTemporaryFile(suffix='.usda', delete=False).name
        out_usdz = tempfile.NamedTemporaryFile(suffix='.usdz', delete=False).name

        self._make_sphere_stage(sphere_usda)

        encoder = UsdPmc.UsdPmcMeshEncoder()
        result = encoder.EncodeStage(sphere_usda, out_usdz)
        self.assertTrue(result, "UsdPmcMeshEncoder.EncodeStage failed")
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
        Test that UsdPmcMeshEncoder can read a .usdz input and write a plain
        .usdc output with PMC files as siblings.
        """
        sphere_usdc = tempfile.NamedTemporaryFile(suffix='.usdc', delete=False).name
        sphere_usdz = tempfile.NamedTemporaryFile(suffix='.usdz', delete=False).name
        out_dir = tempfile.mkdtemp()
        out_usdc = os.path.join(out_dir, 'sphere_crushed.usdc')

        self._make_sphere_stage(sphere_usdc)

        from pxr.UsdUtils import CreateNewUsdzPackage
        success = CreateNewUsdzPackage(sphere_usdc, sphere_usdz)
        self.assertTrue(success, "Failed to create input usdz package")

        encoder = UsdPmc.UsdPmcMeshEncoder()
        result = encoder.EncodeStage(sphere_usdz, out_usdc)
        self.assertTrue(result, "UsdPmcMeshEncoder.EncodeStage failed")
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


    def test_ShortNamedAndIntPrimvarsEncode(self):
        """
        Regression for the GuessAttributeType out-of-range read on primvars
        whose name is shorter than the "_uv" suffix (e.g. "T"), and for
        GetMinMax integer-buffer handling. A mesh carrying such primvars must
        encode successfully and produce a PMC file rather than silently
        failing.
        """
        sphere_usdc = tempfile.NamedTemporaryFile(
            suffix='.usdc', delete=False).name
        out_dir = tempfile.mkdtemp()
        out_usdc = os.path.join(out_dir, 'out.usdc')

        stage = self._make_sphere_stage(sphere_usdc)
        mesh = UsdGeom.Mesh.Get(stage, '/Sphere')
        num_points = len(mesh.GetPointsAttr().Get())

        pv_api = UsdGeom.PrimvarsAPI(mesh.GetPrim())
        # Name shorter than "_uv" - previously underflowed in
        # GuessAttributeType and threw std::out_of_range.
        short_pv = pv_api.CreatePrimvar(
            'T', Sdf.ValueTypeNames.FloatArray, UsdGeom.Tokens.vertex)
        short_pv.Set(Vt.FloatArray([0.5] * num_points))
        # Integer primvar - exercises the integer branch of GetMinMax, which
        # previously returned an empty (degenerate) min/max.
        int_pv = pv_api.CreatePrimvar(
            'segid', Sdf.ValueTypeNames.IntArray, UsdGeom.Tokens.vertex)
        int_pv.Set(Vt.IntArray([i % 4 for i in range(num_points)]))
        stage.Save()

        encoder = UsdPmc.UsdPmcMeshEncoder()
        result = encoder.EncodeStage(sphere_usdc, out_usdc)
        self.assertTrue(result, "Encoding with short/int primvars should "
                                "succeed")
        self.assertTrue(
            os.path.exists(os.path.join(out_dir, 'pmcCodec', '0.pmc')),
            "A PMC file should be produced for the mesh")


    def test_GeomSubsetRoundTrip(self):
        """
        Regression for the FACE_GROUP subset decode path and its bounds
        checks. A mesh with UsdGeomSubsets must encode and decode back with
        the subset names and per-subset membership preserved.
        """
        sphere_usdc = tempfile.NamedTemporaryFile(
            suffix='.usdc', delete=False).name
        out_dir = tempfile.mkdtemp()
        out_usdc = os.path.join(out_dir, 'out.usdc')

        stage = self._make_sphere_stage(sphere_usdc)
        mesh = UsdGeom.Mesh.Get(stage, '/Sphere')
        num_faces = len(mesh.GetFaceVertexCountsAttr().Get())
        half = num_faces // 2
        UsdGeom.Subset.CreateGeomSubset(
            mesh, 'lower', UsdGeom.Tokens.face,
            Vt.IntArray(list(range(half))))
        UsdGeom.Subset.CreateGeomSubset(
            mesh, 'upper', UsdGeom.Tokens.face,
            Vt.IntArray(list(range(half, num_faces))))
        stage.Save()

        encoder = UsdPmc.UsdPmcMeshEncoder()
        self.assertTrue(encoder.EncodeStage(sphere_usdc, out_usdc),
                        "UsdPmcMeshEncoder.EncodeStage failed")
        pmc_file = os.path.join(out_dir, 'pmcCodec', '0.pmc')
        self.assertTrue(os.path.exists(pmc_file), "PMC file should exist")

        # Open the PMC file directly through the file format plugin to decode.
        decoded = Usd.Stage.Open(pmc_file)
        self.assertIsNotNone(decoded, "Decoded PMC stage should be openable")
        decoded_mesh = UsdGeom.Mesh(decoded.GetDefaultPrim())
        self.assertTrue(decoded_mesh, "Decoded default prim should be a mesh")

        subsets = UsdGeom.Subset.GetAllGeomSubsets(decoded_mesh)
        by_name = {s.GetPrim().GetName(): len(s.GetIndicesAttr().Get())
                   for s in subsets}
        self.assertEqual(sorted(by_name.keys()), ['lower', 'upper'],
                         f"Subset names should round-trip, got {by_name}")
        self.assertEqual(by_name['lower'], half,
                         "lower subset face count mismatch")
        self.assertEqual(by_name['upper'], num_faces - half,
                         "upper subset face count mismatch")


    def test_CrushUsdzWithTexturePreservesTextures(self):
        """
        Regression for resource loss when encoding a .usdz containing texture
        resources to a non-.usdz output. The referenced textures must be
        copied alongside the output so the resulting layer still resolves
        (previously they were left in the temp dir and deleted).
        """
        work_dir = tempfile.mkdtemp()
        sphere_usda = os.path.join(work_dir, 'sphere.usda')
        texture_png = os.path.join(work_dir, 'texture.png')
        in_usdz = os.path.join(work_dir, 'sphere.usdz')

        # A minimal valid 1x1 PNG.
        with open(texture_png, 'wb') as f:
            f.write(base64.b64decode(
                'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR4'
                '2mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg=='))

        # Build a sphere that references the texture via a UsdUVTexture
        # shader, so usdz packaging bundles the image as a dependency.
        stage = self._make_sphere_stage(sphere_usda)
        shader = UsdShade.Shader.Define(stage, '/Sphere/Texture')
        shader.CreateIdAttr('UsdUVTexture')
        shader.CreateInput('file', Sdf.ValueTypeNames.Asset).Set(
            './texture.png')
        stage.Save()

        from pxr.UsdUtils import CreateNewUsdzPackage
        self.assertTrue(CreateNewUsdzPackage(sphere_usda, in_usdz),
                        "Failed to create textured usdz package")

        # Identify the texture resources bundled in the input usdz.
        with zipfile.ZipFile(in_usdz, 'r') as zf:
            resources = [n for n in zf.namelist()
                         if os.path.splitext(n)[1].lower()
                         in ('.png', '.jpg', '.jpeg')]
        self.assertTrue(resources,
                        "Input usdz should contain a texture resource")

        out_dir = tempfile.mkdtemp()
        out_usdc = os.path.join(out_dir, 'sphere_crushed.usdc')
        encoder = UsdPmc.UsdPmcMeshEncoder()
        self.assertTrue(encoder.EncodeStage(in_usdz, out_usdc),
                        "UsdPmcMeshEncoder.EncodeStage failed")
        self.assertTrue(os.path.exists(out_usdc),
                        "Output .usdc file should exist")

        # Every texture referenced by the output must be copied as a sibling
        # so the exported layer's asset paths continue to resolve.
        for res in resources:
            self.assertTrue(
                os.path.exists(os.path.join(out_dir, res)),
                f"Referenced texture '{res}' was not copied next to the "
                f"output")


    def test_InvalidTopologySkipped(self):
        """
        A mesh whose topology fails UsdGeomMesh.ValidateTopology (e.g. a face
        vertex index outside the point range) must be skipped by CanEncode
        rather than handed to the encoder.
        """
        stage_path = tempfile.NamedTemporaryFile(
            suffix='.usdc', delete=False).name
        out_dir = tempfile.mkdtemp()
        out_usdc = os.path.join(out_dir, 'out.usdc')

        stage = Usd.Stage.CreateNew(stage_path)
        mesh = UsdGeom.Mesh.Define(stage, '/Bad')
        mesh.CreatePointsAttr().Set(Vt.Vec3fArray([
            Gf.Vec3f(0, 0, 0), Gf.Vec3f(1, 0, 0), Gf.Vec3f(0, 1, 0)]))
        mesh.CreateFaceVertexCountsAttr().Set(Vt.IntArray([3]))
        # Index 9 is out of range for a 3-point mesh.
        mesh.CreateFaceVertexIndicesAttr().Set(Vt.IntArray([0, 1, 9]))
        stage.Save()

        encoder = UsdPmc.UsdPmcMeshEncoder()
        result = encoder.EncodeStage(stage_path, out_usdc)
        self.assertTrue(
            result, "EncodeStage should succeed even when a mesh is skipped")
        self.assertFalse(
            os.path.exists(os.path.join(out_dir, 'pmcCodec', '0.pmc')),
            "A mesh with invalid topology should not produce a PMC file")


if __name__ == '__main__':
    unittest.main()
