//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MESH_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MESH_H

#include "pxr/pxr.h"
#include "hdPrman/gprim.h"
#include "pxr/imaging/hd/mesh.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_Mesh final : public HdPrman_Gprim<HdMesh>
{
public:
    using BASE = HdPrman_Gprim<HdMesh>;

    HF_MALLOC_TAG_NEW("new HdPrman_Mesh");

    HdPrman_Mesh(SdfPath const& id, const bool isMeshLight);

    HdDirtyBits GetInitialDirtyBitsMask() const override;

protected:
    RtPrimVarList
    _ConvertGeometry(HdPrman_RenderParam *renderParam,
                     HdSceneDelegate *sceneDelegate,
                     const SdfPath &id,
                     RtUString *primType,
                     std::vector<HdGeomSubset> *geomSubsets) override;

    bool _PrototypeOnly() override;


private:
    bool _isMeshLight;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_MESH_H
