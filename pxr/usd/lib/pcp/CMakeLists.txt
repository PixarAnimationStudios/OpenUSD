set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE pcp)

pxr_library(pcp
    LIBRARIES
        tf
        tracelite
        vt
        sdf
        work
        ar
        ${Boost_PYTHON_LIBRARY}
        ${TBB_tbb_LIBRARY}

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

    PYTHON_CPPFILES
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

    PYMODULE_FILES
        __init__.py
)

pxr_test_scripts(
    testenv/testPcpCache.py
    testenv/testPcpChanges.py
    testenv/testPcpInstanceKey.py
    testenv/testPcpLayerMuting.py
    testenv/testPcpDependencies.py
    testenv/testPcpPayloadDecorator.py
    testenv/testPcpPathTranslation.py
    testenv/testPcpMapFunction.py
    testenv/testPcpOwner.py
    testenv/testPcpRegressionBugs_bug70951.py
    testenv/testPcpRegressionBugs_bug82180.py
    testenv/testPcpRegressionBugs_bug90508.py
    testenv/testPcpRegressionBugs_bug90706.py
    testenv/testPcpRegressionBugs_bug92955.py
    testenv/testPcpRegressionBugs_bug101300.py
    testenv/testPcpCompositionResults.py
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

pxr_install_test_dir(
    SRC testenv/testPcpOwner.testenv
    DEST testPcpOwner
)

pxr_install_test_dir(
    SRC testenv/testPcpPathTranslation.testenv
    DEST testPcpPathTranslation
)

pxr_install_test_dir(
    SRC testenv/testPcpIterator.testenv
    DEST testPcpIterator
)

pxr_install_test_dir(
    SRC testenv/testPcpDependencies.testenv
    DEST testPcpDependencies
)

pxr_install_test_dir(
    SRC testenv/testPcpPathTranslation_HardToReach.testenv
    DEST testPcpPathTranslation_HardToReach
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicReference.testenv
    DEST testPcpCache
)

pxr_install_test_dir(
    SRC testenv/testPcpChanges.testenv
    DEST testPcpChanges
)

pxr_install_test_dir(
    SRC testenv/testPcpInstanceKey.testenv
    DEST testPcpInstanceKey
)

pxr_install_test_dir(
    SRC testenv/testPcpLayerMuting.testenv
    DEST testPcpLayerMuting
) 

pxr_install_test_dir(
    SRC testenv/testPcpIterator.testenv
    DEST testPcpIterator 
)

pxr_install_test_dir(
    SRC testenv/testPcpOwner.testenv
    DEST testPcpOwner
)
    
pxr_install_test_dir(
    SRC testenv/testPcpChanges.testenv
    DEST testPcpChanges 
)

pxr_install_test_dir(
    SRC testenv/testPcpPathTranslation.testenv
    DEST testPcpPathTranslation
)

pxr_install_test_dir(
    SRC testenv/testPcpPayloadDecorator.testenv
    DEST testPcpPayloadDecorator
)

pxr_install_test_dir(
    SRC testenv/testPcpRegressionBugs_bug69932.testenv
    DEST testPcpRegressionBugs_bug69932
)

pxr_install_test_dir(
    SRC testenv/testPcpRegressionBugs_bug70951.testenv
    DEST testPcpRegressionBugs_bug70951
)

pxr_install_test_dir(
    SRC testenv/testPcpRegressionBugs_bug74847.testenv
    DEST testPcpRegressionBugs_bug74847
)

pxr_install_test_dir(
    SRC testenv/testPcpRegressionBugs_bug82180.testenv
    DEST testPcpRegressionBugs_bug82180
)

pxr_install_test_dir(
    SRC testenv/testPcpRegressionBugs_bug90508.testenv
    DEST testPcpRegressionBugs_bug90508
)

pxr_install_test_dir(
    SRC testenv/testPcpRegressionBugs_bug90706.testenv 
    DEST testPcpRegressionBugs_bug90706
)

pxr_install_test_dir(
    SRC testenv/testPcpRegressionBugs_bug92955.testenv
    DEST testPcpRegressionBugs_bug92955
)

pxr_install_test_dir(
    SRC testenv/testPcpRegressionBugs_bug92827.testenv
    DEST testPcpRegressionBugs_bug92827
)

pxr_install_test_dir(
    SRC testenv/testPcpRegressionBugs_bug101300.testenv
    DEST testPcpRegressionBugs_bug101300
)

pxr_install_test_dir(
    SRC testenv/testPcpRegressionBugs_case1.testenv
    DEST testPcpRegressionBugs_case1
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicInstancing.testenv
    DEST testPcpMuseum_BasicInstancing
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicInstancingAndNestedInstances.testenv
    DEST testPcpMuseum_BasicInstancingAndNestedInstances
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicInstancingAndVariants.testenv
    DEST testPcpMuseum_BasicInstancingAndVariants
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicReference.testenv
    DEST testPcpMuseum_BasicReference
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicReferenceAndClass.testenv
    DEST testPcpMuseum_BasicReferenceAndClass
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicReferenceAndClassDiamond.testenv
    DEST testPcpMuseum_BasicReferenceAndClassDiamond
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicReferenceDiamond.testenv
    DEST testPcpMuseum_BasicReferenceDiamond
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicAncestralReference.testenv
    DEST testPcpMuseum_BasicAncestralReference
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicRelocateToAnimInterface.testenv
    DEST testPcpMuseum_BasicRelocateToAnimInterface
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicInherits.testenv
    DEST testPcpMuseum_BasicInherits
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicLocalAndGlobalClassCombination.testenv
    DEST testPcpMuseum_BasicLocalAndGlobalClassCombination
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicListEditing.testenv
    DEST testPcpMuseum_BasicListEditing
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicListEditingWithInherits.testenv
    DEST testPcpMuseum_BasicListEditingWithInherits
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicDuplicateSublayer.testenv
    DEST testPcpMuseum_BasicDuplicateSublayer
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicTimeOffset.testenv
    DEST testPcpMuseum_BasicTimeOffset
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicNestedPayload.testenv
    DEST testPcpMuseum_BasicNestedPayload
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicNestedVariants.testenv
    DEST testPcpMuseum_BasicNestedVariants
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicNestedVariantsWithSameName.testenv
    DEST testPcpMuseum_BasicNestedVariantsWithSameName
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicVariantWithConnections.testenv
    DEST testPcpMuseum_BasicVariantWithConnections
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicOwner.testenv
    DEST testPcpMuseum_BasicOwner
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicPayload.testenv
    DEST testPcpMuseum_BasicPayload
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicSpecializes.testenv
    DEST testPcpMuseum_BasicSpecializes
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicSpecializesAndInherits.testenv
    DEST testPcpMuseum_BasicSpecializesAndInherits
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicSpecializesAndReferences.testenv
    DEST testPcpMuseum_BasicSpecializesAndReferences
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_BasicSpecializesAndVariants.testenv
    DEST testPcpMuseum_BasicSpecializesAndVariants
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_PayloadsAndAncestralArcs.testenv
    DEST testPcpMuseum_PayloadsAndAncestralArcs
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_RelativePathReferences.testenv
    DEST testPcpMuseum_RelativePathReferences
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_RelocatePrimsWithSameName.testenv
    DEST testPcpMuseum_RelocatePrimsWithSameName
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_SubrootReferenceAndClasses.testenv
    DEST testPcpMuseum_SubrootReferenceAndClasses
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_SubrootReferenceAndVariants.testenv
    DEST testPcpMuseum_SubrootReferenceAndVariants
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TypicalReferenceToRiggedModel.testenv
    DEST testPcpMuseum_TypicalReferenceToRiggedModel
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TypicalReferenceToChargroup.testenv
    DEST testPcpMuseum_TypicalReferenceToChargroup
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TypicalReferenceToChargroupWithRename.testenv
    DEST testPcpMuseum_TypicalReferenceToChargroupWithRename
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickySpookyInherits.testenv
    DEST testPcpMuseum_TrickySpookyInherits
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickySpookyInheritsInSymmetricArmRig.testenv
    DEST testPcpMuseum_TrickySpookyInheritsInSymmetricArmRig
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickySpookyInheritsInSymmetricBrowRig.testenv
    DEST testPcpMuseum_TrickySpookyInheritsInSymmetricBrowRig
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyInheritedVariantSelection.testenv
    DEST testPcpMuseum_TrickyInheritedVariantSelection
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyInheritsAndRelocates.testenv
    DEST testPcpMuseum_TrickyInheritsAndRelocates
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyInheritsAndRelocates2.testenv
    DEST testPcpMuseum_TrickyInheritsAndRelocates2
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyInheritsAndRelocates3.testenv
    DEST testPcpMuseum_TrickyInheritsAndRelocates3
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyInheritsAndRelocates4.testenv
    DEST testPcpMuseum_TrickyInheritsAndRelocates4
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyInheritsAndRelocates5.testenv
    DEST testPcpMuseum_TrickyInheritsAndRelocates5
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyInheritsInVariants.testenv
    DEST testPcpMuseum_TrickyInheritsInVariants
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyInheritsInVariants2.testenv
    DEST testPcpMuseum_TrickyInheritsInVariants2
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyVariantAncestralSelection.testenv
    DEST testPcpMuseum_TrickyVariantAncestralSelection
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyVariantIndependentSelection.testenv
    DEST testPcpMuseum_TrickyVariantIndependentSelection
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyVariantInPayload.testenv
    DEST testPcpMuseum_TrickyVariantInPayload
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyVariantOverrideOfLocalClass.testenv
    DEST testPcpMuseum_TrickyVariantOverrideOfLocalClass
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyVariantOverrideOfRelocatedPrim.testenv
    DEST testPcpMuseum_TrickyVariantOverrideOfRelocatedPrim
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyVariantSelectionInVariant.testenv
    DEST testPcpMuseum_TrickyVariantSelectionInVariant
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyVariantWeakerSelection.testenv
    DEST testPcpMuseum_TrickyVariantWeakerSelection
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyVariantWeakerSelection2.testenv
    DEST testPcpMuseum_TrickyVariantWeakerSelection2
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyVariantWeakerSelection3.testenv
    DEST testPcpMuseum_TrickyVariantWeakerSelection3
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyClassHierarchy.testenv
    DEST testPcpMuseum_TrickyClassHierarchy
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyLocalClassHierarchyWithRelocates.testenv
    DEST testPcpMuseum_TrickyLocalClassHierarchyWithRelocates
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyNestedClasses.testenv
    DEST testPcpMuseum_TrickyNestedClasses
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyNestedClasses2.testenv
    DEST testPcpMuseum_TrickyNestedClasses2
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyNestedClasses3.testenv
    DEST testPcpMuseum_TrickyNestedClasses3
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyNestedSpecializes.testenv
    DEST testPcpMuseum_TrickyNestedSpecializes
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyNestedSpecializes2.testenv
    DEST testPcpMuseum_TrickyNestedSpecializes2
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyNestedClasses4.testenv
    DEST testPcpMuseum_TrickyNestedClasses4
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyNestedVariants.testenv
    DEST testPcpMuseum_TrickyNestedVariants
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickySpecializesAndInherits.testenv
    DEST testPcpMuseum_TrickySpecializesAndInherits
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickySpecializesAndInherits2.testenv
    DEST testPcpMuseum_TrickySpecializesAndInherits2
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickySpecializesAndRelocates.testenv
    DEST testPcpMuseum_TrickySpecializesAndRelocates
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickySpookyVariantSelection.testenv
    DEST testPcpMuseum_TrickySpookyVariantSelection
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickySpookyVariantSelectionInClass.testenv
    DEST testPcpMuseum_TrickySpookyVariantSelectionInClass
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyRelationalAttributeOnRelocatedTarget.testenv
    DEST testPcpMuseum_TrickyRelationalAttributeOnRelocatedTarget
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyConnectionToRelocatedAttribute.testenv
    DEST testPcpMuseum_TrickyConnectionToRelocatedAttribute
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyRelocatedTargetInVariant.testenv
    DEST testPcpMuseum_TrickyRelocatedTargetInVariant
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyRelocationOfPrimFromPayload.testenv
    DEST testPcpMuseum_TrickyRelocationOfPrimFromPayload
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyRelocationOfPrimFromVariant.testenv
    DEST testPcpMuseum_TrickyRelocationOfPrimFromVariant
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyRelocationSquatter.testenv
    DEST testPcpMuseum_TrickyRelocationSquatter
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyMultipleRelocations.testenv
    DEST testPcpMuseum_TrickyMultipleRelocations
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyMultipleRelocations2.testenv
    DEST testPcpMuseum_TrickyMultipleRelocations2
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyMultipleRelocations3.testenv
    DEST testPcpMuseum_TrickyMultipleRelocations3
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyMultipleRelocations4.testenv
    DEST testPcpMuseum_TrickyMultipleRelocations4
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyMultipleRelocationsAndClasses.testenv
    DEST testPcpMuseum_TrickyMultipleRelocationsAndClasses
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyMultipleRelocationsAndClasses2.testenv
    DEST testPcpMuseum_TrickyMultipleRelocationsAndClasses2
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyNonLocalVariantSelection.testenv
    DEST testPcpMuseum_TrickyNonLocalVariantSelection
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_TrickyListEditedTargetPaths.testenv
    DEST testPcpMuseum_TrickyListEditedTargetPaths
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorArcCycle.testenv
    DEST testPcpMuseum_ErrorArcCycle
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorPermissionDenied.testenv
    DEST testPcpMuseum_ErrorPermissionDenied
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorConnectionPermissionDenied.testenv
    DEST testPcpMuseum_ErrorConnectionPermissionDenied
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorInconsistentProperties.testenv
    DEST testPcpMuseum_ErrorInconsistentProperties
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorInvalidInstanceTargetPath.testenv
    DEST testPcpMuseum_ErrorInvalidInstanceTargetPath
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorInvalidPreRelocateTargetPath.testenv
    DEST testPcpMuseum_ErrorInvalidPreRelocateTargetPath
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorInvalidTargetPath.testenv
    DEST testPcpMuseum_ErrorInvalidTargetPath
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorInvalidPayload.testenv
    DEST testPcpMuseum_ErrorInvalidPayload
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorOpinionAtRelocationSource.testenv
    DEST testPcpMuseum_ErrorOpinionAtRelocationSource
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorSublayerCycle.testenv
    DEST testPcpMuseum_ErrorSublayerCycle
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorOwner.testenv
    DEST testPcpMuseum_ErrorOwner
)

pxr_install_test_dir(
    SRC testenv/testPcpMuseum_ErrorRelocateToSelf.testenv
    DEST testPcpMuseum_ErrorRelocateToSelf
)

pxr_register_test(testPcpMuseum_BasicInstancing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicInstancing/root.sdf"
    STDOUT_REDIRECT compositionResults_BasicInstancing.txt
    DIFF_COMPARE compositionResults_BasicInstancing.txt 
    ENV 
        PCP_OVERRIDE_INSTANCEABLE=1
)

pxr_register_test(testPcpMuseum_BasicInstancingAndNestedInstances
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicInstancingAndNestedInstances/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicInstancingAndNestedInstances.txt
    DIFF_COMPARE compositionResults_BasicInstancingAndNestedInstances.txt 
    ENV 
        PCP_OVERRIDE_INSTANCEABLE=1
)

pxr_register_test(testPcpMuseum_BasicInstancingAndVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicInstancingAndVariants/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicInstancingAndVariants.txt
    DIFF_COMPARE compositionResults_BasicInstancingAndVariants.txt 
    ENV 
        PCP_OVERRIDE_INSTANCEABLE=1
)

pxr_register_test(testPcpMuseum_BasicReference
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicReference/root.sdf --session BasicReference/session.sdf" 
    STDOUT_REDIRECT compositionResults_BasicReference.txt
    DIFF_COMPARE compositionResults_BasicReference.txt 
)

pxr_register_test(testPcpMuseum_BasicReferenceAndClass
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicReferenceAndClass/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicReferenceAndClass.txt
    DIFF_COMPARE compositionResults_BasicReferenceAndClass.txt 
)

pxr_register_test(testPcpMuseum_BasicReferenceAndClassDiamond
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicReferenceAndClassDiamond/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicReferenceAndClassDiamond.txt
    DIFF_COMPARE compositionResults_BasicReferenceAndClassDiamond.txt 
)

pxr_register_test(testPcpMuseum_BasicAncestralReference
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicAncestralReference/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicAncestralReference.txt
    DIFF_COMPARE compositionResults_BasicAncestralReference.txt 
)

pxr_register_test(testPcpMuseum_BasicReferenceDiamond
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicReferenceDiamond/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicReferenceDiamond_pcp.txt
    DIFF_COMPARE compositionResults_BasicReferenceDiamond_pcp.txt 
)

pxr_register_test(testPcpMuseum_BasicRelocateToAnimInterface
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicRelocateToAnimInterface/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicRelocateToAnimInterface.txt
    DIFF_COMPARE compositionResults_BasicRelocateToAnimInterface.txt 
)

pxr_register_test(testPcpMuseum_BasicInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicInherits/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicInherits.txt
    DIFF_COMPARE compositionResults_BasicInherits.txt 
)

pxr_register_test(testPcpMuseum_BasicLocalAndGlobalClassCombination
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicLocalAndGlobalClassCombination/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicLocalAndGlobalClassCombination.txt
    DIFF_COMPARE compositionResults_BasicLocalAndGlobalClassCombination.txt 
)

pxr_register_test(testPcpMuseum_BasicListEditing
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicListEditing/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicListEditing.txt
    DIFF_COMPARE compositionResults_BasicListEditing.txt 
)

pxr_register_test(testPcpMuseum_BasicListEditingWithInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicListEditingWithInherits/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicListEditingWithInherits_pcp.txt
    DIFF_COMPARE compositionResults_BasicListEditingWithInherits_pcp.txt 
)

pxr_register_test(testPcpMuseum_BasicDuplicateSublayer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicDuplicateSublayer/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicDuplicateSublayer.txt
    DIFF_COMPARE compositionResults_BasicDuplicateSublayer.txt 
)

pxr_register_test(testPcpMuseum_BasicTimeOffset
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicTimeOffset/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicTimeOffset.txt
    DIFF_COMPARE compositionResults_BasicTimeOffset.txt 
)

pxr_register_test(testPcpMuseum_BasicNestedPayload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicNestedPayload/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicNestedPayload.txt
    DIFF_COMPARE compositionResults_BasicNestedPayload.txt 
)

pxr_register_test(testPcpMuseum_BasicNestedVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicNestedVariants/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicNestedVariants_pcp.txt
    DIFF_COMPARE compositionResults_BasicNestedVariants_pcp.txt 
)

pxr_register_test(testPcpMuseum_BasicNestedVariantsWithSameName
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicNestedVariantsWithSameName/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicNestedVariantsWithSameName.txt
    DIFF_COMPARE compositionResults_BasicNestedVariantsWithSameName.txt 
)

pxr_register_test(testPcpMuseum_BasicVariantWithConnections
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicVariantWithConnections/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicVariantWithConnections.txt
    DIFF_COMPARE compositionResults_BasicVariantWithConnections.txt 
)

pxr_register_test(testPcpMuseum_BasicOwner
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --session BasicOwner/session.sdf BasicOwner/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicOwner.txt
    DIFF_COMPARE compositionResults_BasicOwner.txt 
)

pxr_register_test(testPcpMuseum_BasicPayload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicPayload/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicPayload_pcp.txt
    DIFF_COMPARE compositionResults_BasicPayload_pcp.txt 
)

pxr_register_test(testPcpMuseum_BasicSpecializes
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicSpecializes/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicSpecializes.txt
    DIFF_COMPARE compositionResults_BasicSpecializes.txt 
)

pxr_register_test(testPcpMuseum_BasicSpecializesAndInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicSpecializesAndInherits/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicSpecializesAndInherits.txt
    DIFF_COMPARE compositionResults_BasicSpecializesAndInherits.txt 
)

pxr_register_test(testPcpMuseum_BasicSpecializesAndReferences
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicSpecializesAndReferences/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicSpecializesAndReferences.txt
    DIFF_COMPARE compositionResults_BasicSpecializesAndReferences.txt 
)

pxr_register_test(testPcpMuseum_BasicSpecializesAndVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults BasicSpecializesAndVariants/root.sdf" 
    STDOUT_REDIRECT compositionResults_BasicSpecializesAndVariants.txt
    DIFF_COMPARE compositionResults_BasicSpecializesAndVariants.txt 
)

pxr_register_test(testPcpMuseum_PayloadsAndAncestralArcs
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --payloads ^(?!/Root/UnloadedInheritedPayload) PayloadsAndAncestralArcs/root.sdf" 
    STDOUT_REDIRECT compositionResults_PayloadsAndAncestralArcs.txt
    DIFF_COMPARE compositionResults_PayloadsAndAncestralArcs.txt 
)

pxr_register_test(testPcpMuseum_RelativePathReferences
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults RelativePathReferences/root.sdf" 
    STDOUT_REDIRECT compositionResults_RelativePathReferences.txt
    DIFF_COMPARE compositionResults_RelativePathReferences.txt 
)

pxr_register_test(testPcpMuseum_RelocatePrimsWithSameName
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults RelocatePrimsWithSameName/root.sdf" 
    STDOUT_REDIRECT compositionResults_RelocatePrimsWithSameName.txt
    DIFF_COMPARE compositionResults_RelocatePrimsWithSameName.txt 
)

pxr_register_test(testPcpMuseum_SubrootReferenceAndClasses
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults SubrootReferenceAndClasses/root.sdf" 
    STDOUT_REDIRECT compositionResults_SubrootReferenceAndClasses.txt
    DIFF_COMPARE compositionResults_SubrootReferenceAndClasses.txt 
)

pxr_register_test(testPcpMuseum_SubrootReferenceAndVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults SubrootReferenceAndVariants/root.sdf" 
    STDOUT_REDIRECT compositionResults_SubrootReferenceAndVariants.txt
    DIFF_COMPARE compositionResults_SubrootReferenceAndVariants.txt 
)

pxr_register_test(testPcpMuseum_TypicalReferenceToRiggedModel
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TypicalReferenceToRiggedModel/root.sdf" 
    STDOUT_REDIRECT compositionResults_TypicalReferenceToRiggedModel.txt
    DIFF_COMPARE compositionResults_TypicalReferenceToRiggedModel.txt 
)

pxr_register_test(testPcpMuseum_TypicalReferenceToChargroup
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TypicalReferenceToChargroup/root.sdf" 
    STDOUT_REDIRECT compositionResults_TypicalReferenceToChargroup_pcp.txt
    DIFF_COMPARE compositionResults_TypicalReferenceToChargroup_pcp.txt 
)

pxr_register_test(testPcpMuseum_TypicalReferenceToChargroupWithRename
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TypicalReferenceToChargroupWithRename/root.sdf" 
    STDOUT_REDIRECT compositionResults_TypicalReferenceToChargroupWithRename.txt
    DIFF_COMPARE compositionResults_TypicalReferenceToChargroupWithRename.txt 
)

pxr_register_test(testPcpMuseum_TrickyListEditedTargetPaths
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyListEditedTargetPaths/root.sdf"
    STDOUT_REDIRECT compositionResults_TrickyListEditedTargetPaths.txt
    DIFF_COMPARE compositionResults_TrickyListEditedTargetPaths.txt
)

pxr_register_test(testPcpMuseum_TrickySpookyInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickySpookyInherits/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickySpookyInherits_pcp.txt
    DIFF_COMPARE compositionResults_TrickySpookyInherits_pcp.txt 
)

pxr_register_test(testPcpMuseum_TrickySpookyInheritsInSymmetricArmRig
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickySpookyInheritsInSymmetricArmRig/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickySpookyInheritsInSymmetricArmRig.txt
    DIFF_COMPARE compositionResults_TrickySpookyInheritsInSymmetricArmRig.txt 
)

pxr_register_test(testPcpMuseum_TrickySpookyInheritsInSymmetricBrowRig
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickySpookyInheritsInSymmetricBrowRig/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickySpookyInheritsInSymmetricBrowRig.txt
    DIFF_COMPARE compositionResults_TrickySpookyInheritsInSymmetricBrowRig.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsAndRelocates2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyInheritsAndRelocates2/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyInheritsAndRelocates2.txt
    DIFF_COMPARE compositionResults_TrickyInheritsAndRelocates2.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsAndRelocates3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyInheritsAndRelocates3/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyInheritsAndRelocates3.txt
    DIFF_COMPARE compositionResults_TrickyInheritsAndRelocates3.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsAndRelocates4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyInheritsAndRelocates4/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyInheritsAndRelocates4.txt
    DIFF_COMPARE compositionResults_TrickyInheritsAndRelocates4.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsAndRelocates5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyInheritsAndRelocates5/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyInheritsAndRelocates5.txt
    DIFF_COMPARE compositionResults_TrickyInheritsAndRelocates5.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsInVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyInheritsInVariants/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyInheritsInVariants.txt
    DIFF_COMPARE compositionResults_TrickyInheritsInVariants.txt 
)

pxr_register_test(testPcpMuseum_TrickyInheritsInVariants2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyInheritsInVariants2/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyInheritsInVariants2.txt
    DIFF_COMPARE compositionResults_TrickyInheritsInVariants2.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantAncestralSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyVariantAncestralSelection/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyVariantAncestralSelection_pcp.txt
    DIFF_COMPARE compositionResults_TrickyVariantAncestralSelection_pcp.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantIndependentSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyVariantIndependentSelection/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyVariantIndependentSelection_pcp.txt
    DIFF_COMPARE compositionResults_TrickyVariantIndependentSelection_pcp.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantInPayload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyVariantInPayload/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyVariantInPayload.txt
    DIFF_COMPARE compositionResults_TrickyVariantInPayload.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantOverrideOfLocalClass
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyVariantOverrideOfLocalClass/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyVariantOverrideOfLocalClass.txt
    DIFF_COMPARE compositionResults_TrickyVariantOverrideOfLocalClass.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantOverrideOfRelocatedPrim
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyVariantOverrideOfRelocatedPrim/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyVariantOverrideOfRelocatedPrim.txt
    DIFF_COMPARE compositionResults_TrickyVariantOverrideOfRelocatedPrim.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantSelectionInVariant
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyVariantSelectionInVariant/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyVariantSelectionInVariant_pcp.txt
    DIFF_COMPARE compositionResults_TrickyVariantSelectionInVariant_pcp.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantWeakerSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyVariantWeakerSelection/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyVariantWeakerSelection_pcp.txt
    DIFF_COMPARE compositionResults_TrickyVariantWeakerSelection_pcp.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantWeakerSelection2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyVariantWeakerSelection2/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyVariantWeakerSelection2.txt
    DIFF_COMPARE compositionResults_TrickyVariantWeakerSelection2.txt 
)

pxr_register_test(testPcpMuseum_TrickyVariantWeakerSelection3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyVariantWeakerSelection3/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyVariantWeakerSelection3.txt
    DIFF_COMPARE compositionResults_TrickyVariantWeakerSelection3.txt 
)

pxr_register_test(testPcpMuseum_TrickyClassHierarchy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyClassHierarchy/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyClassHierarchy.txt
    DIFF_COMPARE compositionResults_TrickyClassHierarchy.txt 
)

pxr_register_test(testPcpMuseum_TrickyLocalClassHierarchyWithRelocates
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyLocalClassHierarchyWithRelocates/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyLocalClassHierarchyWithRelocates.txt
    DIFF_COMPARE compositionResults_TrickyLocalClassHierarchyWithRelocates.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedClasses
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyNestedClasses/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyNestedClasses.txt
    DIFF_COMPARE compositionResults_TrickyNestedClasses.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedClasses2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyNestedClasses2/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyNestedClasses2.txt
    DIFF_COMPARE compositionResults_TrickyNestedClasses2.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedClasses3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyNestedClasses3/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyNestedClasses3.txt
    DIFF_COMPARE compositionResults_TrickyNestedClasses3.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedSpecializes
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyNestedSpecializes/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyNestedSpecializes.txt
    DIFF_COMPARE compositionResults_TrickyNestedSpecializes.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedSpecializes2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyNestedSpecializes2/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyNestedSpecializes2.txt
    DIFF_COMPARE compositionResults_TrickyNestedSpecializes2.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedClasses4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyNestedClasses4/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyNestedClasses4.txt
    DIFF_COMPARE compositionResults_TrickyNestedClasses4.txt 
)

pxr_register_test(testPcpMuseum_TrickyNestedVariants
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyNestedVariants/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyNestedVariants.txt
    DIFF_COMPARE compositionResults_TrickyNestedVariants.txt 
)

pxr_register_test(testPcpMuseum_TrickySpecializesAndInherits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickySpecializesAndInherits/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickySpecializesAndInherits.txt
    DIFF_COMPARE compositionResults_TrickySpecializesAndInherits.txt 
)

pxr_register_test(testPcpMuseum_TrickySpecializesAndInherits2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickySpecializesAndInherits2/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickySpecializesAndInherits2.txt
    DIFF_COMPARE compositionResults_TrickySpecializesAndInherits2.txt 
)

pxr_register_test(testPcpMuseum_TrickySpecializesAndRelocates
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickySpecializesAndRelocates/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickySpecializesAndRelocates.txt
    DIFF_COMPARE compositionResults_TrickySpecializesAndRelocates.txt 
)

pxr_register_test(testPcpMuseum_TrickySpookyVariantSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickySpookyVariantSelection/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickySpookyVariantSelection_pcp.txt
    DIFF_COMPARE compositionResults_TrickySpookyVariantSelection_pcp.txt 
)

pxr_register_test(testPcpMuseum_TrickyRelationalAttributeOnRelocatedTarget
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyRelationalAttributeOnRelocatedTarget/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyRelationalAttributeOnRelocatedTarget.txt
    DIFF_COMPARE compositionResults_TrickyRelationalAttributeOnRelocatedTarget.txt 
)

pxr_register_test(testPcpMuseum_TrickyConnectionToRelocatedAttribute
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyConnectionToRelocatedAttribute/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyConnectionToRelocatedAttribute.txt
    DIFF_COMPARE compositionResults_TrickyConnectionToRelocatedAttribute.txt 
)

pxr_register_test(testPcpMuseum_TrickyRelocatedTargetInVariant
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyRelocatedTargetInVariant/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyRelocatedTargetInVariant.txt
    DIFF_COMPARE compositionResults_TrickyRelocatedTargetInVariant.txt 
)

pxr_register_test(testPcpMuseum_TrickyRelocationOfPrimFromPayload
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyRelocationOfPrimFromPayload/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyRelocationOfPrimFromPayload_pcp.txt
    DIFF_COMPARE compositionResults_TrickyRelocationOfPrimFromPayload_pcp.txt 
)

pxr_register_test(testPcpMuseum_TrickyRelocationOfPrimFromVariant
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyRelocationOfPrimFromVariant/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyRelocationOfPrimFromVariant_pcp.txt
    DIFF_COMPARE compositionResults_TrickyRelocationOfPrimFromVariant_pcp.txt 
)

pxr_register_test(testPcpMuseum_TrickyRelocationSquatter
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyRelocationSquatter/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyRelocationSquatter.txt
    DIFF_COMPARE compositionResults_TrickyRelocationSquatter.txt 
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocations
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyMultipleRelocations/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocations.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocations.txt 
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocations2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyMultipleRelocations2/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocations2.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocations2.txt 
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocations3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyMultipleRelocations3/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocations3.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocations3.txt 
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocations4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --errorFile compositionErrors_TrickyMultipleRelocations4.txt TrickyMultipleRelocations4/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocations4.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocations4.txt compositionErrors_TrickyMultipleRelocations4.txt 
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocationsAndClasses
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyMultipleRelocationsAndClasses/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocationsAndClasses.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocationsAndClasses.txt 
)

pxr_register_test(testPcpMuseum_TrickyMultipleRelocationsAndClasses2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyMultipleRelocationsAndClasses2/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyMultipleRelocationsAndClasses2.txt
    DIFF_COMPARE compositionResults_TrickyMultipleRelocationsAndClasses2.txt 
)

pxr_register_test(testPcpMuseum_TrickyNonLocalVariantSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults TrickyNonLocalVariantSelection/root.sdf" 
    STDOUT_REDIRECT compositionResults_TrickyNonLocalVariantSelection.txt
    DIFF_COMPARE compositionResults_TrickyNonLocalVariantSelection.txt 
)

pxr_register_test(testPcpMuseum_ErrorOpinionAtRelocationSource
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --errorFile compositionErrors_ErrorOpinionAtRelocationSource_pcp.txt ErrorOpinionAtRelocationSource/root.sdf" 
    STDOUT_REDIRECT compositionResults_ErrorOpinionAtRelocationSource.txt
    DIFF_COMPARE compositionResults_ErrorOpinionAtRelocationSource.txt
)

pxr_register_test(testPcpMuseum_ErrorRelocateToSelf
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults --errorFile compositionErrors_ErrorRelocateToSelf_pcp.txt ErrorRelocateToSelf/root.sdf" 
    STDOUT_REDIRECT compositionResults_ErrorRelocateToSelf_pcp.txt
    DIFF_COMPARE compositionResults_ErrorRelocateToSelf_pcp.txt compositionErrors_ErrorRelocateToSelf_pcp.txt
)


pxr_register_test(testPcpIterator
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpIterator"
    DIFF_COMPARE iteration_results.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpMapExpression
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpMapExpression"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpPathTranslation_HardToReach
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpPathTranslation_HardToReach"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpCache
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCache"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpChanges
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpChanges"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpDependencies
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpDependencies"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpInstanceKey
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpInstanceKey"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpLayerMuting
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpLayerMuting"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug69932
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults bug69932/root.sdf"
    STDOUT_REDIRECT compositionResults_bug69932.txt
    DIFF_COMPARE compositionResults_bug69932.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug70951
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug70951"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug74847
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults bug74847/root.sdf"
    STDOUT_REDIRECT compositionResults_bug74847.txt
    DIFF_COMPARE compositionResults_bug74847.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug82180
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug82180" 
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug90508
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug90508" 
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug90706
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug90706" 
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug92955
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug92955" 
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug92827
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults bug92827/root.sdf"
    STDOUT_REDIRECT compositionResults_bug92827.txt
    DIFF_COMPARE compositionResults_bug92827.txt
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_bug101300
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpRegressionBugs_bug101300" 
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpRegressionBugs_case1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpCompositionResults case1/root.sdf"
    STDOUT_REDIRECT compositionResults_case1.txt
    DIFF_COMPARE  compositionResults_case1.txt
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
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testPcpPayloadDecorator
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpPayloadDecorator"
    EXPECTED_RETURN_CODE 0  
)

pxr_register_test(testPcpOwner
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testPcpOwner"
    EXPECTED_RETURN_CODE 0
)
