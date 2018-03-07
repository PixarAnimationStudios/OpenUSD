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
#ifndef HD_LIGHT_H
#define HD_LIGHT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include <boost/shared_ptr.hpp>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


#define HD_LIGHT_TOKENS                         \
    (angle)                                     \
    (exposure)                                  \
    (intensity)                                 \
    (params)                                    \
    (shadowCollection)                          \
    (shadowParams)                              \
    (transform)

TF_DECLARE_PUBLIC_TOKENS(HdLightTokens, HD_API, HD_LIGHT_TOKENS);

class HdSceneDelegate;
typedef boost::shared_ptr<class HdLight> HdLightSharedPtr;
typedef std::vector<class HdLight const *> HdLightPtrConstVector;

/// \class HdLight
///
/// A light model, used in conjunction with HdRenderPass.
///
class HdLight : public HdSprim {
public:
    HD_API
    HdLight(SdfPath const & id);
    HD_API
    virtual ~HdLight();

    // Change tracking for HdLight
    enum DirtyBits : HdDirtyBits {
        Clean                 = 0,
        DirtyTransform        = 1 << 0,
        DirtyParams           = 1 << 1,
        DirtyShadowParams     = 1 << 2,
        DirtyCollection       = 1 << 3,
        AllDirty              = (DirtyTransform
                                 |DirtyParams
                                 |DirtyShadowParams
                                 |DirtyCollection)
    };
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_LIGHT_H
