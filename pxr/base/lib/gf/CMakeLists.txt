# Don't get the wrong half.h!
set(CMAKE_INCLUDE_CURRENT_DIR OFF)

set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE gf)

pxr_shared_library(gf
    LIBRARIES
        arch
        tf
        ${OPENEXR_Half_LIBRARY}

    INCLUDE_DIRS
        ${OPENEXR_INCLUDE_DIRS}

    PUBLIC_CLASSES
        bbox3d
        camera
        colorRamp
        frustum
        gamma
        half
        homogeneous
        interval
        line
        line2d
        lineSeg
        lineSeg2d
        math
        matrixData
        matrix2d
        matrix2f
        matrix3f
        matrix3d
        matrix4f
        matrix4d
        multiInterval
        plane
        pyBufferUtils
        quatd
        quatf
        quath
        quaternion
        range1d
        range1f
        range2d
        range2f
        range3d
        range3f
        ray
        rect2i
        rgb
        rgba
        rotation
        size2
        size3
        transform
        vec2d
        vec2f
        vec2h
        vec2i
        vec3d
        vec3f
        vec3h
        vec3i
        vec4d
        vec4f
        vec4h
        vec4i

    PUBLIC_HEADERS
        api.h
        declare.h
        limits.h
        traits.h

    PRIVATE_CLASSES
        ostreamHelpers

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapBBox3d.cpp
        wrapCamera.cpp
        wrapColorRamp.cpp
        wrapFrustum.cpp
        wrapGamma.cpp
        wrapHalf.cpp
        wrapHomogeneous.cpp
        wrapInterval.cpp
        wrapLimits.cpp
        wrapLine.cpp
        wrapLineSeg.cpp
        wrapMath.cpp
        wrapMatrix2d.cpp
        wrapMatrix2f.cpp
        wrapMatrix3f.cpp
        wrapMatrix3d.cpp
        wrapMatrix4f.cpp
        wrapMatrix4d.cpp
        wrapMultiInterval.cpp
        wrapPlane.cpp
        wrapQuaternion.cpp
        wrapQuatd.cpp
        wrapQuatf.cpp
        wrapQuath.cpp
        wrapRange1d.cpp
        wrapRange1f.cpp
        wrapRange2d.cpp
        wrapRange2f.cpp
        wrapRange3d.cpp
        wrapRange3f.cpp
        wrapRay.cpp
        wrapRGB.cpp
        wrapRGBA.cpp
        wrapRect2i.cpp
        wrapRotation.cpp
        wrapSize2.cpp
        wrapSize3.cpp
        wrapTransform.cpp
        wrapVec2d.cpp
        wrapVec2f.cpp
        wrapVec2h.cpp
        wrapVec2i.cpp
        wrapVec3d.cpp
        wrapVec3f.cpp
        wrapVec3h.cpp
        wrapVec3i.cpp
        wrapVec4d.cpp
        wrapVec4f.cpp
        wrapVec4h.cpp
        wrapVec4i.cpp

    PYTHON_FILES
        __init__.py
)

# gfGenCode --validate ensures that the generated GfVec, GfMatrix, GfRange, etc.
# source files are up to date with the contents of the templates used to
# generate them. If not, a diff displaying the difference will be printed to
# stdout and the build will fail.
if (WIN32)
    message(WARNING "Skipping validation of sdf generated code because of Windows")
elseif (NOT ${PXR_VALIDATE_GENERATED_CODE})
    message(WARNING "Skipping validation of gf generated code because PXR_VALIDATE_GENERATED_CODE=OFF")
elseif(NOT JINJA2_FOUND)
    message(WARNING "Skipping validation of gf generated code because of missing dependency: jinja2")
else()
    add_custom_command(
        TARGET gf 
        PRE_BUILD 
        COMMENT "Validating gf generated code..."
        COMMAND
            ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gfGenCode.py 
                --srcDir ${CMAKE_CURRENT_SOURCE_DIR}
                --validate 
    )
endif()
        
if(UNIX)        
    pxr_build_test(testGfHardToReach
        LIBRARIES
            gf
        CPPFILES
            testenv/testGfHardToReach.cpp
    ) 

    pxr_test_scripts(
        testenv/testGfBBox3d.py
        testenv/testGfCamera.py
        testenv/testGfColorRamp.py
        testenv/testGfDecomposeRotation.py
        testenv/testGfFrustum.py
        testenv/testGfGamma.py
        testenv/testGfHomogeneous.py
        testenv/testGfInterval.py
        testenv/testGfLine.py
        testenv/testGfLineSeg.py
        testenv/testGfMath.py
        testenv/testGfMatrix.py
        testenv/testGfMultiInterval.py
        testenv/testGfPlane.py
        testenv/testGfQuaternion.py
        testenv/testGfRange.py
        testenv/testGfRay.py
        testenv/testGfRect2i.py
        testenv/testGfRGB.py
        testenv/testGfRotation.py
        testenv/testGfSize.py
        testenv/testGfTransform.py
        testenv/testGfVec.py
    )

    pxr_register_test(testGfBBox3d
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfBBox3d"
    )
    pxr_register_test(testGfColorRamp
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfColorRamp"
    )
    pxr_register_test(testGfDecomposeRotation
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfDecomposeRotation"
    )
    pxr_register_test(testGfFrustum
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfFrustum"
    )
    pxr_register_test(testGfGamma
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfGamma"
    )
    pxr_register_test(testGfHardToReach
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfHardToReach"
    )
    pxr_register_test(testGfHomogeneous
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfHomogeneous"
    )
    pxr_register_test(testGfInterval
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfInterval"
    )
    pxr_register_test(testGfMultiInterval
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfMultiInterval"
    )
    pxr_register_test(testGfLine
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfLine"
    )
    pxr_register_test(testGfLineSeg
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfLineSeg"
    )
    pxr_register_test(testGfMath
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfMath"
    )
    pxr_register_test(testGfMatrix
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfMatrix"
    )
    pxr_register_test(testGfPlane
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfPlane"
    )
    pxr_register_test(testGfQuaternion
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfQuaternion"
    )
    pxr_register_test(testGfRange
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfRange"
    )
    pxr_register_test(testGfRay
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfRay"
    )
    pxr_register_test(testGfRect2i
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfRect2i"
    )
    pxr_register_test(testGfRGB
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfRGB"
    )
    pxr_register_test(testGfRotation
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfRotation"
    )
    pxr_register_test(testGfSize
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfSize"
    )
    pxr_register_test(testGfTransform
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfTransform"
    )
    pxr_register_test(testGfVec
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfVec"
    )
    pxr_register_test(testGfCamera
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testGfCamera"
    )
endif()