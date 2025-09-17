//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HIO_OPENVDB_TEXTURE_DATA_H
#define PXR_IMAGING_HIO_OPENVDB_TEXTURE_DATA_H

/// \file hioOpenVDB/vdbTextureData.h

#include "pxr/pxr.h"
#include "pxr/imaging/hio/api.h"
#include "pxr/imaging/hio/fieldTextureData.h"

#include "pxr/base/gf/bbox3d.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HioOpenVDB_TextureData_DenseGridHolderBase;

/// \class HioOpenVDB_TextureData
///
/// Implements HioFieldTextureData to read grid with given name from
/// OpenVDB file at given path.
///
class HioOpenVDB_TextureData final : public HioFieldTextureData
{
public:
    using Base = HioFieldTextureData;

    HioOpenVDB_TextureData(std::string const & filePath,
                           std::string const & gridName,
                           size_t targetMemory);
    ~HioOpenVDB_TextureData() override;

    const GfBBox3d &GetBoundingBox() const override;

    int ResizedWidth() const override;

    int ResizedHeight() const override;

    int ResizedDepth() const override;

    HioFormat GetFormat() const override;
    
    bool Read() override;
    
    bool HasRawBuffer() const override;

    unsigned char const * GetRawBuffer() const override;

private:
    const std::string _filePath;
    const std::string _gridName;

    const size_t _targetMemory;

    int _resizedWidth, _resizedHeight, _resizedDepth;

    HioFormat _format;

    GfBBox3d _boundingBox;

    std::unique_ptr<HioOpenVDB_TextureData_DenseGridHolderBase> _denseGrid;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
