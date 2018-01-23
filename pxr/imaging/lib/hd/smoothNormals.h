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
#ifndef HD_SMOOTH_NORMALS_H
#define HD_SMOOTH_NORMALS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/computation.h"

#include "pxr/base/tf/token.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


typedef std::shared_ptr<class HdResourceRegistry> HdResourceRegistrySharedPtr;

class Hd_VertexAdjacency;

/// \class Hd_SmoothNormalsComputation
///
/// Smooth normal computation CPU.
///
class Hd_SmoothNormalsComputation : public HdComputedBufferSource {
public:
    HD_API
    Hd_SmoothNormalsComputation(Hd_VertexAdjacency const *adjacency,
                                HdBufferSourceSharedPtr const &points,
                                TfToken const &dstName,
                                HdBufferSourceSharedPtr const &adjacencyBuilder,
                                bool packed);

    /// overrides
    HD_API
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const override;
    HD_API
    virtual bool Resolve() override;
    HD_API
    virtual TfToken const &GetName() const;

protected:
    HD_API
    virtual bool _CheckValid() const;

private:
    Hd_VertexAdjacency const *_adjacency;
    HdBufferSourceSharedPtr _points;
    TfToken _dstName;
    HdBufferSourceSharedPtr _adjacencyBuilder;
    bool _packed;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_SMOOTH_NORMALS_H
