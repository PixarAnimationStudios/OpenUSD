set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE sdf)

pxr_shared_library(sdf
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
        subLayerListEditor
        textParserContext
        valueTypeRegistry
        vectorListEditor

    PRIVATE_HEADERS
        pathParser.h
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
    message(WARNING "Skipping validation of sdf generated code because PXR_VALIDATE_GENERATED_CODE=OFF")
elseif(NOT BISON_FOUND)
    message(WARNING "Skipping validation of sdf generated code because of missing dependency: bison")
elseif(NOT FLEX_FOUND)
    message(WARNING "Skipping validation of sdf generated code because of missing dependency: flex")
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
