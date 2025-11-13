set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usd)

pxr_python_bin(usdcompress
    DEPENDENCIES
        tf
        gf
        sdf
        usd
        usdDraco
        usdGeom
)

# Checks that compression succeeds with return code of zero.
pxr_register_test(testUsdCompressExitCodeSuccess
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress a.usda -o out.usda"
    TESTENV testenv/testExitCodes
    EXPECTED_RETURN_CODE 0
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that compression fails when an invalid input file name is given.
pxr_register_test(testUsdCompressExitCodeBadInputFileName
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress bad.usda -o out.usda"
    TESTENV testenv/testExitCodes
    EXPECTED_RETURN_CODE 1
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that compression fails when an invalid command-line flag is given.
pxr_register_test(testUsdCompressExitCodeBadArgumentName
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress a.usda --bad -o out.usda"
    TESTENV testenv/testExitCodes
    EXPECTED_RETURN_CODE 2
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that compression fails when property has one custom opinion.
pxr_register_test(testUsdCompressPropertyOpinionsOneCustom
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeOneCustomOpinion.usda -o out.usda"
    TESTENV testenv/testPropertyOpinions
    EXPECTED_RETURN_CODE 1
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that compression fails when property has multiple opinions.
pxr_register_test(testUsdCompressPropertyOpinionsMultiple
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeMultipleOpinions.usda -o out.usda"
    TESTENV testenv/testPropertyOpinions
    EXPECTED_RETURN_CODE 1
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that compression finishes with flag to ignore opinion errors.
pxr_register_test(testUsdCompressPropertyOpinionsIgnoreErrors
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeMultipleOpinions.usda -o CubeMultipleOpinionsCompressed.usda --ignore_opinion_errors"
    TESTENV testenv/testPropertyOpinions
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeMultipleOpinionsCompressed.usda
    DIFF_COMPARE CubeMultipleOpinionsCompressed.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that a complex set scene can be successfully compressed.
pxr_register_test(testUsdCompressComplexSet
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress ComplexSet.usda -o ComplexSetCompressed.usda"
    TESTENV testenv/testComplexSet
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE ComplexSetCompressed.usda
    DIFF_COMPARE ComplexSetCompressed.usda.draco/ComplexSet_Geom_CubesFill_CubesFillGeom101.drc
    DIFF_COMPARE ComplexSetCompressed.usda.draco/ComplexSet_Geom_CubesFill_CubesFillGeom102.drc
    DIFF_COMPARE ComplexSetCompressed.usda.draco/ComplexSet_Geom_CubesFill_CubesFillGeom103.drc
    DIFF_COMPARE ComplexSetCompressed.usda.draco/ComplexSet_Geom_CubesHero_CubesHeroGeom101.drc
    DIFF_COMPARE ComplexSetCompressed.usda.draco/ComplexSet_Geom_CubesHero_CubesHeroGeom102.drc
    DIFF_COMPARE ComplexSetCompressed.usda.draco/ComplexSet_Geom_CubesHero_CubesHeroGeom103.drc
    DIFF_COMPARE ComplexSetCompressed.usda.draco/ComplexSet_Geom_Ref_BlueCube.drc
    DIFF_COMPARE ComplexSetCompressed.usda.draco/ComplexSet_Geom_Ref_GreenCube.drc
    DIFF_COMPARE ComplexSetCompressed.usda.draco/ComplexSet_Geom_Ref_RedCube.drc
    DIFF_COMPARE ComplexSetCompressed.usda.draco/ComplexSet_Geom_terrain.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that cube with triangles can be compressed.
pxr_register_test(testUsdCompressTriangles
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeWithTriangles.usda -o CubeWithTrianglesCompressed.usda"
    TESTENV testenv/testTriangles
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeWithTrianglesCompressed.usda
    DIFF_COMPARE CubeWithTrianglesCompressed.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that cube with quads can be compressed with quads preserved.
pxr_register_test(testUsdCompressQuads
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeWithQuads.usda -o CubeWithQuadsCompressed.usda"
    TESTENV testenv/testQuads
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeWithQuadsCompressed.usda
    DIFF_COMPARE CubeWithQuadsCompressed.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that cube with quads can be compressed with quads split into triangles.
pxr_register_test(testUsdCompressDiscardQuads
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeWithQuads.usda -o CubeWithQuadsCompressedDiscarded.usda --preserve_polygons 0"
    TESTENV testenv/testQuads
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeWithQuadsCompressedDiscarded.usda
    DIFF_COMPARE CubeWithQuadsCompressedDiscarded.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that supported generic primvars of various types and shapes are
# successfully compressed and unsupported primvars remain in USDA file.
pxr_register_test(testUsdCompressGenericPrimvars
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeWithGenericPrimvars.usda -o CubeWithGenericPrimvarsCompressed.usda"
    TESTENV testenv/testGenericPrimvars
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeWithGenericPrimvarsCompressed.usda
    DIFF_COMPARE CubeWithGenericPrimvarsCompressed.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that attributes with vertex interpolation are comporessed correctly
# when position order is discarded (default).
pxr_register_test(testUsdCompressVertexColor
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeWithVertexColor.usda -o CubeWithVertexColorCompressed.usda --discard_subdivision 1"
    TESTENV testenv/testPositionOrder
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeWithVertexColorCompressed.usda
    DIFF_COMPARE CubeWithVertexColorCompressed.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that attributes with vertex interpolation are comporessed correctly when
# position order is preserved (with --discard_subdivision 0).
pxr_register_test(testUsdCompressVertexColorPreservePositionOrder
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeWithVertexColor.usda -o CubeWithVertexColorCompressedWithPositionOrder.usda --discard_subdivision 0"
    TESTENV testenv/testPositionOrder
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeWithVertexColorCompressedWithPositionOrder.usda
    DIFF_COMPARE CubeWithVertexColorCompressedWithPositionOrder.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that attributes with face-varying interpolation are comporessed
# correctly when position order is discarded (default).
pxr_register_test(testUsdCompressCornerColor
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeWithCornerColor.usda -o CubeWithCornerColorCompressed.usda"
    TESTENV testenv/testPositionOrder
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeWithCornerColorCompressed.usda
    DIFF_COMPARE CubeWithCornerColorCompressed.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that attributes with face-varying interpolation are comporessed
# correctly when position order is preserved (with --discard_subdivision 0).
pxr_register_test(testUsdCompressCornerColorPreservePositionOrder
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeWithCornerColor.usda -o CubeWithCornerColorCompressedWithPositionOrder.usda --discard_subdivision 0"
    TESTENV testenv/testPositionOrder
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeWithCornerColorCompressedWithPositionOrder.usda
    DIFF_COMPARE CubeWithCornerColorCompressedWithPositionOrder.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that cube with holes is successfully compressed.
pxr_register_test(testUsdCompressHoles
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeWithHoles.usda -o CubeWithHolesCompressed.usda"
    TESTENV testenv/testHoles
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeWithHolesCompressed.usda
    DIFF_COMPARE CubeWithHolesCompressed.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that cube holes are discarded (with --discard_subdivision 1).
pxr_register_test(testUsdCompressDiscardHoles
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeWithHoles.usda -o CubeWithHolesCompressedDiscarded.usda --discard_subdivision 1"
    TESTENV testenv/testHoles
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeWithHolesCompressedDiscarded.usda
    DIFF_COMPARE CubeWithHolesCompressedDiscarded.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that mesh with creases is successfully compressed.
pxr_register_test(testUsdCompressCreases
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress MeshWithCreases.usda -o MeshWithCreasesCompressed.usda"
    TESTENV testenv/testCreases
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE MeshWithCreasesCompressed.usda
    DIFF_COMPARE MeshWithCreasesCompressed.usda.draco/xform_bottomPanel.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that mesh creases are discarded (with --discard_subdivision 1).
pxr_register_test(testUsdCompressDiscardCreases
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress MeshWithCreases.usda -o MeshWithCreasesCompressedDiscarded.usda --discard_subdivision 1"
    TESTENV testenv/testCreases
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE MeshWithCreasesCompressedDiscarded.usda
    DIFF_COMPARE MeshWithCreasesCompressedDiscarded.usda.draco/xform_bottomPanel.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

# Checks that cube with one time sample for color is successfully compressed.
pxr_register_test(testUsdCompressTimeSample
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/usdcompress CubeWithTimeSample.usda -o CubeWithTimeSampleCompressed.usda"
    TESTENV testenv/testTimeSample
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE CubeWithTimeSampleCompressed.usda
    DIFF_COMPARE CubeWithTimeSampleCompressed.usda.draco/Cube_Geom_Cube.drc
    PRE_PATH
        ${CMAKE_INSTALL_PREFIX}/bin
)

