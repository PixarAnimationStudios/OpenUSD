set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE pcp)

pxr_library(pcp
    LIBRARIES
        tf
        trace
        vt
        sdf
        work
        ar
        TBB::tbb
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        arc
        cache
        changes
        composeSite
        dependency
        dependentNamespaceEditUtils
        dynamicFileFormatContext
        dynamicFileFormatDependencyData
        dynamicFileFormatInterface
        errors
        expressionVariables
        expressionVariablesDependencyData
        expressionVariablesSource
        instanceKey
        iterator
        layerRelocatesEditBuilder
        layerStack
        layerStackIdentifier
        mapExpression
        mapFunction
        namespaceEdits
        node
        pathTranslation
        primIndex
        propertyIndex
        site
        strengthOrdering
        targetIndex
        types

    PYTHON_PUBLIC_CLASSES
        pyUtils

    PRIVATE_CLASSES
        debugCodes
        dependencies
        diagnostic
        instancing
        layerStackRegistry
        node_Iterator
        primIndex_Graph
        primIndex_StackFrame
        statistics
        utils

    PRIVATE_HEADERS
        traversalCache.h

    PYMODULE_CPPFILES
        module.cpp
        wrapCache.cpp
        wrapDependency.cpp
        wrapDynamicFileFormatDependencyData.cpp
        wrapErrors.cpp
        wrapExpressionVariables.cpp
        wrapExpressionVariablesSource.cpp
        wrapInstanceKey.cpp
        wrapPrimIndex.cpp
        wrapLayerRelocatesEditBuilder.cpp
        wrapLayerStack.cpp
        wrapLayerStackIdentifier.cpp
        wrapMapExpression.cpp
        wrapMapFunction.cpp
        wrapNode.cpp
        wrapPathTranslation.cpp
        wrapPropertyIndex.cpp
        wrapSite.cpp
        wrapTestChangeProcessor.cpp
        wrapTypes.cpp

    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        overview.dox
)

pxr_test_scripts(
    testenv/testPcpCache.py
    testenv/testPcpChanges.py
    testenv/testPcpExpressionComposition.py
    testenv/testPcpExpressionVariables.py
    testenv/testPcpExpressionVariablesSource.py
    testenv/testPcpInstanceKey.py
    testenv/testPcpLayerMuting.py
    testenv/testPcpDependencies.py
    testenv/testPcpPathTranslation.py
    testenv/testPcpPrimIndex.py
    testenv/testPcpDynamicFileFormatPlugin.py
    testenv/testPcpMapFunction.py
    testenv/testPcpOwner.py
    testenv/testPcpRegressionBugs_bug70951.py
    testenv/testPcpRegressionBugs_bug82180.py
    testenv/testPcpRegressionBugs_bug90508.py
    testenv/testPcpRegressionBugs_bug90706.py
    testenv/testPcpRegressionBugs_bug92955.py
    testenv/testPcpRegressionBugs_bug101300.py
    testenv/testPcpLayerRelocatesEditBuilder.py
    testenv/testPcpResolvedPathChange.py
    testenv/testPcpStreamingLayerReload.py
    testenv/testPcpCompositionResults.py
    testenv/testPcpNegativeOffsetScale.py
)

pxr_build_test_shared_lib(TestPcpDynamicFileFormatPlugin
    INSTALL_PREFIX PcpPlugins
    LIBRARIES
        tf
        pcp
        sdf
        
    CPPFILES
        testenv/TestPcpDynamicFileFormatPlugin.cpp
)

pxr_build_test_shared_lib(TestPcpAssetPathDynamicFileFormatPlugin
    INSTALL_PREFIX PcpPlugins
    LIBRARIES
        tf
        pcp
        sdf
        
    CPPFILES
        testenv/TestPcpAssetPathDynamicFileFormatPlugin.cpp
)

pxr_build_test_shared_lib(TestPcpStreamingLayerReload
    INSTALL_PREFIX PcpPlugins
    LIBRARIES
        tf
        pcp
        sdf
        
    CPPFILES
        testenv/TestPcpStreamingLayerReload.cpp
)

pxr_build_test(testPcpHardToReach
    LIBRARIES
        tf 
        pcp 
        sdf

    CPPFILES
        testenv/testPcpHardToReach.cpp
)

pxr_build_test(testPcpIterator
    LIBRARIES
        tf 
        pcp 
        sdf

    CPPFILES
        testenv/testPcpIterator.cpp
)

pxr_build_test(testPcpMapExpression
    LIBRARIES
        tf 
        pcp
        sdf

    CPPFILES
        testenv/testPcpMapExpression.cpp
)

pxr_build_test(testPcpPathTranslation_HardToReach
    LIBRARIES
        tf 
        pcp 
        sdf

    CPPFILES
        testenv/testPcpPathTranslation_HardToReach.cpp
)

# Enable self-consistency checks for all tests in this library.
set(PXR_TEST_ENV_VARS PCP_ENABLE_CONSISTENCY_CHECKS=1)

pxr_register_test(testPcpMuseum_BasicInstancing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicInstancing/root.usda"
    TESTENV testenv/testPcpMuseum_BasicInstancing.testenv
    STDOUT_REDIRECT compositionResults_BasicInstancing.txt
    DIFF_COMPARE compositionResults_BasicInstancing.txt 
    ENV 
        PCP_OVERRIDE_INSTANCEABLE=1
)

pxr_register_test(testPcpMuseum_BasicInstancingAndNestedInstances
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicInstancingAndNestedInstances/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicInstancingAndNestedInstances.testenv
    STDOUT_REDIRECT compositionResults_BasicInstancingAndNestedInstances.txt
    DIFF_COMPARE compositionResults_BasicInstancingAndNestedInstances.txt 
    ENV 
        PCP_OVERRIDE_INSTANCEABLE=1
)

pxr_register_test(testPcpMuseum_BasicInstancingAndVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicInstancingAndVariants/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicInstancingAndVariants.testenv
    STDOUT_REDIRECT compositionResults_BasicInstancingAndVariants.txt
    DIFF_COMPARE compositionResults_BasicInstancingAndVariants.txt 
    ENV 
        PCP_OVERRIDE_INSTANCEABLE=1
)

pxr_register_test(testPcpMuseum_BasicPayloadDiamond
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --payloads ^(?!/RootUnloaded) BasicPayloadDiamond/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicPayloadDiamond.testenv
    STDOUT_REDIRECT compositionResults_BasicPayloadDiamond.txt
    DIFF_COMPARE compositionResults_BasicPayloadDiamond.txt 
    ENV 
        PCP_OVERRIDE_INSTANCEABLE=1
)

pxr_register_test(testPcpMuseum_BasicReference
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicReference/root.usda --session BasicReference/session.usda" 
    TESTENV testenv/testPcpMuseum_BasicReference.testenv
    STDOUT_REDIRECT compositionResults_BasicReference.txt
    DIFF_COMPARE compositionResults_BasicReference.txt 
)

pxr_register_test(testPcpMuseum_BasicReferenceAndClass
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicReferenceAndClass/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicReferenceAndClass.testenv
    STDOUT_REDIRECT compositionResults_BasicReferenceAndClass.txt
    DIFF_COMPARE compositionResults_BasicReferenceAndClass.txt 
)

pxr_register_test(testPcpMuseum_BasicReferenceAndClassDiamond
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicReferenceAndClassDiamond/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicReferenceAndClassDiamond.testenv
    STDOUT_REDIRECT compositionResults_BasicReferenceAndClassDiamond.txt
    DIFF_COMPARE compositionResults_BasicReferenceAndClassDiamond.txt 
)

pxr_register_test(testPcpMuseum_BasicAncestralReference
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicAncestralReference/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicAncestralReference.testenv
    STDOUT_REDIRECT compositionResults_BasicAncestralReference.txt
    DIFF_COMPARE compositionResults_BasicAncestralReference.txt 
)

pxr_register_test(testPcpMuseum_BasicReferenceDiamond
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicReferenceDiamond/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicReferenceDiamond.testenv
    STDOUT_REDIRECT compositionResults_BasicReferenceDiamond.txt
    DIFF_COMPARE compositionResults_BasicReferenceDiamond.txt 
)

pxr_register_test(testPcpMuseum_BasicRelocateToAnimInterface
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicRelocateToAnimInterface/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicRelocateToAnimInterface.testenv
    STDOUT_REDIRECT compositionResults_BasicRelocateToAnimInterface.txt
    DIFF_COMPARE compositionResults_BasicRelocateToAnimInterface.txt 
)

pxr_register_test(testPcpMuseum_BasicRelocateToAnimInterfaceAsNewRootPrim
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_BasicRelocateToAnimInterfaceAsNewRootPrim_usd.txt BasicRelocateToAnimInterfaceAsNewRootPrim/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicRelocateToAnimInterfaceAsNewRootPrim.testenv
    STDOUT_REDIRECT compositionResults_BasicRelocateToAnimInterfaceAsNewRootPrim_usd.txt
    DIFF_COMPARE compositionResults_BasicRelocateToAnimInterfaceAsNewRootPrim_usd.txt compositionErrors_BasicRelocateToAnimInterfaceAsNewRootPrim_usd.txt
)

pxr_register_test(testPcpMuseum_BasicInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_BasicInherits.txt BasicInherits/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicInherits.testenv
    STDOUT_REDIRECT compositionResults_BasicInherits.txt
    DIFF_COMPARE compositionResults_BasicInherits.txt compositionErrors_BasicInherits.txt
    ENV 
        SDF_SCHEMA_PROHIBIT_INVALID_VARIANT_SELECTIONS=0
)

pxr_register_test(testPcpMuseum_BasicLocalAndGlobalClassCombination
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicLocalAndGlobalClassCombination/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicLocalAndGlobalClassCombination.testenv
    STDOUT_REDIRECT compositionResults_BasicLocalAndGlobalClassCombination.txt
    DIFF_COMPARE compositionResults_BasicLocalAndGlobalClassCombination.txt 
)

pxr_register_test(testPcpMuseum_BasicListEditing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicListEditing/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicListEditing.testenv
    STDOUT_REDIRECT compositionResults_BasicListEditing_usd.txt
    DIFF_COMPARE compositionResults_BasicListEditing_usd.txt 
)

pxr_register_test(testPcpMuseum_BasicListEditingWithInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicListEditingWithInherits/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicListEditingWithInherits.testenv
    STDOUT_REDIRECT compositionResults_BasicListEditingWithInherits.txt
    DIFF_COMPARE compositionResults_BasicListEditingWithInherits.txt 
)

pxr_register_test(testPcpMuseum_BasicDuplicateSublayer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicDuplicateSublayer/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicDuplicateSublayer.testenv
    STDOUT_REDIRECT compositionResults_BasicDuplicateSublayer.txt
    DIFF_COMPARE compositionResults_BasicDuplicateSublayer.txt 
)

pxr_register_test(testPcpMuseum_BasicTimeOffset
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicTimeOffset/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicTimeOffset.testenv
    STDOUT_REDIRECT compositionResults_BasicTimeOffset.txt
    DIFF_COMPARE compositionResults_BasicTimeOffset.txt 
)

pxr_register_test(testPcpMuseum_BasicNestedPayload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicNestedPayload/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicNestedPayload.testenv
    STDOUT_REDIRECT compositionResults_BasicNestedPayload.txt
    DIFF_COMPARE compositionResults_BasicNestedPayload.txt 
)

pxr_register_test(testPcpMuseum_BasicNestedVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicNestedVariants/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicNestedVariants.testenv
    STDOUT_REDIRECT compositionResults_BasicNestedVariants.txt
    DIFF_COMPARE compositionResults_BasicNestedVariants.txt 
)

pxr_register_test(testPcpMuseum_BasicNestedVariantsWithSameName
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicNestedVariantsWithSameName/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicNestedVariantsWithSameName.testenv
    STDOUT_REDIRECT compositionResults_BasicNestedVariantsWithSameName.txt
    DIFF_COMPARE compositionResults_BasicNestedVariantsWithSameName.txt 
)

pxr_register_test(testPcpMuseum_BasicVariantWithConnections
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicVariantWithConnections/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicVariantWithConnections.testenv
    STDOUT_REDIRECT compositionResults_BasicVariantWithConnections.txt
    DIFF_COMPARE compositionResults_BasicVariantWithConnections.txt 
)

pxr_register_test(testPcpMuseum_BasicVariantWithReference
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicVariantWithReference/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicVariantWithReference.testenv
    STDOUT_REDIRECT compositionResults_BasicVariantWithReference.txt
    DIFF_COMPARE compositionResults_BasicVariantWithReference.txt 
)

pxr_register_test(testPcpMuseum_BasicOwner
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --session BasicOwner/session.usda BasicOwner/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicOwner.testenv
    STDOUT_REDIRECT compositionResults_BasicOwner.txt
    DIFF_COMPARE compositionResults_BasicOwner.txt 
)

pxr_register_test(testPcpMuseum_BasicPayload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --session BasicPayload/session.usda --errorFile compositionErrors_BasicPayload.txt BasicPayload/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicPayload.testenv
    STDOUT_REDIRECT compositionResults_BasicPayload.txt
    DIFF_COMPARE compositionResults_BasicPayload.txt compositionErrors_BasicPayload.txt
)

pxr_register_test(testPcpMuseum_BasicSpecializes
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicSpecializes/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicSpecializes.testenv
    STDOUT_REDIRECT compositionResults_BasicSpecializes.txt
    DIFF_COMPARE compositionResults_BasicSpecializes.txt 
)

pxr_register_test(testPcpMuseum_BasicSpecializesAndInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicSpecializesAndInherits/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicSpecializesAndInherits.testenv
    STDOUT_REDIRECT compositionResults_BasicSpecializesAndInherits.txt
    DIFF_COMPARE compositionResults_BasicSpecializesAndInherits.txt 
)

pxr_register_test(testPcpMuseum_BasicSpecializesAndReferences
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicSpecializesAndReferences/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicSpecializesAndReferences.testenv
    STDOUT_REDIRECT compositionResults_BasicSpecializesAndReferences.txt
    DIFF_COMPARE compositionResults_BasicSpecializesAndReferences.txt 
)

pxr_register_test(testPcpMuseum_BasicSpecializesAndVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd BasicSpecializesAndVariants/root.usda" 
    TESTENV testenv/testPcpMuseum_BasicSpecializesAndVariants.testenv
    STDOUT_REDIRECT compositionResults_BasicSpecializesAndVariants.txt
    DIFF_COMPARE compositionResults_BasicSpecializesAndVariants.txt 
)

pxr_register_test(testPcpMuseum_ElidedAncestralRelocates
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd ElidedAncestralRelocates/root.usda" 
    TESTENV testenv/testPcpMuseum_ElidedAncestralRelocates.testenv
    STDOUT_REDIRECT compositionResults_ElidedAncestralRelocates.txt
    DIFF_COMPARE compositionResults_ElidedAncestralRelocates.txt
)

pxr_register_test(testPcpMuseum_ExpressionsInPayloads
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd ExpressionsInPayloads/root.usda" 
    TESTENV testenv/testPcpMuseum_ExpressionsInPayloads.testenv
    STDOUT_REDIRECT compositionResults_ExpressionsInPayloads.txt
    DIFF_COMPARE compositionResults_ExpressionsInPayloads.txt
)

pxr_register_test(testPcpMuseum_ExpressionsInReferences
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd ExpressionsInReferences/root.usda" 
    TESTENV testenv/testPcpMuseum_ExpressionsInReferences.testenv
    STDOUT_REDIRECT compositionResults_ExpressionsInReferences.txt
    DIFF_COMPARE compositionResults_ExpressionsInReferences.txt
)

pxr_register_test(testPcpMuseum_ExpressionsInSublayers
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd ExpressionsInSublayers/root.usda --session ExpressionsInSublayers/session.usda --errorFile compositionErrors_ExpressionsInSublayers.txt" 
    TESTENV testenv/testPcpMuseum_ExpressionsInSublayers.testenv
    STDOUT_REDIRECT compositionResults_ExpressionsInSublayers.txt
    DIFF_COMPARE compositionResults_ExpressionsInSublayers.txt compositionErrors_ExpressionsInSublayers.txt
)

pxr_register_test(testPcpMuseum_ExpressionsInVariantSelections
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd ExpressionsInVariantSelections/root.usda --errorFile compositionErrors_ExpressionsInVariantSelections.txt" 
    TESTENV testenv/testPcpMuseum_ExpressionsInVariantSelections.testenv
    STDOUT_REDIRECT compositionResults_ExpressionsInVariantSelections.txt
    DIFF_COMPARE compositionResults_ExpressionsInVariantSelections.txt compositionErrors_ExpressionsInVariantSelections.txt
)

pxr_register_test(testPcpMuseum_ImpliedAndAncestralInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd ImpliedAndAncestralInherits/root.usda" 
    TESTENV testenv/testPcpMuseum_ImpliedAndAncestralInherits.testenv
    STDOUT_REDIRECT compositionResults_ImpliedAndAncestralInherits.txt
    DIFF_COMPARE compositionResults_ImpliedAndAncestralInherits.txt
)

pxr_register_test(testPcpMuseum_ImpliedAndAncestralInherits_graph
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --dumpPath /World/CharGroup/Sim/includes/Hand/Geom --dumpMaps ImpliedAndAncestralInherits/root.usda" 
    TESTENV testenv/testPcpMuseum_ImpliedAndAncestralInherits.testenv
    STDOUT_REDIRECT compositionResults_ImpliedAndAncestralInherits_graph.txt
    DIFF_COMPARE compositionResults_ImpliedAndAncestralInherits_graph.txt 
)

pxr_register_test(testPcpMuseum_ImpliedAndAncestralInherits_ComplexEvaluation
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd ImpliedAndAncestralInherits_ComplexEvaluation/root.usda" 
    TESTENV testenv/testPcpMuseum_ImpliedAndAncestralInherits_ComplexEvaluation.testenv
    STDOUT_REDIRECT compositionResults_ImpliedAndAncestralInherits_ComplexEvaluation.txt
    DIFF_COMPARE compositionResults_ImpliedAndAncestralInherits_ComplexEvaluation.txt
)

pxr_register_test(testPcpMuseum_PayloadsAndAncestralArcs
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --payloads ^(?!/Root/UnloadedInheritedPayload) PayloadsAndAncestralArcs/root.usda" 
    TESTENV testenv/testPcpMuseum_PayloadsAndAncestralArcs.testenv
    STDOUT_REDIRECT compositionResults_PayloadsAndAncestralArcs.txt
    DIFF_COMPARE compositionResults_PayloadsAndAncestralArcs.txt 
)

pxr_register_test(testPcpMuseum_PayloadsAndAncestralArcs2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd PayloadsAndAncestralArcs2/root.usda" 
    TESTENV testenv/testPcpMuseum_PayloadsAndAncestralArcs2.testenv
    STDOUT_REDIRECT compositionResults_PayloadsAndAncestralArcs2.txt
    DIFF_COMPARE compositionResults_PayloadsAndAncestralArcs2.txt 
)

pxr_register_test(testPcpMuseum_PayloadsAndAncestralArcs3_allpayloads
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd PayloadsAndAncestralArcs3/root.usda" 
    TESTENV testenv/testPcpMuseum_PayloadsAndAncestralArcs3.testenv
    STDOUT_REDIRECT compositionResults_PayloadsAndAncestralArcs3_allpayloads.txt
    DIFF_COMPARE compositionResults_PayloadsAndAncestralArcs3_allpayloads.txt 
)

pxr_register_test(testPcpMuseum_PayloadsAndAncestralArcs3_refpayloadsonly
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --payloads '.*Reference.*' PayloadsAndAncestralArcs3/root.usda" 
    TESTENV testenv/testPcpMuseum_PayloadsAndAncestralArcs3.testenv
    STDOUT_REDIRECT compositionResults_PayloadsAndAncestralArcs3_refpayloadsonly.txt
    DIFF_COMPARE compositionResults_PayloadsAndAncestralArcs3_refpayloadsonly.txt 
)

pxr_register_test(testPcpMuseum_ReferenceListOpsWithOffsets
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd ReferenceListOpsWithOffsets/root.usda" 
    TESTENV testenv/testPcpMuseum_ReferenceListOpsWithOffsets.testenv
    STDOUT_REDIRECT compositionResults_ReferenceListOpsWithOffsets.txt
    DIFF_COMPARE compositionResults_ReferenceListOpsWithOffsets.txt 
)

pxr_register_test(testPcpMuseum_RelativePathPayloads
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd RelativePathPayloads/root.usda" 
    TESTENV testenv/testPcpMuseum_RelativePathPayloads.testenv
    STDOUT_REDIRECT compositionResults_RelativePathPayloads.txt
    DIFF_COMPARE compositionResults_RelativePathPayloads.txt 
)

pxr_register_test(testPcpMuseum_RelativePathReferences
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd RelativePathReferences/root.usda" 
    TESTENV testenv/testPcpMuseum_RelativePathReferences.testenv
    STDOUT_REDIRECT compositionResults_RelativePathReferences.txt
    DIFF_COMPARE compositionResults_RelativePathReferences.txt 
)

pxr_register_test(testPcpMuseum_RelocatePrimsWithSameName
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_RelocatePrimsWithSameName.txt RelocatePrimsWithSameName/root.usda" 
    TESTENV testenv/testPcpMuseum_RelocatePrimsWithSameName.testenv
    STDOUT_REDIRECT compositionResults_RelocatePrimsWithSameName.txt
    DIFF_COMPARE compositionResults_RelocatePrimsWithSameName.txt compositionErrors_RelocatePrimsWithSameName.txt
)

pxr_register_test(testPcpMuseum_RelocateToNone
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_RelocateToNone_usd.txt RelocateToNone/root.usda" 
    TESTENV testenv/testPcpMuseum_RelocateToNone.testenv
    STDOUT_REDIRECT compositionResults_RelocateToNone_usd.txt
    DIFF_COMPARE compositionResults_RelocateToNone_usd.txt compositionErrors_RelocateToNone_usd.txt
)
pxr_register_test(testPcpMuseum_SpecializesAndAncestralArcs
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SpecializesAndAncestralArcs/root.usda" 
    TESTENV testenv/testPcpMuseum_SpecializesAndAncestralArcs.testenv
    STDOUT_REDIRECT compositionResults_SpecializesAndAncestralArcs.txt
    DIFF_COMPARE compositionResults_SpecializesAndAncestralArcs.txt 
)

pxr_register_test(testPcpMuseum_SpecializesAndAncestralArcs2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SpecializesAndAncestralArcs2/root.usda" 
    TESTENV testenv/testPcpMuseum_SpecializesAndAncestralArcs2.testenv
    STDOUT_REDIRECT compositionResults_SpecializesAndAncestralArcs2.txt
    DIFF_COMPARE compositionResults_SpecializesAndAncestralArcs2.txt 
)

pxr_register_test(testPcpMuseum_SpecializesAndAncestralArcs3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SpecializesAndAncestralArcs3/root.usda" 
    TESTENV testenv/testPcpMuseum_SpecializesAndAncestralArcs3.testenv
    STDOUT_REDIRECT compositionResults_SpecializesAndAncestralArcs3.txt
    DIFF_COMPARE compositionResults_SpecializesAndAncestralArcs3.txt 
)

pxr_register_test(testPcpMuseum_SpecializesAndAncestralArcs4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SpecializesAndAncestralArcs4/root.usda" 
    TESTENV testenv/testPcpMuseum_SpecializesAndAncestralArcs4.testenv
    STDOUT_REDIRECT compositionResults_SpecializesAndAncestralArcs4.txt
    DIFF_COMPARE compositionResults_SpecializesAndAncestralArcs4.txt 
)

pxr_register_test(testPcpMuseum_SpecializesAndAncestralArcs4_graph
    PYTHON
    TESTENV testenv/testPcpMuseum_SpecializesAndAncestralArcs4.testenv
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --dumpPath /Parent/Child SpecializesAndAncestralArcs4/root.usda" 
    STDOUT_REDIRECT compositionResults_SpecializesAndAncestralArcs4_graph.txt
    DIFF_COMPARE compositionResults_SpecializesAndAncestralArcs4_graph.txt 
)

pxr_register_test(testPcpMuseum_SpecializesAndAncestralArcs5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SpecializesAndAncestralArcs5/root.usda" 
    TESTENV testenv/testPcpMuseum_SpecializesAndAncestralArcs5.testenv
    STDOUT_REDIRECT compositionResults_SpecializesAndAncestralArcs5.txt
    DIFF_COMPARE compositionResults_SpecializesAndAncestralArcs5.txt 
)

pxr_register_test(testPcpMuseum_SpecializesAndDuplicateArcs
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SpecializesAndDuplicateArcs/root.usda" 
    TESTENV testenv/testPcpMuseum_SpecializesAndDuplicateArcs.testenv
    STDOUT_REDIRECT compositionResults_SpecializesAndDuplicateArcs.txt
    DIFF_COMPARE compositionResults_SpecializesAndDuplicateArcs.txt
)

pxr_register_test(testPcpMuseum_SpecializesAndVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SpecializesAndVariants/root.usda" 
    TESTENV testenv/testPcpMuseum_SpecializesAndVariants.testenv
    STDOUT_REDIRECT compositionResults_SpecializesAndVariants.txt
    DIFF_COMPARE compositionResults_SpecializesAndVariants.txt 
)

pxr_register_test(testPcpMuseum_SpecializesAndVariants2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SpecializesAndVariants2/root.usda" 
    TESTENV testenv/testPcpMuseum_SpecializesAndVariants2.testenv
    STDOUT_REDIRECT compositionResults_SpecializesAndVariants2.txt
    DIFF_COMPARE compositionResults_SpecializesAndVariants2.txt 
)

pxr_register_test(testPcpMuseum_SpecializesAndVariants3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SpecializesAndVariants3/root.usda" 
    TESTENV testenv/testPcpMuseum_SpecializesAndVariants3.testenv
    STDOUT_REDIRECT compositionResults_SpecializesAndVariants3.txt
    DIFF_COMPARE compositionResults_SpecializesAndVariants3.txt 
)

# XXX: Disabled pending fix for GitHub #3750
#
# pxr_register_test(testPcpMuseum_SpecializesAndVariants4
#     PYTHON
#     COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SpecializesAndVariants4/root.usda" 
#     TESTENV testenv/testPcpMuseum_SpecializesAndVariants4.testenv
#     STDOUT_REDIRECT compositionResults_SpecializesAndVariants4.txt
#     DIFF_COMPARE compositionResults_SpecializesAndVariants4.txt 
# )

pxr_register_test(testPcpMuseum_SubrootInheritsAndVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SubrootInheritsAndVariants/root.usda" 
    TESTENV testenv/testPcpMuseum_SubrootInheritsAndVariants.testenv
    STDOUT_REDIRECT compositionResults_SubrootInheritsAndVariants.txt
    DIFF_COMPARE compositionResults_SubrootInheritsAndVariants.txt 
)

pxr_register_test(testPcpMuseum_SubrootReferenceAndClasses
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SubrootReferenceAndClasses/root.usda" 
    TESTENV testenv/testPcpMuseum_SubrootReferenceAndClasses.testenv
    STDOUT_REDIRECT compositionResults_SubrootReferenceAndClasses.txt
    DIFF_COMPARE compositionResults_SubrootReferenceAndClasses.txt 
)

pxr_register_test(testPcpMuseum_SubrootReferenceAndRelocates
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SubrootReferenceAndRelocates/root.usda" 
    TESTENV testenv/testPcpMuseum_SubrootReferenceAndRelocates.testenv
    STDOUT_REDIRECT compositionResults_SubrootReferenceAndRelocates.txt
    DIFF_COMPARE compositionResults_SubrootReferenceAndRelocates.txt 
)

pxr_register_test(testPcpMuseum_SubrootReferenceAndVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_SubrootReferenceAndVariants.txt SubrootReferenceAndVariants/root.usda" 
    TESTENV testenv/testPcpMuseum_SubrootReferenceAndVariants.testenv
    STDOUT_REDIRECT compositionResults_SubrootReferenceAndVariants.txt
    DIFF_COMPARE compositionResults_SubrootReferenceAndVariants.txt compositionErrors_SubrootReferenceAndVariants.txt 
    ENV 
        SDF_SCHEMA_PROHIBIT_INVALID_VARIANT_SELECTIONS=0
)

pxr_register_test(testPcpMuseum_SubrootReferenceAndVariants2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd SubrootReferenceAndVariants2/root.usda" 
    TESTENV testenv/testPcpMuseum_SubrootReferenceAndVariants2.testenv
    STDOUT_REDIRECT compositionResults_SubrootReferenceAndVariants2.txt
    DIFF_COMPARE compositionResults_SubrootReferenceAndVariants2.txt
)

pxr_register_test(testPcpMuseum_TimeCodesPerSecond1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TimeCodesPerSecond/root.usda" 
    TESTENV testenv/testPcpMuseum_TimeCodesPerSecond.testenv
    STDOUT_REDIRECT compositionResults_TimeCodesPerSecond1.txt
    DIFF_COMPARE compositionResults_TimeCodesPerSecond1.txt 
)

pxr_register_test(testPcpMuseum_TimeCodesPerSecond2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TimeCodesPerSecond/root_48tcps.usda" 
    TESTENV testenv/testPcpMuseum_TimeCodesPerSecond.testenv
    STDOUT_REDIRECT compositionResults_TimeCodesPerSecond2.txt
    DIFF_COMPARE compositionResults_TimeCodesPerSecond2.txt 
)

pxr_register_test(testPcpMuseum_TimeCodesPerSecond3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TimeCodesPerSecond/root_24tcps_12fps.usda" 
    TESTENV testenv/testPcpMuseum_TimeCodesPerSecond.testenv
    STDOUT_REDIRECT compositionResults_TimeCodesPerSecond3.txt
    DIFF_COMPARE compositionResults_TimeCodesPerSecond3.txt 
)

pxr_register_test(testPcpMuseum_TimeCodesPerSecond4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TimeCodesPerSecond/root_12fps.usda" 
    TESTENV testenv/testPcpMuseum_TimeCodesPerSecond.testenv
    STDOUT_REDIRECT compositionResults_TimeCodesPerSecond4.txt
    DIFF_COMPARE compositionResults_TimeCodesPerSecond4.txt 
)

pxr_register_test(testPcpMuseum_TimeCodesPerSecondWithSessionLayer1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TimeCodesPerSecond/root_24tcps_12fps.usda --session TimeCodesPerSecond/session_48tcps.usda" 
    TESTENV testenv/testPcpMuseum_TimeCodesPerSecond.testenv
    STDOUT_REDIRECT compositionResults_TimeCodesPerSecondWithSessionLayer1.txt
    DIFF_COMPARE compositionResults_TimeCodesPerSecondWithSessionLayer1.txt 
)

pxr_register_test(testPcpMuseum_TimeCodesPerSecondWithSessionLayer2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TimeCodesPerSecond/root_48tcps.usda --session TimeCodesPerSecond/session_24fps.usda" 
    TESTENV testenv/testPcpMuseum_TimeCodesPerSecond.testenv
    STDOUT_REDIRECT compositionResults_TimeCodesPerSecondWithSessionLayer2.txt
    DIFF_COMPARE compositionResults_TimeCodesPerSecondWithSessionLayer2.txt 
)

pxr_register_test(testPcpMuseum_TimeCodesPerSecondWithSessionLayer3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TimeCodesPerSecond/root_12fps.usda --session TimeCodesPerSecond/session_24fps.usda" 
    TESTENV testenv/testPcpMuseum_TimeCodesPerSecond.testenv
    STDOUT_REDIRECT compositionResults_TimeCodesPerSecondWithSessionLayer3.txt
    DIFF_COMPARE compositionResults_TimeCodesPerSecondWithSessionLayer3.txt 
)

pxr_register_test(testPcpMuseum_TimeCodesPerSecondWithSessionLayer4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TimeCodesPerSecond/root_12fps.usda --session TimeCodesPerSecond/session.usda" 
    TESTENV testenv/testPcpMuseum_TimeCodesPerSecond.testenv
    STDOUT_REDIRECT compositionResults_TimeCodesPerSecondWithSessionLayer4.txt
    DIFF_COMPARE compositionResults_TimeCodesPerSecondWithSessionLayer4.txt 
)

pxr_register_test(testPcpMuseum_TypicalReferenceToRiggedModel
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TypicalReferenceToRiggedModel/root.usda" 
    TESTENV testenv/testPcpMuseum_TypicalReferenceToRiggedModel.testenv
    STDOUT_REDIRECT compositionResults_TypicalReferenceToRiggedModel.txt
    DIFF_COMPARE compositionResults_TypicalReferenceToRiggedModel.txt 
)

pxr_register_test(testPcpMuseum_TypicalReferenceToChargroup
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TypicalReferenceToChargroup/root.usda" 
    TESTENV testenv/testPcpMuseum_TypicalReferenceToChargroup.testenv
    STDOUT_REDIRECT compositionResults_TypicalReferenceToChargroup.txt
    DIFF_COMPARE compositionResults_TypicalReferenceToChargroup.txt 
)

pxr_register_test(testPcpMuseum_TypicalReferenceToChargroupWithRename
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TypicalReferenceToChargroupWithRename/root.usda" 
    TESTENV testenv/testPcpMuseum_TypicalReferenceToChargroupWithRename.testenv
    STDOUT_REDIRECT compositionResults_TypicalReferenceToChargroupWithRename.txt
    DIFF_COMPARE compositionResults_TypicalReferenceToChargroupWithRename.txt 
)

pxr_register_test(testPcpMuseum_TrickyListEditedTargetPaths
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyListEditedTargetPaths/root.usda"
    TESTENV testenv/testPcpMuseum_TrickyListEditedTargetPaths.testenv
    STDOUT_REDIRECT compositionResults_TrickyListEditedTargetPaths.txt
    DIFF_COMPARE compositionResults_TrickyListEditedTargetPaths.txt
)

pxr_register_test(testPcpMuseum_TrickySpookyInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickySpookyInherits/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickySpookyInherits.testenv
    STDOUT_REDIRECT compositionResults_TrickySpookyInherits.txt
    DIFF_COMPARE compositionResults_TrickySpookyInherits.txt 
)

pxr_register_test(testPcpMuseum_TrickySpookyInheritsInSymmetricArmRig
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickySpookyInheritsInSymmetricArmRig/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickySpookyInheritsInSymmetricArmRig.testenv
    STDOUT_REDIRECT compositionResults_TrickySpookyInheritsInSymmetricArmRig.txt
    DIFF_COMPARE compositionResults_TrickySpookyInheritsInSymmetricArmRig.txt 
)

pxr_register_test(testPcpMuseum_TrickySpookyInheritsInSymmetricBrowRig
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickySpookyInheritsInSymmetricBrowRig/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickySpookyInheritsInSymmetricBrowRig.testenv
    STDOUT_REDIRECT compositionResults_TrickySpookyInheritsInSymmetricBrowRig.txt
    DIFF_COMPARE compositionResults_TrickySpookyInheritsInSymmetricBrowRig.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsAndRelocates
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd  --errorFile compositionErrors_TrickyInheritsAndRelocates.txt TrickyInheritsAndRelocates/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyInheritsAndRelocates.testenv
    STDOUT_REDIRECT compositionResults_TrickyInheritsAndRelocates.txt
    DIFF_COMPARE compositionResults_TrickyInheritsAndRelocates.txt compositionErrors_TrickyInheritsAndRelocates.txt
)

pxr_register_test(testPcpMuseum_TrickyInheritsAndRelocates2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyInheritsAndRelocates2/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyInheritsAndRelocates2.testenv
    STDOUT_REDIRECT compositionResults_TrickyInheritsAndRelocates2.txt
    DIFF_COMPARE compositionResults_TrickyInheritsAndRelocates2.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsAndRelocates3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyInheritsAndRelocates3/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyInheritsAndRelocates3.testenv
    STDOUT_REDIRECT compositionResults_TrickyInheritsAndRelocates3.txt
    DIFF_COMPARE compositionResults_TrickyInheritsAndRelocates3.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsAndRelocates4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyInheritsAndRelocates4/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyInheritsAndRelocates4.testenv
    STDOUT_REDIRECT compositionResults_TrickyInheritsAndRelocates4.txt
    DIFF_COMPARE compositionResults_TrickyInheritsAndRelocates4.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsAndRelocates5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyInheritsAndRelocates5/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyInheritsAndRelocates5.testenv
    STDOUT_REDIRECT compositionResults_TrickyInheritsAndRelocates5.txt
    DIFF_COMPARE compositionResults_TrickyInheritsAndRelocates5.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsAndRelocatesToNewRootPrim
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd  --errorFile compositionErrors_TrickyInheritsAndRelocatesToNewRootPrim.txt TrickyInheritsAndRelocatesToNewRootPrim/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyInheritsAndRelocatesToNewRootPrim.testenv
    STDOUT_REDIRECT compositionResults_TrickyInheritsAndRelocatesToNewRootPrim.txt
    DIFF_COMPARE compositionResults_TrickyInheritsAndRelocatesToNewRootPrim.txt compositionErrors_TrickyInheritsAndRelocatesToNewRootPrim.txt
)

pxr_register_test(testPcpMuseum_TrickyInheritsInVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyInheritsInVariants/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyInheritsInVariants.testenv
    STDOUT_REDIRECT compositionResults_TrickyInheritsInVariants.txt
    DIFF_COMPARE compositionResults_TrickyInheritsInVariants.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsInVariants2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyInheritsInVariants2/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyInheritsInVariants2.testenv
    STDOUT_REDIRECT compositionResults_TrickyInheritsInVariants2.txt
    DIFF_COMPARE compositionResults_TrickyInheritsInVariants2.txt 
)

pxr_register_test(testPcpMuseum_SubrootReferenceNonCycle
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_SubrootReferenceNonCycle.txt SubrootReferenceNonCycle/root.usda" 
    TESTENV testenv/testPcpMuseum_SubrootReferenceNonCycle.testenv
    STDOUT_REDIRECT compositionResults_SubrootReferenceNonCycle.txt
    DIFF_COMPARE compositionResults_SubrootReferenceNonCycle.txt compositionErrors_SubrootReferenceNonCycle.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantAncestralSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyVariantAncestralSelection/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyVariantAncestralSelection.testenv
    STDOUT_REDIRECT compositionResults_TrickyVariantAncestralSelection.txt
    DIFF_COMPARE compositionResults_TrickyVariantAncestralSelection.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantIndependentSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyVariantIndependentSelection/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyVariantIndependentSelection.testenv
    STDOUT_REDIRECT compositionResults_TrickyVariantIndependentSelection.txt
    DIFF_COMPARE compositionResults_TrickyVariantIndependentSelection.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantInPayload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyVariantInPayload/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyVariantInPayload.testenv
    STDOUT_REDIRECT compositionResults_TrickyVariantInPayload.txt
    DIFF_COMPARE compositionResults_TrickyVariantInPayload.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantOverrideOfLocalClass
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyVariantOverrideOfLocalClass/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyVariantOverrideOfLocalClass.testenv
    STDOUT_REDIRECT compositionResults_TrickyVariantOverrideOfLocalClass.txt
    DIFF_COMPARE compositionResults_TrickyVariantOverrideOfLocalClass.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantOverrideOfRelocatedPrim
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyVariantOverrideOfRelocatedPrim/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyVariantOverrideOfRelocatedPrim.testenv
    STDOUT_REDIRECT compositionResults_TrickyVariantOverrideOfRelocatedPrim.txt
    DIFF_COMPARE compositionResults_TrickyVariantOverrideOfRelocatedPrim.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantSelectionInVariant
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyVariantSelectionInVariant/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyVariantSelectionInVariant.testenv
    STDOUT_REDIRECT compositionResults_TrickyVariantSelectionInVariant.txt
    DIFF_COMPARE compositionResults_TrickyVariantSelectionInVariant.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantSelectionInVariant2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyVariantSelectionInVariant2/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyVariantSelectionInVariant2.testenv
    STDOUT_REDIRECT compositionResults_TrickyVariantSelectionInVariant2.txt
    DIFF_COMPARE compositionResults_TrickyVariantSelectionInVariant2.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantWeakerSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyVariantWeakerSelection/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyVariantWeakerSelection.testenv
    STDOUT_REDIRECT compositionResults_TrickyVariantWeakerSelection.txt
    DIFF_COMPARE compositionResults_TrickyVariantWeakerSelection.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantWeakerSelection2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyVariantWeakerSelection2/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyVariantWeakerSelection2.testenv
    STDOUT_REDIRECT compositionResults_TrickyVariantWeakerSelection2.txt
    DIFF_COMPARE compositionResults_TrickyVariantWeakerSelection2.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantWeakerSelection3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyVariantWeakerSelection3/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyVariantWeakerSelection3.testenv
    STDOUT_REDIRECT compositionResults_TrickyVariantWeakerSelection3.txt
    DIFF_COMPARE compositionResults_TrickyVariantWeakerSelection3.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantWeakerSelection4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyVariantWeakerSelection4/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyVariantWeakerSelection4.testenv
    STDOUT_REDIRECT compositionResults_TrickyVariantWeakerSelection4.txt
    DIFF_COMPARE compositionResults_TrickyVariantWeakerSelection4.txt 
)

pxr_register_test(testPcpMuseum_TrickyClassHierarchy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyClassHierarchy/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyClassHierarchy.testenv
    STDOUT_REDIRECT compositionResults_TrickyClassHierarchy.txt
    DIFF_COMPARE compositionResults_TrickyClassHierarchy.txt 
)

pxr_register_test(testPcpMuseum_TrickyLocalClassHierarchyWithRelocates
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyLocalClassHierarchyWithRelocates/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyLocalClassHierarchyWithRelocates.testenv
    STDOUT_REDIRECT compositionResults_TrickyLocalClassHierarchyWithRelocates.txt
    DIFF_COMPARE compositionResults_TrickyLocalClassHierarchyWithRelocates.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedClasses
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyNestedClasses/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyNestedClasses.testenv
    STDOUT_REDIRECT compositionResults_TrickyNestedClasses.txt
    DIFF_COMPARE compositionResults_TrickyNestedClasses.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedClasses2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyNestedClasses2/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyNestedClasses2.testenv
    STDOUT_REDIRECT compositionResults_TrickyNestedClasses2.txt
    DIFF_COMPARE compositionResults_TrickyNestedClasses2.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedClasses3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyNestedClasses3/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyNestedClasses3.testenv
    STDOUT_REDIRECT compositionResults_TrickyNestedClasses3.txt
    DIFF_COMPARE compositionResults_TrickyNestedClasses3.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedSpecializes
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyNestedSpecializes/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyNestedSpecializes.testenv
    STDOUT_REDIRECT compositionResults_TrickyNestedSpecializes.txt
    DIFF_COMPARE compositionResults_TrickyNestedSpecializes.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedSpecializes2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyNestedSpecializes2/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyNestedSpecializes2.testenv
    STDOUT_REDIRECT compositionResults_TrickyNestedSpecializes2.txt
    DIFF_COMPARE compositionResults_TrickyNestedSpecializes2.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedClasses4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyNestedClasses4/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyNestedClasses4.testenv
    STDOUT_REDIRECT compositionResults_TrickyNestedClasses4.txt
    DIFF_COMPARE compositionResults_TrickyNestedClasses4.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyNestedVariants/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyNestedVariants.testenv
    STDOUT_REDIRECT compositionResults_TrickyNestedVariants.txt
    DIFF_COMPARE compositionResults_TrickyNestedVariants.txt 
)

pxr_register_test(testPcpMuseum_TrickySpecializesAndInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickySpecializesAndInherits/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickySpecializesAndInherits.testenv
    STDOUT_REDIRECT compositionResults_TrickySpecializesAndInherits.txt
    DIFF_COMPARE compositionResults_TrickySpecializesAndInherits.txt 
)

pxr_register_test(testPcpMuseum_TrickySpecializesAndInherits2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickySpecializesAndInherits2/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickySpecializesAndInherits2.testenv
    STDOUT_REDIRECT compositionResults_TrickySpecializesAndInherits2.txt
    DIFF_COMPARE compositionResults_TrickySpecializesAndInherits2.txt 
)

pxr_register_test(testPcpMuseum_TrickySpecializesAndInherits3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickySpecializesAndInherits3/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickySpecializesAndInherits3.testenv
    STDOUT_REDIRECT compositionResults_TrickySpecializesAndInherits3.txt
    DIFF_COMPARE compositionResults_TrickySpecializesAndInherits3.txt 
)

pxr_register_test(testPcpMuseum_TrickySpecializesAndRelocates
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickySpecializesAndRelocates/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickySpecializesAndRelocates.testenv
    STDOUT_REDIRECT compositionResults_TrickySpecializesAndRelocates.txt
    DIFF_COMPARE compositionResults_TrickySpecializesAndRelocates.txt 
)

pxr_register_test(testPcpMuseum_TrickySpookyVariantSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickySpookyVariantSelection/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickySpookyVariantSelection.testenv
    STDOUT_REDIRECT compositionResults_TrickySpookyVariantSelection.txt
    DIFF_COMPARE compositionResults_TrickySpookyVariantSelection.txt 
)

pxr_register_test(testPcpMuseum_TrickySpookyVariantSelectionInClass
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickySpookyVariantSelectionInClass/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickySpookyVariantSelectionInClass.testenv
    STDOUT_REDIRECT compositionResults_TrickySpookyVariantSelectionInClass.txt
    DIFF_COMPARE compositionResults_TrickySpookyVariantSelectionInClass.txt 
)

pxr_register_test(testPcpMuseum_TrickyConnectionToRelocatedAttribute
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyConnectionToRelocatedAttribute/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyConnectionToRelocatedAttribute.testenv
    STDOUT_REDIRECT compositionResults_TrickyConnectionToRelocatedAttribute.txt
    DIFF_COMPARE compositionResults_TrickyConnectionToRelocatedAttribute.txt 
)

pxr_register_test(testPcpMuseum_TrickyRelocatedTargetInVariant
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyRelocatedTargetInVariant/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyRelocatedTargetInVariant.testenv
    STDOUT_REDIRECT compositionResults_TrickyRelocatedTargetInVariant.txt
    DIFF_COMPARE compositionResults_TrickyRelocatedTargetInVariant.txt 
)

pxr_register_test(testPcpMuseum_TrickyRelocationOfPrimFromPayload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyRelocationOfPrimFromPayload/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyRelocationOfPrimFromPayload.testenv
    STDOUT_REDIRECT compositionResults_TrickyRelocationOfPrimFromPayload.txt
    DIFF_COMPARE compositionResults_TrickyRelocationOfPrimFromPayload.txt 
)

pxr_register_test(testPcpMuseum_TrickyRelocationOfPrimFromVariant
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyRelocationOfPrimFromVariant/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyRelocationOfPrimFromVariant.testenv
    STDOUT_REDIRECT compositionResults_TrickyRelocationOfPrimFromVariant.txt
    DIFF_COMPARE compositionResults_TrickyRelocationOfPrimFromVariant.txt 
)

pxr_register_test(testPcpMuseum_TrickyRelocationSquatter
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyRelocationSquatter/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyRelocationSquatter.testenv
    STDOUT_REDIRECT compositionResults_TrickyRelocationSquatter.txt
    DIFF_COMPARE compositionResults_TrickyRelocationSquatter.txt 
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocations
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyMultipleRelocations/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyMultipleRelocations.testenv
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocations.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocations.txt 
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocations2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyMultipleRelocations2/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyMultipleRelocations2.testenv
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocations2.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocations2.txt 
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocations3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyMultipleRelocations3/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyMultipleRelocations3.testenv
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocations3.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocations3.txt 
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocations4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyMultipleRelocations4/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyMultipleRelocations4.testenv
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocations4.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocations4.txt
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocations5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyMultipleRelocations5/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyMultipleRelocations5.testenv
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocations5.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocations5.txt
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocationsAndClasses
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyMultipleRelocationsAndClasses/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyMultipleRelocationsAndClasses.testenv
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocationsAndClasses.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocationsAndClasses.txt 
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocationsAndClasses2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyMultipleRelocationsAndClasses2/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyMultipleRelocationsAndClasses2.testenv
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocationsAndClasses2.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocationsAndClasses2.txt 
)

pxr_register_test(testPcpMuseum_TrickyNonLocalVariantSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd TrickyNonLocalVariantSelection/root.usda" 
    TESTENV testenv/testPcpMuseum_TrickyNonLocalVariantSelection.testenv
    STDOUT_REDIRECT compositionResults_TrickyNonLocalVariantSelection.txt
    DIFF_COMPARE compositionResults_TrickyNonLocalVariantSelection.txt 
)

pxr_register_test(testPcpMuseum_ErrorOpinionAtRelocationSource
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorOpinionAtRelocationSource.txt ErrorOpinionAtRelocationSource/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorOpinionAtRelocationSource.testenv
    STDOUT_REDIRECT compositionResults_ErrorOpinionAtRelocationSource.txt
    DIFF_COMPARE compositionResults_ErrorOpinionAtRelocationSource.txt compositionErrors_ErrorOpinionAtRelocationSource.txt
)

pxr_register_test(testPcpMuseum_ErrorArcCycle
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorArcCycle.txt ErrorArcCycle/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorArcCycle.testenv
    STDOUT_REDIRECT compositionResults_ErrorArcCycle.txt
    DIFF_COMPARE compositionResults_ErrorArcCycle.txt compositionErrors_ErrorArcCycle.txt
)

pxr_register_test(testPcpMuseum_ErrorPermissionDenied
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd  ErrorPermissionDenied/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorPermissionDenied.testenv
    STDOUT_REDIRECT compositionResults_ErrorPermissionDenied_usd.txt
    DIFF_COMPARE compositionResults_ErrorPermissionDenied_usd.txt
)

pxr_register_test(testPcpMuseum_ErrorConnectionPermissionDenied
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd  ErrorConnectionPermissionDenied/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorConnectionPermissionDenied.testenv
    STDOUT_REDIRECT compositionResults_ErrorConnectionPermissionDenied_usd.txt
    DIFF_COMPARE compositionResults_ErrorConnectionPermissionDenied_usd.txt
)

pxr_register_test(testPcpMuseum_ErrorInconsistentProperties
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorInconsistentProperties_usd.txt ErrorInconsistentProperties/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorInconsistentProperties.testenv
    STDOUT_REDIRECT compositionResults_ErrorInconsistentProperties_usd.txt
    DIFF_COMPARE compositionResults_ErrorInconsistentProperties_usd.txt compositionErrors_ErrorInconsistentProperties_usd.txt
)

pxr_register_test(testPcpMuseum_ErrorInvalidInstanceTargetPath
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorInvalidInstanceTargetPath_usd.txt ErrorInvalidInstanceTargetPath/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorInvalidInstanceTargetPath.testenv
    STDOUT_REDIRECT compositionResults_ErrorInvalidInstanceTargetPath_usd.txt
    DIFF_COMPARE compositionResults_ErrorInvalidInstanceTargetPath_usd.txt compositionErrors_ErrorInvalidInstanceTargetPath_usd.txt
)

pxr_register_test(testPcpMuseum_ErrorInvalidPreRelocateTargetPath
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd  ErrorInvalidPreRelocateTargetPath/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorInvalidPreRelocateTargetPath.testenv
    STDOUT_REDIRECT compositionResults_ErrorInvalidPreRelocateTargetPath_usd.txt
    DIFF_COMPARE compositionResults_ErrorInvalidPreRelocateTargetPath_usd.txt
)

pxr_register_test(testPcpMuseum_ErrorInvalidTargetPath
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorInvalidTargetPath.txt ErrorInvalidTargetPath/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorInvalidTargetPath.testenv
    STDOUT_REDIRECT compositionResults_ErrorInvalidTargetPath.txt
    DIFF_COMPARE compositionResults_ErrorInvalidTargetPath.txt compositionErrors_ErrorInvalidTargetPath.txt
)

pxr_register_test(testPcpMuseum_ErrorInvalidPayload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorInvalidPayload.txt ErrorInvalidPayload/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorInvalidPayload.testenv
    STDOUT_REDIRECT compositionResults_ErrorInvalidPayload.txt
    DIFF_COMPARE compositionResults_ErrorInvalidPayload.txt compositionErrors_ErrorInvalidPayload.txt
)

pxr_register_test(testPcpMuseum_ErrorSublayerCycle
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorSublayerCycle.txt ErrorSublayerCycle/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorSublayerCycle.testenv
    STDOUT_REDIRECT compositionResults_ErrorSublayerCycle.txt
    DIFF_COMPARE compositionResults_ErrorSublayerCycle.txt compositionErrors_ErrorSublayerCycle.txt
)

pxr_register_test(testPcpMuseum_ErrorOwner
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorOwner.txt --session ErrorOwner/session.usda ErrorOwner/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorOwner.testenv
    STDOUT_REDIRECT compositionResults_ErrorOwner.txt
    DIFF_COMPARE compositionResults_ErrorOwner.txt compositionErrors_ErrorOwner.txt
)

pxr_register_test(testPcpMuseum_ErrorInvalidAuthoredRelocates
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorInvalidAuthoredRelocates.txt ErrorInvalidAuthoredRelocates/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorInvalidAuthoredRelocates.testenv
    STDOUT_REDIRECT compositionResults_ErrorInvalidAuthoredRelocates.txt
    DIFF_COMPARE compositionResults_ErrorInvalidAuthoredRelocates.txt compositionErrors_ErrorInvalidAuthoredRelocates.txt
)

# XXX:
# Temporarily disable this test on Windows; the compositionErrors output 
# contains extra new lines that don't show up for any other build.
if (NOT WIN32)
    pxr_register_test(testPcpMuseum_ErrorInvalidConflictingRelocates
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorInvalidConflictingRelocates.txt ErrorInvalidConflictingRelocates/root.usda" 
        TESTENV testenv/testPcpMuseum_ErrorInvalidConflictingRelocates.testenv
        STDOUT_REDIRECT compositionResults_ErrorInvalidConflictingRelocates.txt
        DIFF_COMPARE compositionResults_ErrorInvalidConflictingRelocates.txt compositionErrors_ErrorInvalidConflictingRelocates.txt
        ENV 
            PCP_ENABLE_LEGACY_RELOCATES_BEHAVIOR=0
    )
endif()

pxr_register_test(testPcpMuseum_ErrorInvalidReferenceToRelocationSource
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorInvalidReferenceToRelocationSource.txt ErrorInvalidReferenceToRelocationSource/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorInvalidReferenceToRelocationSource.testenv
    STDOUT_REDIRECT compositionResults_ErrorInvalidReferenceToRelocationSource.txt
    DIFF_COMPARE compositionResults_ErrorInvalidReferenceToRelocationSource.txt compositionErrors_ErrorInvalidReferenceToRelocationSource.txt
)

pxr_register_test(testPcpMuseum_ErrorRelocateWithVariantSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_ErrorRelocateWithVariantSelection.txt ErrorRelocateWithVariantSelection/root.usda" 
    TESTENV testenv/testPcpMuseum_ErrorRelocateWithVariantSelection.testenv
    STDOUT_REDIRECT compositionResults_ErrorRelocateWithVariantSelection.txt
    DIFF_COMPARE compositionResults_ErrorRelocateWithVariantSelection.txt compositionErrors_ErrorRelocateWithVariantSelection.txt
    ENV 
        SDF_SCHEMA_PROHIBIT_INVALID_VARIANT_SELECTIONS=0
)

pxr_register_test(testPcpMuseum_VariantSpecializesAndReferenceSurprisingBehavior
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd VariantSpecializesAndReferenceSurprisingBehavior/root.usda" 
    TESTENV testenv/testPcpMuseum_VariantSpecializesAndReferenceSurprisingBehavior.testenv
    STDOUT_REDIRECT compositionResults_VariantSpecializesAndReferenceSurprisingBehavior.txt
    DIFF_COMPARE compositionResults_VariantSpecializesAndReferenceSurprisingBehavior.txt
)

pxr_register_test(testPcpMuseum_VariantSpecializesAndReference
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd VariantSpecializesAndReference/root.usda" 
    TESTENV testenv/testPcpMuseum_VariantSpecializesAndReference.testenv
    STDOUT_REDIRECT compositionResults_VariantSpecializesAndReference.txt
    DIFF_COMPARE compositionResults_VariantSpecializesAndReference.txt
)

pxr_register_test(testPcpIterator
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpIterator"
    DIFF_COMPARE iteration_results.txt
    TESTENV testenv/testPcpIterator.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpMapExpression
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpMapExpression"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpPathTranslation_HardToReach
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpPathTranslation_HardToReach"
    TESTENV testenv/testPcpPathTranslation_HardToReach.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpCache
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCache"
    TESTENV testenv/testPcpMuseum_BasicReference.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpChanges
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpChanges"
    TESTENV testenv/testPcpChanges.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpDependencies
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpDependencies"
    TESTENV testenv/testPcpDependencies.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpHardToReach
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpHardToReach"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpInstanceKey
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpInstanceKey"
    TESTENV testenv/testPcpInstanceKey.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpLayerMuting
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpLayerMuting"
    TESTENV testenv/testPcpLayerMuting.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug69932
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd bug69932/root.usda"
    TESTENV testenv/testPcpRegressionBugs_bug69932.testenv
    STDOUT_REDIRECT compositionResults_bug69932.txt
    DIFF_COMPARE compositionResults_bug69932.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug70951
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug70951"
    TESTENV testenv/testPcpRegressionBugs_bug70951.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug74847
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd bug74847/root.usda"
    TESTENV testenv/testPcpRegressionBugs_bug74847.testenv
    STDOUT_REDIRECT compositionResults_bug74847.txt
    DIFF_COMPARE compositionResults_bug74847.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug82180
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug82180" 
    TESTENV testenv/testPcpRegressionBugs_bug82180.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug90508
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug90508" 
    TESTENV testenv/testPcpRegressionBugs_bug90508.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug90706
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug90706" 
    TESTENV testenv/testPcpRegressionBugs_bug90706.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug92955
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug92955" 
    TESTENV testenv/testPcpRegressionBugs_bug92955.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug92827
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors_bug92827.txt bug92827/root.usda"
    TESTENV testenv/testPcpRegressionBugs_bug92827.testenv
    STDOUT_REDIRECT compositionResults_bug92827.txt
    DIFF_COMPARE compositionResults_bug92827.txt compositionErrors_bug92827.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug101300
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug101300" 
    TESTENV testenv/testPcpRegressionBugs_bug101300.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_USD-6755
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd --errorFile compositionErrors.txt root.usda" 
    TESTENV testenv/testPcpRegressionBugs_USD-6755.testenv
    STDOUT_REDIRECT compositionResults.txt
    DIFF_COMPARE compositionResults.txt compositionErrors.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_case1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --usd case1/root.usda"
    TESTENV testenv/testPcpRegressionBugs_case1.testenv
    STDOUT_REDIRECT compositionResults_case1.txt
    DIFF_COMPARE  compositionResults_case1.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpResolvedPathChange
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpResolvedPathChange"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpMapFunction
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpMapFunction"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpPathTranslation
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpPathTranslation"
    TESTENV testenv/testPcpPathTranslation.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpPrimIndex
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpPrimIndex"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpDynamicFileFormatPlugin
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpDynamicFileFormatPlugin"
    TESTENV testenv/testPcpDynamicFileFormatPlugin.testenv
    EXPECTED_RETURN_CODE 0  
    ENV
        TEST_PCP_DYNAMIC_FILE_FORMAT_TOKENS_USE_ATTRIBUTE_INPUTS=0
)

pxr_register_test(testPcpDynamicFileFormatPlugin_AttributeInputs
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpDynamicFileFormatPlugin"
    TESTENV testenv/testPcpDynamicFileFormatPlugin.testenv
    EXPECTED_RETURN_CODE 0  
    ENV
        TEST_PCP_DYNAMIC_FILE_FORMAT_TOKENS_USE_ATTRIBUTE_INPUTS=1
)

pxr_register_test(testPcpExpressionComposition
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpExpressionComposition"
    TESTENV testenv/testPcpExpressionComposition.testenv
    EXPECTED_RETURN_CODE 0  
)

pxr_register_test(testPcpExpressionVariables
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpExpressionVariables"
    EXPECTED_RETURN_CODE 0  
)

pxr_register_test(testPcpExpressionVariablesSource
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpExpressionVariablesSource"
    EXPECTED_RETURN_CODE 0  
)

pxr_register_test(testPcpStreamingLayerReload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpStreamingLayerReload"
    TESTENV testenv/testPcpStreamingLayerReload.testenv
    EXPECTED_RETURN_CODE 0  
)

pxr_register_test(testPcpOwner
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpOwner"
    TESTENV testenv/testPcpOwner.testenv
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpLayerRelocatesEditBuilder
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpLayerRelocatesEditBuilder"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpNegativeOffsetScale
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpNegativeOffsetScale"
    EXPECTED_RETURN_CODE 0
)
