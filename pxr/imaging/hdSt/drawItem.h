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
#ifndef PXR_IMAGING_HD_ST_DRAW_ITEM_H
#define PXR_IMAGING_HD_ST_DRAW_ITEM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/drawItem.h"

PXR_NAMESPACE_OPEN_SCOPE


using HdSt_GeometricShaderSharedPtr =
        std::shared_ptr<class HdSt_GeometricShader>;
using HdSt_MaterialNetworkShaderSharedPtr =
        std::shared_ptr<class HdSt_MaterialNetworkShader>;

class HdStDrawItem : public HdDrawItem
{
public:
    HF_MALLOC_TAG_NEW("new HdStDrawItem");

    HDST_API
    HdStDrawItem(HdRprimSharedData const *sharedData);

    HDST_API
    ~HdStDrawItem() override;

    /// Returns true if the drawItem has an instancer.
    bool HasInstancer() const {
        TF_VERIFY(_GetSharedData()->instancerLevels != -1);
        return (_GetSharedData()->instancerLevels > 0);
    }

    /// Returns the number of nested levels of instance primvars.
    int GetInstancePrimvarNumLevels() const {
        TF_VERIFY(_GetSharedData()->instancerLevels != -1);
        return _GetSharedData()->instancerLevels;
    }

    /// Returns a BufferArrayRange of instance primvars at \p level
    /// the level is assigned to nested instancers in a bottom-up manner.
    ///
    /// example: (numLevels = 2)
    ///
    ///     instancerA         (level = 1)
    ///       |
    ///       +-- instancerB   (level = 0)
    ///             |
    ///             +-- mesh_prototype
    ///
    HdBufferArrayRangeSharedPtr const &
    GetInstancePrimvarRange(int level) const {
        return _GetSharedData()->barContainer.Get(
            _GetDrawingCoord().GetInstancePrimvarIndex(level));
    }

    /// Returns instance-index indirection BAR.
    HdBufferArrayRangeSharedPtr const &GetInstanceIndexRange() const {
        return _GetSharedData()->barContainer.Get(
            _GetDrawingCoord().GetInstanceIndexIndex());
    }

    /// Returns constant primvar BAR.
    HdBufferArrayRangeSharedPtr const &GetConstantPrimvarRange() const {
        return _GetSharedData()->barContainer.Get(
            _GetDrawingCoord().GetConstantPrimvarIndex());
    }

    /// Returns element primvar BAR.
    HdBufferArrayRangeSharedPtr const &GetElementPrimvarRange() const {
        return _GetSharedData()->barContainer.Get(
            _GetDrawingCoord().GetElementPrimvarIndex());
    }

    /// Returns vertex primvar BAR.
    HdBufferArrayRangeSharedPtr const &GetVertexPrimvarRange() const {
        return _GetSharedData()->barContainer.Get(
            _GetDrawingCoord().GetVertexPrimvarIndex());
    }

    /// Returns varying primvar BAR.
    HdBufferArrayRangeSharedPtr const &GetVaryingPrimvarRange() const {
        return _GetSharedData()->barContainer.Get(
            _GetDrawingCoord().GetVaryingPrimvarIndex());
    }

    /// Returns face varying primvar BAR.
    HdBufferArrayRangeSharedPtr const &GetFaceVaryingPrimvarRange() const {
        return _GetSharedData()->barContainer.Get(
            _GetDrawingCoord().GetFaceVaryingPrimvarIndex());
    }

    /// Returns topology BAR.
    HdBufferArrayRangeSharedPtr const &GetTopologyRange() const {
        return _GetSharedData()->barContainer.Get(
            _GetDrawingCoord().GetTopologyIndex());
    }

    /// Returns topological visibility BAR (e.g. per-face, per-point, etc.)
    HdBufferArrayRangeSharedPtr const &GetTopologyVisibilityRange() const {
        return _GetSharedData()->barContainer.Get(
            _GetDrawingCoord().GetTopologyVisibilityIndex());
    }

    /// Returns mapping from refined fvar channels to named primvar.
    TopologyToPrimvarVector const &GetFvarTopologyToPrimvarVector() const {
        return _GetSharedData()->fvarTopologyToPrimvarVector;
    }

    void SetGeometricShader(HdSt_GeometricShaderSharedPtr const &shader) {
        _geometricShader = shader;
    }

    HdSt_GeometricShaderSharedPtr const &GetGeometricShader() const {
        return _geometricShader;
    }

    HdSt_MaterialNetworkShaderSharedPtr const &
    GetMaterialNetworkShader() const {
        return _materialNetworkShader;
    }

    void SetMaterialNetworkShader(
                HdSt_MaterialNetworkShaderSharedPtr const &shader) {
        _materialNetworkShader = shader;
    }

    bool GetMaterialIsFinal() const {
        return _materialIsFinal;
    }

    void SetMaterialIsFinal(bool isFinal) {
        _materialIsFinal = isFinal;
    }

    /// Tests intersection with the specified view projection matrix.
    /// Returns true if this drawItem is in the frustum.
    ///
    /// XXX: Currently if this drawitem uses instancing, always returns true.
    HDST_API
    bool IntersectsViewVolume(GfMatrix4d const &viewProjMatrix) const;

    /// Returns the hash of the versions of underlying buffers. When the
    /// hash changes, it means the drawing coord might have been reassigned,
    /// so any drawing coord caching buffer (e.g. indirect dispatch buffer)
    /// has to be rebuilt at the moment.
    /// Note that this value is a hash, not sequential.
    HDST_API
    size_t GetBufferArraysHash() const;

    /// Returns the hash of the element offsets of the underlying BARs.
    /// When the hash changes, it means that any drawing coord caching
    /// buffer (e.g. the indirect dispatch buffer) has to be rebuilt.
    /// Note that this value is a hash, not sequential.
    HDST_API
    size_t GetElementOffsetsHash() const;

private:
    HdSt_GeometricShaderSharedPtr _geometricShader;
    HdSt_MaterialNetworkShaderSharedPtr _materialNetworkShader;
    bool _materialIsFinal;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_DRAW_ITEM_H
