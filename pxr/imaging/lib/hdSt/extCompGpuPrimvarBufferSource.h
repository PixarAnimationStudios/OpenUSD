//
// Copyright 2018 Pixar
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
#ifndef HDST_EXT_COMP_GPU_PRIMVAR_BUFFER_SOURCE_H
#define HDST_EXT_COMP_GPU_PRIMVAR_BUFFER_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef boost::shared_ptr<class HdStExtCompGpuPrimvarBufferSource>
    HdStExtCompGpuPrimvarBufferSourceSharedPtr;

/// \class HdStExtCompGpuPrimvarBufferSource
/// A buffer source mapped to an output of an ExtComp CPU computation.
///
class HdStExtCompGpuPrimvarBufferSource final : public HdNullBufferSource {
public:
    HdStExtCompGpuPrimvarBufferSource(TfToken const & name,
                                      VtValue const & value,
                                      int numElements);

    HDST_API
    virtual ~HdStExtCompGpuPrimvarBufferSource() = default;

    HDST_API
    virtual bool Resolve() override;

    HD_API
    virtual TfToken const &GetName() const override;

    HDST_API
    virtual int GetNumElements() const override;

    HD_API
    virtual HdTupleType GetTupleType() const override;

    HDST_API
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const override;

protected:
    virtual bool _CheckValid() const override;
    
private:
    TfToken _name;
    HdTupleType _tupleType;
    int _numElements;

    HdStExtCompGpuPrimvarBufferSource()                = delete;
    HdStExtCompGpuPrimvarBufferSource(
            const HdStExtCompGpuPrimvarBufferSource &) = delete;
    HdStExtCompGpuPrimvarBufferSource &operator = (
            const HdStExtCompGpuPrimvarBufferSource &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_EXT_COMP_GPU_PRIMVAR_BUFFER_SOURCE_H
