//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HIO_FIELD_TEXTURE_DATA_H
#define PXR_IMAGING_HIO_FIELD_TEXTURE_DATA_H

/// \file hio/fieldTextureData.h

#include "pxr/pxr.h"
#include "pxr/imaging/hio/api.h"
#include "pxr/imaging/hio/image.h"

#include "pxr/base/gf/bbox3d.h"

#include "pxr/base/tf/type.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

using HioFieldTextureDataSharedPtr = std::shared_ptr<class HioFieldTextureData>;

/// \class HioFieldTextureData
///
/// An interface class for reading volume files having a
/// transformation.
///
class HioFieldTextureData
{
public:
    HIO_API
    HioFieldTextureData();

    HIO_API
    virtual ~HioFieldTextureData();

    /// Load Volume Field Data
    ///
    /// fieldName corresponds to the gridName in an OpenVDB file or
    /// to the layer/attribute name in a Field3D file.
    /// fieldIndex corresponds to the partition index
    /// fieldPurpose corresponds to the partition name/grouping
    ///
    /// Returns nullptr and posts an error if the specified data cannot be
    /// loaded.
    ///
    HIO_API
    static HioFieldTextureDataSharedPtr New(
        std::string const & filePath,
        std::string const & fieldName,
        int fieldIndex,
        std::string const & fieldPurpose,
        size_t targetMemory);

    /// Bounding box describing how 3d texture maps into
    /// world space.
    ///
    virtual const GfBBox3d &GetBoundingBox() const = 0;

    virtual int ResizedWidth() const = 0;

    virtual int ResizedHeight() const = 0;

    virtual int ResizedDepth() const = 0;

    virtual HioFormat GetFormat() const = 0;

    virtual bool Read() = 0;
    
    virtual bool HasRawBuffer() const = 0;

    virtual unsigned char const * GetRawBuffer() const = 0;   

private:
    // Disallow copies
    HioFieldTextureData(const HioFieldTextureData&) = delete;
    HioFieldTextureData& operator=(const HioFieldTextureData&) = delete;

};

/// \class HioFieldTextureDataFactoryBase
///
/// A base class to make HioFieldTextureData objects, implemented by plugins.
///
class HIO_API HioFieldTextureDataFactoryBase : public TfType::FactoryBase
{
protected:
    friend class HioFieldTextureData;

    virtual HioFieldTextureDataSharedPtr _New(
        std::string const & filePath,
        std::string const & fieldName,
        int fieldIndex,
        std::string const & fieldPurpose,
        size_t targetMemory) const = 0;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
