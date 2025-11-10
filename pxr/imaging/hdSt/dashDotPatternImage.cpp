//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hio/types.h"

#include "pxr/imaging/hdSt/dashDotLines.h"

#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"

// use gf types to read and write metadata
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdStDashDotLines
///
/// The image class for dash-dot patterns.
/// This class generates a dash-dot pattern image based on the provided
/// DashDotPattern data. The created image will not be saved to disk, 
/// but can be used for rendering purposes.
/// The filename for this type of image has the extension ".dashdot".
/// But it is not a real file on disk; it will be generated in memory.
/// 
class HioDashDotPattern_Image : public HioImage
{
public:
    using Base = HioImage;

    HioDashDotPattern_Image();

    ~HioDashDotPattern_Image() override;

    // HioImage overrides
    std::string const & GetFilename() const override;
    int GetWidth() const override;
    int GetHeight() const override;
    HioFormat GetFormat() const override;
    int GetBytesPerPixel() const override;
    int GetNumMipLevels() const override;

    bool IsColorSpaceSRGB() const override;

    bool GetMetadata(TfToken const & key,
                     VtValue * value) const override;
    bool GetSamplerMetadata(HioAddressDimension pname,
                            HioAddressMode * param) const override;

    bool Read(StorageSpec const & storage) override;
    bool ReadCropped(int const cropTop,
                     int const cropBottom,
                     int const cropLeft,
                     int const cropRight,
                     StorageSpec const & storage) override;

    bool Write(StorageSpec const & storage,
               VtDictionary const & metadata) override;

protected:
    bool _OpenForReading(std::string const & filename, 
                         int subimage,
                         int mip,
                         HioImage::SourceColorSpace sourceColorSpace,
                         bool suppressErrors) override;
    bool _OpenForWriting(std::string const & filename) override;

private:
    std::string _GetFilenameExtension() const;

    std::string _filename;
    int _width = 0;
    int _height = 0;
    DashDotPattern _dashDotPattern;
};

TF_REGISTRY_FUNCTION(TfType)
{
    using Image = HioDashDotPattern_Image;
    TfType t = TfType::Define<Image, TfType::Bases<Image::Base> >();
    t.SetFactory< HioImageFactory<Image> >();
}

HioDashDotPattern_Image::HioDashDotPattern_Image()
{
}

/* virtual */
HioDashDotPattern_Image::~HioDashDotPattern_Image() = default;

/* virtual */
std::string const &
HioDashDotPattern_Image::GetFilename() const
{
    return _filename;
}

/* virtual */
int
HioDashDotPattern_Image::GetWidth() const
{
    return _width;
}

/* virtual */
int
HioDashDotPattern_Image::GetHeight() const
{
    return _height;
}

/* virtual */
HioFormat
HioDashDotPattern_Image::GetFormat() const
{
    return HioFormatFloat32Vec4;
}

/* virtual */
int
HioDashDotPattern_Image::GetBytesPerPixel() const
{
    return 16; // 4 channels of 4 bytes each
}

/* virtual */
bool
HioDashDotPattern_Image::IsColorSpaceSRGB() const
{
    return false;
}

/* virtual */
bool
HioDashDotPattern_Image::GetMetadata(TfToken const & /*key*/, VtValue * /*value*/) const
{
    return false; // No metadata for dash-dot pattern images
}

/* virtual */
bool
HioDashDotPattern_Image::GetSamplerMetadata(HioAddressDimension pname,
                                  HioAddressMode * param) const
{
    switch (pname) {
    case HioAddressDimensionU:
    case HioAddressDimensionV:
    {
        // Dash-dot pattern images are always clamped to edge
        *param = HioAddressModeClampToEdge;
        return true;
    }
    default:
        return false;
    }
}

/* virtual */
int
HioDashDotPattern_Image::GetNumMipLevels() const
{
    // Dash-dot pattern images do not have mipmaps.
    return 1;
}

std::string
HioDashDotPattern_Image::_GetFilenameExtension() const
{
    std::string fileExtension = ArGetResolver().GetExtension(_filename);
    return TfStringToLowerAscii(fileExtension);
}

/* virtual */
bool
HioDashDotPattern_Image::_OpenForReading(std::string const & filename,
                                         int subimage,
                                         int mip,
                                         HioImage::SourceColorSpace sourceColorSpace,
                                         bool suppressErrors)
{
    if(subimage != 0 || mip != 0) {
        return false;
    }
    _filename = filename;
    std::string extension = _GetFilenameExtension();
    // Check if the file extension is ".dashdot".
    if (extension == "dashdot")
    {
        _dashDotPattern = PathTokenToPattern(TfToken(filename));
        if (_dashDotPattern._period <= 0 || _dashDotPattern._pattern.size() == 0)
            return false;
        else if (_dashDotPattern._period > 512)
        {
            TF_WARN("DashDotPattern has a period of %d, which is larger than "
                "the maximum supported size of 512. The pattern may not render correctly.", 
                _dashDotPattern._period);
            return false;
        }
        // The width of the image is twice the period of the dash-dot pattern.
        _width = (int)_dashDotPattern._period * 2;
        _height = 1;
        return true;
    }
    return false;
}

/* virtual */
bool
HioDashDotPattern_Image::Read(StorageSpec const & storage)
{
    return ReadCropped(0, 0, 0, 0, storage);
}

/* virtual */
bool
HioDashDotPattern_Image::ReadCropped(int const cropTop,
                                     int const cropBottom,
                                     int const cropLeft,
                                     int const cropRight,
                                     StorageSpec const & storage)
{
    const int imageSize = _width * 4;
    float* imageData = static_cast<float*>(storage.data);

    // Fill the image data with the dash-dot pattern
    // The pattern is defined as a series of pairs of floats, where each pair
    // represents the length of the previous gap and the length of the current dash.
    // The width of the image is twice the period of the dash-dot pattern. So two
    // pixels represent one unit of the pattern. 
    // For each pixel, there are four channels. The first channel represents if the
    // pixel is part of a dash or gap. If it is at the body of a dash, the value in
    // channel is 0.0f. If it is at the gap, and if it is closer to the end of 
    // previous dash, the value is 2.0f. If it is closer to the start of the next
    // dash, the value is 1.0f. The second and third channels represent the start and
    // end of the dash that this pixel belongs to. If it is a gap, these values are
    // set to the start and end of the closest dash. The last channel is always 0.0f.
    // For example, if the pattern period is 10, and the pattern is [(0, 5), (3, 0)],
    // which means a dash of 5 pixels followed by a gap of 3 pixels, and then a dot.
    // The image will have width of 20, and the data will look like this:
    // All the first 10 pixels will be in the body of the dash, so their value will
    // be (0.0f, 0.0f, 5.0f, 0.0f). The next 6 pixels will be in the gap. The first 3
    // pixels will be closer to the previous dash, so their value will be (2.0f, 0.0f,
    // 5.0f, 0.0f), and the next 3 pixels will be closer to the next dot, so their 
    // value will be (1.0f, 8.0f, 8.0f, 0.0f). The last 4 pixels will be in the next 
    // gap. The first 2 pixels are closer to the dot, so their value will be (2.0f, 
    // 8.0f, 8.0f, 0.0f). The next 2 pixels are closer to the next dash, so their
    // value will be (1.0f, 10.0f, 15.0f, 0.0f).
    size_t patternSize = _dashDotPattern._pattern.size();
    int currentPatternIndex = 0;
    GfVec2f pattern = _dashDotPattern._pattern[currentPatternIndex];
    float currentStart = (int)pattern[0] * 2.0f;
    float currentEnd = (int)pattern[1] * 2.0f;
    currentEnd += currentStart;
    float nextStart, nextEnd;
    float leftOfNextPattern = _width;
    if (currentPatternIndex + 1 < patternSize)
    {
        pattern = _dashDotPattern._pattern[currentPatternIndex + 1];
        nextStart = (int)pattern[0] * 2.0f;
        nextEnd = (int)pattern[1] * 2.0f;
        nextStart += currentEnd;
        nextEnd += nextStart;
        leftOfNextPattern = (currentEnd + nextStart) / 2.0f;
    }
    for (int i = 0; i < _width; ++i) {
        if(i >= leftOfNextPattern)
        {
            currentStart = nextStart;
            currentEnd = nextEnd;
            currentPatternIndex++;
            if (currentPatternIndex + 1 < patternSize)
            {
                pattern = _dashDotPattern._pattern[currentPatternIndex + 1];
                nextStart = (int)pattern[0] * 2.0f;
                nextEnd = (int)pattern[1] * 2.0f;
                nextStart += currentEnd;
                nextEnd += nextStart;
                leftOfNextPattern = (currentEnd + nextStart) / 2.0f;
            }
            else if (currentPatternIndex == patternSize)
            {
                leftOfNextPattern = (currentEnd + _width) / 2.0f;
                nextStart = nextEnd = _width;
            }
        }
        if (i < currentStart)
        {
            imageData[i * 4] = 1.0f;
        }
        else if (i < currentEnd)
        {
            imageData[i * 4] = 0.0f;
        }
        else
        {
            imageData[i * 4] = 2.0f;
        }
        imageData[i * 4 + 1] = currentStart / 2;
        imageData[i * 4 + 2] = currentEnd / 2;
        imageData[i * 4 + 3] = 0.0f;
    }

    return true;
}

/* virtual */
bool
HioDashDotPattern_Image::_OpenForWriting(std::string const & filename)
{
    // This is not supported.
    return false;
}

bool
HioDashDotPattern_Image::Write(StorageSpec const & storage,
                               VtDictionary const & metadata)
{
    // This is not supported.
    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE