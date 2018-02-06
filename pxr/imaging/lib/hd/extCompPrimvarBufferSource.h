//
// Copyright 2017 Pixar
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
#ifndef HD_EXT_COMP_PRIMVAR_BUFFER_SOURCE_H
#define HD_EXT_COMP_PRIMVAR_BUFFER_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdExtCompCpuComputation;

typedef boost::shared_ptr<HdExtCompCpuComputation>
                                               HdExtCompCpuComputationSharedPtr;

/// Hd Buffer Source that binds a PrimVar to a Ext Computation output.
/// This buffer source is compatible with being bound to a Bar.
class HdExtCompPrimvarBufferSource final : public HdBufferSource {
public:

    /// Constructs a new primVar buffer source called primvarName and
    /// binds it to the output called sourceOutputName from the
    /// computation identified by source.
    ///
    /// Default value provides type information for the primVar and may
    /// be used in the event of an error.
    HD_API
    HdExtCompPrimvarBufferSource(const TfToken &primvarName,
                                 const HdExtCompCpuComputationSharedPtr &source,
                                 const TfToken &sourceOutputName,
                                 const VtValue &defaultValue);

    HD_API
    virtual ~HdExtCompPrimvarBufferSource() = default;

    /// Returns the name of the primVar.
    HD_API
    virtual TfToken const &GetName() const override;

    /// Adds this Primvar's buffer description to the buffer spec vector.
    HD_API
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const override;

    /// Extracts the primVar from the source computation.
    HD_API
    virtual bool Resolve() override;

    /// Returns a raw pointer to the primVar data.
    HD_API
    virtual void const *GetData() const override;

    /// Returns the tuple data format of the primVar data.
    HD_API
    virtual HdTupleType GetTupleType() const override;

    /// Returns a count of the number of elements.
    HD_API
    virtual int GetNumElements() const override;

protected:
    /// Returns true if the binding to the source computation was successful.
    HD_API
    virtual bool _CheckValid() const override;

private:
    TfToken                          _primvarName;
    HdExtCompCpuComputationSharedPtr _source;
    size_t                           _sourceOutputIdx;
    HdTupleType                      _tupleType;
    void const                      *_rawDataPtr;

    HdExtCompPrimvarBufferSource()                                     = delete;
    HdExtCompPrimvarBufferSource(const HdExtCompPrimvarBufferSource &)
                                                                       = delete;
    HdExtCompPrimvarBufferSource &operator = (
                                           const HdExtCompPrimvarBufferSource &)
                                                                       = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_EXT_COMP_PRIMVAR_BUFFER_SOURCE_H
