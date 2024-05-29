//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_COMP_PRIMVAR_BUFFER_SOURCE_H
#define PXR_IMAGING_HD_ST_EXT_COMP_PRIMVAR_BUFFER_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/base/tf/token.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


using HdStExtCompCpuComputationSharedPtr = 
    std::shared_ptr<class HdStExtCompCpuComputation>;

/// Hd Buffer Source that binds a primvar to a Ext Computation output.
/// This buffer source is compatible with being bound to a Bar.
class HdStExtCompPrimvarBufferSource final : public HdBufferSource
{
public:

    /// Constructs a new primvar buffer source called primvarName and
    /// binds it to the output called sourceOutputName from the
    /// computation identified by source.
    ///
    /// Default value provides type information for the primvar and may
    /// be used in the event of an error.
    HDST_API
    HdStExtCompPrimvarBufferSource(
        const TfToken &primvarName,
        const HdStExtCompCpuComputationSharedPtr &source,
        const TfToken &sourceOutputName,
        const HdTupleType &valueType);

    HDST_API
    ~HdStExtCompPrimvarBufferSource() override;

    /// Returns the name of the primvar.
    HDST_API
    TfToken const &GetName() const override;

    /// Adds this Primvar's buffer description to the buffer spec vector.
    HDST_API
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;

    /// Computes and returns a hash value for the underlying data.
    HDST_API
    size_t ComputeHash() const override;

    /// Extracts the primvar from the source computation.
    HDST_API
    bool Resolve() override;

    /// Returns a raw pointer to the primvar data.
    HDST_API
    void const *GetData() const override;

    /// Returns the tuple data format of the primvar data.
    HDST_API
    HdTupleType GetTupleType() const override;

    /// Returns a count of the number of elements.
    HDST_API
    size_t GetNumElements() const override;

protected:
    /// Returns true if the binding to the source computation was successful.
    HDST_API
    bool _CheckValid() const override;

private:
    // TfHash support.
    template <class HashState>
    friend void TfHashAppend(HashState &h,
                             HdStExtCompPrimvarBufferSource const &);

    TfToken                            _primvarName;
    HdStExtCompCpuComputationSharedPtr _source;
    size_t                             _sourceOutputIdx;
    HdTupleType                        _tupleType;
    void const                        *_rawDataPtr;

    HdStExtCompPrimvarBufferSource() = delete;
    HdStExtCompPrimvarBufferSource(
        const HdStExtCompPrimvarBufferSource &) = delete;
    HdStExtCompPrimvarBufferSource &operator = (
        const HdStExtCompPrimvarBufferSource &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_COMP_PRIMVAR_BUFFER_SOURCE_H
