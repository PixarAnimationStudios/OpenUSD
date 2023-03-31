#include "pxr/usdImaging/usdImagingMtlx/adapter.h"

#include "pxr/usdImaging/usdImaging/materialParamUtils.h"

#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdMtlx/reader.h"
#include "pxr/usd/usdMtlx/utils.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/ar/resolverScopedCache.h"

#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"

#include "pxr/base/arch/fileSystem.h"

PXR_NAMESPACE_OPEN_SCOPE

void UsdImagingMtlxConvertMtlxToHdMaterialNetworkMap(
    const std::string& mtlxPath,
    const TfTokenVector& shaderSourceTypes,
    const TfTokenVector& renderContexts,
    HdMaterialNetworkMap* out)
{
    if (mtlxPath.empty()) {
        return;
    }

    std::string basePath = TfGetPathName(mtlxPath);
    if (basePath.empty()) {
        basePath = ".";
    }

    ArResolver& resolver = ArGetResolver();
    const ArResolverContext context = resolver.CreateDefaultContextForAsset(mtlxPath);
    ArResolverContextBinder binder(context);
    ArResolverScopedCache resolverCache;

    std::string mtlxName = TfGetBaseName(mtlxPath);
    std::string stage_id = TfStringPrintf(
        "%s%s%s.usda", basePath.c_str(), ARCH_PATH_SEP, mtlxName.c_str());
    UsdStageRefPtr stage = UsdStage::CreateInMemory(stage_id, context);

    try {
        MaterialX::DocumentPtr doc = UsdMtlxReadDocument(mtlxPath);
        UsdMtlxRead(doc, stage);
    } catch (MaterialX::ExceptionFoundCycle &x) {
        Tf_PostErrorHelper(TF_CALL_CONTEXT,
                           TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE,
                           "MaterialX cycle found: %s\n",
                           x.what());
        return;
    } catch (MaterialX::Exception &x) {
        Tf_PostErrorHelper(TF_CALL_CONTEXT,
                           TF_DIAGNOSTIC_RUNTIME_ERROR_TYPE,
                           "MaterialX error: %s\n",
                           x.what());
        return;
    }

    SdfPath materialsPath("/MaterialX/Materials");
    if (UsdPrim materials = stage->GetPrimAtPath(materialsPath)) {
        if (UsdPrimSiblingRange children = materials.GetChildren()) {
            if (auto material = UsdShadeMaterial(*children.begin())) {
                if (UsdShadeShader mtlxSurface = material.ComputeSurfaceSource(renderContexts)) {
                    UsdImagingBuildHdMaterialNetworkFromTerminal(
                        mtlxSurface.GetPrim(),
                        HdMaterialTerminalTokens->surface,
                        shaderSourceTypes,
                        renderContexts,
                        out,
                        UsdTimeCode::Default());
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
