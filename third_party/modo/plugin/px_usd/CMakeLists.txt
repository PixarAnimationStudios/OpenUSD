set(PXR_PACKAGE px_usd)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        usd
        usdGeom
        usdUtils
        ${Boost_LIBRARIES}
        ${MODO_LIBRARIES}

    INCLUDE_DIRS
        ${MODO_INCLUDE_DIRS}

    PRIVATE_CLASSES
        px_modoExportUSD
)

install(FILES index.cfg
    DESTINATION ${PXR_INSTALL_SUBDIR}/Kits
)
