//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_IMAGE_SHADER_H
#define PXR_IMAGING_HD_IMAGE_SHADER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/dictionary.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HD_IMAGE_SHADER_TOKENS                              \
    (enabled)                                               \
    (priority)                                              \
    (filePath)                                              \
    (constants)                                             \
    (materialNetwork)

TF_DECLARE_PUBLIC_TOKENS(HdImageShaderTokens, HD_API, HD_IMAGE_SHADER_TOKENS);

class HdSceneDelegate;

using HdMaterialNetworkInterfaceUniquePtr =
    std::unique_ptr<class HdMaterialNetworkInterface>;

/// \class HdImageShader
///
/// An image shader.
///
class HdImageShader : public HdSprim
{
public:
    HD_API
    HdImageShader(SdfPath const & id);
    HD_API
    ~HdImageShader() override;

    // Change tracking for HdImageShader
    enum DirtyBits : HdDirtyBits {
        Clean                 = 0,
        DirtyEnabled          = 1 << 0,
        DirtyPriority         = 1 << 1,
        DirtyFilePath         = 1 << 2,
        DirtyConstants        = 1 << 3,
        DirtyMaterialNetwork  = 1 << 4,

        AllDirty              = (DirtyEnabled
                                 |DirtyPriority
                                 |DirtyFilePath
                                 |DirtyConstants
                                 |DirtyMaterialNetwork)
    };

    // ---------------------------------------------------------------------- //
    /// Sprim API
    // ---------------------------------------------------------------------- //
 
    /// Synchronizes state from the delegate to this object.
    HD_API
    void Sync(
        HdSceneDelegate* sceneDelegate,
        HdRenderParam* renderParam,
        HdDirtyBits* dirtyBits) override;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HD_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    // ---------------------------------------------------------------------- //
    /// Image shader parameters accessor API
    // ---------------------------------------------------------------------- //
    HD_API
    bool GetEnabled() const;

    HD_API
    int GetPriority() const;

    HD_API
    const std::string& GetFilePath() const;

    HD_API
    const VtDictionary& GetConstants() const;

    HD_API
    const HdMaterialNetworkInterface* GetMaterialNetwork() const;

private:
    bool _enabled;
    int _priority;
    std::string _filePath;
    VtDictionary _constants;
    HdMaterialNetwork2 _materialNetwork;
    HdMaterialNetworkInterfaceUniquePtr _materialNetworkInterface;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_IMAGE_SHADER_H
