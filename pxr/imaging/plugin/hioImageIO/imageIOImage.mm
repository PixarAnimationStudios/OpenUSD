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
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hio/types.h"

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

#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>

#define _DEBUG_PRINT_IMAGE_ATTRS 0

PXR_NAMESPACE_OPEN_SCOPE

class HioImageIO_Image : public HioImage
{
public:
    using Base = HioImage;

    HioImageIO_Image();

    ~HioImageIO_Image() override;

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
    bool _OpenForReading(std::string const & filename, int subimage,
                                 int mip, 
                                 HioImage::SourceColorSpace sourceColorSpace,
                                 bool suppressErrors) override;
    bool _OpenForWriting(std::string const & filename) override;

private:
    enum _ImagePixelFormat {
        _IOUINT8,
        _IOUINT16,
        _IOUINT32,
        _IOHALF,
        _IOFLOAT
    };
    
    std::string _GetFilenameExtension() const;
    bool _OpenImageSource(std::string const& filename);
    VtValue _FindAttribute(std::string const & metadataKey) const;
    void _SetAttribute(std::string const & metadataKey, VtValue const & value,
                       NSMutableDictionary* properties);
    HioFormat _GetHioFormatFromImageData() const;
    size_t _GetNumChannels() const;
    _ImagePixelFormat _GetImagePixelFormat() const;
    std::string _filename;
    int _subimage;
    int _miplevel;
    CGImageSourceRef _imageSourceRef; // source for the TIFF
    CGImageRef _imageRef;             // the subimage selected
    HioImage::SourceColorSpace _sourceColorSpace;
};

TF_REGISTRY_FUNCTION(TfType)
{
    using Image = HioImageIO_Image;
    TfType t = TfType::Define<Image, TfType::Bases<Image::Base> >();
    t.SetFactory< HioImageFactory<Image> >();
}

HioImageIO_Image::_ImagePixelFormat
HioImageIO_Image::_GetImagePixelFormat() const {
    bool isFloatingPoint = CGImageGetBitmapInfo(_imageRef) & kCGBitmapFloatComponents;
    size_t bitsPerComponent = CGImageGetBitsPerComponent(_imageRef);
    if (isFloatingPoint) {
        switch (bitsPerComponent) {
            case 16:
                return _IOHALF;
            case 32:
                return _IOFLOAT;
        }
    } else {
        switch (bitsPerComponent) {
            case 8:
                return _IOUINT8;
            case 16:
                return _IOUINT16;
                break;
            case 32:
                return _IOUINT32;
        }
    }
    
    return _IOUINT8;
}

size_t
HioImageIO_Image::_GetNumChannels() const {
    size_t bitsPerComponent = CGImageGetBitsPerComponent(_imageRef);
    size_t bitsPerPixel = CGImageGetBitsPerPixel(_imageRef);
    return bitsPerPixel / bitsPerComponent;
}

/// Determine the HioFormat
HioFormat
HioImageIO_Image::_GetHioFormatFromImageData() const
{
    _ImagePixelFormat format = _GetImagePixelFormat();
    size_t nchannels = _GetNumChannels();
    bool isSRGB = IsColorSpaceSRGB();
    
    switch (nchannels) {
        case 1:
            switch (format) {
                case _IOUINT8:
                    if (isSRGB) {
                        return HioFormatUNorm8srgb;
                    }
                    return HioFormatUNorm8;
                case _IOUINT16:
                    return HioFormatUInt16;
                case _IOUINT32:
                    return HioFormatUInt32;
                case _IOHALF:
                    return HioFormatFloat16;
                case _IOFLOAT:
                    return HioFormatFloat32;
            }
        case 2:
            switch (format) {
                case _IOUINT8:
                    if (isSRGB) {
                        return HioFormatUNorm8Vec2srgb;
                    }
                    return HioFormatUNorm8Vec2;
                case _IOUINT16:
                    return HioFormatUInt16Vec2;
                case _IOUINT32:
                    return HioFormatUInt32Vec2;
                case _IOHALF:
                    return HioFormatFloat16Vec2;
                case _IOFLOAT:
                    return HioFormatFloat32Vec2;
            }
        case 3:
            switch (format) {
                case _IOUINT8:
                    if (isSRGB) {
                        return HioFormatUNorm8Vec3srgb;
                    }
                    return HioFormatUNorm8Vec3;
                case _IOUINT16:
                    return HioFormatUInt16Vec3;
                case _IOUINT32:
                    return HioFormatUInt32Vec3;
                case _IOHALF:
                    return HioFormatFloat16Vec3;
                case _IOFLOAT:
                    return HioFormatFloat32Vec3;
            }
        case 4:
            switch (format) {
                case _IOUINT8:
                    if (isSRGB) {
                        return HioFormatUNorm8Vec4srgb;
                    }
                    return HioFormatUNorm8Vec4;
                case _IOUINT16:
                    return HioFormatUInt16Vec4;
                case _IOUINT32:
                    return HioFormatUInt32Vec4;
                case _IOHALF:
                    return HioFormatFloat16Vec4;
                case _IOFLOAT:
                    return HioFormatFloat32Vec4;
            }
        default:
            TF_CODING_ERROR("Unsupported type");
            return HioFormatUNorm8Vec3;
    }
}

static bool
_IsHioFormatFloat(HioFormat hioFormat) {
    switch (hioFormat) {
        case HioFormatFloat16:
        case HioFormatFloat16Vec2:
        case HioFormatFloat16Vec3:
        case HioFormatFloat16Vec4:
        case HioFormatFloat32:
        case HioFormatFloat32Vec2:
        case HioFormatFloat32Vec3:
        case HioFormatFloat32Vec4:
            return true;
        default:
            return false;
    }
}

/// Returns the number bytes per component for the corresponding HioFormat
static size_t
_GetComponentByteSizeFromHIOFormat(HioFormat hioFormat)
{
    switch (hioFormat) {
        case HioFormatUNorm8:
        case HioFormatUNorm8Vec2:
        case HioFormatUNorm8Vec3:
        case HioFormatUNorm8Vec4:
        case HioFormatUNorm8srgb:
        case HioFormatUNorm8Vec2srgb:
        case HioFormatUNorm8Vec3srgb:
        case HioFormatUNorm8Vec4srgb:
        case HioFormatSNorm8:
        case HioFormatSNorm8Vec2:
        case HioFormatSNorm8Vec3:
        case HioFormatSNorm8Vec4:
            return 1;
        case HioFormatUInt16:
        case HioFormatUInt16Vec2:
        case HioFormatUInt16Vec3:
        case HioFormatUInt16Vec4:
        case HioFormatInt16:
        case HioFormatInt16Vec2:
        case HioFormatInt16Vec3:
        case HioFormatInt16Vec4:
        case HioFormatFloat16:
        case HioFormatFloat16Vec2:
        case HioFormatFloat16Vec3:
        case HioFormatFloat16Vec4:
            return 2;
        case HioFormatUInt32:
        case HioFormatUInt32Vec2:
        case HioFormatUInt32Vec3:
        case HioFormatUInt32Vec4:
        case HioFormatInt32:
        case HioFormatInt32Vec2:
        case HioFormatInt32Vec3:
        case HioFormatInt32Vec4:
        case HioFormatFloat32:
        case HioFormatFloat32Vec2:
        case HioFormatFloat32Vec3:
        case HioFormatFloat32Vec4:
            return 4;
        default:
            TF_CODING_ERROR("Unsupported type");
            return 4;
    }
}

/// Returns the number channels per pixel for the corresponding HioFormat
static size_t
_GetNumChannelsFromHIOFormat(HioFormat hioFormat)
{
    switch (hioFormat) {
        case HioFormatUNorm8:
        case HioFormatUNorm8srgb:
        case HioFormatSNorm8:
        case HioFormatUInt16:
        case HioFormatInt16:
        case HioFormatFloat16:
        case HioFormatUInt32:
        case HioFormatInt32:
        case HioFormatFloat32:
            return 1;
        case HioFormatUNorm8Vec2:
        case HioFormatUNorm8Vec2srgb:
        case HioFormatSNorm8Vec2:
        case HioFormatUInt16Vec2:
        case HioFormatInt16Vec2:
        case HioFormatFloat16Vec2:
        case HioFormatUInt32Vec2:
        case HioFormatInt32Vec2:
        case HioFormatFloat32Vec2:
            return 2;
        case HioFormatUNorm8Vec3:
        case HioFormatUNorm8Vec3srgb:
        case HioFormatSNorm8Vec3:
        case HioFormatUInt16Vec3:
        case HioFormatInt16Vec3:
        case HioFormatFloat16Vec3:
        case HioFormatUInt32Vec3:
        case HioFormatInt32Vec3:
        case HioFormatFloat32Vec3:
            return 3;
        case HioFormatUNorm8Vec4:
        case HioFormatUNorm8Vec4srgb:
        case HioFormatSNorm8Vec4:
        case HioFormatUInt16Vec4:
        case HioFormatInt16Vec4:
        case HioFormatFloat16Vec4:
        case HioFormatUInt32Vec4:
        case HioFormatInt32Vec4:
        case HioFormatFloat32Vec4:
            return 4;
        default:
            TF_CODING_ERROR("Unsupported type");
            return 4;
    }
}

// For compatability with Ice/Imr we transmogrify some matrix metadata
static NSString*
_TranslateMetadataKey(std::string const & metadataKey)
{
    if (metadataKey == "NP") {
        return @"worldtoscreen";
    } else
    if (metadataKey == "Nl") {
        return @"worldtocamera";
    } else {
        return [NSString stringWithCString:metadataKey.c_str()
                                  encoding:NSUTF8StringEncoding];
    }
}

id _FindValueFromDictionaryForKey(NSDictionary* properties, NSString* key) {

    id value = [properties valueForKey:key];
    if (value) 
        return value;

    // Search entries for dictionaries which may have the key
    for (NSString *_key in properties) {
        value = [properties valueForKey:_key];
        // key may be in nested dictionary
        if ([value isKindOfClass:[NSDictionary class]]) {
            value = _FindValueFromDictionaryForKey(value, key);
            if (value) {
                return value;
            }
        }
    }
    
    return nil;
}

VtValue
HioImageIO_Image::_FindAttribute(std::string const & metadataKey) const
{
    NSString* key = _TranslateMetadataKey(metadataKey);
    
    NSDictionary* sourceProps = (NSDictionary*)CFBridgingRelease(CGImageSourceCopyPropertiesAtIndex(_imageSourceRef, _subimage, NULL));
    id value = _FindValueFromDictionaryForKey(sourceProps, key);
    
    if (!value) {
        return VtValue();
    }
    
    if ([value isKindOfClass:[NSString class]]) {
        NSString* stringValue = (NSString*)value;
        return VtValue(std::string([stringValue UTF8String]));
    }
    
    if ([value isKindOfClass:[NSNumber class]]) {
        CFNumberType numberType = CFNumberGetType((CFNumberRef)value);
        NSNumber* numberValue = (NSNumber*)value;
        switch (numberType) {
            case kCFNumberSInt8Type:
                return VtValue((SInt8)[numberValue charValue]);
            case kCFNumberSInt16Type:
                return VtValue((SInt16)[numberValue shortValue]);
            case kCFNumberSInt32Type:
                return VtValue((SInt32)[numberValue integerValue]);
            case kCFNumberSInt64Type:
                return VtValue((SInt64)[numberValue integerValue]);
            case kCFNumberFloat32Type:
            case kCFNumberFloatType:
            case kCFNumberCGFloatType:
                return VtValue((float)[numberValue floatValue]);
            case kCFNumberFloat64Type:
            case kCFNumberDoubleType:
                return VtValue((double)[numberValue doubleValue]);
            case kCFNumberCharType:
                return VtValue((char)[numberValue charValue]);
            case kCFNumberShortType:
                return VtValue((short)[numberValue shortValue]);
            case kCFNumberIntType:
                return VtValue((int)[numberValue integerValue]);
            case kCFNumberLongType:
                return VtValue((long)[numberValue longValue]);
            case kCFNumberLongLongType:
                return VtValue((long long)[numberValue longLongValue]);
                /* Other */
            case kCFNumberCFIndexType:
                return VtValue((CFIndex)[numberValue integerValue]);
            case kCFNumberNSIntegerType:
                return VtValue((NSInteger)[numberValue integerValue]);
            default:
                return VtValue();
        }
    }

    return VtValue();
}

void
HioImageIO_Image::_SetAttribute(std::string const & metadataKey, VtValue const & value,
                             NSMutableDictionary* properties)
{
    NSString* key = _TranslateMetadataKey(metadataKey);

    if (value.IsHolding<std::string>()) {
        properties[key] = [NSString stringWithCString:value.Get<std::string>().c_str()
                                             encoding:NSUTF8StringEncoding];
    } else
    if (value.IsHolding<char>()) {
        properties[key] = [NSNumber numberWithChar:value.Get<char>()];
    } else
    if (value.IsHolding<unsigned char>()) {
        properties[key] = [NSNumber numberWithUnsignedChar:value.Get<unsigned char>()];
    } else
    if (value.IsHolding<int>()) {
        properties[key] = [NSNumber numberWithInt:value.Get<int>()];
    } else
    if (value.IsHolding<unsigned int>()) {
        properties[key] = [NSNumber numberWithUnsignedInt:value.Get<unsigned int>()];
    } else
    if (value.IsHolding<float>()) {
        properties[key] = [NSNumber numberWithFloat:value.Get<float>()];
    } else
    if (value.IsHolding<double>()) {
        properties[key] = [NSNumber numberWithDouble:value.Get<double>()];
    }
}

HioImageIO_Image::HioImageIO_Image()
    : _subimage(0), _miplevel(0), _imageSourceRef(NULL), _imageRef(NULL)
{
}

/* virtual */
HioImageIO_Image::~HioImageIO_Image() {
    if (_imageRef != NULL)
        CGImageRelease(_imageRef);
    if (_imageSourceRef != NULL)
        CFRelease(_imageSourceRef);
}

/* virtual */
std::string const &
HioImageIO_Image::GetFilename() const
{
    return _filename;
}

/* virtual */
int
HioImageIO_Image::GetWidth() const
{
    if (!_imageRef) {
        return 0;
    }
    return (int)CGImageGetWidth(_imageRef);
}

/* virtual */
int
HioImageIO_Image::GetHeight() const
{
    if (!_imageRef) {
        return 0;
    }
    return (int)CGImageGetHeight(_imageRef);
}

/* virtual */
HioFormat
HioImageIO_Image::GetFormat() const
{
    if (!_imageRef) {
        TF_CODING_ERROR("Unsupported type");
        return HioFormatUNorm8Vec3;
    }
    return _GetHioFormatFromImageData();
}

/* virtual */
int
HioImageIO_Image::GetBytesPerPixel() const
{
    if (!_imageRef) {
        return 0;
    }
    return (int)CGImageGetBitsPerPixel(_imageRef) / 8;
}

/* virtual */
bool
HioImageIO_Image::IsColorSpaceSRGB() const
{
    if (_sourceColorSpace == HioImage::SRGB) {
        return true;
    } 
    if (_sourceColorSpace == HioImage::Raw) {
        return false;
    }

    if (!_imageRef) {
        return false;
    }
    
    size_t numChannels = _GetNumChannels();
    _ImagePixelFormat format = _GetImagePixelFormat();

    // In the spirit of the original hioOiio plugin
    // we follow the same logic however contrived it may be
    // see ../hioOiio/oiioImage.cpp HioOIIO_Image::IsColorSpaceSRGB()
    return (numChannels == 3 || numChannels == 4) && format == _IOUINT8;
}

/* virtual */
bool
HioImageIO_Image::GetMetadata(TfToken const & key, VtValue * value) const
{
    VtValue result = _FindAttribute(key.GetString());
    if (!result.IsEmpty()) {
        *value = result;
        return true;
    }
    
    return false;
}

static HioAddressMode
_TranslateWrap(std::string const & wrapMode)
{
    if (wrapMode == "black")
        return HioAddressModeClampToBorderColor;
    if (wrapMode == "clamp")
        return HioAddressModeClampToEdge;
    if (wrapMode == "periodic")
        return HioAddressModeRepeat;
    if (wrapMode == "mirror")
        return HioAddressModeMirrorRepeat;

    return HioAddressModeClampToEdge;
}

/* virtual */
bool
HioImageIO_Image::GetSamplerMetadata(HioAddressDimension pname,
                                  HioAddressMode * param) const
{
    switch (pname) {
        case HioAddressDimensionU: {
                const VtValue smode = _FindAttribute("s mode");
                if (!smode.IsEmpty() && smode.IsHolding<std::string>()) {
                    *param = _TranslateWrap(smode.Get<std::string>());
                    return true;
                }
            } return false;
        case HioAddressDimensionV: {
                const VtValue tmode = _FindAttribute("t mode");
                if (!tmode.IsEmpty() && tmode.IsHolding<std::string>()) {
                    *param = _TranslateWrap(tmode.Get<std::string>());
                    return true;
                }
            } return false;
        default:
            return false;
    }
}

/* virtual */
int
HioImageIO_Image::GetNumMipLevels() const
{
    // XXX Add support for mip counting
    return 1;
}

std::string 
HioImageIO_Image::_GetFilenameExtension() const
{
    std::string fileExtension = ArGetResolver().GetExtension(_filename);
    return TfStringToLower(fileExtension);
}

bool HioImageIO_Image::_OpenImageSource(std::string const& filename) {
    NSURL* fileURL = [NSURL fileURLWithPath:[NSString stringWithCString:filename.c_str() encoding:NSUTF8StringEncoding]];
    [fileURL retain];
    if (_imageSourceRef) {
        CFRelease(_imageSourceRef);
        _imageSourceRef = NULL;
    }
    
    CFURLRef fileCFURL = (CFURLRef)fileURL;
    if (!fileCFURL) {
        [fileURL release];
        return false;
    }
    
    _imageSourceRef = CGImageSourceCreateWithURL(fileCFURL, NULL);
    CFRelease(fileCFURL);
    if (_imageSourceRef == NULL) {
        return false;
    }
    
    if (_imageRef) {
        CGImageRelease(_imageRef);
        _imageRef = NULL;
    }
    
    _imageRef = CGImageSourceCreateImageAtIndex(_imageSourceRef, _subimage, nil);
    if (_imageRef == NULL) {
        return false;
    }
    
    return true;
}

/* virtual */
bool
HioImageIO_Image::_OpenForReading(std::string const & filename, int subimage,
                               int mip, 
                               HioImage::SourceColorSpace sourceColorSpace,
                               bool suppressErrors)
{
    // This implementation currently only supports TIFF
    // TIFF doesn't explicitly supports mips, but it can contain multiple images
    // Currently in OpenImageIO implementation it emulates mip levels when a Pixar attribute is specified on the TIFF itself
    // See https://github.com/AcademySoftwareFoundation/OpenImageIO/blob/master/src/tiff.imageio/tiffinput.cpp#L79
    // and https://github.com/AcademySoftwareFoundation/OpenImageIO/blob/master/src/libOpenImageIO/exif.cpp#L389
    // So if TIFFTAG_PIXAR_TEXTUREFORMAT, "textureformat" is specified then subimage emulates the mip level.
    // Since this attribute is proprietary to Pixar we'll ignore for now and assume mip == 0
    // What this means is Pixar flavored TIFF formats aren't fully supported yet
    _filename = filename;
    _miplevel = 0;
    _subimage = subimage;
    _sourceColorSpace = sourceColorSpace;

    if(!_OpenImageSource(filename)) {
        return false;
    }

    return true;
}

/* virtual */
bool
HioImageIO_Image::Read(StorageSpec const & storage)
{
    return ReadCropped(0, 0, 0, 0, storage);
}

/* virtual */
bool
HioImageIO_Image::ReadCropped(int const cropTop,
                           int const cropBottom,
                           int const cropLeft,
                           int const cropRight,
                           StorageSpec const & storage)
{

    if(_imageSourceRef == NULL || _imageRef == NULL) {
        if(!_OpenImageSource(_filename)) {
            return false;
        }
    }
    
    int width = GetWidth();
    int height = GetHeight();
    int bytesPerRow = (int)CGImageGetBytesPerRow(_imageRef);
    int bitsPerComponent = (int)CGImageGetBitsPerComponent(_imageRef);

    CGImageRef croppedImage = NULL;
    if (cropTop || cropBottom || cropLeft || cropRight) {
        croppedImage = CGImageCreateWithImageInRect(_imageRef, CGRectMake(cropLeft, cropTop, width - cropRight, height - cropBottom));
        if (croppedImage == NULL) {
            return false;
        }
    }
    
    CGColorSpaceRef colorSpace;
    CGBitmapInfo bitmapInfo;
    if (IsColorSpaceSRGB()) {
        // if this is recognized as sRGB then set the colorSpace and bitmapInfo accordingly
        // this is for cases in which the input format may be CMYK,
        // which OpenImageIO converts to rgb when ImageSpec's attribute oiio:RawColor is false (default)
        colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        if (colorSpace == NULL) {
            return false;
        }
        bitmapInfo = kCGImageAlphaPremultipliedLast | kCGImagePixelFormatPacked;
    } else {
        // else use the description from the imageRef itself
        colorSpace = CGImageGetColorSpace(_imageRef);
        if (colorSpace == NULL) {
            return false;
        }
        colorSpace = (CGColorSpaceRef)CFRetain(colorSpace);
        bitmapInfo = CGImageGetBitmapInfo(_imageRef);
    }
    
    size_t storageBytesPerComponent = _GetComponentByteSizeFromHIOFormat(storage.format);
    size_t storageNumChannels = _GetNumChannelsFromHIOFormat(storage.format);
    size_t storageBytesPerRow = storageNumChannels * storageBytesPerComponent * storage.width;
    CGContextRef context = CGBitmapContextCreate(storage.data, storage.width, storage.height,
                                                 storageBytesPerComponent * 8,
                                                 storageBytesPerRow, colorSpace, bitmapInfo);
    CGColorSpaceRelease(colorSpace);
    
    if (context == NULL) {
        return false;
    }
    
    if (storage.flipped) {
        CGContextTranslateCTM(context, 0, height);
        CGContextScaleCTM(context, 1, -1);
    }
    
    CGContextDrawImage(context, CGRectMake(0, 0, storage.width, storage.height), 
                       croppedImage ? croppedImage : _imageRef);
    if (croppedImage != NULL) {
        CGImageRelease(croppedImage);
    }
    CGContextRelease(context);

    return true;
}

/* virtual */
bool
HioImageIO_Image::_OpenForWriting(std::string const & filename)
{
    _filename = filename;
    return true;
}

bool
HioImageIO_Image::Write(StorageSpec const & storage,
                     VtDictionary const & metadata)
{
    int nchannels = HioGetComponentCount(storage.format);
    size_t bytesPerComponent = _GetComponentByteSizeFromHIOFormat(storage.format);
    size_t bytesPerRow = storage.width * nchannels * bytesPerComponent;
    size_t bitsPerComponent = bytesPerComponent * 8;
    size_t bitsPerPixel = bitsPerComponent * nchannels;
    size_t storageSizeBytes = storage.height * bytesPerRow;
    
    CGColorSpaceRef colorSpace = nil;
    CFDataRef storageData = nil;
    CGDataProviderRef storageDataProvider = nil;
    CGImageRef storageImage = nil;
    CGContextRef context = nil;
    CGImageRef destinationImage = nil;
    CFURLRef cfurl = nil;
    CGImageDestinationRef destination = nil;
    NSMutableDictionary* imageProperties = nil;
    
    @try {
        colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
        if (colorSpace == nil) return false;
        
        bool isFloatingPoint = _IsHioFormatFloat(storage.format);
        
        CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault;
        switch (bytesPerComponent) {
            case 2:
                bitmapInfo = kCGBitmapByteOrder16Host;
                break;
            case 4:
                bitmapInfo = kCGBitmapByteOrder32Host;
                break;
        }
        
        bitmapInfo = bitmapInfo | (isFloatingPoint ? kCGBitmapFloatComponents : 0) | kCGImageAlphaPremultipliedLast;
        
        storageData = CFDataCreate(kCFAllocatorDefault , (UInt8*)storage.data, storageSizeBytes);
        if (storageData == nil) return false;
        
        storageDataProvider = CGDataProviderCreateWithCFData(storageData);
        if (storageDataProvider == nil) return false;
        
        storageImage = CGImageCreate(storage.width, storage.height, bitsPerComponent, bitsPerPixel,
                                                bytesPerRow, colorSpace, bitmapInfo, storageDataProvider, nil,
                                                false, kCGRenderingIntentDefault);
        if (storageImage == nil) return false;
        
        std::unique_ptr<uint8_t[]> pixelData(new uint8_t[storageSizeBytes]);
        
        if (storage.flipped) {
            context = CGBitmapContextCreate(pixelData.get(), storage.width, storage.height, bitsPerComponent, bytesPerRow, colorSpace, bitmapInfo);
            
            if (context == nil) return false;
            
            CGContextTranslateCTM(context, 0, storage.height);
            CGContextScaleCTM(context, 1, -1);
            CGContextDrawImage(context, CGRectMake(0, 0, storage.width, storage.height), storageImage);
            CGImageRelease(storageImage);
            storageImage = nil;
            destinationImage = CGBitmapContextCreateImage(context);
            if (destinationImage == NULL) return false;
        } else {
            destinationImage = storageImage;
            storageImage = nil;
        }
        
        NSURL* writeURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:_filename.c_str()]];
        
        if (writeURL == nil) return false;
        
        cfurl = (CFURLRef)CFBridgingRetain(writeURL);
        if (cfurl == nil) return false;
        
        destination = CGImageDestinationCreateWithURL(cfurl, CFSTR("public.tiff"), 1, nil);

        if (destination == nil) return false;
        
        imageProperties = [[NSMutableDictionary alloc] init];
        if (imageProperties == nil) return false;
        
        for (const std::pair<std::string, VtValue> m : metadata) {
            _SetAttribute(m.first, m.second, imageProperties);
        }
        
        CFDictionaryRef properties = (CFDictionaryRef)imageProperties;
        CGImageDestinationAddImage(destination, destinationImage, properties);
        CGImageDestinationFinalize(destination);
    }
    @finally {
        if (destination) CFRelease(destination);
        if (imageProperties) [imageProperties release];
        if (cfurl) CFRelease(cfurl);
        if (destinationImage) CGImageRelease(destinationImage);
        if (context) CFRelease(context);
        if (storageImage) CGImageRelease(storageImage);
        if (storageDataProvider) CFRelease(storageDataProvider);
        if (storageData) CFRelease(storageData);
        if (colorSpace) CGColorSpaceRelease(colorSpace);
    }

    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

