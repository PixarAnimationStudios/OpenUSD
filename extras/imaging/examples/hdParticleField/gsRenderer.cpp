//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Original implementation by Lee Kerley on 3/18/24.

#include "gsRenderer.h"

#include "pxr/base/gf/rect2i.h"
#include "pxr/base/gf/matrix2f.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec4f.h"

#include <algorithm>
#include <memory>
#include <vector>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

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

float clamp01(float v) { return std::min(1.0f, std::max(0.0f, v)); }
GfVec3f clamp01(GfVec3f v) {
    return {clamp01(v[0]), clamp01(v[1]), clamp01(v[2])};
}

struct Splat {
    // currently using camera z depth - but could use distance from camera -
    // different sorting metrics in different files?
    float getDepth(const GfMatrix4f& worldToViewMtx) const {
        return worldToViewMtx.Transform(position)[2];
    }

    GfVec3f getColor(const GfVec3f& camDir) const {
        unsigned int sh_size = sh_weights.size();

        GfVec3f color            = sh_weights[0];

        // clang-format off
        if (sh_size > 1) {
            float x = camDir[0], y = camDir[1], z = camDir[2];

            color = color -
                ( sh_weights[1] * y ) +
                ( sh_weights[2] * z ) -
                ( sh_weights[3] * x );

            if (sh_size > 4) {
                float xx = x * x, yy = y * y, zz = z * z;
                float xy = x * y, yz = y * z, xz = x * z;

                color = color +
                    ( sh_weights[4] * xy                    ) +
                    ( sh_weights[5] * yz                    ) +
                    ( sh_weights[6] * (2.0f * zz - xx - yy) ) +
                    ( sh_weights[7] * xz                    ) +
                    ( sh_weights[8] * (xx - yy)             );

                if (sh_size > 9) {
                    color = color +
                        ( sh_weights[9]  * (y * (3.0f * xx - yy))      ) +
                        ( sh_weights[10] * (xy * z)                    ) +
                        ( sh_weights[11] * (y * (4.0f * zz - xx - yy)) ) +
                        ( sh_weights[12] * (z *
                            (2.0f * zz - 3.0f * xx - 3.0f * yy))       ) +
                        ( sh_weights[13] * (x * (4.0f * zz - xx - yy)) ) +
                        ( sh_weights[14] * (z * (xx - yy))             ) +
                        ( sh_weights[15] * (x * (xx - 3.0f * yy))      );
                }
            }
        }
        // clang-format on

        color = clamp01(color + GfVec3f(0.5));

        return color;
    }

    // lifted from Imath:
    // https://github.com/AcademySoftwareFoundation/Imath/blob/4de9a1dabdf517a7df9bc350b7395bc8db2f681d/src/Imath/ImathQuat.h#L856C1-L856C20
    static GfMatrix3f toMatrix3f(const GfQuatf& quat) {
        float x = quat.GetImaginary()[0];
        float y = quat.GetImaginary()[1];
        float z = quat.GetImaginary()[2];
        float r = quat.GetReal();

        return GfMatrix3f(
            1 - 2 * (y * y + z * z),
            2 * (x * y + z * r),
            2 * (z * x - y * r),

            2 * (x * y - z * r),
            1 - 2 * (z * z + x * x),
            2 * (y * z + x * r),

            2 * (z * x + y * r),
            2 * (y * z - x * r),
            1 - 2 * (y * y + x * x));
    }

    void setCov3D(GfVec3f scale, GfQuatf quat) {
        GfMatrix3f rot = toMatrix3f(quat);
        GfMatrix3f scaleMtx = GfMatrix3f(
            scale[0] * scale[0], 0.0, 0.0,
            0.0, scale[1] * scale[1], 0.0,
            0.0, 0.0, scale[2] * scale[2]);

        // create the covariance matrix by rotating the splat back to its local
        // space, scaling along local x/y/z axes and then rotating back.
        cov3D = rot.GetTranspose() * scaleMtx * rot;
    }

    GfVec3f position;
    GfMatrix3f cov3D;
    float opacity;
    VtVec3fArray sh_weights;
};

class GaussianSplatsRenderer::Impl {
  public:
    Impl()  = default;
    ~Impl() = default;

    void setWorldToViewMatrix(const GfMatrix4f& m) {
        _worldToViewMtx     = m;
        _needsIndicesSorted = true;
    }
    void setProjMatrix(const GfMatrix4f& m) { _projMatrix = m; }
    void addGaussianSplats(const std::string& splatName,
                           GaussianSplats::Ptr newSplats);
    void removeGaussianSplats(const std::string& splatName);
    bool renderGaussianSplatScene(
        HdParticleFieldRenderBuffer* colorRenderBuffer,
        HdParticleFieldRenderBuffer* depthRenderBuffer,
        HdParticleFieldRenderBuffer* primIDRenderBuffer) const;

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

    GfMatrix4f _worldToViewMtx;
    GfMatrix4f _projMatrix;
};

void GaussianSplatsRenderer::Impl::addGaussianSplats(
    const std::string& splatName, GaussianSplats::Ptr newSplats) {
    if (newSplats->positions.empty()) {
        return;
    }

    SplatVector splatVec;
    size_t numNewSplats = newSplats->positions.size();

    bool hasValidSphericalHarmonics = false;
    size_t sphericalHarmonics_stride = 0;
    if (!newSplats->sphericalHarmonics.empty()) {
        sphericalHarmonics_stride =
            (newSplats->sphericalHarmonicsDegree + 1) *
            (newSplats->sphericalHarmonicsDegree + 1);
        if (sphericalHarmonics_stride ==
            (newSplats->sphericalHarmonics.size() / numNewSplats)) {
            hasValidSphericalHarmonics = true;
        }
    }

    bool hasValidOpacity  = newSplats->opacities.size() == numNewSplats;
    bool hasValidScale    = newSplats->scales.size() == numNewSplats;
    bool hasValidRotation = newSplats->rotations.size() == numNewSplats;

    splatVec.resize(numNewSplats);

    for (unsigned int i = 0, n = numNewSplats; i < n; ++i) {
        splatVec[i].position =
            newSplats->xform.Transform(newSplats->positions[i]);

        if (hasValidOpacity) {
            splatVec[i].opacity = newSplats->opacities[i];
        } else {
            splatVec[i].opacity = 1.0f;
        }

        if (hasValidScale && hasValidRotation) {
            splatVec[i].setCov3D(newSplats->scales[i],
                newSplats->rotations[i]);
        } else if (hasValidRotation && !hasValidScale) {
            splatVec[i].setCov3D(GfVec3f(1.0, 1.0, 1.0),
                newSplats->rotations[i]);
        } else if (!hasValidRotation && hasValidScale) {
            splatVec[i].setCov3D(newSplats->scales[i], GfQuatf(1.0));
        } else {
            splatVec[i].setCov3D(GfVec3f(1.0, 1.0, 1.0), GfQuatf(1.0));
        }

        // extract the scale/rotation component from the transform matrix
        // and use it to modify the cov3D matrix to account
        // for the transformation
        GfMatrix3f xform_SR = GfMatrix3f(
            newSplats->xform[0][0], newSplats->xform[0][1],
                newSplats->xform[0][2],
            newSplats->xform[1][0], newSplats->xform[1][1],
                newSplats->xform[1][2],
            newSplats->xform[2][0], newSplats->xform[2][1],
                newSplats->xform[2][2]);
        splatVec[i].cov3D =
            xform_SR * splatVec[i].cov3D * xform_SR.GetTranspose();

        // TODO - I think we need to figure out how to account for the xform in
        // the SH data too.
        if (hasValidSphericalHarmonics) {
            // unpack this splats list of SH weights
            auto sh_it = newSplats->sphericalHarmonics.begin() +
                         sphericalHarmonics_stride * i;

            VtVec3fArray &sh_weights = splatVec[i].sh_weights;
            sh_weights.resize(sphericalHarmonics_stride);
            for (unsigned int j = 0; j < sphericalHarmonics_stride; j += 1) {
                sh_weights[j] = (sh_it[j]);
            }

            // we pre-weight the SH weights by their respective SH coefficients
            // once on load.
            size_t sh_size = sh_weights.size();

            if (sh_size > 0) {
                sh_weights[0] *= SH_C0;
            }

            if (sh_size > 1) {
                sh_weights[1] *= SH_C1;
                sh_weights[2] *= SH_C1;
                sh_weights[3] *= SH_C1;
            }

            if (sh_size > 4) {
                sh_weights[4] *= SH_C2_0;
                sh_weights[5] *= SH_C2_1;
                sh_weights[6] *= SH_C2_2;
                sh_weights[7] *= SH_C2_3;
                sh_weights[8] *= SH_C2_4;
            }

            if (sh_size > 9) {
                sh_weights[9] *= SH_C3_0;
                sh_weights[10] *= SH_C3_1;
                sh_weights[11] *= SH_C3_2;
                sh_weights[12] *= SH_C3_3;
                sh_weights[13] *= SH_C3_4;
                sh_weights[14] *= SH_C3_5;
                sh_weights[15] *= SH_C3_6;
            }
        } else {
            // Add DC weights for (0.5, 0.5, 0.5). Note these are supposed
            // to be premultiplied by SH_C0, which makes things easier.
            splatVec[i].sh_weights.push_back(GfVec3f(0));
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

    std::vector<Index> oldIndices = _sortedIndices;

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
    HdParticleFieldRenderBuffer* colorRenderBuffer,
    HdParticleFieldRenderBuffer* depthRenderBuffer,
    HdParticleFieldRenderBuffer* primIDRenderBuffer) const
{
    if (_needsIndicesSorted) {
        updateSortedIndices();
    }

    if (!colorRenderBuffer)
        return false;

    constexpr GfVec2f oneVec(1.0, 1.0);


    const int w = colorRenderBuffer->GetWidth();
    const int h = colorRenderBuffer->GetHeight();
    const GfVec2f wh(w, h);

    const float half_w  = w / 2.0f;
    const float half_h = h / 2.0f;
    const GfVec2f half_wh(half_w, half_h);

    const GfRect2i imageROI = GfRect2i({0,0}, w, h);

    // calculate the camera location in order to later determine the view
    // direction to the splat.
    GfMatrix4f viewToWorldMtx = _worldToViewMtx.GetInverse();
    GfVec3f cam_loc(viewToWorldMtx[3][0], viewToWorldMtx[3][1],
                viewToWorldMtx[3][2]);

    float aspect    = (float)h / (float)w;
    float htan_fovy = 1 / _projMatrix[1][1];
    float htan_fovx = htan_fovy * aspect;
    float focal     = (float)h / (2.0 * htan_fovy);

    float limx      = htan_fovx;
    float limy      = htan_fovy;

    // rotational and scale portion of the world to view matrix
    GfMatrix3f W(
           _worldToViewMtx[0][0], _worldToViewMtx[1][0], _worldToViewMtx[2][0],
           _worldToViewMtx[0][1], _worldToViewMtx[1][1], _worldToViewMtx[2][1],
           _worldToViewMtx[0][2], _worldToViewMtx[1][2], _worldToViewMtx[2][2]);

    // loop over the sorted splat indices and render the splats from back to
    // front - overing the splats as we go.
    for (const Index &splat_index: _sortedIndices) {
        const Splat &splat = _splats[splat_index.first][splat_index.second];

        // skip any splats that are completely transparent
        if (splat.opacity <= 0.0) {
            continue;
        }

        GfVec4f cameraPos =
            GfVec4f(splat.position[0], splat.position[1], splat.position[2], 1)
            * _worldToViewMtx;
        GfVec4f ndcPos    = cameraPos * _projMatrix;
        if (ndcPos[3] < 0) {
            continue;
        }

        GfVec2f pos_ndc_2d =
            GfVec2f(ndcPos[0] / ndcPos[3], ndcPos[1] / ndcPos[3]);

        float txtz     = cameraPos[0] / cameraPos[2];
        float tytz     = cameraPos[1] / cameraPos[2];

        float tx       = std::min(limx, std::max(-limx, txtz)) * cameraPos[2];
        float ty       = std::min(limy, std::max(-limy, tytz)) * cameraPos[2];
        float tz       = cameraPos[2];

        GfMatrix3f J(
                focal / tz, 0.0, -(focal * tx) / (tz * tz),
                0.0, focal / tz, -(focal * ty) / (tz * tz),
                0.0, 0.0, 0.0);
        GfMatrix3f T     = J * W;
        GfMatrix3f cov   = T * splat.cov3D * T.GetTranspose();

        GfMatrix2f cov2d =
            GfMatrix2f(cov[0][0], cov[0][1], cov[1][0], cov[1][1]);

        double det   = cov2d.GetDeterminant();
        if (det == 0.0)
            continue;

        GfVec2f bboxsize_cam(3.0f * std::sqrt(cov2d[0][0]),
                             3.0f * std::sqrt(cov2d[1][1]));

        GfVec2f bboxsize_ndc = GfCompDiv(bboxsize_cam, wh) * 2.0f;

        GfVec2f bbox_ndc_min    = -bboxsize_ndc + pos_ndc_2d;
        GfVec2f bbox_ndc_max    =  bboxsize_ndc + pos_ndc_2d;

        GfVec2f bbox_screen_min = GfCompMult(bbox_ndc_min + oneVec, half_wh);
        GfVec2f bbox_screen_max = GfCompMult(bbox_ndc_max + oneVec, half_wh);

        int x1 = static_cast<int>(floor(bbox_screen_min[0]));
        int y1 = static_cast<int>(floor(bbox_screen_min[1]));

        int x2 = static_cast<int>(ceil(bbox_screen_max[0]));
        int y2 = static_cast<int>(ceil(bbox_screen_max[1]));

        if (x1 > w || x2 < 0 || y1 > h || y2 < 0) {
            // if the entire splat bound is outside the image then we can skip
            // the whole splat
            continue;
        }

        GfVec2i splatPixelSize = GfVec2i(x2 - x1, y2 - y1);

        // calculate the conic for the gaussian falloff.
        double det_inv = 1.0 / det;
        GfVec3f conic = GfVec3f(cov2d[1][1] * det_inv, -cov2d[0][1] * det_inv,
                        cov2d[0][0] * det_inv);

        GfVec3f camera_dir = splat.position - cam_loc;
        GfNormalize(&camera_dir);

        GfVec3f splat_color = splat.getColor(camera_dir);
        float splat_depth = ndcPos[2];
        int splat_primID = _splatPrimIDs[splat_index.first];

        // step in camera space of the splat for each pixel.
        GfVec2f bbox_cam_step_per_pixel =
            GfCompDiv(bboxsize_cam * 2.0f, splatPixelSize);

        // calculate an OIIO ROI for the region covered by the splat.
        GfRect2i splatROI = GfRect2i({x1, y1}, {x2, y2});

        // clip the splatROI to the image
        GfRect2i clippedSplatROI = splatROI.GetIntersection(imageROI);

        for (int y = clippedSplatROI.GetMin()[1];
             y < clippedSplatROI.GetMax()[1]; ++y)
        {
            for (int x = clippedSplatROI.GetMin()[0];
                 x < clippedSplatROI.GetMax()[0]; ++x)
            {
                GfVec2i imagePixelIndex(x, y);
                GfVec2i splatPixelIndex = imagePixelIndex - splatROI.GetMin();

                // calculate the corresponding point in camera space.
                GfVec2f pixel_cam = GfVec2f(
                    -bboxsize_cam[0] +
                        splatPixelIndex[0] * bbox_cam_step_per_pixel[0],
                    -bboxsize_cam[1] +
                        splatPixelIndex[1] * bbox_cam_step_per_pixel[1]);

                // calculate the gaussian falloff in the space of the splat.
                // (note we defer the outer exp() call to after the early exit)
                double power = -(conic[0] * pow(pixel_cam[0], 2) +
                                 conic[2] * pow(pixel_cam[1], 2)) / 2.0 -
                    (conic[1] * pixel_cam[0] * pixel_cam[1]);
                if (power > 0)
                    continue;

                float alpha = splat.opacity * exp(power);
                if (alpha <= 0)
                    continue;
                alpha = std::min(0.99f, alpha);

                if (alpha > 0.1) {
                    if (primIDRenderBuffer) {
                        primIDRenderBuffer->Write(
                            imagePixelIndex, 1, &splat_primID);
                    }
                    if (depthRenderBuffer) {
                        depthRenderBuffer->Write(
                            imagePixelIndex, 1, &splat_depth);
                    }
                }

                if (colorRenderBuffer) {
                    colorRenderBuffer->OverColor(
                        imagePixelIndex, splat_color, alpha);
                }
            }
        }
    }

    return true;
}

GaussianSplatsRenderer::GaussianSplatsRenderer()
    : pImpl(std::make_unique<Impl>()) {}

GaussianSplatsRenderer::~GaussianSplatsRenderer() = default;

void GaussianSplatsRenderer::setWorldToViewMatrix(const GfMatrix4f& m) {
    pImpl->setWorldToViewMatrix(m);
}

void GaussianSplatsRenderer::setProjMatrix(const GfMatrix4f& m) {
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
    HdParticleFieldRenderBuffer* colorRenderBuffer,
    HdParticleFieldRenderBuffer* depthRenderBuffer,
    HdParticleFieldRenderBuffer* primIDRenderBuffer) const {
    return pImpl->renderGaussianSplatScene(
        colorRenderBuffer, depthRenderBuffer, primIDRenderBuffer);
}

PXR_NAMESPACE_CLOSE_SCOPE
