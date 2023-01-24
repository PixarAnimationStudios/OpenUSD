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

PXR_NAMESPACE_OPEN_SCOPE

///
/// XXX Empty for now, but will be filled up in a follow-up change to mirror
///     UsdRenderSpec.
///
struct HdRenderSettingsParams
{
};

///
/// Abstract hydra prim backing render settings scene description.
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
/// opinions (above 2) to using render settings scene description to drive
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
        Clean                 = 0,
        DirtyActive           = 1 << 1,
        DirtyParams           = 1 << 2,
        AllDirty              = (DirtyActive | DirtyParams)
    };

    HD_API
    ~HdRenderSettings() override;

    // ------------------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------------------
    HD_API
    bool IsActive() const;

    HD_API
    const HdRenderSettingsParams& GetParams() const;

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
    HdRenderSettingsParams _params;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_RENDER_SETTINGS_H
