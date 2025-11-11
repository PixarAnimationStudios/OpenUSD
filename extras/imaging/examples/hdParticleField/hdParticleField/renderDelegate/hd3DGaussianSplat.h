//
// Created by Lee Kerley on 3/26/24.
//

#ifndef HDPARTICLEFIELD_HD3DGAUSSIANSPLAT_H
#define HDPARTICLEFIELD_HD3DGAUSSIANSPLAT_H

#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/quatf.h>
#include <pxr/imaging/hd/points.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

class Hd3DGaussianSplat : public HdRprim {
  public:
    HF_MALLOC_TAG_NEW("new Hd3DGaussianSplat");

    Hd3DGaussianSplat(SdfPath const& id);
    ~Hd3DGaussianSplat() override = default;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

    void Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits,
              TfToken const& reprToken) override;

    HD_API
    TfTokenVector const& GetBuiltinPrimvarNames() const override;

    const VtVec3fArray& GetPositions() const { return _positions; }
    const VtQuatfArray& GetOrientations() const { return _orientations; }
    const VtVec3fArray& GetScales() const { return _scales; }
    const VtFloatArray& GetOpacities() const { return _opacities; }
    const VtVec3fArray& GetSphericalHarmonics() const { return _sphericalHarmonics; }
    const GfMatrix4f& GetTransform() const { return _transform; }

  protected:
    void _InitRepr(TfToken const& reprToken, HdDirtyBits* dirtyBits) override;

    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

  private:
    // This class does not support copying.
    Hd3DGaussianSplat(const Hd3DGaussianSplat&)            = delete;
    Hd3DGaussianSplat& operator=(const Hd3DGaussianSplat&) = delete;

    VtVec3fArray _positions;
    VtQuatfArray _orientations;
    VtVec3fArray _scales;
    VtFloatArray _opacities;
    VtVec3fArray _sphericalHarmonics;
    GfMatrix4f _transform;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDPARTICLEFIELD_HD3DGAUSSIANSPLAT_H
