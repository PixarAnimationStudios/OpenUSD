set(PXR_PACKAGE OP_gusd)

pxr_plugin(${PXR_PACKAGE}

    LIBRARIES
        ar 
        gf 
        js 
        kind 
        plug 
        sdf 
        tf
        usd
        usdGeom
        usdRi
        usdShade
        usdUtils
        gusd

    INCLUDE_DIRS
        ${HOUDINI_INCLUDE_DIRS}

    PRIVATE_CLASSES
        OBJ_usdcamera
        OP_Utils
        ROP_usdoutput
        SOP_usdimport
        SOP_usdunpack

    CPPFILES
        plugin.cpp
)

install( 
    DIRECTORY 
        help
    DESTINATION
        ${PXR_INSTALL_SUBDIR}
)

install(
    FILES
        ROP_usdcoalesce.otl
        ROP_usdlayer.otl
        ROP_usdreference.otl
        SOP_camerafrustum.otl
        SOP_usdbindproxy.otl
        SOP_usdexportattributes.hda
        SOP_usdinstanceprototypes.hda
        SOP_usdretime.hda
    DESTINATION
        ${PXR_INSTALL_SUBDIR}/otls
)

install(
    FILES
        ROP_usdcoalesce.py
        ROP_usdlayer.py
        ROP_usdreference.py
    DESTINATION
        ${PXR_INSTALL_SUBDIR}/soho/python2.7
)

install(
    FILES
        UsdImport.pypanel
    DESTINATION
        ${PXR_INSTALL_SUBDIR}/python_panels
)

install(
    FILES
        obj/pixar-usdcamera.py
    DESTINATION
        ${PXR_INSTALL_SUBDIR}/scripts/obj
)

