//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usdRender/spec.h"
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usdRender/product.h"
#include "pxr/usd/usdRender/var.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/frustum.h"

PXR_NAMESPACE_OPEN_SCOPE

static void
_ReadExtraSettings(UsdPrim const& prim, VtDictionary *extraSettings,
                   std::vector<std::string> const& namespaces)
{
    std::vector<UsdAttribute> attrs = prim.GetAuthoredAttributes();
    for (UsdAttribute attr: attrs) {
        if (namespaces.empty()) {
            if (attr.HasFallbackValue()) {
                // Skip attributes built into schemas.
                continue;
            }
        } else {
            bool attrIsInRequestedNS = false;
            for (std::string const& ns: namespaces) {
                if (TfStringStartsWith(attr.GetName(), ns)) {
                    attrIsInRequestedNS = true;
                    break;
                }
            }
            if (!attrIsInRequestedNS) {
                continue;
            }
        }
        VtValue val;
        if (attr.Get(&val)) {
            (*extraSettings)[attr.GetName()] = val;
        }
    }
}

template<typename T>
inline bool
_Get(UsdAttribute const &attr, T* val, bool sparse=false)
{
    if (!sparse || attr.HasAuthoredValue()) {
        return attr.Get(val);
    }
    return false;
}

static void
_ReadSettingsBase(UsdRenderSettingsBase const& base,
                  UsdRenderSpec::Product *pd,
                  bool sparse)
{
    SdfPathVector targets; 
    base.GetCameraRel().GetForwardedTargets(&targets);
    if (!targets.empty()) {
        pd->cameraPath = targets[0];
    }
    _Get( base.GetResolutionAttr(), &pd->resolution, sparse );
    _Get( base.GetPixelAspectRatioAttr(), &pd->pixelAspectRatio, sparse );
    _Get( base.GetAspectRatioConformPolicyAttr(),
          &pd->aspectRatioConformPolicy, sparse );
    {
        // Convert dataWindowNDC from vec4 to range2.
        GfVec4f dataWindowNDCVec;
        if (_Get( base.GetDataWindowNDCAttr(), &dataWindowNDCVec, sparse )) {
            pd->dataWindowNDC = GfRange2f(
                GfVec2f(dataWindowNDCVec[0], dataWindowNDCVec[1]),
                GfVec2f(dataWindowNDCVec[2], dataWindowNDCVec[3]));
        }
    }
    _Get( base.GetInstantaneousShutterAttr(),
          &pd->instantaneousShutter, sparse );
}

// TODO: Consolidate with CameraUtilConformedWindow().  Resolve policy
// name mismatches; also CameraUtil cannot compensate pixelAspectRatio.
static void _ApplyAspectRatioPolicy(
    UsdRenderSpec::Product *product)
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

UsdRenderSpec
UsdRenderComputeSpec(
    UsdRenderSettings const& settings,
    UsdTimeCode time,
    std::vector<std::string> const& extraNamespaces)
{
    UsdRenderSpec rd;
    UsdPrim prim = settings.GetPrim();
    UsdStageWeakPtr stage = prim.GetStage();
    if (!stage) {
        TF_CODING_ERROR("Invalid stage\n");
        return rd;
    }

    // Read shared settings as a "base product".
    UsdRenderSpec::Product base;
    _ReadSettingsBase(UsdRenderSettingsBase(prim), &base, false);
    _ReadExtraSettings(prim, &base.extraSettings, extraNamespaces);

    // Products
    SdfPathVector targets; 
    settings.GetProductsRel().GetForwardedTargets(&targets);
    for (SdfPath const& target: targets) {
        if (UsdRenderProduct product =
            UsdRenderProduct(stage->GetPrimAtPath(target))) {
            UsdRenderSpec::Product pd = base;

            // Read product-specific overrides to base render settings.
            _ReadSettingsBase(UsdRenderSettingsBase(product), &pd, true);

            // Read camera aperture and apply aspectRatioConformPolicy.
            if (UsdGeomCamera cam =
                UsdGeomCamera(stage->GetPrimAtPath(pd.cameraPath))) {
                cam.GetHorizontalApertureAttr().Get(&pd.apertureSize[0]);
                cam.GetVerticalApertureAttr().Get(&pd.apertureSize[1]);
                _ApplyAspectRatioPolicy(&pd);
            } else {
                TF_RUNTIME_ERROR("UsdRenderSettings: Could not find camera "
                                 "<%s> for product <%s>\n",
                                 pd.cameraPath.GetText(), target.GetText());
                continue;
            }

            // Read product-only settings.
            product.GetProductTypeAttr().Get(&pd.type);
            product.GetProductNameAttr().Get(&pd.name);

            // Read render vars.
            SdfPathVector renderVarPaths;
            product.GetOrderedVarsRel().GetForwardedTargets(&renderVarPaths);
            for (SdfPath const& renderVarPath: renderVarPaths ) {
                bool foundExisting = false;
                for (size_t i=0; i < rd.renderVars.size(); ++i) {
                    if (rd.renderVars[i].renderVarPath == renderVarPath) {
                        pd.renderVarIndices.push_back(i);
                        foundExisting = true;
                        break;
                    }
                }
                if (!foundExisting) {
                    UsdPrim prim = stage->GetPrimAtPath(renderVarPath);
                    if (prim.IsA<UsdRenderVar>()) {
                        UsdRenderVar rv(prim);
                        UsdRenderSpec::RenderVar rvd;
                        // Store schema-defined attributes in explicit fields.
                        rvd.renderVarPath = renderVarPath;
                        rv.GetDataTypeAttr().Get(&rvd.dataType);
                        rv.GetSourceNameAttr().Get(&rvd.sourceName);
                        rv.GetSourceTypeAttr().Get(&rvd.sourceType);
                        // Store any other custom attributes in extraSettings.
                        _ReadExtraSettings(prim, &rvd.extraSettings,
                                           extraNamespaces);
                        // Record new render var.
                        pd.renderVarIndices.push_back(rd.renderVars.size());
                        rd.renderVars.emplace_back(rvd);
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
            // Store any other custom attributes in extraSettings.
            _ReadExtraSettings(product.GetPrim(), &pd.extraSettings,
                               extraNamespaces);
            rd.products.emplace_back(pd);
        }
    }

    // Scene configuration
    settings.GetMaterialBindingPurposesAttr().Get(&rd.materialBindingPurposes);
    settings.GetIncludedPurposesAttr().Get(&rd.includedPurposes);
    // Store any other custom attributes in extraSettings.
    _ReadExtraSettings(prim, &rd.extraSettings, extraNamespaces);

    return rd;
}

PXR_NAMESPACE_CLOSE_SCOPE
