//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRender/spec.h"
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usdRender/product.h"
#include "pxr/usd/usdRender/var.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

// Return the outermost namespace of an attribute name.
static std::string
_GetAttrNamespace(std::string const& name)
{
    size_t pos = name.find(UsdObject::GetNamespaceDelimiter());
    if (pos != std::string::npos) {
        return name.substr(0, pos);
    } else {
        return std::string();
    }
}

static void
_ReadNamespacedSettings(
    UsdPrim const& prim,
    TfTokenVector const& requestedNamespaces, 
    VtDictionary *namespacedSettings)
{
    for (UsdAttribute attr: prim.GetAuthoredAttributes()) {
        const TfToken name = attr.GetName();

        // Use UsdShadeOutput to strip "outputs:"
        UsdShadeOutput shadeOutput(attr);
        const TfToken basename = shadeOutput ? shadeOutput.GetBaseName() : name;
        const std::string attrNamespace = _GetAttrNamespace(basename);

        // Only collect namespaced settings
        if (attrNamespace.empty()) {
            continue;
        }

        // If specific namespaces were requested, require a match
        if (!requestedNamespaces.empty() &&
            std::find(requestedNamespaces.begin(), requestedNamespaces.end(),
                      attrNamespace) == requestedNamespaces.end()) {
            continue;
        }

        // Connections are stronger than values authored on the attribute,
        // so check for connections first.
        if (shadeOutput) {
            UsdShadeAttributeVector targets =
                UsdShadeUtils::GetValueProducingAttributes(shadeOutput);
            if (!targets.empty()) {
                SdfPathVector outputConnectedPaths;
                outputConnectedPaths.reserve(targets.size());
                for (auto const& targetPath : targets) {
                    outputConnectedPaths.push_back(targetPath.GetPrimPath());
                }
                (*namespacedSettings)[name] = VtValue(outputConnectedPaths);
                continue;
            }
        }

        // Base case: use the attribute value
        VtValue val;
        if (attr.Get(&val)) {
            (*namespacedSettings)[name] = val;
        }
    }
}

template<typename T>
inline bool
_Get(UsdAttribute const &attr, T* val, bool getDefaultValue)
{
    if (getDefaultValue || attr.HasAuthoredValue()) {
        return attr.Get(val);
    }
    return false;
}

static void
_ReadSettingsBase(UsdRenderSettingsBase const& rsBase,
                  UsdRenderSpec::Product *pd,
                  bool getDefault=true)
{
    SdfPathVector targets; 
    rsBase.GetCameraRel().GetForwardedTargets(&targets);
    if (!targets.empty()) {
        pd->cameraPath = targets[0];
    }
    _Get(rsBase.GetResolutionAttr(), &pd->resolution, getDefault);
    _Get(rsBase.GetPixelAspectRatioAttr(), &pd->pixelAspectRatio, getDefault);
    _Get(rsBase.GetAspectRatioConformPolicyAttr(),
        &pd->aspectRatioConformPolicy, getDefault);
    
    {
        // Convert dataWindowNDC from vec4 to range2.
        GfVec4f dataWindowNDCVec;
        if (_Get(rsBase.GetDataWindowNDCAttr(), &dataWindowNDCVec, getDefault)){
            pd->dataWindowNDC = GfRange2f(
                GfVec2f(dataWindowNDCVec[0], dataWindowNDCVec[1]),
                GfVec2f(dataWindowNDCVec[2], dataWindowNDCVec[3]));
        }
    }

    _Get(rsBase.GetDisableMotionBlurAttr(), &pd->disableMotionBlur, getDefault);

    {
        // For backwards-compatibility:
        // instantaneousShutter disables motion blur
        bool instantaneousShutter = false;
        _Get(rsBase.GetDisableMotionBlurAttr(),
             &instantaneousShutter, getDefault);
        if (instantaneousShutter) {
            pd->disableMotionBlur = true;
        }
    }

    _Get(rsBase.GetDisableDepthOfFieldAttr(), 
        &pd->disableDepthOfField, getDefault);
}

// TODO: Consolidate with CameraUtilConformedWindow().  Resolve policy
// name mismatches; also CameraUtil cannot compensate pixelAspectRatio.
static void _ApplyAspectRatioPolicy(UsdRenderSpec::Product *product)
{
    // Gather dimensions
    GfVec2i res = product->resolution;
    GfVec2f size = product->apertureSize;
    // Validate dimensions
    if (res[0] <= 0.0 || res[1] <= 0.0 || size[0] <= 0.0 || size[1] <= 0.0) {
        return;
    }
    // Compute aspect ratios
    float resAspectRatio = float(res[0]) / float(res[1]);
    float imageAspectRatio = product->pixelAspectRatio * resAspectRatio;
    if (imageAspectRatio <= 0.0) {
        return;
    }
    float apertureAspectRatio = size[0] / size[1];
    // Apply policy
    TfToken const& policy = product->aspectRatioConformPolicy;
    enum { Width, Height, None } adjust = None;
    if (policy == UsdRenderTokens->adjustPixelAspectRatio) {
        product->pixelAspectRatio = apertureAspectRatio / resAspectRatio;
    } else if (policy == UsdRenderTokens->adjustApertureHeight) {
        adjust = Height;
    } else if (policy == UsdRenderTokens->adjustApertureWidth) {
        adjust = Width;
    } else if (policy == UsdRenderTokens->expandAperture) {
        adjust = (apertureAspectRatio > imageAspectRatio) ? Height : Width;
    } else if (policy == UsdRenderTokens->cropAperture) {
        adjust = (apertureAspectRatio > imageAspectRatio) ? Width : Height;
    }
    // Adjust aperture so that size[0] / size[1] == imageAspectRatio.
    if (adjust == Width) {
        product->apertureSize[0] = size[1] * imageAspectRatio;
    } else if (adjust == Height) {
        product->apertureSize[1] = size[0] / imageAspectRatio;
    }
}

// -------------------------------------------------------------------------- //

UsdRenderSpec
UsdRenderComputeSpec(
    UsdRenderSettings const& settings,
    TfTokenVector const& namespaces)
{
    UsdRenderSpec renderSpec;
    UsdPrim rsPrim = settings.GetPrim();
    UsdStageWeakPtr stage = rsPrim.GetStage();
    if (!stage) {
        TF_CODING_ERROR("Invalid stage\n");
        return renderSpec;
    }

    // Read shared base settings as a "base product". Note that this excludes
    // namespaced attributes that are gathered under namespacedSettings.
    UsdRenderSpec::Product baseProduct;
    _ReadSettingsBase(UsdRenderSettingsBase(rsPrim), &baseProduct);

    // Products
    SdfPathVector targets; 
    settings.GetProductsRel().GetForwardedTargets(&targets);
    for (SdfPath const& target: targets) {
        if (UsdRenderProduct rpPrim =
                UsdRenderProduct(stage->GetPrimAtPath(target))) {
            // Initialize the render spec product with the base render product
            UsdRenderSpec::Product rpSpec = baseProduct;
            rpSpec.renderProductPath = target;

            // Read product-specific overrides to base render settings.
            _ReadSettingsBase(UsdRenderSettingsBase(rpPrim), &rpSpec, 
                false /* only get Authored values*/);

            // Read camera aperture and apply aspectRatioConformPolicy.
            // Use the camera path from the rpSpec if authored, otherwise
            // the camera path on the render settings prim. 
            const SdfPath camPath = rpSpec.cameraPath.IsEmpty()
                ? baseProduct.cameraPath
                : rpSpec.cameraPath;
            if (UsdGeomCamera cam =
                    UsdGeomCamera(stage->GetPrimAtPath(camPath))) {
                cam.GetHorizontalApertureAttr().Get(&rpSpec.apertureSize[0]);
                cam.GetVerticalApertureAttr().Get(&rpSpec.apertureSize[1]);
                _ApplyAspectRatioPolicy(&rpSpec);
            } else {
                TF_RUNTIME_ERROR("UsdRenderSettings: Could not find camera "
                                 "<%s> for the render product <%s>.\n",
                                 camPath.GetText(), target.GetText());
                continue;
            }

            // Read product-only settings.
            rpPrim.GetProductTypeAttr().Get(&rpSpec.type);
            rpPrim.GetProductNameAttr().Get(&rpSpec.name);

            // Read render vars.
            SdfPathVector renderVarPaths;
            rpPrim.GetOrderedVarsRel().GetForwardedTargets(&renderVarPaths);
            for (SdfPath const& renderVarPath: renderVarPaths ) {
                bool foundExisting = false;
                for (size_t i=0; i < renderSpec.renderVars.size(); ++i) {
                    if (renderSpec.renderVars[i].renderVarPath == renderVarPath) {
                        rpSpec.renderVarIndices.push_back(i);
                        foundExisting = true;
                        break;
                    }
                }
                if (!foundExisting) {
                    UsdPrim prim = stage->GetPrimAtPath(renderVarPath);
                    if (prim && prim.IsA<UsdRenderVar>()) {
                        UsdRenderVar rvPrim(prim);
                        UsdRenderSpec::RenderVar rvSpec;

                        // Store schema-defined attributes in explicit fields.
                        rvSpec.renderVarPath = renderVarPath;
                        rvPrim.GetDataTypeAttr().Get(&rvSpec.dataType);
                        rvPrim.GetSourceNameAttr().Get(&rvSpec.sourceName);
                        rvPrim.GetSourceTypeAttr().Get(&rvSpec.sourceType);
                        // Store any other custom render var attributes in
                        // namespacedSettings.
                        _ReadNamespacedSettings(
                            prim, namespaces, &rvSpec.namespacedSettings);

                        // Record new render var.
                        rpSpec.renderVarIndices.push_back(
                            renderSpec.renderVars.size());
                        renderSpec.renderVars.emplace_back(rvSpec);
                    } else {
                        TF_RUNTIME_ERROR("Render product <%s> includes "
                                         "render var at path <%s>, but "
                                         "no suitable UsdRenderVar prim "
                                         "was found.  Skipping.",
                                         target.GetText(),
                                         renderVarPath.GetText());
                    }
                }
            }
            // Store any other custom render product attributes in
            // namespacedSettings.
            _ReadNamespacedSettings(
                rpPrim.GetPrim(), namespaces, &rpSpec.namespacedSettings);

            renderSpec.products.emplace_back(rpSpec);
        }
    }

    // Scene configuration
    settings.GetMaterialBindingPurposesAttr().Get(
        &renderSpec.materialBindingPurposes);
    settings.GetIncludedPurposesAttr().Get(&renderSpec.includedPurposes);

    // Store any other custom render settings attributes in namespacedSettings.
    _ReadNamespacedSettings(rsPrim, namespaces, &renderSpec.namespacedSettings);

    return renderSpec;
}

VtDictionary
UsdRenderComputeNamespacedSettings(
    UsdPrim const& prim,
    TfTokenVector const& namespaces)
{
    VtDictionary dict;
    _ReadNamespacedSettings(prim, namespaces, &dict);
    return dict;
}

PXR_NAMESPACE_CLOSE_SCOPE
