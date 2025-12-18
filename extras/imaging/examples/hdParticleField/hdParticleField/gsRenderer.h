#ifndef LIBGAUSSIANSPLATSRENDERER_GAUSSIANSPLATSRENDERER_H
#define LIBGAUSSIANSPLATSRENDERER_GAUSSIANSPLATSRENDERER_H

#include <Imath/ImathMatrix.h>
#include <Imath/ImathQuat.h>
#include <Imath/ImathVec.h>
#include <OpenImageIO/imagebuf.h>
#include <memory>
#include <vector>

struct GaussianSplats {
    using Ptr = std::shared_ptr<GaussianSplats>;
    static Ptr create() { return std::make_shared<GaussianSplats>(); }

    Imath::M44f xform;
    int primID;
    std::vector<Imath::V3f> positions;
    std::vector<Imath::Quatf> rotations;
    std::vector<Imath::V3f> scales;
    std::vector<float> opacities;
    std::vector<Imath::V3f> sphericalHarmonics;
};

class GaussianSplatsRenderer {
  public:
    GaussianSplatsRenderer();
    ~GaussianSplatsRenderer();

    void setWorldToViewMatrix(const Imath::M44f& m);
    void setProjMatrix(const Imath::M44f& m);
    void addGaussianSplats(const std::string& splatName,
                           GaussianSplats::Ptr newSplats);
    void removeGaussianSplats(const std::string& splatName);
    bool renderGaussianSplatScene(OIIO::ImageBuf* colorBuf,
                                  OIIO::ImageBuf* depthBuf,
                                  OIIO::ImageBuf* primIDBuf) const;

  private:
    class Impl;
    std::unique_ptr<Impl> pImpl{nullptr};

    /// Cannot copy.
    GaussianSplatsRenderer(const GaussianSplatsRenderer&)            = delete;
    GaussianSplatsRenderer& operator=(const GaussianSplatsRenderer&) = delete;
};

#endif // USDSPLATS_SIMPLEGSRENDERERCPU_H
