//
// Created by Lee Kerley on 3/18/24.
//

#include "gsRenderer.h"
#include <Imath/ImathMatrix.h>
#include <Imath/ImathQuat.h>
#include <Imath/ImathVec.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imageio.h>
#include <algorithm>
#include <memory>
#include <vector>

// Spherical harmonic coefficients.
constexpr float SH_C0   = 0.28209479177387814;
constexpr float SH_C1   = 0.4886025119029199;
constexpr float SH_C2_0 = 1.0925484305920792;
constexpr float SH_C2_1 = -1.0925484305920792;
constexpr float SH_C2_2 = 0.31539156525252005;
constexpr float SH_C2_3 = -1.0925484305920792;
constexpr float SH_C2_4 = 0.5462742152960396;
constexpr float SH_C3_0 = -0.5900435899266435;
constexpr float SH_C3_1 = 2.890611442640554;
constexpr float SH_C3_2 = -0.4570457994644658;
constexpr float SH_C3_3 = 0.3731763325901154;
constexpr float SH_C3_4 = -0.4570457994644658;
constexpr float SH_C3_5 = 1.445305721320277;
constexpr float SH_C3_6 = -0.5900435899266435;

using namespace OIIO;
using namespace Imath;

class ImageBufCollection {
  public:
    ImageBufCollection(ImageBuf* colorBuf, ImageBuf* depthBuf,
                       ImageBuf* primIDBuf)
        : _colorBuf(colorBuf), _depthBuf(depthBuf), _primIDBuf(primIDBuf) {
        if (colorBuf)
            _spec = colorBuf->spec();
        else if (depthBuf)
            _spec = depthBuf->spec();
        else if (primIDBuf)
            _spec = primIDBuf->spec();
    }

    void initIterators(ROI region) {
        if (_colorBuf) {
            _colorPIt = std::make_unique<ImageBuf::Iterator<u_char>>(
                *_colorBuf, region);
            _validPIt = _colorPIt.get();
        }
        if (_depthBuf) {
            _depthPIt = std::make_unique<ImageBuf::Iterator<float>>(
                *_depthBuf, region);
            _validPIt = _depthPIt.get();
        }
        if (_primIDBuf) {
            _primIDPIt = std::make_unique<ImageBuf::Iterator<int>>(
                *_primIDBuf, region);
            _validPIt = _primIDPIt.get();
        }
    }

    void incrementIterators() {
        if (_colorPIt)
            ++(*_colorPIt);
        if (_depthPIt)
            ++(*_depthPIt);
        if (_primIDPIt)
            ++(*_primIDPIt);
    }

    bool done() { return _validPIt->done(); }
    int x() { return _validPIt->x(); }
    int y() { return _validPIt->y(); }

    int width() const { return _spec.width; }
    int height() const { return _spec.height; }

    void setColor(V3f color, float alpha) {
        if (_colorPIt) {
            auto& p        = *_colorPIt;

            float invAlpha = (1.0f - alpha);
            p[0]           = color[0] * alpha + p[0] * invAlpha;
            p[1]           = color[1] * alpha + p[1] * invAlpha;
            p[2]           = color[2] * alpha + p[2] * invAlpha;
            p[3]           = alpha + p[3] * invAlpha;
        }
    }

    void setDepth(float depth) {
        if (_depthPIt) {
            (*_depthPIt)[0] = depth;
        }
    }

    void setPrimID(int id) {
        if (_primIDPIt) {
            (*_primIDPIt)[0] = id;
        }
    }

  private:
    ImageSpec _spec;
    ImageBuf* _colorBuf{nullptr};
    ImageBuf* _depthBuf{nullptr};
    ImageBuf* _primIDBuf{nullptr};

    ImageBuf::IteratorBase* _validPIt{nullptr};
    std::unique_ptr<ImageBuf::Iterator<u_char>> _colorPIt{nullptr};
    std::unique_ptr<ImageBuf::Iterator<float>> _depthPIt{nullptr};
    std::unique_ptr<ImageBuf::Iterator<int>> _primIDPIt{nullptr};
};

float clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }

V3f clamp01(V3f v) { return {clamp01(v[0]), clamp01(v[1]), clamp01(v[2])}; }

struct Splat {
    // currently using camera z depth - but could use distance from camera -
    // different sorting metrics in different files?
    float getDepth(const M44f& worldToViewMtx) const {
        auto pos4   = V4f(position);
        auto camPos = pos4 * worldToViewMtx;
        return camPos.z;
    }

    V3f getColor(const V3f& camDir) const {
        unsigned int sh_size = sh_weights.size();

        V3f color            = sh_weights[0];

        // clang-format off
        if (sh_size > 1) {
            float x = camDir[0];
            float y = camDir[1];
            float z = camDir[2];

            color = color -
                    ( sh_weights[1] * y ) +
                    ( sh_weights[2] * z ) -
                    ( sh_weights[3] * x );

            if (sh_size > 4) {
                float xx = x * x;
                float yy = y * y;
                float zz = z * z;
                float xy = x * y;
                float yz = y * z;
                float xz = x * z;

                color = color +
                        ( sh_weights[4] * xy                    ) +
                        ( sh_weights[5] * yz                    ) +
                        ( sh_weights[6] * (2.0f * zz - xx - yy) ) +
                        ( sh_weights[7] * xz                    ) +
                        ( sh_weights[8] * (xx - yy)             );

                if (sh_size > 9) {
                    color = color +
                            ( sh_weights[9]  * (y * (3.0f * xx - yy))                       ) +
                            ( sh_weights[10]  * (xy * z)                                     ) +
                            ( sh_weights[11] * (y * (4.0f * zz - xx - yy))                  ) +
                            ( sh_weights[12] * (z * (2.0f * zz - 3.0f * xx - 3.0f * yy))    ) +
                            ( sh_weights[13] * (x * (4.0f * zz - xx - yy))                  ) +
                            ( sh_weights[14] * (z * (xx - yy))                              ) +
                            ( sh_weights[15] * (x * (xx - 3.0f * yy))                       );
                }
            }
        }
        // clang-format on

        return clamp01(color + V3f(0.5));
    }

    void setCov3D(V3f scale, Quatf quat) {
        auto rot          = quat.toMatrix33();

        auto scaleSquared = scale * scale;
        auto scaleMtx = M33f(scaleSquared[0], 0.0, 0.0, 0.0, scaleSquared[1],
                             0.0, 0.0, 0.0, scaleSquared[2]);

        // create the covariance matrix by rotating the splat back to its local
        // space, scaling along local x/y/z axes and then rotating back.
        cov3D = rot.transposed() * scaleMtx * rot;
    }

    V3f position;
    M33f cov3D;
    float opacity;
    std::vector<V3f> sh_weights;
};

class GaussianSplatsRenderer::Impl {
  public:
    Impl()  = default;
    ~Impl() = default;

    void setWorldToViewMatrix(const M44f& m) {
        _worldToViewMtx     = m;
        _needsIndicesSorted = true;
    }
    void setProjMatrix(const M44f& m) { _projMatrix = m; }
    void addGaussianSplats(const std::string& splatName,
                           GaussianSplats::Ptr newSplats);
    void removeGaussianSplats(const std::string& splatName);
    bool renderGaussianSplatScene(ImageBuf* colorBuf,
                                  ImageBuf* depthBuf,
                                  ImageBuf* primIDBuf) const;

  private:
    void updateSortedIndices() const {
        std::sort(
            _sortedIndices.begin(), _sortedIndices.end(),
            [&](const Index& a, const Index& b) {
                // needs to return true if the first element `a` is "less" or
                // "before" in the list than the second element `b` we want the
                // sorted list to be directly renderable, so the splats should
                // be ordered from back to front which means we want to return
                // true if the first splat is "deeper" ie. larger depth, than
                // the second splat.
                return _splats[a.first][a.second].getDepth(_worldToViewMtx) <
                       _splats[b.first][b.second].getDepth(_worldToViewMtx);
            });
        _needsIndicesSorted = false;
    }

    std::mutex _sceneMutex;

    using SplatVector = std::vector<Splat>;

    size_t _numSplats{0};
    std::vector<SplatVector> _splats;
    std::vector<std::string> _splatNames;
    std::vector<int> _splatPrimIDs;

    // mutable data storage for sorting the splats.
    typedef std::pair<size_t, size_t> Index;
    mutable std::vector<Index> _sortedIndices;
    mutable bool _needsIndicesSorted{true};

    M44f _worldToViewMtx;
    M44f _projMatrix;
};

void GaussianSplatsRenderer::Impl::addGaussianSplats(
    const std::string& splatName, GaussianSplats::Ptr newSplats) {
    if (newSplats->positions.empty()) {
        return;
    }

    SplatVector splatVec;
    size_t numNewSplats = newSplats->positions.size();

    size_t sphericalHarmonics_stride =
        newSplats->sphericalHarmonics.size() / numNewSplats;

    bool hasValidOpacity  = newSplats->opacities.size() == numNewSplats;
    bool hasValidScale    = newSplats->scales.size() == numNewSplats;
    bool hasValidRotation = newSplats->rotations.size() == numNewSplats;

    splatVec.resize(numNewSplats);

    for (unsigned int i = 0, n = numNewSplats; i < n; ++i) {
        splatVec[i].position = newSplats->positions[i] * newSplats->xform;

        if (hasValidOpacity) {
            splatVec[i].opacity = newSplats->opacities[i];
        }

        if (hasValidScale && hasValidRotation) {
            splatVec[i].setCov3D(newSplats->scales[i], newSplats->rotations[i]);
        } else if (hasValidRotation && !hasValidScale) {
            splatVec[i].setCov3D(V3f(1.0, 1.0, 1.0), newSplats->rotations[i]);
        } else if (!hasValidRotation && hasValidScale) {
            splatVec[i].setCov3D(newSplats->scales[i], Quatf());
        }

        if (!newSplats->sphericalHarmonics.empty()) {
            // unpack this splats list of SH weights
            auto sh_it = newSplats->sphericalHarmonics.begin() +
                         sphericalHarmonics_stride * i;

            auto& sh_weights = splatVec[i].sh_weights;
            sh_weights.resize(sphericalHarmonics_stride);
            for (unsigned int j = 0; j < sphericalHarmonics_stride; j += 1) {
                sh_weights[j] = (sh_it[j]);
            }

            // we pre-weight the SH weights by their respective SH coefficients
            // once on load.
            sh_weights[0] *= SH_C0;

            size_t sh_size = sh_weights.size();
            if (sh_size > 1) {
                sh_weights[1] *= SH_C1;
                sh_weights[2] *= SH_C1;
                sh_weights[3] *= SH_C1;

                if (sh_size > 4) {
                    sh_weights[4] *= SH_C2_0;
                    sh_weights[5] *= SH_C2_1;
                    sh_weights[6] *= SH_C2_2;
                    sh_weights[7] *= SH_C2_3;
                    sh_weights[8] *= SH_C2_4;

                    if (sh_size > 9) {
                        sh_weights[9] *= SH_C3_0;
                        sh_weights[10] *= SH_C3_1;
                        sh_weights[11] *= SH_C3_2;
                        sh_weights[12] *= SH_C3_3;
                        sh_weights[13] *= SH_C3_4;
                        sh_weights[14] *= SH_C3_5;
                        sh_weights[15] *= SH_C3_6;
                    }
                }
            }
        }
    }
    {
        std::lock_guard<std::mutex> guard(_sceneMutex);

        _numSplats += numNewSplats;
        _splats.emplace_back(splatVec);
        _splatNames.emplace_back(splatName);
        _splatPrimIDs.emplace_back(newSplats->primID);

        // we need the index of the set of splats we've just added to add to the
        // indices to be sorted.
        size_t i = _splats.size() - 1;

        // add new indices for each of the splats that have been added in this
        // set.
        for (size_t j = 0, m = _splats[i].size(); j < m; ++j) {
            _sortedIndices.emplace_back(Index({i, j}));
        }
        _needsIndicesSorted = true;
    }
}

void GaussianSplatsRenderer::Impl::removeGaussianSplats(
    const std::string& splatName) {
    std::lock_guard<std::mutex> guard(_sceneMutex);

    auto nameIt = std::find(_splatNames.begin(), _splatNames.end(), splatName);
    if (nameIt == _splatNames.end()) {
        // deal with removing something that doesn't exist
        return;
    }

    size_t offset   = nameIt - _splatNames.begin();

    auto oldIndices = _sortedIndices;

    _sortedIndices.clear();
    for (const auto& it : oldIndices) {
        if (it.first != offset) {
            _sortedIndices.emplace_back(it);
        }
    }

    auto splatIt       = _splats.begin() + offset;
    auto splatPrimIDIt = _splatPrimIDs.begin() + offset;

    _numSplats -= splatIt->size();

    *nameIt = "";
    splatIt->clear();
    *splatPrimIDIt      = -1;

    _needsIndicesSorted = false;
}

bool GaussianSplatsRenderer::Impl::renderGaussianSplatScene(
    ImageBuf* colorBuf, ImageBuf* depthBuf,
    ImageBuf* primIDBuf) const {
    ImageBufCollection imageBufs(colorBuf, depthBuf, primIDBuf);

    constexpr V2f oneVec(1.0, 1.0);

    if (_needsIndicesSorted) {
        updateSortedIndices();
    }

    const int w = imageBufs.width();
    const int h = imageBufs.height();
    const V2f wh(w, h);

    const float halfWidth  = w / 2.0f;
    const float halfHeight = h / 2.0f;
    const V2f half_wh(halfWidth, halfHeight);

    const auto imageROI = ROI(0, w, 0, h);

    // calculate the camera location in order to later determine the view
    // direction to the splat.
    auto viewToWorldMtx = _worldToViewMtx.inverse();
    V3f cam_loc(viewToWorldMtx[3][0], viewToWorldMtx[3][1],
                viewToWorldMtx[3][2]);

    float aspect    = (float)h / (float)w;
    float htan_fovy = 1 / _projMatrix[1][1];
    float htan_fovx = htan_fovy * aspect;
    float focal     = (float)h / (2.0 * htan_fovy);

    float limx      = htan_fovx;
    float limy      = htan_fovy;

    // rotational and scale portion of the world to view matrix
    M33f W(_worldToViewMtx[0][0], _worldToViewMtx[1][0], _worldToViewMtx[2][0],
           _worldToViewMtx[0][1], _worldToViewMtx[1][1], _worldToViewMtx[2][1],
           _worldToViewMtx[0][2], _worldToViewMtx[1][2], _worldToViewMtx[2][2]);

    // loop over the sorted splat indices and render the splats from back to
    // front - overing the splats as we go.
    for (const auto splat_index : _sortedIndices) {
        const auto& splat = _splats[splat_index.first][splat_index.second];

        // skip any splats that are completely transparent
        if (splat.opacity <= 0.0) {
            continue;
        }

        V4f cameraPos = V4f(splat.position) * _worldToViewMtx;
        auto ndcPos   = cameraPos * _projMatrix;
        if (ndcPos[3] < 0) {
            continue;
        }

        V2f pos_ndc_2d = V2f(ndcPos[0] / ndcPos[3], ndcPos[1] / ndcPos[3]);

        float txtz     = cameraPos.x / cameraPos.z;
        float tytz     = cameraPos.y / cameraPos.z;

        float tx       = std::min(limx, std::max(-limx, txtz)) * cameraPos.z;
        float ty       = std::min(limy, std::max(-limy, tytz)) * cameraPos.z;
        float tz       = cameraPos.z;

        M33f J(focal / tz, 0.0, -(focal * tx) / (tz * tz), 0.0, focal / tz,
               -(focal * ty) / (tz * tz), 0.0, 0.0, 0.0);
        auto T     = J * W;
        auto cov   = T * splat.cov3D * T.transposed();

        auto cov2d = M22f(cov[0][0], cov[0][1], cov[1][0], cov[1][1]);

        auto det   = cov2d.determinant();
        if (det == 0.0)
            continue;

        V2f bboxsize_cam(3.0 * std::sqrt(cov2d[0][0]),
                         3.0 * std::sqrt(cov2d[1][1]));

        V2f bboxsize_ndc     = (bboxsize_cam / wh) * 2.0;

        auto bbox_ndc_min    = -bboxsize_ndc + pos_ndc_2d;
        auto bbox_ndc_max    = bboxsize_ndc + pos_ndc_2d;

        auto bbox_screen_min = (bbox_ndc_min + oneVec) * half_wh;
        auto bbox_screen_max = (bbox_ndc_max + oneVec) * half_wh;

        auto x1              = int(floor(bbox_screen_min[0]));
        auto y1              = int(floor(bbox_screen_min[1]));

        auto x2              = int(ceil(bbox_screen_max[0]));
        auto y2              = int(ceil(bbox_screen_max[1]));

        if (x1 > w || x2 < 0 || y1 > h || y2 < 0) {
            // if the entire splat bound is outside the image then we can skip
            // the whole splat
            continue;
        }

        V2f splatPixelSize = V2f(x2 - x1, y2 - y1);

        // calculate the conic for the gaussian falloff.
        auto det_inv   = 1.0 / det;
        auto conic     = V3f(cov2d[1][1] * det_inv, -cov2d[0][1] * det_inv,
                             cov2d[0][0] * det_inv);

        V3f camera_dir = splat.position - cam_loc;
        camera_dir.normalize();

        auto splat_color = splat.getColor(camera_dir);
        auto splat_depth = ndcPos[2];
        int splat_primID = _splatPrimIDs[splat_index.first];

        // step in camera space of the splat for each pixel.
        auto bbox_cam_step_per_pixel = (bboxsize_cam * 2.0f) / splatPixelSize;

        // calculate an OIIO ROI for the region covered by the splat.
        auto splatROI = ROI(x1, x2, y1, y2, 0, 1, 0, 4);

        // clip the splatROI to the image
        auto clippedSplatROI = roi_intersection(splatROI, imageROI);

        for (imageBufs.initIterators(clippedSplatROI); !imageBufs.done();
             imageBufs.incrementIterators()) {
            // note pixel coord WRT to the splat BB needs to be taken from the
            // un-clipped ROI.
            V2f splatPixelIndex(imageBufs.x() - splatROI.xbegin,
                                imageBufs.y() - splatROI.ybegin);

            // calculate the corresponding point in camera space.
            V2f pixel_cam =
                -bboxsize_cam + splatPixelIndex * bbox_cam_step_per_pixel;

            // calculate the gaussian falloff in the space of the splat. (note
            // we defer the outer exp() call to after the early exit)
            auto power = -(conic.x * pow(pixel_cam[0], 2) +
                           conic.z * pow(pixel_cam[1], 2)) /
                             2.0 -
                         (conic.y * pixel_cam[0] * pixel_cam[1]);
            if (power > 0)
                continue;

            float alpha = splat.opacity * exp(power);
            if (alpha <= 0)
                continue;
            alpha = std::min(0.99f, alpha);

            if (alpha > 0.1) {
                imageBufs.setPrimID(splat_primID);
                imageBufs.setDepth(splat_depth);
            }
            imageBufs.setColor(splat_color, alpha);
        }
    }

    return true;
}

GaussianSplatsRenderer::GaussianSplatsRenderer()
    : pImpl(std::make_unique<Impl>()) {}

GaussianSplatsRenderer::~GaussianSplatsRenderer() = default;

void GaussianSplatsRenderer::setWorldToViewMatrix(const M44f& m) {
    pImpl->setWorldToViewMatrix(m);
}

void GaussianSplatsRenderer::setProjMatrix(const M44f& m) {
    pImpl->setProjMatrix(m);
}

void GaussianSplatsRenderer::addGaussianSplats(const std::string& splatName,
                                               GaussianSplats::Ptr newSplats) {
    pImpl->addGaussianSplats(splatName, newSplats);
}

void GaussianSplatsRenderer::removeGaussianSplats(
    const std::string& splatName) {
    pImpl->removeGaussianSplats(splatName);
}

bool GaussianSplatsRenderer::renderGaussianSplatScene(
    ImageBuf* colorBuf, ImageBuf* depthBuf,
    ImageBuf* primIDBuf) const {
    return pImpl->renderGaussianSplatScene(colorBuf, depthBuf, primIDBuf);
}
