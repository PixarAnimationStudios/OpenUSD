//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXTERNAL_GPU_BUFFER_SOURCE_H
#define PXR_IMAGING_HD_ST_EXTERNAL_GPU_BUFFER_SOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/extBufferDesc.h"
#include "pxr/imaging/hd/bufferSource.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStExtGpuBufferSource
///
/// An HdBufferSource for a stream that already lives in a GPU buffer an
/// application shared (see HdExtGpuBufferSchema).
///
/// Unlike HdVtBufferSource it holds no CPU data, so GetData() returns null and
/// IsGpuBacked() returns true. Consumers ask the latter and then take the
/// GPU-to-GPU path, reading the resource through GetDescriptor(); nothing needs
/// to probe the concrete type with RTTI to find out what kind of source this
/// is.
///
class HdStExtGpuBufferSource final : public HdBufferSource
{
public:
    HDST_API
    HdStExtGpuBufferSource(
        TfToken const &name,
        HdStExtGpuBufferDesc const &desc);

    HDST_API
    ~HdStExtGpuBufferSource() override;

    // --- HdBufferSource overrides ---

    HDST_API TfToken const &GetName() const override;
    HDST_API void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    HDST_API bool Resolve() override;
    HDST_API size_t ComputeHash() const override;

    HDST_API void const *GetData() const override;
    HDST_API bool IsGpuBacked() const override;
    HDST_API HdTupleType GetTupleType() const override;
    HDST_API size_t GetNumElements() const override;

    // --- External-buffer-specific accessors ---

    /// The layout and the buffer this stream lives in. A consumer that has
    /// established IsGpuBacked() reads the GPU resource through here.
    HdStExtGpuBufferDesc const &GetDescriptor() const {
        return _descriptor;
    }

protected:
    bool _CheckValid() const override;

private:
    TfToken _name;
    HdStExtGpuBufferDesc _descriptor;
};

using HdStExtGpuBufferSourceSharedPtr =
    std::shared_ptr<HdStExtGpuBufferSource>;

/// If \p source is a GPU-backed Storm source, the source; otherwise null.
///
/// The one place the cast happens, guarded by the virtual predicate rather
/// than by RTTI. Generic Storm code -- the memory managers, the alias-BAR
/// builder -- goes through this instead of testing types itself.
HDST_API
HdStExtGpuBufferSource const *
HdSt_GetExtGpuBufferSource(HdBufferSourceSharedPtr const &source);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXTERNAL_GPU_BUFFER_SOURCE_H
