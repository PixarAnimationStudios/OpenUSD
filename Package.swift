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
      path: "pxr",
      exclude: [
        "base/tf/testenv",
        "base/tf/module.cpp",
        "base/tf/moduleDeps.cpp",
        "base/tf/pyWeakObject.cpp",
        "base/tf/wrapAnyWeakPtr.cpp",
        "base/tf/wrapCallContext.cpp",
        "base/tf/wrapDebug.cpp",
        "base/tf/wrapDiagnostic.cpp",
        "base/tf/wrapDiagnosticBase.cpp",
        "base/tf/wrapEnum.cpp",
        "base/tf/wrapEnvSetting.cpp",
        "base/tf/wrapError.cpp",
        "base/tf/wrapException.cpp",
        "base/tf/wrapFileUtils.cpp",
        "base/tf/wrapFunction.cpp",
        "base/tf/wrapMallocTag.cpp",
        "base/tf/wrapNotice.cpp",
        "base/tf/wrapPathUtils.cpp",
        "base/tf/wrapPyContainerConversions.cpp",
        "base/tf/wrapPyModuleNotice.cpp",
        "base/tf/wrapPyObjWrapper.cpp",
        "base/tf/wrapPyOptional.cpp",
        "base/tf/wrapRefPtrTracker.cpp",
        "base/tf/wrapScopeDescription.cpp",
        "base/tf/wrapScriptModuleLoader.cpp",
        "base/tf/wrapSingleton.cpp",
        "base/tf/wrapStackTrace.cpp",
        "base/tf/wrapStatus.cpp",
        "base/tf/wrapStopwatch.cpp",
        "base/tf/wrapStringUtils.cpp",
        "base/tf/wrapTemplateString.cpp",
        "base/tf/wrapTestPyAnnotatedBoolResult.cpp",
        "base/tf/wrapTestPyContainerConversions.cpp",
        "base/tf/wrapTestPyStaticTokens.cpp",
        "base/tf/wrapTestTfPyOptional.cpp",
        "base/tf/wrapTestTfPython.cpp",
        "base/tf/wrapToken.cpp",
        "base/tf/wrapType.cpp",
        "base/tf/wrapTypeHelpers.cpp",
        "base/tf/wrapWarning.cpp",
        "base/arch",
        "base/gf",
        "base/js",
        "base/plug",
        "base/trace",
        "base/vt",
        "base/work",
        "imaging",
        "usd",
        "usdImaging",
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
      path: "pxr",
      exclude: [
        "imaging",
        "usd",
        "usdImaging",
        "base/js/testenv",
        "base/arch",
        "base/tf",
        "base/gf",
        "base/plug",
        "base/trace",
        "base/vt",
        "base/work"
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
      path: "pxr",
      exclude: [
        "imaging",
        "usd",
        "usdImaging",
        "base/gf/testenv",
        "base/gf/module.cpp",
        "base/gf/moduleDeps.cpp",
        "base/gf/wrapBBox3d.cpp",
        "base/gf/wrapCamera.cpp",
        "base/gf/wrapDualQuatd.cpp",
        "base/gf/wrapDualQuatf.cpp",
        "base/gf/wrapDualQuath.cpp",
        "base/gf/wrapFrustum.cpp",
        "base/gf/wrapGamma.cpp",
        "base/gf/wrapHalf.cpp",
        "base/gf/wrapHomogeneous.cpp",
        "base/gf/wrapInterval.cpp",
        "base/gf/wrapLimits.cpp",
        "base/gf/wrapLine.cpp",
        "base/gf/wrapLineSeg.cpp",
        "base/gf/wrapMath.cpp",
        "base/gf/wrapMatrix2d.cpp",
        "base/gf/wrapMatrix2f.cpp",
        "base/gf/wrapMatrix3d.cpp",
        "base/gf/wrapMatrix3f.cpp",
        "base/gf/wrapMatrix4d.cpp",
        "base/gf/wrapMatrix4f.cpp",
        "base/gf/wrapMultiInterval.cpp",
        "base/gf/wrapPlane.cpp",
        "base/gf/wrapQuatd.cpp",
        "base/gf/wrapQuaternion.cpp",
        "base/gf/wrapQuatf.cpp",
        "base/gf/wrapQuath.cpp",
        "base/gf/wrapRange1d.cpp",
        "base/gf/wrapRange1f.cpp",
        "base/gf/wrapRange2d.cpp",
        "base/gf/wrapRange2f.cpp",
        "base/gf/wrapRange3d.cpp",
        "base/gf/wrapRange3f.cpp",
        "base/gf/wrapRay.cpp",
        "base/gf/wrapRect2i.cpp",
        "base/gf/wrapRotation.cpp",
        "base/gf/wrapSize2.cpp",
        "base/gf/wrapSize3.cpp",
        "base/gf/wrapTransform.cpp",
        "base/gf/wrapVec2d.cpp",
        "base/gf/wrapVec2f.cpp",
        "base/gf/wrapVec2h.cpp",
        "base/gf/wrapVec2i.cpp",
        "base/gf/wrapVec3d.cpp",
        "base/gf/wrapVec3f.cpp",
        "base/gf/wrapVec3h.cpp",
        "base/gf/wrapVec3i.cpp",
        "base/gf/wrapVec4d.cpp",
        "base/gf/wrapVec4f.cpp",
        "base/gf/wrapVec4h.cpp",
        "base/gf/wrapVec4i.cpp",
        "base/arch",
        "base/tf",
        "base/js",
        "base/plug",
        "base/trace",
        "base/vt",
        "base/work",
        "base/gf/dualQuat.template.cpp",
        "base/gf/matrix.template.cpp",
        "base/gf/matrix2.template.cpp",
        "base/gf/matrix3.template.cpp",
        "base/gf/matrix4.template.cpp",
        "base/gf/quat.template.cpp",
        "base/gf/range.template.cpp",
        "base/gf/vec.template.cpp",
        "base/gf/wrapDualQuat.template.cpp",
        "base/gf/wrapMatrix.template.cpp",
        "base/gf/wrapMatrix2.template.cpp",
        "base/gf/wrapMatrix3.template.cpp",
        "base/gf/wrapMatrix4.template.cpp",
        "base/gf/wrapQuat.template.cpp",
        "base/gf/wrapRange.template.cpp",
        "base/gf/wrapVec.template.cpp",
        "base/gf/dualQuat.template.h",
        "base/gf/matrix.template.h",
        "base/gf/matrix2.template.h",
        "base/gf/matrix3.template.h",
        "base/gf/matrix4.template.h",
        "base/gf/quat.template.h",
        "base/gf/range.template.h",
        "base/gf/vec.template.h"
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
      path: "pxr",
      exclude: [
        "imaging",
        "usd",
        "usdImaging",
        "base/trace/testenv",
        "base/trace/module.cpp",
        "base/trace/moduleDeps.cpp",
        "base/trace/wrapAggregateNode.cpp",
        "base/trace/wrapCollector.cpp",
        "base/trace/wrapReporter.cpp",
        "base/trace/wrapTestTrace.cpp",
        "base/arch",
        "base/tf",
        "base/gf",
        "base/js",
        "base/plug",
        "base/vt",
        "base/work"
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
      path: "pxr",
      exclude: [
        "imaging",
        "usd",
        "usdImaging",
        "base/vt/testenv",
        "base/vt/module.cpp",
        "base/vt/moduleDeps.cpp",
        "base/vt/wrapArray.cpp",
        "base/vt/wrapArrayBase.cpp",
        "base/vt/wrapArrayDualQuaternion.cpp",
        "base/vt/wrapArrayFloat.cpp",
        "base/vt/wrapArrayIntegral.cpp",
        "base/vt/wrapArrayMatrix.cpp",
        "base/vt/wrapArrayQuaternion.cpp",
        "base/vt/wrapArrayRange.cpp",
        "base/vt/wrapArrayString.cpp",
        "base/vt/wrapArrayToken.cpp",
        "base/vt/wrapArrayVec.cpp",
        "base/vt/wrapDictionary.cpp",
        "base/vt/wrapValue.cpp",
        "base/arch",
        "base/tf",
        "base/gf",
        "base/js",
        "base/plug",
        "base/trace",
        "base/work"
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
      path: "pxr",
      exclude: [
        "imaging",
        "usd",
        "usdImaging",
        "base/work/testenv",
        "base/work/module.cpp",
        "base/work/moduleDeps.cpp",
        "base/work/wrapThreadLimits.cpp",
        "base/arch",
        "base/tf",
        "base/gf",
        "base/js",
        "base/plug",
        "base/vt",
        "base/trace"
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
      path: "pxr",
      exclude: [
        "imaging",
        "usd",
        "usdImaging",
        "base/plug/testenv",
        "base/plug/module.cpp",
        "base/plug/moduleDeps.cpp",
        "base/plug/wrapNotice.cpp",
        "base/plug/wrapPlugin.cpp",
        "base/plug/wrapRegistry.cpp",
        "base/plug/wrapTestPlugBase.cpp",
        "base/arch",
        "base/tf",
        "base/gf",
        "base/js",
        "base/trace",
        "base/vt",
        "base/work"
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
      path: "pxr",
      exclude: [
        "imaging",
        "base",
        "usdImaging",
        "usd/ar/testenv",
        "usd/ar/module.cpp",
        "usd/ar/moduleDeps.cpp",
        "usd/ar/pyResolverContext.cpp",
        "usd/ar/wrapAssetInfo.cpp",
        "usd/ar/wrapDefaultResolver.cpp",
        "usd/ar/wrapDefaultResolverContext.cpp",
        "usd/ar/wrapNotice.cpp",
        "usd/ar/wrapPackageUtils.cpp",
        "usd/ar/wrapResolvedPath.cpp",
        "usd/ar/wrapResolver.cpp",
        "usd/ar/wrapResolverContext.cpp",
        "usd/ar/wrapResolverContextBinder.cpp",
        "usd/ar/wrapResolverScopedCache.cpp",
        "usd/ar/wrapTimestamp.cpp",
        "usd/bin",
        "usd/kind",
        "usd/ndr",
        "usd/pcp",
        "usd/plugin",
        "usd/sdf",
        "usd/sdr",
        "usd/usd",
        "usd/usdGeom",
        "usd/usdHydra",
        "usd/usdLux",
        "usd/usdMedia",
        "usd/usdMtlx",
        "usd/usdPhysics",
        "usd/usdProc",
        "usd/usdRender",
        "usd/usdRi",
        "usd/usdShade",
        "usd/usdSkel",
        "usd/usdUI",
        "usd/usdUtils",
        "usd/usdVol",
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
      path: "pxr",
      exclude: [
        "imaging",
        "base",
        "usdImaging",
        "usd/kind/testenv",
        "usd/kind/module.cpp",
        "usd/kind/moduleDeps.cpp",
        "usd/kind/wrapRegistry.cpp",
        "usd/kind/wrapTokens.cpp",
        "usd/ar",
        "usd/bin",
        "usd/ndr",
        "usd/pcp",
        "usd/plugin",
        "usd/sdf",
        "usd/sdr",
        "usd/usd",
        "usd/usdGeom",
        "usd/usdHydra",
        "usd/usdLux",
        "usd/usdMedia",
        "usd/usdMtlx",
        "usd/usdPhysics",
        "usd/usdProc",
        "usd/usdRender",
        "usd/usdRi",
        "usd/usdShade",
        "usd/usdSkel",
        "usd/usdUI",
        "usd/usdUtils",
        "usd/usdVol",
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
      path: "pxr",
      exclude: [
        "imaging",
        "base",
        "usdImaging",
        "usd/sdf/testenv",
        "usd/sdf/module.cpp",
        "usd/sdf/moduleDeps.cpp",
        "usd/sdf/wrapArrayAssetPath.cpp",
        "usd/sdf/wrapArrayPath.cpp",
        "usd/sdf/wrapArrayTimeCode.cpp",
        "usd/sdf/wrapAssetPath.cpp",
        "usd/sdf/wrapAttributeSpec.cpp",
        "usd/sdf/wrapChangeBlock.cpp",
        "usd/sdf/wrapCleanupEnabler.cpp",
        "usd/sdf/wrapCopyUtils.cpp",
        "usd/sdf/wrapFileFormat.cpp",
        "usd/sdf/wrapLayer.cpp",
        "usd/sdf/wrapLayerOffset.cpp",
        "usd/sdf/wrapLayerTree.cpp",
        "usd/sdf/wrapNamespaceEdit.cpp",
        "usd/sdf/wrapNotice.cpp",
        "usd/sdf/wrapOpaqueValue.cpp",
        "usd/sdf/wrapPath.cpp",
        "usd/sdf/wrapPathExpression.cpp",
        "usd/sdf/wrapPayload.cpp",
        "usd/sdf/wrapPredicateExpression.cpp",
        "usd/sdf/wrapPrimSpec.cpp",
        "usd/sdf/wrapPropertySpec.cpp",
        "usd/sdf/wrapPseudoRootSpec.cpp",
        "usd/sdf/wrapReference.cpp",
        "usd/sdf/wrapRelationshipSpec.cpp",
        "usd/sdf/wrapSpec.cpp",
        "usd/sdf/wrapTimeCode.cpp",
        "usd/sdf/wrapTypes.cpp",
        "usd/sdf/wrapValueTypeName.cpp",
        "usd/sdf/wrapVariableExpression.cpp",
        "usd/sdf/wrapVariantSetSpec.cpp",
        "usd/sdf/wrapVariantSpec.cpp",
        "usd/ar",
        "usd/bin",
        "usd/kind",
        "usd/ndr",
        "usd/pcp",
        "usd/plugin",
        "usd/sdr",
        "usd/usd",
        "usd/usdGeom",
        "usd/usdHydra",
        "usd/usdLux",
        "usd/usdMedia",
        "usd/usdMtlx",
        "usd/usdPhysics",
        "usd/usdProc",
        "usd/usdRender",
        "usd/usdRi",
        "usd/usdShade",
        "usd/usdSkel",
        "usd/usdUI",
        "usd/usdUtils",
        "usd/usdVol",
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
      path: "pxr",
      exclude: [
        "imaging",
        "base",
        "usdImaging",
        "usd/pcp/testenv",
        "usd/pcp/module.cpp",
        "usd/pcp/moduleDeps.cpp",
        "usd/pcp/wrapCache.cpp",
        "usd/pcp/wrapDependency.cpp",
        "usd/pcp/wrapDynamicFileFormatDependencyData.cpp",
        "usd/pcp/wrapErrors.cpp",
        "usd/pcp/wrapExpressionVariables.cpp",
        "usd/pcp/wrapExpressionVariablesSource.cpp",
        "usd/pcp/wrapInstanceKey.cpp",
        "usd/pcp/wrapLayerStack.cpp",
        "usd/pcp/wrapLayerStackIdentifier.cpp",
        "usd/pcp/wrapMapExpression.cpp",
        "usd/pcp/wrapMapFunction.cpp",
        "usd/pcp/wrapNode.cpp",
        "usd/pcp/wrapPathTranslation.cpp",
        "usd/pcp/wrapPrimIndex.cpp",
        "usd/pcp/wrapPropertyIndex.cpp",
        "usd/pcp/wrapSite.cpp",
        "usd/pcp/wrapTestChangeProcessor.cpp",
        "usd/pcp/wrapTypes.cpp",
        "usd/ar",
        "usd/bin",
        "usd/kind",
        "usd/ndr",
        "usd/plugin",
        "usd/sdf",
        "usd/sdr",
        "usd/usd",
        "usd/usdGeom",
        "usd/usdHydra",
        "usd/usdLux",
        "usd/usdMedia",
        "usd/usdMtlx",
        "usd/usdPhysics",
        "usd/usdProc",
        "usd/usdRender",
        "usd/usdRi",
        "usd/usdShade",
        "usd/usdSkel",
        "usd/usdUI",
        "usd/usdUtils",
        "usd/usdVol",
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
      path: "pxr",
      exclude: [
        "imaging",
        "base",
        "usdImaging",
        "usd/usd/testenv",
        "usd/usd/module.cpp",
        "usd/usd/moduleDeps.cpp",
        "usd/usd/wrapAPISchemaBase.cpp",
        "usd/usd/wrapAttribute.cpp",
        "usd/usd/wrapAttributeQuery.cpp",
        "usd/usd/wrapClipsAPI.cpp",
        "usd/usd/wrapCollectionAPI.cpp",
        "usd/usd/wrapCollectionMembershipQuery.cpp",
        "usd/usd/wrapCommon.cpp",
        "usd/usd/wrapCrateInfo.cpp",
        "usd/usd/wrapEditContext.cpp",
        "usd/usd/wrapEditTarget.cpp",
        "usd/usd/wrapFlattenUtils.cpp",
        "usd/usd/wrapInherits.cpp",
        "usd/usd/wrapInterpolation.cpp",
        "usd/usd/wrapModelAPI.cpp",
        "usd/usd/wrapNotice.cpp",
        "usd/usd/wrapObject.cpp",
        "usd/usd/wrapPayloads.cpp",
        "usd/usd/wrapPrim.cpp",
        "usd/usd/wrapPrimCompositionQuery.cpp",
        "usd/usd/wrapPrimDefinition.cpp",
        "usd/usd/wrapPrimFlags.cpp",
        "usd/usd/wrapPrimRange.cpp",
        "usd/usd/wrapPrimTypeInfo.cpp",
        "usd/usd/wrapProperty.cpp",
        "usd/usd/wrapReferences.cpp",
        "usd/usd/wrapRelationship.cpp",
        "usd/usd/wrapResolveInfo.cpp",
        "usd/usd/wrapResolveTarget.cpp",
        "usd/usd/wrapSchemaBase.cpp",
        "usd/usd/wrapSchemaRegistry.cpp",
        "usd/usd/wrapSpecializes.cpp",
        "usd/usd/wrapStage.cpp",
        "usd/usd/wrapStageCache.cpp",
        "usd/usd/wrapStageCacheContext.cpp",
        "usd/usd/wrapStageLoadRules.cpp",
        "usd/usd/wrapStagePopulationMask.cpp",
        "usd/usd/wrapTimeCode.cpp",
        "usd/usd/wrapTokens.cpp",
        "usd/usd/wrapTyped.cpp",
        "usd/usd/wrapUsdFileFormat.cpp",
        "usd/usd/wrapUtils.cpp",
        "usd/usd/wrapVariantSets.cpp",
        "usd/usd/wrapVersion.cpp",
        "usd/usd/wrapZipFile.cpp",
        "usd/usd/examples.cpp",
        "usd/ar",
        "usd/bin",
        "usd/kind",
        "usd/ndr",
        "usd/pcp",
        "usd/plugin",
        "usd/sdf",
        "usd/sdr",
        "usd/usdGeom",
        "usd/usdHydra",
        "usd/usdLux",
        "usd/usdMedia",
        "usd/usdMtlx",
        "usd/usdPhysics",
        "usd/usdProc",
        "usd/usdRender",
        "usd/usdRi",
        "usd/usdShade",
        "usd/usdSkel",
        "usd/usdUI",
        "usd/usdUtils",
        "usd/usdVol",
      ],
      resources: [
        .copy("usd/usd/codegenTemplates"),
        // .process("Resources")
      ],
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
      path: "pxr",
      sources: [
        "base/tf/module.cpp",
        "base/tf/moduleDeps.cpp",
        "base/tf/pyWeakObject.cpp",
        "base/tf/wrapAnyWeakPtr.cpp",
        "base/tf/wrapCallContext.cpp",
        "base/tf/wrapDebug.cpp",
        "base/tf/wrapDiagnostic.cpp",
        "base/tf/wrapDiagnosticBase.cpp",
        "base/tf/wrapEnum.cpp",
        "base/tf/wrapEnvSetting.cpp",
        "base/tf/wrapError.cpp",
        "base/tf/wrapException.cpp",
        "base/tf/wrapFileUtils.cpp",
        "base/tf/wrapFunction.cpp",
        "base/tf/wrapMallocTag.cpp",
        "base/tf/wrapNotice.cpp",
        "base/tf/wrapPathUtils.cpp",
        "base/tf/wrapPyContainerConversions.cpp",
        "base/tf/wrapPyModuleNotice.cpp",
        "base/tf/wrapPyObjWrapper.cpp",
        "base/tf/wrapPyOptional.cpp",
        "base/tf/wrapRefPtrTracker.cpp",
        "base/tf/wrapScopeDescription.cpp",
        "base/tf/wrapScriptModuleLoader.cpp",
        "base/tf/wrapSingleton.cpp",
        "base/tf/wrapStackTrace.cpp",
        "base/tf/wrapStatus.cpp",
        "base/tf/wrapStopwatch.cpp",
        "base/tf/wrapStringUtils.cpp",
        "base/tf/wrapTemplateString.cpp",
        "base/tf/wrapTestPyAnnotatedBoolResult.cpp",
        "base/tf/wrapTestPyContainerConversions.cpp",
        "base/tf/wrapTestPyStaticTokens.cpp",
        "base/tf/wrapTestTfPyOptional.cpp",
        "base/tf/wrapTestTfPython.cpp",
        "base/tf/wrapToken.cpp",
        "base/tf/wrapType.cpp",
        "base/tf/wrapTypeHelpers.cpp",
        "base/tf/wrapWarning.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "base/tf/include",
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
      path: "pxr",
      sources: [
        "base/gf/module.cpp",
        "base/gf/moduleDeps.cpp",
        "base/gf/wrapBBox3d.cpp",
        "base/gf/wrapCamera.cpp",
        "base/gf/wrapDualQuatd.cpp",
        "base/gf/wrapDualQuatf.cpp",
        "base/gf/wrapDualQuath.cpp",
        "base/gf/wrapFrustum.cpp",
        "base/gf/wrapGamma.cpp",
        "base/gf/wrapHalf.cpp",
        "base/gf/wrapHomogeneous.cpp",
        "base/gf/wrapInterval.cpp",
        "base/gf/wrapLimits.cpp",
        "base/gf/wrapLine.cpp",
        "base/gf/wrapLineSeg.cpp",
        "base/gf/wrapMath.cpp",
        "base/gf/wrapMatrix2d.cpp",
        "base/gf/wrapMatrix2f.cpp",
        "base/gf/wrapMatrix3d.cpp",
        "base/gf/wrapMatrix3f.cpp",
        "base/gf/wrapMatrix4d.cpp",
        "base/gf/wrapMatrix4f.cpp",
        "base/gf/wrapMultiInterval.cpp",
        "base/gf/wrapPlane.cpp",
        "base/gf/wrapQuatd.cpp",
        "base/gf/wrapQuaternion.cpp",
        "base/gf/wrapQuatf.cpp",
        "base/gf/wrapQuath.cpp",
        "base/gf/wrapRange1d.cpp",
        "base/gf/wrapRange1f.cpp",
        "base/gf/wrapRange2d.cpp",
        "base/gf/wrapRange2f.cpp",
        "base/gf/wrapRange3d.cpp",
        "base/gf/wrapRange3f.cpp",
        "base/gf/wrapRay.cpp",
        "base/gf/wrapRect2i.cpp",
        "base/gf/wrapRotation.cpp",
        "base/gf/wrapSize2.cpp",
        "base/gf/wrapSize3.cpp",
        "base/gf/wrapTransform.cpp",
        "base/gf/wrapVec2d.cpp",
        "base/gf/wrapVec2f.cpp",
        "base/gf/wrapVec2h.cpp",
        "base/gf/wrapVec2i.cpp",
        "base/gf/wrapVec3d.cpp",
        "base/gf/wrapVec3f.cpp",
        "base/gf/wrapVec3h.cpp",
        "base/gf/wrapVec3i.cpp",
        "base/gf/wrapVec4d.cpp",
        "base/gf/wrapVec4f.cpp",
        "base/gf/wrapVec4h.cpp",
        "base/gf/wrapVec4i.cpp"
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "base/gf/include",
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
      path: "pxr",
      sources: [
        "base/trace/module.cpp",
        "base/trace/moduleDeps.cpp",
        "base/trace/wrapAggregateNode.cpp",
        "base/trace/wrapCollector.cpp",
        "base/trace/wrapReporter.cpp",
        "base/trace/wrapTestTrace.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "base/trace/include",
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
      path: "pxr",
      sources: [
        "base/vt/module.cpp",
        "base/vt/moduleDeps.cpp",
        "base/vt/wrapArray.cpp",
        "base/vt/wrapArrayBase.cpp",
        "base/vt/wrapArrayDualQuaternion.cpp",
        "base/vt/wrapArrayFloat.cpp",
        "base/vt/wrapArrayIntegral.cpp",
        "base/vt/wrapArrayMatrix.cpp",
        "base/vt/wrapArrayQuaternion.cpp",
        "base/vt/wrapArrayRange.cpp",
        "base/vt/wrapArrayString.cpp",
        "base/vt/wrapArrayToken.cpp",
        "base/vt/wrapArrayVec.cpp",
        "base/vt/wrapDictionary.cpp",
        "base/vt/wrapValue.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "base/vt/include",
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
      path: "pxr",
      sources: [
        "base/work/module.cpp",
        "base/work/moduleDeps.cpp",
        "base/work/wrapThreadLimits.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "base/work/include",
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
      path: "pxr",
      sources: [
        "base/plug/module.cpp",
        "base/plug/moduleDeps.cpp",
        "base/plug/wrapNotice.cpp",
        "base/plug/wrapPlugin.cpp",
        "base/plug/wrapRegistry.cpp",
        "base/plug/wrapTestPlugBase.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "base/plug/include",
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
      path: "pxr",
      sources: [
        "usd/ar/module.cpp",
        "usd/ar/moduleDeps.cpp",
        "usd/ar/pyResolverContext.cpp",
        "usd/ar/wrapAssetInfo.cpp",
        "usd/ar/wrapDefaultResolver.cpp",
        "usd/ar/wrapDefaultResolverContext.cpp",
        "usd/ar/wrapNotice.cpp",
        "usd/ar/wrapPackageUtils.cpp",
        "usd/ar/wrapResolvedPath.cpp",
        "usd/ar/wrapResolver.cpp",
        "usd/ar/wrapResolverContext.cpp",
        "usd/ar/wrapResolverContextBinder.cpp",
        "usd/ar/wrapResolverScopedCache.cpp",
        "usd/ar/wrapTimestamp.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "usd/ar/include",
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
      path: "pxr",
      sources: [
        "usd/kind/module.cpp",
        "usd/kind/moduleDeps.cpp",
        "usd/kind/wrapRegistry.cpp",
        "usd/kind/wrapTokens.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "usd/kind/include",
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
      path: "pxr",
      sources: [
        "usd/sdf/module.cpp",
        "usd/sdf/moduleDeps.cpp",
        "usd/sdf/wrapArrayAssetPath.cpp",
        "usd/sdf/wrapArrayPath.cpp",
        "usd/sdf/wrapArrayTimeCode.cpp",
        "usd/sdf/wrapAssetPath.cpp",
        "usd/sdf/wrapAttributeSpec.cpp",
        "usd/sdf/wrapChangeBlock.cpp",
        "usd/sdf/wrapCleanupEnabler.cpp",
        "usd/sdf/wrapCopyUtils.cpp",
        "usd/sdf/wrapFileFormat.cpp",
        "usd/sdf/wrapLayer.cpp",
        "usd/sdf/wrapLayerOffset.cpp",
        "usd/sdf/wrapLayerTree.cpp",
        "usd/sdf/wrapNamespaceEdit.cpp",
        "usd/sdf/wrapNotice.cpp",
        "usd/sdf/wrapOpaqueValue.cpp",
        "usd/sdf/wrapPath.cpp",
        "usd/sdf/wrapPathExpression.cpp",
        "usd/sdf/wrapPayload.cpp",
        "usd/sdf/wrapPredicateExpression.cpp",
        "usd/sdf/wrapPrimSpec.cpp",
        "usd/sdf/wrapPropertySpec.cpp",
        "usd/sdf/wrapPseudoRootSpec.cpp",
        "usd/sdf/wrapReference.cpp",
        "usd/sdf/wrapRelationshipSpec.cpp",
        "usd/sdf/wrapSpec.cpp",
        "usd/sdf/wrapTimeCode.cpp",
        "usd/sdf/wrapTypes.cpp",
        "usd/sdf/wrapValueTypeName.cpp",
        "usd/sdf/wrapVariableExpression.cpp",
        "usd/sdf/wrapVariantSetSpec.cpp",
        "usd/sdf/wrapVariantSpec.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "usd/sdf/include",
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
      path: "pxr",
      sources: [
        "usd/pcp/module.cpp",
        "usd/pcp/moduleDeps.cpp",
        "usd/pcp/wrapCache.cpp",
        "usd/pcp/wrapDependency.cpp",
        "usd/pcp/wrapDynamicFileFormatDependencyData.cpp",
        "usd/pcp/wrapErrors.cpp",
        "usd/pcp/wrapExpressionVariables.cpp",
        "usd/pcp/wrapExpressionVariablesSource.cpp",
        "usd/pcp/wrapInstanceKey.cpp",
        "usd/pcp/wrapLayerStack.cpp",
        "usd/pcp/wrapLayerStackIdentifier.cpp",
        "usd/pcp/wrapMapExpression.cpp",
        "usd/pcp/wrapMapFunction.cpp",
        "usd/pcp/wrapNode.cpp",
        "usd/pcp/wrapPathTranslation.cpp",
        "usd/pcp/wrapPrimIndex.cpp",
        "usd/pcp/wrapPropertyIndex.cpp",
        "usd/pcp/wrapSite.cpp",
        "usd/pcp/wrapTestChangeProcessor.cpp",
        "usd/pcp/wrapTypes.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "usd/pcp/include",
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
      path: "pxr",
      sources: [
        "usd/usd/module.cpp",
        "usd/usd/moduleDeps.cpp",
        "usd/usd/wrapAPISchemaBase.cpp",
        "usd/usd/wrapAttribute.cpp",
        "usd/usd/wrapAttributeQuery.cpp",
        "usd/usd/wrapClipsAPI.cpp",
        "usd/usd/wrapCollectionAPI.cpp",
        "usd/usd/wrapCollectionMembershipQuery.cpp",
        "usd/usd/wrapCommon.cpp",
        "usd/usd/wrapCrateInfo.cpp",
        "usd/usd/wrapEditContext.cpp",
        "usd/usd/wrapEditTarget.cpp",
        "usd/usd/wrapFlattenUtils.cpp",
        "usd/usd/wrapInherits.cpp",
        "usd/usd/wrapInterpolation.cpp",
        "usd/usd/wrapModelAPI.cpp",
        "usd/usd/wrapNotice.cpp",
        "usd/usd/wrapObject.cpp",
        "usd/usd/wrapPayloads.cpp",
        "usd/usd/wrapPrim.cpp",
        "usd/usd/wrapPrimCompositionQuery.cpp",
        "usd/usd/wrapPrimDefinition.cpp",
        "usd/usd/wrapPrimFlags.cpp",
        "usd/usd/wrapPrimRange.cpp",
        "usd/usd/wrapPrimTypeInfo.cpp",
        "usd/usd/wrapProperty.cpp",
        "usd/usd/wrapReferences.cpp",
        "usd/usd/wrapRelationship.cpp",
        "usd/usd/wrapResolveInfo.cpp",
        "usd/usd/wrapResolveTarget.cpp",
        "usd/usd/wrapSchemaBase.cpp",
        "usd/usd/wrapSchemaRegistry.cpp",
        "usd/usd/wrapSpecializes.cpp",
        "usd/usd/wrapStage.cpp",
        "usd/usd/wrapStageCache.cpp",
        "usd/usd/wrapStageCacheContext.cpp",
        "usd/usd/wrapStageLoadRules.cpp",
        "usd/usd/wrapStagePopulationMask.cpp",
        "usd/usd/wrapTimeCode.cpp",
        "usd/usd/wrapTokens.cpp",
        "usd/usd/wrapTyped.cpp",
        "usd/usd/wrapUsdFileFormat.cpp",
        "usd/usd/wrapUtils.cpp",
        "usd/usd/wrapVariantSets.cpp",
        "usd/usd/wrapVersion.cpp",
        "usd/usd/wrapZipFile.cpp",
      ],
      resources: [
        // .process("Resources"),
      ],
      publicHeadersPath: "usd/usd/include",
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
