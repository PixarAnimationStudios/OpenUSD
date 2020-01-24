//
// Copyright 2019 Pixar
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
#ifndef EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_H
#define EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_H

#include "pxr/pxr.h"
#include "hdPrman/gprim.h"
#include "pxr/imaging/hd/field.h"
#include "pxr/imaging/hd/volume.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/vertexAdjacency.h"
#include "pxr/base/gf/matrix4f.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_Field final : public HdField {
public:
    HdPrman_Field(TfToken const& typeId, SdfPath const& id);
    virtual void Sync(HdSceneDelegate *sceneDelegate,
                      HdRenderParam *renderParam,
                      HdDirtyBits *dirtyBits) override;
    virtual void Finalize(HdRenderParam *renderParam) override;
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;
private:
    TfToken const _typeId;
};

class HdPrman_Volume final : public HdPrman_Gprim<HdVolume> {
public:
    typedef HdPrman_Gprim<HdVolume> BASE;
public:
    HF_MALLOC_TAG_NEW("new HdPrman_Volume");
    HdPrman_Volume(SdfPath const& id,
                SdfPath const& instancerId = SdfPath());
    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;
protected:
    virtual void
    _ConvertGeometry(HdPrman_Context *context,
                      RixRileyManager *mgr,
                      HdSceneDelegate *sceneDelegate,
                      const SdfPath &id,
                      RtUString *primType,
                      std::vector<HdGeomSubset> *geomSubsets,
                      RixParamList* &primvars) override;

    virtual riley::MaterialId
    _GetFallbackMaterial(HdPrman_Context *context) override {
        return context->fallbackVolumeMaterial;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_22_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_VOLUME_H
