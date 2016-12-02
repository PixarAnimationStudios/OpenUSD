set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE pcp)

pxr_shared_library(pcp
    LIBRARIES
        tf
        tracelite
        vt
        sdf
        work
        ar
        ${Boost_PYTHON_LIBRARY}
        ${TBB_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        arc
        cache
        changes
        composeSite
        dependency
        diagnostic
        errors
        instanceKey
        iterator
        layerPrefetchRequest
        layerStack
        layerStackIdentifier
        mapExpression
        mapFunction
        namespaceEdits
        node
        pathTranslation
        payloadContext
        payloadDecorator
        primIndex
        propertyIndex
        pyUtils
        site
        strengthOrdering
        targetIndex
        types

    PRIVATE_CLASSES
        debugCodes
        dependencies
        instancing
        layerStackRegistry
        node_Iterator
        primIndex_Graph
        primIndex_StackFrame
        statistics
        utils

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapCache.cpp
        wrapDependency.cpp
        wrapErrors.cpp
        wrapInstanceKey.cpp
        wrapPrimIndex.cpp
        wrapLayerStack.cpp
        wrapLayerStackIdentifier.cpp
        wrapMapExpression.cpp
        wrapMapFunction.cpp
        wrapNode.cpp
        wrapPathTranslation.cpp
        wrapPropertyIndex.cpp
        wrapPayloadContext.cpp
        wrapPayloadDecorator.cpp
        wrapSite.cpp
        wrapTestChangeProcessor.cpp
        wrapTypes.cpp

    PYTHON_FILES
        __init__.py
)
