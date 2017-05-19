set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE sdf)

pxr_library(sdf
    LIBRARIES
        arch
        tf
        gf
        tracelite
        vt
        work
        ar
        ${Boost_PYTHON_LIBRARY}
        ${Boost_REGEX_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        abstractData
        allowed
        assetPath
        attributeSpec
        changeBlock
        changeList
        children
        childrenPolicies
        childrenProxy
        childrenUtils
        childrenView
        cleanupEnabler
        data
        declareHandles
        fileFormat
        identity
        layer
        layerBase
        layerOffset
        layerStateDelegate
        layerTree
        layerUtils
        listProxy
        listEditor
        listEditorProxy
        listOp
        mapperArgSpec
        mapperSpec
        mapEditProxy
        mapEditor
        namespaceEdit
        notice
        path
        pathNode
        pathTable
        payload
        primSpec
        propertySpec
        proxyPolicies
        proxyTypes
        pseudoRootSpec
        pyChildrenProxy
        pyChildrenView
        pyListEditorProxy
        pyListOp
        pyListProxy
        pyMapEditProxy
        pyMarkerProxy
        pySpec
        pyUtils
        reference
        relationshipSpec
        schema
        site
        siteUtils
        spec
        specType
        textFileFormat
        textReferenceParser
        tokens
        types
        valueTypeName
        variantSetSpec
        variantSpec

        # Flex/Bison generated code
        layerIdentifier.tab
        path.tab
        textFileFormat.tab

    PUBLIC_HEADERS
        api.h
        accessorHelpers.h
        declareSpec.h
        schemaTypeRegistration.h

    PRIVATE_CLASSES
        assetPathResolver
        changeManager
        cleanupTracker
        connectionListEditor
        debugCodes
        fileFormatRegistry
        fileIO
        fileIO_Common
        layerRegistry
        listOpListEditor
        markerUtils
        parserHelpers
        parserValueContext
        pathParser
        subLayerListEditor
        textParserContext
        valueTypeRegistry
        vectorListEditor

    PRIVATE_HEADERS
        valueTypePrivate.h

    CPPFILES
        moduleDeps.cpp
        # Flex/Bison generated code
        layerIdentifier.lex.cpp
        path.lex.cpp
        textFileFormat.lex.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapArrayAssetPath.cpp
        wrapAssetPath.cpp
        wrapAttributeSpec.cpp
        wrapChangeBlock.cpp
        wrapCleanupEnabler.cpp
        wrapFileFormat.cpp
        wrapLayer.cpp
        wrapLayerBase.cpp
        wrapLayerOffset.cpp
        wrapLayerTree.cpp
        wrapMapperSpec.cpp
        wrapMapperArgSpec.cpp
        wrapNamespaceEdit.cpp
        wrapNotice.cpp
        wrapPath.cpp
        wrapPayload.cpp
        wrapPrimSpec.cpp
        wrapPropertySpec.cpp
        wrapPseudoRootSpec.cpp
        wrapRelationshipSpec.cpp
        wrapReference.cpp
        wrapSpec.cpp
        wrapTextReferenceParser.cpp
        wrapTypes.cpp
        wrapValueTypeName.cpp
        wrapVariantSpec.cpp
        wrapVariantSetSpec.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)

# Validate that the checked in generated code matches the sources. This requires
# a specific version of bison/flex to get the same results
if (NOT ${PXR_VALIDATE_GENERATED_CODE})
    message(STATUS "Skipping validation of sdf generated code because PXR_VALIDATE_GENERATED_CODE=OFF")
elseif(NOT BISON_FOUND)
    message(STATUS "Skipping validation of sdf generated code because of missing dependency: bison")
elseif(NOT FLEX_FOUND)
    message(STATUS "Skipping validation of sdf generated code because of missing dependency: flex")
else()
    add_custom_command(
        TARGET sdf
        PRE_BUILD 
        COMMENT "Validating sdf generated code..."
        COMMAND
            ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/sdfGenAsciiParsers.py 
            --srcDir ${CMAKE_CURRENT_SOURCE_DIR}
            --bison ${BISON_EXECUTABLE}
            --flex ${FLEX_EXECUTABLE}
            --bases textFileFormat path layerIdentifier
            --validate
    )
endif()

pxr_test_scripts(
    testenv/testSdfAttributeBlocking.py
    testenv/testSdfBatchNamespaceEdit.py
    testenv/testSdfCustomLayerData.py
    testenv/testSdfParsing.py
    testenv/testSdfPath.py
    testenv/testSdfPath2.py
    testenv/testSdfPath2Construct.py
    testenv/testSdfPayload.py
    testenv/testSdfReference.py
    testenv/testSdfSpecHash.py
    testenv/testSdfTextReferenceParser.py
    testenv/testSdfTypes.py
)

pxr_build_test(testSdfAttributeBlocking_Cpp
    LIBRARIES
        sdf
    CPPFILES
        testenv/testSdfAttributeBlocking.cpp
) 

pxr_build_test(testSdfHardToReach
    LIBRARIES
        sdf
    CPPFILES
        testenv/testSdfHardToReach.cpp
)

pxr_build_test(testSdfMetaDataPlugInfo
    LIBRARIES
        sdf
    CPPFILES
        testenv/testSdfMetaDataPlugInfo.cpp
)

pxr_build_test(testSdfPathParser
    LIBRARIES
        sdf
    CPPFILES
        testenv/testSdfPathParser.cpp
)

pxr_build_test(testSdfPathTable
    LIBRARIES
        sdf
    CPPFILES
        testenv/testSdfPathTable.cpp
)

pxr_build_test(testSdfPathThreading
    LIBRARIES
        sdf
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
    CPPFILES
        testenv/testSdfPathThreading.cpp
)

pxr_install_test_dir(
    SRC testenv/testSdfBatchNamespaceEdit.testenv
    DEST testSdfBatchNamespaceEdit/testSdfBatchNamespaceEdit.testenv
)

pxr_install_test_dir(
    SRC testenv/testSdfCustomLayerData.testenv
    DEST testSdfCustomLayerData/testSdfCustomLayerData.testenv
)

pxr_install_test_dir(
    SRC testenv/testSdfMetaDataPlugInfo.testenv
    DEST testSdfMetaDataPlugInfo/testSdfMetaDataPlugInfo.testenv
)

pxr_install_test_dir(
    SRC testenv/testSdfParsingOld.testenv
    DEST testSdfParsingOld/testSdfParsingOld.testenv
)

pxr_install_test_dir(
    SRC testenv/testSdfParsingOld.testenv
    DEST testSdfParsingNew/testSdfParsingNew.testenv
)

pxr_install_test_dir(
    SRC testenv/testSdfParsing.testenv
    DEST testSdfParsing/testSdfParsing.testenv
)

pxr_install_test_dir(
    SRC testenv/testSdfTextReferenceParser.testenv
    DEST testSdfTextReferenceParser
)

pxr_register_test(testSdfAttributeBlocking
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfAttributeBlocking"
)

pxr_register_test(testSdfAttributeBlocking_Cpp
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfAttributeBlocking_Cpp"
)

pxr_register_test(testSdfBatchNamespaceEdit
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfBatchNamespaceEdit"
)

pxr_register_test(testSdfCustomLayerData
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfCustomLayerData"
)

pxr_register_test(testSdfHardToReach
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfHardToReach"
)

pxr_register_test(testSdfTextReferenceParser
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfTextReferenceParser"
    STDOUT_REDIRECT result.txt
    DIFF_COMPARE result.txt
)

pxr_register_test(testSdfMetaDataPlugInfo
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfMetaDataPlugInfo"
)

pxr_register_test(testSdfParsingNew
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfParsing"
    ENV
        SDF_CONVERT_TO_NEW_TYPENAMES=1
)

pxr_register_test(testSdfParsing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfParsing"
)

pxr_register_test(testSdfParsingOld
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfParsing"
    ENV
        SDF_WRITE_OLD_TYPENAMES=1
)

pxr_register_test(testSdfPath
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfPath"
)

pxr_register_test(testSdfPath2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfPath2"
)
pxr_register_test(testSdfPath2Construct
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfPath2Construct"
)

pxr_register_test(testSdfPathParser
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfPathParser"
)

pxr_register_test(testSdfPathTable
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfPathTable"
)

pxr_register_test(testSdfPathThreading
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfPathThreading"
)
pxr_register_test(testSdfPayload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfPayload"
)
pxr_register_test(testSdfReference
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfReference"
)
pxr_register_test(testSdfSpecHash
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfSpecHash"
)
pxr_register_test(testSdfTypes
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testSdfTypes"
)
