// swift-tools-version: 5.9
import PackageDescription

let package = Package(
  name: "SwiftUSD",
  platforms: [
    .macOS(.v12),
    .visionOS(.v1),
    .iOS(.v12),
    .tvOS(.v12),
    .watchOS(.v4)
  ],
  products: [
    .library(
      name: "pxr",
      targets: ["pxr"]
    ),
    .library(
      name: "Arch",
      targets: ["Arch"]
    ),
    .library(
      name: "Tf",
      targets: ["Tf"]
    ),
    .library(
      name: "Js",
      targets: ["Js"]
    ),
    .library(
      name: "Gf",
      targets: ["Gf"]
    ),
    .library(
      name: "Trace",
      targets: ["Trace"]
    ),
    .library(
      name: "Vt",
      targets: ["Vt"]
    ),
    .library(
      name: "Work",
      targets: ["Work"]
    ),
    .library(
      name: "Plug",
      targets: ["Plug"]
    ),
    .library(
      name: "Ar",
      targets: ["Ar"]
    ),
    .library(
      name: "Kind",
      targets: ["Kind"]
    ),
    .library(
      name: "Sdf",
      targets: ["Sdf"]
    ),
    .library(
      name: "Pcp",
      targets: ["Pcp"]
    ),
    .library(
      name: "Usd",
      targets: ["Usd"]
    ),
    .library(
      name: "PyTf",
      type: .dynamic,
      targets: ["PyTf"]
    ),
    .library(
      name: "PyPlug",
      type: .dynamic,
      targets: ["PyPlug"]
    ),
    .library(
      name: "PyAr",
      type: .dynamic,
      targets: ["PyAr"]
    ),
    .library(
      name: "PyKind",
      type: .dynamic,
      targets: ["PyKind"]
    ),
    .library(
      name: "PyGf",
      type: .dynamic,
      targets: ["PyGf"]
    ),
    .library(
      name: "PyTrace",
      type: .dynamic,
      targets: ["PyTrace"]
    ),
    .library(
      name: "PyVt",
      type: .dynamic,
      targets: ["PyVt"]
    ),
    .library(
      name: "PyWork",
      type: .dynamic,
      targets: ["PyWork"]
    ),
    .library(
      name: "PySdf",
      type: .dynamic,
      targets: ["PySdf"]
    ),
    .library(
      name: "PyPcp",
      type: .dynamic,
      targets: ["PyPcp"]
    ),
    .library(
      name: "PyUsd",
      type: .dynamic,
      targets: ["PyUsd"]
    ),
    .library(
      name: "PyPixar",
      targets: [
        "PyTf",
        "PyGf",
        "PyTrace",
        "PyVt",
        "PyWork",
        "PyPlug",
        "PyAr",
        "PyKind",
        "PySdf",
        "PyPcp",
        "PyUsd"
      ]
    ),
    .library(
      name: "Pixar",
      targets: ["Pixar"]
    ),
  ],
  dependencies: [
    /* ----------------- a single dependency to rule them all. ----------------- */
    .package(url: "https://github.com/wabiverse/MetaverseKit.git", from: "1.3.4"),
    /* ------------------------------------------------------------------------- */
  ],
  targets: [
    .target(
      name: "pxr",
      path: "pxr",
      exclude: [
        "base",
        "imaging",
        "usd",
        "usdImaging",
      ],
      publicHeadersPath: "."
    ),

    .target(
      name: "Arch",
      dependencies: [
        /* ------------ pxr Namespace. ---------- */
        .target(name: "pxr"),
        /* ------------ VFX Platform. ----------- */
        .product(name: "MetaTBB", package: "MetaverseKit"),
        .product(name: "MaterialX", package: "MetaverseKit"),
        .product(name: "Boost", package: "MetaverseKit"),
        .product(name: "MetaPy", package: "MetaverseKit"),
        .product(name: "Alembic", package: "MetaverseKit"),
        .product(name: "OpenColorIO", package: "MetaverseKit"),
        .product(name: "OpenImageIO", package: "MetaverseKit"),
        .product(name: "OpenEXR", package: "MetaverseKit"),
        .product(name: "OpenSubdiv", package: "MetaverseKit"),
        .product(name: "OpenVDB", package: "MetaverseKit"),
        .product(name: "Ptex", package: "MetaverseKit"),
        .product(name: "Draco", package: "MetaverseKit"),
        .product(name: "Eigen", package: "MetaverseKit"),
        /* ---------- Apple only libs. ---------- */
        .product(name: "Apple", package: "MetaverseKit", condition: .when(platforms: Arch.OS.apple.platform)),
        .product(name: "MoltenVK", package: "MetaverseKit", condition: .when(platforms: Arch.OS.apple.platform)),
      ],
      path: ".",
      exclude: [
        ".git",
        ".build",
        ".github",
        "BUILDING.md",
        "CHANGELOG.md",
        "CMakeLists.txt",
        "CODE_OF_CONDUCT.md",
        "CONTRIBUTING.md",
        "LICENSE.txt",
        "NOTICE.txt",
        "Package.resolved",
        "Package.swift",
        "README.md",
        "USD_CLA_Corporate.pdf",
        "USD_CLA_Individual.pdf",
        "VERSIONS.md",
        "azure-pipelines.yml",
        "azure-pypi-pipeline.yml",
        "build_scripts",
        "cmake",
        "docs",
        "extras",
        "swift",
        "third_party",
        "pxr/imaging",
        "pxr/usd",
        "pxr/usdImaging",
        "pxr/base/arch/pch.h",
        "pxr/base/arch/testenv",
        "pxr/base/tf",
        "pxr/base/gf",
        "pxr/base/js",
        "pxr/base/plug",
        "pxr/base/trace",
        "pxr/base/vt",
        "pxr/base/work"
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        /* ---------- Turn everything on. ---------- */
        .define("PXR_USE_NAMESPACES", to: "1"),
        .define("PXR_PYTHON_SUPPORT_ENABLED", to: "1"),
        .define("PXR_PREFER_SAFETY_OVER_SPEED", to: "1"),
        .define("PXR_VULKAN_SUPPORT_ENABLED", to: "1"),
        .define("PXR_OCIO_PLUGIN_ENABLED", to: "1"),
        .define("PXR_OIIO_PLUGIN_ENABLED", to: "1"),
        .define("PXR_PTEX_SUPPORT_ENABLED", to: "1"),
        .define("PXR_OPENVDB_SUPPORT_ENABLED", to: "1"),
        .define("PXR_MATERIALX_SUPPORT_ENABLED", to: "1"),
        .define("PXR_HDF5_SUPPORT_ENABLED", to: "1"),
        /* --------- OSL is temp disabled. --------- */
        .define("PXR_OSL_SUPPORT_ENABLED", to: "0"),
        /* --------- Apple platforms only. --------- */
        .define("PXR_METAL_SUPPORT_ENABLED", to: "1", .when(platforms: Arch.OS.apple.platform)),
        .define("MFB_PACKAGE_NAME", to: "Arch"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Arch"),
        .define("MFB_PACKAGE_MODULE", to: "Arch"),
        .define("ARCH_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Tf",
      dependencies: [
        .target(name: "Arch"),
      ],
      path: "pxr/base/tf",
      exclude: [
        "pch.h",
        "testenv",
        "module.cpp",
        "moduleDeps.cpp",
        "pyWeakObject.cpp",
        "wrapAnyWeakPtr.cpp",
        "wrapCallContext.cpp",
        "wrapDebug.cpp",
        "wrapDiagnostic.cpp",
        "wrapDiagnosticBase.cpp",
        "wrapEnum.cpp",
        "wrapEnvSetting.cpp",
        "wrapError.cpp",
        "wrapException.cpp",
        "wrapFileUtils.cpp",
        "wrapFunction.cpp",
        "wrapMallocTag.cpp",
        "wrapNotice.cpp",
        "wrapPathUtils.cpp",
        "wrapPyContainerConversions.cpp",
        "wrapPyModuleNotice.cpp",
        "wrapPyObjWrapper.cpp",
        "wrapPyOptional.cpp",
        "wrapRefPtrTracker.cpp",
        "wrapScopeDescription.cpp",
        "wrapScriptModuleLoader.cpp",
        "wrapSingleton.cpp",
        "wrapStackTrace.cpp",
        "wrapStatus.cpp",
        "wrapStopwatch.cpp",
        "wrapStringUtils.cpp",
        "wrapTemplateString.cpp",
        "wrapTestPyAnnotatedBoolResult.cpp",
        "wrapTestPyContainerConversions.cpp",
        "wrapTestPyStaticTokens.cpp",
        "wrapTestTfPyOptional.cpp",
        "wrapTestTfPython.cpp",
        "wrapToken.cpp",
        "wrapType.cpp",
        "wrapTypeHelpers.cpp",
        "wrapWarning.cpp",
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Tf"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Tf"),
        .define("MFB_PACKAGE_MODULE", to: "Tf"),
        .define("TF_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Js",
      dependencies: [
        .target(name: "Arch"),
        .target(name: "Tf"),
      ],
      path: "pxr/base/js",
      exclude: [
        "pch.h",
        "testenv"
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Js"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Js"),
        .define("MFB_PACKAGE_MODULE", to: "Js"),
        .define("JS_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Gf",
      dependencies: [
        .target(name: "Arch"),
        .target(name: "Tf"),
      ],
      path: "pxr/base/gf",
      exclude: [
        "pch.h",
        "testenv",
        "module.cpp",
        "moduleDeps.cpp",
        "wrapBBox3d.cpp",
        "wrapCamera.cpp",
        "wrapDualQuatd.cpp",
        "wrapDualQuatf.cpp",
        "wrapDualQuath.cpp",
        "wrapFrustum.cpp",
        "wrapGamma.cpp",
        "wrapHalf.cpp",
        "wrapHomogeneous.cpp",
        "wrapInterval.cpp",
        "wrapLimits.cpp",
        "wrapLine.cpp",
        "wrapLineSeg.cpp",
        "wrapMath.cpp",
        "wrapMatrix2d.cpp",
        "wrapMatrix2f.cpp",
        "wrapMatrix3d.cpp",
        "wrapMatrix3f.cpp",
        "wrapMatrix4d.cpp",
        "wrapMatrix4f.cpp",
        "wrapMultiInterval.cpp",
        "wrapPlane.cpp",
        "wrapQuatd.cpp",
        "wrapQuaternion.cpp",
        "wrapQuatf.cpp",
        "wrapQuath.cpp",
        "wrapRange1d.cpp",
        "wrapRange1f.cpp",
        "wrapRange2d.cpp",
        "wrapRange2f.cpp",
        "wrapRange3d.cpp",
        "wrapRange3f.cpp",
        "wrapRay.cpp",
        "wrapRect2i.cpp",
        "wrapRotation.cpp",
        "wrapSize2.cpp",
        "wrapSize3.cpp",
        "wrapTransform.cpp",
        "wrapVec2d.cpp",
        "wrapVec2f.cpp",
        "wrapVec2h.cpp",
        "wrapVec2i.cpp",
        "wrapVec3d.cpp",
        "wrapVec3f.cpp",
        "wrapVec3h.cpp",
        "wrapVec3i.cpp",
        "wrapVec4d.cpp",
        "wrapVec4f.cpp",
        "wrapVec4h.cpp",
        "wrapVec4i.cpp",
        "dualQuat.template.cpp",
        "matrix.template.cpp",
        "matrix2.template.cpp",
        "matrix3.template.cpp",
        "matrix4.template.cpp",
        "quat.template.cpp",
        "range.template.cpp",
        "vec.template.cpp",
        "wrapDualQuat.template.cpp",
        "wrapMatrix.template.cpp",
        "wrapMatrix2.template.cpp",
        "wrapMatrix3.template.cpp",
        "wrapMatrix4.template.cpp",
        "wrapQuat.template.cpp",
        "wrapRange.template.cpp",
        "wrapVec.template.cpp",
        "dualQuat.template.h",
        "matrix.template.h",
        "matrix2.template.h",
        "matrix3.template.h",
        "matrix4.template.h",
        "quat.template.h",
        "range.template.h",
        "vec.template.h"
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Gf"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Gf"),
        .define("MFB_PACKAGE_MODULE", to: "Gf"),
        .define("GF_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Trace",
      dependencies: [
        .target(name: "Arch"),
        .target(name: "Tf"),
        .target(name: "Js"),
      ],
      path: "pxr/base/trace",
      exclude: [
        "pch.h",
        "testenv",
        "module.cpp",
        "moduleDeps.cpp",
        "wrapAggregateNode.cpp",
        "wrapCollector.cpp",
        "wrapReporter.cpp",
        "wrapTestTrace.cpp"
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Trace"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Trace"),
        .define("MFB_PACKAGE_MODULE", to: "Trace"),
        .define("TRACE_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Vt",
      dependencies: [
        .target(name: "Arch"),
        .target(name: "Tf"),
        .target(name: "Gf"),
        .target(name: "Trace"),
      ],
      path: "pxr/base/vt",
      exclude: [
        "pch.h",
        "testenv",
        "module.cpp",
        "moduleDeps.cpp",
        "wrapArray.cpp",
        "wrapArrayBase.cpp",
        "wrapArrayDualQuaternion.cpp",
        "wrapArrayFloat.cpp",
        "wrapArrayIntegral.cpp",
        "wrapArrayMatrix.cpp",
        "wrapArrayQuaternion.cpp",
        "wrapArrayRange.cpp",
        "wrapArrayString.cpp",
        "wrapArrayToken.cpp",
        "wrapArrayVec.cpp",
        "wrapDictionary.cpp",
        "wrapValue.cpp"
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Vt"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Vt"),
        .define("MFB_PACKAGE_MODULE", to: "Vt"),
        .define("VT_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Work",
      dependencies: [
        .target(name: "Arch"),
        .target(name: "Tf"),
        .target(name: "Trace"),
      ],
      path: "pxr/base/work",
      exclude: [
        "pch.h",
        "testenv",
        "module.cpp",
        "moduleDeps.cpp",
        "wrapThreadLimits.cpp"
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Work"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Work"),
        .define("MFB_PACKAGE_MODULE", to: "Work"),
        .define("WORK_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Plug",
      dependencies: [
        .target(name: "Arch"),
        .target(name: "Tf"),
        .target(name: "Js"),
        .target(name: "Trace"),
        .target(name: "Work"),
      ],
      path: "pxr/base/plug",
      exclude: [
        "pch.h",
        "testenv",
        "module.cpp",
        "moduleDeps.cpp",
        "wrapNotice.cpp",
        "wrapPlugin.cpp",
        "wrapRegistry.cpp",
        "wrapTestPlugBase.cpp"
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Plug"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Plug"),
        .define("MFB_PACKAGE_MODULE", to: "Plug"),
        .define("PLUG_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Ar",
      dependencies: [
        .target(name: "Arch"),
        .target(name: "Tf"),
        .target(name: "Js"),
        .target(name: "Plug"),
        .target(name: "Vt"),
      ],
      path: "pxr/usd/ar",
      exclude: [
        "pch.h",
        "testenv",
        "module.cpp",
        "moduleDeps.cpp",
        "pyResolverContext.cpp",
        "wrapAssetInfo.cpp",
        "wrapDefaultResolver.cpp",
        "wrapDefaultResolverContext.cpp",
        "wrapNotice.cpp",
        "wrapPackageUtils.cpp",
        "wrapResolvedPath.cpp",
        "wrapResolver.cpp",
        "wrapResolverContext.cpp",
        "wrapResolverContextBinder.cpp",
        "wrapResolverScopedCache.cpp",
        "wrapTimestamp.cpp"
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Ar"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Ar"),
        .define("MFB_PACKAGE_MODULE", to: "Ar"),
        .define("AR_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Kind",
      dependencies: [
        .target(name: "Arch"),
        .target(name: "Tf"),
        .target(name: "Plug"),
      ],
      path: "pxr/usd/kind",
      exclude: [
        "pch.h",
        "testenv",
        "module.cpp",
        "moduleDeps.cpp",
        "wrapRegistry.cpp",
        "wrapTokens.cpp"
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Kind"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Kind"),
        .define("MFB_PACKAGE_MODULE", to: "Kind"),
        .define("KIND_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Sdf",
      dependencies: [
        .target(name: "Arch"),
        .target(name: "Tf"),
        .target(name: "Gf"),
        .target(name: "Plug"),
        .target(name: "Trace"),
        .target(name: "Work"),
        .target(name: "Vt"),
        .target(name: "Ar"),
      ],
      path: "pxr/usd/sdf",
      exclude: [
        "pch.h",
        "testenv",
        "module.cpp",
        "moduleDeps.cpp",
        "wrapArrayAssetPath.cpp",
        "wrapArrayPath.cpp",
        "wrapArrayTimeCode.cpp",
        "wrapAssetPath.cpp",
        "wrapAttributeSpec.cpp",
        "wrapChangeBlock.cpp",
        "wrapCleanupEnabler.cpp",
        "wrapCopyUtils.cpp",
        "wrapFileFormat.cpp",
        "wrapLayer.cpp",
        "wrapLayerOffset.cpp",
        "wrapLayerTree.cpp",
        "wrapNamespaceEdit.cpp",
        "wrapNotice.cpp",
        "wrapOpaqueValue.cpp",
        "wrapPath.cpp",
        "wrapPathExpression.cpp",
        "wrapPayload.cpp",
        "wrapPredicateExpression.cpp",
        "wrapPrimSpec.cpp",
        "wrapPropertySpec.cpp",
        "wrapPseudoRootSpec.cpp",
        "wrapReference.cpp",
        "wrapRelationshipSpec.cpp",
        "wrapSpec.cpp",
        "wrapTimeCode.cpp",
        "wrapTypes.cpp",
        "wrapValueTypeName.cpp",
        "wrapVariableExpression.cpp",
        "wrapVariantSetSpec.cpp",
        "wrapVariantSpec.cpp"
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Sdf"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Sdf"),
        .define("MFB_PACKAGE_MODULE", to: "Sdf"),
        .define("SDF_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Pcp",
      dependencies: [
        .target(name: "Arch"),
        .target(name: "Tf"),
        .target(name: "Trace"),
        .target(name: "Work"),
        .target(name: "Vt"),
        .target(name: "Ar"),
        .target(name: "Sdf"),
      ],
      path: "pxr/usd/pcp",
      exclude: [
        "pch.h",
        "testenv",
        "module.cpp",
        "moduleDeps.cpp",
        "wrapCache.cpp",
        "wrapDependency.cpp",
        "wrapDynamicFileFormatDependencyData.cpp",
        "wrapErrors.cpp",
        "wrapExpressionVariables.cpp",
        "wrapExpressionVariablesSource.cpp",
        "wrapInstanceKey.cpp",
        "wrapLayerStack.cpp",
        "wrapLayerStackIdentifier.cpp",
        "wrapMapExpression.cpp",
        "wrapMapFunction.cpp",
        "wrapNode.cpp",
        "wrapPathTranslation.cpp",
        "wrapPrimIndex.cpp",
        "wrapPropertyIndex.cpp",
        "wrapSite.cpp",
        "wrapTestChangeProcessor.cpp",
        "wrapTypes.cpp"
      ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Pcp"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Pcp"),
        .define("MFB_PACKAGE_MODULE", to: "Pcp"),
        .define("PCP_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "Usd",
      dependencies: [
        .target(name: "Arch"),
        .target(name: "Tf"),
        .target(name: "Trace"),
        .target(name: "Work"),
        .target(name: "Vt"),
        .target(name: "Plug"),
        .target(name: "Gf"),
        .target(name: "Kind"),
        .target(name: "Ar"),
        .target(name: "Sdf"),
        .target(name: "Pcp"),
      ],
      path: "pxr/usd/usd",
      exclude: [
        "pch.h",
        "codegenTemplates",
        "testenv",
        "module.cpp",
        "moduleDeps.cpp",
        "wrapAPISchemaBase.cpp",
        "wrapAttribute.cpp",
        "wrapAttributeQuery.cpp",
        "wrapClipsAPI.cpp",
        "wrapCollectionAPI.cpp",
        "wrapCollectionMembershipQuery.cpp",
        "wrapCommon.cpp",
        "wrapCrateInfo.cpp",
        "wrapEditContext.cpp",
        "wrapEditTarget.cpp",
        "wrapFlattenUtils.cpp",
        "wrapInherits.cpp",
        "wrapInterpolation.cpp",
        "wrapModelAPI.cpp",
        "wrapNotice.cpp",
        "wrapObject.cpp",
        "wrapPayloads.cpp",
        "wrapPrim.cpp",
        "wrapPrimCompositionQuery.cpp",
        "wrapPrimDefinition.cpp",
        "wrapPrimFlags.cpp",
        "wrapPrimRange.cpp",
        "wrapPrimTypeInfo.cpp",
        "wrapProperty.cpp",
        "wrapReferences.cpp",
        "wrapRelationship.cpp",
        "wrapResolveInfo.cpp",
        "wrapResolveTarget.cpp",
        "wrapSchemaBase.cpp",
        "wrapSchemaRegistry.cpp",
        "wrapSpecializes.cpp",
        "wrapStage.cpp",
        "wrapStageCache.cpp",
        "wrapStageCacheContext.cpp",
        "wrapStageLoadRules.cpp",
        "wrapStagePopulationMask.cpp",
        "wrapTimeCode.cpp",
        "wrapTokens.cpp",
        "wrapTyped.cpp",
        "wrapUsdFileFormat.cpp",
        "wrapUtils.cpp",
        "wrapVariantSets.cpp",
        "wrapVersion.cpp",
        "wrapZipFile.cpp",
        "examples.cpp"
      ],
      // resources: [
      //   .copy("usd/usd/codegenTemplates"),
      //   .process("Resources")
      // ],
      publicHeadersPath: ".",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Usd"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Usd"),
        .define("MFB_PACKAGE_MODULE", to: "Usd"),
        .define("USD_EXPORTS", to: "1")
      ]
    ),

    .target(
      name: "PyTf",
      dependencies: [
        .target(name: "Pixar"),
      ],
      path: "pxr/base/tf",
      exclude: [
        "pch.h"
      ],
      sources: [
        "module.cpp",
        "moduleDeps.cpp",
        "pyWeakObject.cpp",
        "wrapAnyWeakPtr.cpp",
        "wrapCallContext.cpp",
        "wrapDebug.cpp",
        "wrapDiagnostic.cpp",
        "wrapDiagnosticBase.cpp",
        "wrapEnum.cpp",
        "wrapEnvSetting.cpp",
        "wrapError.cpp",
        "wrapException.cpp",
        "wrapFileUtils.cpp",
        "wrapFunction.cpp",
        "wrapMallocTag.cpp",
        "wrapNotice.cpp",
        "wrapPathUtils.cpp",
        "wrapPyContainerConversions.cpp",
        "wrapPyModuleNotice.cpp",
        "wrapPyObjWrapper.cpp",
        "wrapPyOptional.cpp",
        "wrapRefPtrTracker.cpp",
        "wrapScopeDescription.cpp",
        "wrapScriptModuleLoader.cpp",
        "wrapSingleton.cpp",
        "wrapStackTrace.cpp",
        "wrapStatus.cpp",
        "wrapStopwatch.cpp",
        "wrapStringUtils.cpp",
        "wrapTemplateString.cpp",
        "wrapTestPyAnnotatedBoolResult.cpp",
        "wrapTestPyContainerConversions.cpp",
        "wrapTestPyStaticTokens.cpp",
        "wrapTestTfPyOptional.cpp",
        "wrapTestTfPython.cpp",
        "wrapToken.cpp",
        "wrapType.cpp",
        "wrapTypeHelpers.cpp",
        "wrapWarning.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "include",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Tf"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Tf"),
        .define("MFB_PACKAGE_MODULE", to: "Tf"),
      ]
    ),

    .target(
      name: "PyGf",
      dependencies: [
        .target(name: "Pixar"),
      ],
      path: "pxr/base/gf",
      exclude: [
        "pch.h"
      ],
      sources: [
        "module.cpp",
        "moduleDeps.cpp",
        "wrapBBox3d.cpp",
        "wrapCamera.cpp",
        "wrapDualQuatd.cpp",
        "wrapDualQuatf.cpp",
        "wrapDualQuath.cpp",
        "wrapFrustum.cpp",
        "wrapGamma.cpp",
        "wrapHalf.cpp",
        "wrapHomogeneous.cpp",
        "wrapInterval.cpp",
        "wrapLimits.cpp",
        "wrapLine.cpp",
        "wrapLineSeg.cpp",
        "wrapMath.cpp",
        "wrapMatrix2d.cpp",
        "wrapMatrix2f.cpp",
        "wrapMatrix3d.cpp",
        "wrapMatrix3f.cpp",
        "wrapMatrix4d.cpp",
        "wrapMatrix4f.cpp",
        "wrapMultiInterval.cpp",
        "wrapPlane.cpp",
        "wrapQuatd.cpp",
        "wrapQuaternion.cpp",
        "wrapQuatf.cpp",
        "wrapQuath.cpp",
        "wrapRange1d.cpp",
        "wrapRange1f.cpp",
        "wrapRange2d.cpp",
        "wrapRange2f.cpp",
        "wrapRange3d.cpp",
        "wrapRange3f.cpp",
        "wrapRay.cpp",
        "wrapRect2i.cpp",
        "wrapRotation.cpp",
        "wrapSize2.cpp",
        "wrapSize3.cpp",
        "wrapTransform.cpp",
        "wrapVec2d.cpp",
        "wrapVec2f.cpp",
        "wrapVec2h.cpp",
        "wrapVec2i.cpp",
        "wrapVec3d.cpp",
        "wrapVec3f.cpp",
        "wrapVec3h.cpp",
        "wrapVec3i.cpp",
        "wrapVec4d.cpp",
        "wrapVec4f.cpp",
        "wrapVec4h.cpp",
        "wrapVec4i.cpp"
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "include",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Gf"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Gf"),
        .define("MFB_PACKAGE_MODULE", to: "Gf"),
      ]
    ),

    .target(
      name: "PyTrace",
      dependencies: [
        .target(name: "Pixar"),
      ],
      path: "pxr/base/trace",
      exclude: [
        "pch.h"
      ],
      sources: [
        "module.cpp",
        "moduleDeps.cpp",
        "wrapAggregateNode.cpp",
        "wrapCollector.cpp",
        "wrapReporter.cpp",
        "wrapTestTrace.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "include",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Trace"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Trace"),
        .define("MFB_PACKAGE_MODULE", to: "Trace"),
      ]
    ),

    .target(
      name: "PyVt",
      dependencies: [
        .target(name: "Pixar"),
      ],
      path: "pxr/base/vt",
      exclude: [
        "pch.h"
      ],
      sources: [
        "module.cpp",
        "moduleDeps.cpp",
        "wrapArray.cpp",
        "wrapArrayBase.cpp",
        "wrapArrayDualQuaternion.cpp",
        "wrapArrayFloat.cpp",
        "wrapArrayIntegral.cpp",
        "wrapArrayMatrix.cpp",
        "wrapArrayQuaternion.cpp",
        "wrapArrayRange.cpp",
        "wrapArrayString.cpp",
        "wrapArrayToken.cpp",
        "wrapArrayVec.cpp",
        "wrapDictionary.cpp",
        "wrapValue.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "include",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Vt"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Vt"),
        .define("MFB_PACKAGE_MODULE", to: "Vt"),
      ]
    ),

    .target(
      name: "PyWork",
      dependencies: [
        .target(name: "Pixar"),
      ],
      path: "pxr/base/work",
      exclude: [
        "pch.h"
      ],
      sources: [
        "module.cpp",
        "moduleDeps.cpp",
        "wrapThreadLimits.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "include",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Work"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Work"),
        .define("MFB_PACKAGE_MODULE", to: "Work"),
      ]
    ),

    .target(
      name: "PyPlug",
      dependencies: [
        .target(name: "Pixar"),
      ],
      path: "pxr/base/plug",
      exclude: [
        "pch.h"
      ],
      sources: [
        "module.cpp",
        "moduleDeps.cpp",
        "wrapNotice.cpp",
        "wrapPlugin.cpp",
        "wrapRegistry.cpp",
        "wrapTestPlugBase.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "include",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Plug"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Plug"),
        .define("MFB_PACKAGE_MODULE", to: "Plug"),
      ]
    ),

    .target(
      name: "PyAr",
      dependencies: [
        .target(name: "Pixar"),
      ],
      path: "pxr/usd/ar",
      exclude: [
        "pch.h"
      ],
      sources: [
        "module.cpp",
        "moduleDeps.cpp",
        "pyResolverContext.cpp",
        "wrapAssetInfo.cpp",
        "wrapDefaultResolver.cpp",
        "wrapDefaultResolverContext.cpp",
        "wrapNotice.cpp",
        "wrapPackageUtils.cpp",
        "wrapResolvedPath.cpp",
        "wrapResolver.cpp",
        "wrapResolverContext.cpp",
        "wrapResolverContextBinder.cpp",
        "wrapResolverScopedCache.cpp",
        "wrapTimestamp.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "include",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Ar"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Ar"),
        .define("MFB_PACKAGE_MODULE", to: "Ar"),
      ]
    ),

    .target(
      name: "PyKind",
      dependencies: [
        .target(name: "Pixar"),
      ],
      path: "pxr/usd/kind",
      exclude: [
        "pch.h"
      ],
      sources: [
        "module.cpp",
        "moduleDeps.cpp",
        "wrapRegistry.cpp",
        "wrapTokens.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "include",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Kind"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Kind"),
        .define("MFB_PACKAGE_MODULE", to: "Kind"),
      ]
    ),

    .target(
      name: "PySdf",
      dependencies: [
        .target(name: "Pixar"),
      ],
      path: "pxr/usd/sdf",
      exclude: [
        "pch.h"
      ],
      sources: [
        "module.cpp",
        "moduleDeps.cpp",
        "wrapArrayAssetPath.cpp",
        "wrapArrayPath.cpp",
        "wrapArrayTimeCode.cpp",
        "wrapAssetPath.cpp",
        "wrapAttributeSpec.cpp",
        "wrapChangeBlock.cpp",
        "wrapCleanupEnabler.cpp",
        "wrapCopyUtils.cpp",
        "wrapFileFormat.cpp",
        "wrapLayer.cpp",
        "wrapLayerOffset.cpp",
        "wrapLayerTree.cpp",
        "wrapNamespaceEdit.cpp",
        "wrapNotice.cpp",
        "wrapOpaqueValue.cpp",
        "wrapPath.cpp",
        "wrapPathExpression.cpp",
        "wrapPayload.cpp",
        "wrapPredicateExpression.cpp",
        "wrapPrimSpec.cpp",
        "wrapPropertySpec.cpp",
        "wrapPseudoRootSpec.cpp",
        "wrapReference.cpp",
        "wrapRelationshipSpec.cpp",
        "wrapSpec.cpp",
        "wrapTimeCode.cpp",
        "wrapTypes.cpp",
        "wrapValueTypeName.cpp",
        "wrapVariableExpression.cpp",
        "wrapVariantSetSpec.cpp",
        "wrapVariantSpec.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "include",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Sdf"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Sdf"),
        .define("MFB_PACKAGE_MODULE", to: "Sdf"),
      ]
    ),

    .target(
      name: "PyPcp",
      dependencies: [
        .target(name: "Pixar"),
      ],
      path: "pxr/usd/pcp",
      exclude: [
        "pch.h"
      ],
      sources: [
        "module.cpp",
        "moduleDeps.cpp",
        "wrapCache.cpp",
        "wrapDependency.cpp",
        "wrapDynamicFileFormatDependencyData.cpp",
        "wrapErrors.cpp",
        "wrapExpressionVariables.cpp",
        "wrapExpressionVariablesSource.cpp",
        "wrapInstanceKey.cpp",
        "wrapLayerStack.cpp",
        "wrapLayerStackIdentifier.cpp",
        "wrapMapExpression.cpp",
        "wrapMapFunction.cpp",
        "wrapNode.cpp",
        "wrapPathTranslation.cpp",
        "wrapPrimIndex.cpp",
        "wrapPropertyIndex.cpp",
        "wrapSite.cpp",
        "wrapTestChangeProcessor.cpp",
        "wrapTypes.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "include",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Pcp"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Pcp"),
        .define("MFB_PACKAGE_MODULE", to: "Pcp"),
      ]
    ),

    .target(
      name: "PyUsd",
      dependencies: [
        .target(name: "Pixar"),
      ],
      path: "pxr/usd/usd",
      exclude: [
        "pch.h"
      ],
      sources: [
        "module.cpp",
        "moduleDeps.cpp",
        "wrapAPISchemaBase.cpp",
        "wrapAttribute.cpp",
        "wrapAttributeQuery.cpp",
        "wrapClipsAPI.cpp",
        "wrapCollectionAPI.cpp",
        "wrapCollectionMembershipQuery.cpp",
        "wrapCommon.cpp",
        "wrapCrateInfo.cpp",
        "wrapEditContext.cpp",
        "wrapEditTarget.cpp",
        "wrapFlattenUtils.cpp",
        "wrapInherits.cpp",
        "wrapInterpolation.cpp",
        "wrapModelAPI.cpp",
        "wrapNotice.cpp",
        "wrapObject.cpp",
        "wrapPayloads.cpp",
        "wrapPrim.cpp",
        "wrapPrimCompositionQuery.cpp",
        "wrapPrimDefinition.cpp",
        "wrapPrimFlags.cpp",
        "wrapPrimRange.cpp",
        "wrapPrimTypeInfo.cpp",
        "wrapProperty.cpp",
        "wrapReferences.cpp",
        "wrapRelationship.cpp",
        "wrapResolveInfo.cpp",
        "wrapResolveTarget.cpp",
        "wrapSchemaBase.cpp",
        "wrapSchemaRegistry.cpp",
        "wrapSpecializes.cpp",
        "wrapStage.cpp",
        "wrapStageCache.cpp",
        "wrapStageCacheContext.cpp",
        "wrapStageLoadRules.cpp",
        "wrapStagePopulationMask.cpp",
        "wrapTimeCode.cpp",
        "wrapTokens.cpp",
        "wrapTyped.cpp",
        "wrapUsdFileFormat.cpp",
        "wrapUtils.cpp",
        "wrapVariantSets.cpp",
        "wrapVersion.cpp",
        "wrapZipFile.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "include",
      cxxSettings: [
        .define("MFB_PACKAGE_NAME", to: "Usd"),
        .define("MFB_ALT_PACKAGE_NAME", to: "Usd"),
        .define("MFB_PACKAGE_MODULE", to: "Usd"),
      ]
    ),

    .target(
      name: "Pixar",
      dependencies: [
        /* ------ base. ------ */
        .target(name: "Arch"),
        .target(name: "Tf"),
        .target(name: "Js"),
        .target(name: "Gf"),
        .target(name: "Trace"),
        .target(name: "Vt"),
        .target(name: "Work"),
        .target(name: "Plug"),
        /* ------- usd. ------ */
        .target(name: "Ar"),
        .target(name: "Kind"),
        .target(name: "Sdf"),
        .target(name: "Pcp"),
        .target(name: "Usd"),
        /* ------------------- */
      ],
      path: "swift/Pixar",
      swiftSettings: [
        .interoperabilityMode(.Cxx),
      ]
    )
  ],
  cxxLanguageStandard: .cxx17
)

/* --- xxx --- */

/* ------------------------------------------------------------------------------
 * :: :  M  E  T  A  V  E  R  S  E  :                                          ::
 * ------------------------------------------------------------------------------
 * Pixar's USD is a pretty complicated build. That being said, this single config
 * is less than 100 lines of code. Compare that to the ~21419 lines of CMake code
 * across ~156 files as of the current release branch, SwiftPM brings substantial
 * improvements which drive maintainability and readability, and that's before we
 * even get to the benefits of Swift, ...or the fact it's now connected to a very
 * capable package manager, in a lot of ways, this is a game changer. Through the
 * Swift Package Manager, we can now leverage the power of USD, and what it means
 * to build plugins, and applications, in a way that is much more accessible to
 * the wider community. This is a very exciting time for the USD project, and we
 * are very excited to be a part of it.
 *
 *                       Copyright (C) 2023 Wabi Foundation. All Rights Reserved.
 * ------------------------------------------------------------------------------
 *  . x x x . o o o . x x x . : : : .  o  x  o    . : : : .
 * ------------------------------------------------------------------------------ */

/* --- xxx --- */

/** ------------------------------------------------
 * Just to tidy up the package configuration above,
 * we define some helper functions and types below.
 * ------------------------------------------------ */
enum Arch
{
  /** OS platforms, grouped by family. */
  enum OS
  {
    case apple
    case linux
    case windows
    case web

    public var platform: [Platform]
    {
      switch self
      {
        case .apple: [.macOS, .iOS, .visionOS, .tvOS, .watchOS]
        case .linux: [.linux, .android, .openbsd]
        case .windows: [.windows]
        case .web: [.wasi]
      }
    }
  }
}
