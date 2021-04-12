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
#ifndef PXR_IMAGING_HD_ST_LIGHT_H
#define PXR_IMAGING_HD_ST_LIGHT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/light.h"

#include "pxr/imaging/glf/simpleLight.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStLight
///
/// A light model for use in Storm.
/// Note: This class simply stores the light parameters and relies on an
/// external task (HdxSimpleLightTask) to upload them to the GPU.
///
class HdStLight final : public HdLight {
public:
    HDST_API
    HdStLight(SdfPath const & id, TfToken const &lightType);
    HDST_API
    ~HdStLight() override;

    /// Synchronizes state from the delegate to this object.
    HDST_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;
    
    /// Finalizes object resources. This function might not delete resources,
    /// but it should deal with resource ownership so that the sprim is
    /// deletable.
    HDST_API
    void Finalize(HdRenderParam *renderParam) override;
    
    /// Accessor for tasks to get the parameters cached in this object.
    HDST_API
    VtValue Get(TfToken const &token) const;

    /// Returns the minimal set of dirty bits to place in the
    /// change tracker for use in the first sync of this prim.
    /// Typically this would be all dirty bits.
    HDST_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;
    
private:
    // Converts area lights (sphere lights and distant lights) into
    // glfSimpleLights and inserts them in the dictionary so 
    // SimpleLightTask can use them later on as if they were regular lights.
    GlfSimpleLight _ApproximateAreaLight(SdfPath const &id, 
                                         HdSceneDelegate *sceneDelegate);

    // Collects data such as the environment map texture path for a
    // dome light. The lighting shader is responsible for pre-calculating
    // the different textures needed for IBL.
    GlfSimpleLight _PrepareDomeLight(SdfPath const &id, 
                                     HdSceneDelegate *sceneDelegate);
    
private:
    // Stores the internal light type of this light.
    TfToken _lightType;

    // Cached states.
    TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _params;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_LIGHT_H
