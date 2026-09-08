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

class HdStExtGpuBuffer;

/// \class HdStExtGpuBufferSource
///
/// An HdBufferSource subclass that wraps an externally-owned GPU buffer
/// (described by an HdStExtGpuBufferDesc).
///
/// Unlike HdVtBufferSource, this source does not hold CPU data.
/// \c GetData() returns nullptr; downstream \c CopyData() implementations
/// must detect this source type via a \c dynamic_cast to
/// HdStExtGpuBufferSource and issue a GPU-to-GPU copy instead of a
/// CPU-to-GPU upload.
///
/// Owns the non-owning HgiBuffer wrapper (HdStExtGpuBuffer) that
/// allows CopyData to reference the external raw GPU handle via Hgi blit ops.
///
class HdStExtGpuBufferSource final : public HdBufferSource
{
public:
    HDST_API
    HdStExtGpuBufferSource(
        TfToken const &name,
        HdStExtGpuBufferDesc const &hdDesc);

    HDST_API
    ~HdStExtGpuBufferSource() override;

    // --- HdBufferSource overrides ---

    HDST_API TfToken const &GetName() const override;
    HDST_API void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    HDST_API bool Resolve() override;
    HDST_API size_t ComputeHash() const override;

    HDST_API void const *GetData() const override;
    HDST_API HdTupleType GetTupleType() const override;
    HDST_API size_t GetNumElements() const override;

    // --- External-buffer-specific accessors ---

    HdStExtGpuBufferDesc const &GetDescriptor() const {
        return _descriptor;
    }

protected:
    bool _CheckValid() const override;

private:
    TfToken _name;
    HdStExtGpuBufferDesc _descriptor;
    std::unique_ptr<HdStExtGpuBuffer> _ownedExternalGpuBuffer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXTERNAL_GPU_BUFFER_SOURCE_H
