//
// Copyright 2022 Pixar
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
#ifndef PXR_IMAGING_HD_RENDER_SETTINGS_H
#define PXR_IMAGING_HD_RENDER_SETTINGS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/bprim.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/range2f.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

///
/// Hydra prim backing render settings scene description.
/// While it is a state prim (Sprim) in spirit, it is made to be a Bprim to
/// ensure that it is sync'd prior to Sprims and Rprims to allow render setting 
/// opinions to be discovered and inform the sync process of those prims.
///
/// \note Hydra has several "render settings" concepts as of this writing, which
/// can be confusing. We have:
/// - HdRenderSettingsMap: A dictionary of token-value pairs that is provided
///   as an argument for render delegate construction.
/// - HdRenderSettingsDescriptorList: A mechanism to discover and update
///   render settings on the render delegate.
/// - Render task params: This currently captures opinions such as the camera to
///   use, the AOV outputs, etc.
///
/// We aim to transition away from the API and task based render settings
/// opinions above to using render settings scene description to drive
/// rendering in Hydra.
///
/// \sa HdRenderSettingsPrimTokens (defined in hd/tokens.h) for tokens
///     permitted in (legacy) scene delegate queries via Get(...).
///
/// \sa HdRenderSettingsSchema for querying locators and building container 
///     data sources when using scene indices.
///
class HdRenderSettings : public HdBprim
{
public:
    // Change tracking for HdRenderSettings.
    enum DirtyBits : HdDirtyBits {
        Clean                        = 0,
        DirtyActive                  = 1 << 1,
        DirtyNamespacedSettings      = 1 << 2,
        DirtyRenderProducts          = 1 << 3,
        DirtyIncludedPurposes        = 1 << 4,
        DirtyMaterialBindingPurposes = 1 << 5,
        DirtyRenderingColorSpace     = 1 << 6,
        AllDirty                     =    DirtyActive
                                        | DirtyNamespacedSettings
                                        | DirtyRenderProducts
                                        | DirtyIncludedPurposes
                                        | DirtyMaterialBindingPurposes
                                        | DirtyRenderingColorSpace
    };

    // Parameters that may be queried and invalidated.
    //
    // \note This mirrors UsdRender except that the render products and vars
    //       are "flattened out" similar to UsdRenderSpec.
    struct RenderProduct {
        struct RenderVar {
            SdfPath varPath;
            TfToken dataType;
            std::string sourceName;
            TfToken sourceType;
            VtDictionary namespacedSettings;
        };

        /// Identification & output information
        //
        // Path to product prim in scene description.
        SdfPath productPath;
        // The type of product, ex: "raster".
        TfToken type;
        // The name of the product, which uniquely identifies it.
        TfToken name;
        // The pixel resolution of the product.
        GfVec2i resolution;
        // The render vars that the product is comprised of.
        std::vector<RenderVar> renderVars;

        /// Camera and framing
        //
        // Path to the camera to use for this product.
        SdfPath cameraPath;
        // The pixel aspect ratio as adjusted by aspectRatioConformPolicy.
        float pixelAspectRatio;
        // The policy that was applied to conform aspect ratio
        // mismatches between the aperture and image.
        TfToken aspectRatioConformPolicy;
        // The camera aperture size as adjusted by aspectRatioConformPolicy.
        GfVec2f apertureSize;
        // The data window, in NDC terms relative to the aperture.
        // (0,0) corresponds to bottom-left and (1,1) corresponds to
        // top-right.  Note that the data window can partially cover
        // or extend beyond the unit range, for representing overscan
        // or cropped renders.
        GfRange2f dataWindowNDC;

        /// Settings overrides
        //
        bool disableMotionBlur;
        VtDictionary namespacedSettings;
    };

    using RenderProducts = std::vector<RenderProduct>;
    using NamespacedSettings = VtDictionary;

    HD_API
    ~HdRenderSettings() override;

    // ------------------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------------------
    HD_API
    bool IsActive() const;

    HD_API
    const NamespacedSettings& GetNamespacedSettings() const;

    HD_API
    unsigned int GetSettingsVersion() const;

    HD_API
    const RenderProducts& GetRenderProducts() const;

    HD_API
    const VtArray<TfToken>& GetIncludedPurposes() const;

    HD_API
    const VtArray<TfToken>& GetMaterialBindingPurposes() const;

    HD_API
    const TfToken& GetRenderingColorSpace() const;

    // XXX Add API to query AOV bindings.

    // ------------------------------------------------------------------------
    // Satisfying HdBprim
    // ------------------------------------------------------------------------
    HD_API
    void
    Sync(HdSceneDelegate *sceneDelegate,
         HdRenderParam *renderParam,
         HdDirtyBits *dirtyBits) override final;
    
    HD_API
    HdDirtyBits
    GetInitialDirtyBitsMask() const override;

protected:
    HD_API
    HdRenderSettings(SdfPath const& id);

    // ------------------------------------------------------------------------
    // Virtual API
    // ------------------------------------------------------------------------
    // This is called during Sync after dirty processing and before clearing the
    // dirty bits.
    virtual void
    _Sync(HdSceneDelegate *sceneDelegate,
          HdRenderParam *renderParam,
          const HdDirtyBits *dirtyBits);


private:
    // Class cannot be default constructed or copied.
    HdRenderSettings()                                     = delete;
    HdRenderSettings(const HdRenderSettings &)             = delete;
    HdRenderSettings &operator =(const HdRenderSettings &) = delete;

    bool _active;
    NamespacedSettings _namespacedSettings;
    unsigned int _settingsVersion;
    RenderProducts _products;
    VtArray<TfToken> _includedPurposes;
    VtArray<TfToken> _materialBindingPurposes;
    TfToken _renderingColorSpace;
};

// VtValue requirements
HD_API
size_t hash_value(HdRenderSettings::RenderProduct const &rp);

HD_API
std::ostream& operator<<(
    std::ostream& out, const HdRenderSettings::RenderProduct&);

HD_API
bool operator==(const HdRenderSettings::RenderProduct& lhs, 
                const HdRenderSettings::RenderProduct& rhs);
HD_API
bool operator!=(const HdRenderSettings::RenderProduct& lhs, 
                const HdRenderSettings::RenderProduct& rhs);
HD_API
std::ostream& operator<<(
    std::ostream& out, const HdRenderSettings::RenderProduct::RenderVar&);

HD_API
bool operator==(const HdRenderSettings::RenderProduct::RenderVar& lhs, 
                const HdRenderSettings::RenderProduct::RenderVar& rhs);
HD_API
bool operator!=(const HdRenderSettings::RenderProduct::RenderVar& lhs, 
                const HdRenderSettings::RenderProduct::RenderVar& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_RENDER_SETTINGS_H
