//
// Copyright 2020 Pixar
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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/textureObject.h"

#include "pxr/imaging/hdSt/textureObjectRegistry.h"
#include "pxr/imaging/hdSt/subtextureIdentifier.h"
#include "pxr/imaging/hdSt/textureIdentifier.h"

#include "pxr/imaging/glf/uvTextureData.h"
#ifdef PXR_OPENVDB_SUPPORT_ENABLED
#include "pxr/imaging/glf/vdbTextureData.h"
#endif
#include "pxr/imaging/glf/ptexTexture.h"
#include "pxr/imaging/glf/udimTexture.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/blitCmds.h"

#include "pxr/usd/ar/resolver.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
// HdStTextureObject

HdStTextureObject::HdStTextureObject(
    const HdStTextureIdentifier &textureId,
    HdSt_TextureObjectRegistry * const textureObjectRegistry)
  : _textureObjectRegistry(textureObjectRegistry)
  , _textureId(textureId)
  , _targetMemory(0)
{
}

void
HdStTextureObject::SetTargetMemory(const size_t targetMemory)
{
    if (_targetMemory == targetMemory) {
        return;
    }
    _targetMemory = targetMemory;
    _textureObjectRegistry->MarkTextureObjectDirty(shared_from_this());
}

Hgi *
HdStTextureObject::_GetHgi() const
{
    if (!TF_VERIFY(_textureObjectRegistry)) {
        return nullptr;
    }

    Hgi * const hgi = _textureObjectRegistry->GetHgi();
    TF_VERIFY(hgi);

    return hgi;
}

HdStTextureObject::~HdStTextureObject() = default;

///////////////////////////////////////////////////////////////////////////////
// Helpers

static
std::string
_GetDebugName(const HdStTextureIdentifier &textureId)
{
    if (const HdStVdbSubtextureIdentifier * const vdbSubtextureId =
            dynamic_cast<const HdStVdbSubtextureIdentifier*>(
                textureId.GetSubtextureIdentifier())) {
        return
            textureId.GetFilePath().GetString() + " - " +
            vdbSubtextureId->GetGridName().GetString();
    }

    if (const HdStUvOrientationSubtextureIdentifier * const subId =
            dynamic_cast<const HdStUvOrientationSubtextureIdentifier*>(
                textureId.GetSubtextureIdentifier())) {
        return
            textureId.GetFilePath().GetString()
            + " - flipVertically="
            + std::to_string(int(subId->GetFlipVertically()));
    }
     
    return
        textureId.GetFilePath().GetString();
}

static
HgiTextureType
_GetTextureType(int numDimensions)
{
    switch(numDimensions) {
    case 2:
        return HgiTextureType2D;
    case 3:
        return HgiTextureType3D;
    default:
        TF_CODING_ERROR("Unsupported number of dimensions");
        return HgiTextureType2D;
    }
}

// A helper class that creates an HgiTextureDesc from GlfBaseTextureData.
//
// It will convert RGB to RGBA if necessary and manages the life time
// of the CPU buffers (either by keeping GlfBaseTextureData or its own
// buffer alive).
// 
class HdSt_TextureObjectCpuData
{
public:
    // Created using texture data and a debug name used for the
    // texture descriptor.
    HdSt_TextureObjectCpuData(GlfBaseTextureDataRefPtr const &textureData,
                              const std::string &debugName,
                              bool generateMips = false,
                              GlfImage::ImageOriginLocation originLocation
                                          = GlfImage::OriginUpperLeft);

    ~HdSt_TextureObjectCpuData() = default;

    // Texture descriptor, including initialData pointer.
    const HgiTextureDesc &GetTextureDesc() const { return _textureDesc; }

    // Texture data valid? False if, e.g., no file at given path.
    bool IsValid() const { return _textureDesc.initialData; }

private:
    // Determine format for texture descriptor.
    //
    // If necessary, converts the RGB to RGBA data updating 
    // _textureDesc.initialData to point to the newly allocated data
    // (and dropping _textureData).
    //
    HgiFormat _DetermineFormatAndConvertIfNecessary(
        const GLenum glFormat,
        const GLenum glType,
        const GLenum glInternalFormat);

    // The result, including a pointer to the potentially
    // converted texture data in _textureDesc.initialData.
    HgiTextureDesc _textureDesc;

    // To avoid a copy, hold on to original data if we
    // can use them.
    GlfBaseTextureDataRefPtr _textureData;
    // Buffer if we had to convert the data.
    std::unique_ptr<const unsigned char[]> _convertedRawData;
};

// Compute the number of mip levels given the dimensions of a texture using
// the same formula as OpenGL.
static
uint16_t _ComputeNumMipLevels(const GfVec3i &dimensions)
{
    const int dim = std::max({dimensions[0], dimensions[1], dimensions[2]});

    for (uint16_t i = 1; i < 8 * sizeof(int) - 1; i++) {
        const int powerTwo = 1 << i;
        if (powerTwo > dim) {
            return i;
        }
    }
    
    // Can never be reached, but compiler doesn't know that.
    return 1;
}

static
bool
_IsValid(GlfBaseTextureDataRefPtr const &textureData)
{
    return
        textureData->ResizedWidth() > 0 &&
        textureData->ResizedHeight() > 0 &&
        textureData->ResizedDepth() > 0 &&
        textureData->HasRawBuffer();
}

HdSt_TextureObjectCpuData::HdSt_TextureObjectCpuData(
    GlfBaseTextureDataRefPtr const &textureData,
    const std::string &debugName,
    const bool generateMips,
    const GlfImage::ImageOriginLocation originLocation)
  : _textureData(textureData)
{
    TRACE_FUNCTION();

    _textureDesc.debugName = debugName;

    // Bail if we don't have texture data.
    if (!textureData) {
        return;
    }

    // Read texture file
    if (!textureData->Read(0, false, originLocation)) {
        return;
    }

    // Sanity checks
    if (!_IsValid(textureData)) {
        return;
    }

    // If there is no file at the given path, we should have bailed
    // by now and left _textureDesc.initalData null indicating to
    // our clients that the texture is invalid.

    // Is this 2D or 3D texture?
    _textureDesc.type = _GetTextureType(textureData->NumDimensions());
    _textureDesc.dimensions = GfVec3i(
        textureData->ResizedWidth(),
        textureData->ResizedHeight(),
        textureData->ResizedDepth());
    // Image data - might need RGB to RGBA conversion.
    _textureDesc.initialData = textureData->GetRawBuffer();

    if (generateMips) {
        _textureDesc.mipLevels = _ComputeNumMipLevels(_textureDesc.dimensions);
    }

    // Determine the format (e.g., float/byte, RED/RGBA).
    // Convert data if necessary, setting initialData to the buffer
    // with the new data and freeing _textureData
    _textureDesc.format = _DetermineFormatAndConvertIfNecessary(
        textureData->GLFormat(),
        textureData->GLType(),
        textureData->GLInternalFormat());

    // Size of initial data (note that textureData->ComputeBytesUSed()
    // includes the mip maps).
    _textureDesc.pixelsByteSize =
        textureData->ResizedWidth() *
        textureData->ResizedHeight() *
        textureData->ResizedDepth() *
        HgiDataSizeOfFormat(_textureDesc.format);
}

template<typename T>
static
std::unique_ptr<const unsigned char[]>
_ConvertRGBToRGBA(
    const unsigned char * const data,
    const GfVec3i &dimensions,
    const T alpha)
{
    TRACE_FUNCTION();

    const T * const typedData = reinterpret_cast<const T*>(data);

    const size_t num = dimensions[0] * dimensions[1] * dimensions[2];

    std::unique_ptr<unsigned char[]> result =
        std::make_unique<unsigned char[]>(num * 4 * sizeof(T));

    T * const typedConvertedData = reinterpret_cast<T*>(result.get());

    for (size_t i = 0; i < num; i++) {
        typedConvertedData[4 * i + 0] = typedData[3 * i + 0];
        typedConvertedData[4 * i + 1] = typedData[3 * i + 1];
        typedConvertedData[4 * i + 2] = typedData[3 * i + 2];
        typedConvertedData[4 * i + 3] = alpha;
    }

    return std::move(result);
}

// Some of these formats have been aliased to HgiFormatInvalid because
// they are not available on MTL. Guard against us trying to use
// formats that are no longer available.
template<HgiFormat f>
static
constexpr HgiFormat _CheckValid()
{
    static_assert(f != HgiFormatInvalid, "Invalid HgiFormat");
    return f;
}

HgiFormat
HdSt_TextureObjectCpuData::_DetermineFormatAndConvertIfNecessary(
    const GLenum glFormat,
    const GLenum glType,
    const GLenum glInternalFormat)
{
    // Format dispatch, mostly we can just use the CPU buffer from
    // the texture data provided.
    switch(glFormat) {
    case GL_RED:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            return _CheckValid<HgiFormatUNorm8>();
        case GL_HALF_FLOAT:
            return _CheckValid<HgiFormatFloat16>();
        case GL_FLOAT:
            return _CheckValid<HgiFormatFloat32>();
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RGBA 0x%04x",
                            glType);
            return HgiFormatInvalid;
        }
    case GL_RG:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            return _CheckValid<HgiFormatUNorm8Vec2>();
        case GL_HALF_FLOAT:
            return _CheckValid<HgiFormatFloat16Vec2>();
        case GL_FLOAT:
            return _CheckValid<HgiFormatFloat32Vec2>();
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RGBA 0x%04x",
                            glType);
            return HgiFormatInvalid;
        }
    case GL_RGB:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            // RGB (24bit) is not supported on MTL, so we need to convert it.
            _convertedRawData = 
                _ConvertRGBToRGBA<unsigned char>(
                    reinterpret_cast<const unsigned char *>(
                        _textureDesc.initialData),
                    _textureDesc.dimensions,
                    255);
            // Point to the buffer with the converted data.
            _textureDesc.initialData = _convertedRawData.get();
            // Drop the old buffer.
            _textureData = TfNullPtr;

            if (glInternalFormat == GL_SRGB8) {
                return _CheckValid<HgiFormatUNorm8Vec4srgb>();
            } else {
                return _CheckValid<HgiFormatUNorm8Vec4>();
            }
        case GL_HALF_FLOAT:
            return _CheckValid<HgiFormatFloat16Vec3>();
        case GL_FLOAT:
            return _CheckValid<HgiFormatFloat32Vec3>();
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RGBA 0x%04x",
                            glType);
            return HgiFormatInvalid;
        }
    case GL_RGBA:
        switch(glType) {
        case GL_UNSIGNED_BYTE:
            if (glInternalFormat == GL_SRGB8_ALPHA8) {
                return _CheckValid<HgiFormatUNorm8Vec4srgb>();
            } else {
                return _CheckValid<HgiFormatUNorm8Vec4>();
            }
        case GL_HALF_FLOAT:
            return _CheckValid<HgiFormatFloat16Vec4>();
        case GL_FLOAT:
            return _CheckValid<HgiFormatFloat32Vec4>();
        default:
            TF_CODING_ERROR("Unsupported texture format GL_RGBA 0x%04x",
                            glType);
            return HgiFormatInvalid;
        }
    case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
        switch(glType) {
        case GL_FLOAT:
            return _CheckValid<HgiFormatBC6UFloatVec3>();
        default:
            TF_CODING_ERROR(
                "Unsupported texture format "
                "GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x%04x",
                glType);
            return HgiFormatInvalid;
        }
    case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT:
        switch(glType) {
        case GL_FLOAT:
            return _CheckValid<HgiFormatBC6FloatVec3>();
        default:
            TF_CODING_ERROR(
                "Unsupported texture format "
                "GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x%04x",
                glType);
            return HgiFormatInvalid;
        }
    default:
        TF_CODING_ERROR("Unsupported texture format 0x%04x 0x%04x",
                        glFormat, glType);
        return HgiFormatInvalid;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Uv texture

HdStUvTextureObject::HdStUvTextureObject(
    const HdStTextureIdentifier &textureId,
    HdSt_TextureObjectRegistry * textureObjectRegistry)
  : HdStTextureObject(textureId, textureObjectRegistry)
{
}


HdTextureType
HdStUvTextureObject::GetTextureType() const
{
    return HdTextureType::Uv;
}

///////////////////////////////////////////////////////////////////////////////
// Uv asset texture

static
HdWrap
_GetWrapParameter(const bool hasWrapMode, const GLenum wrapMode)
{
    if (hasWrapMode) {
        switch(wrapMode) {
        case GL_CLAMP_TO_EDGE: return HdWrapClamp;
        case GL_REPEAT: return HdWrapRepeat;
        case GL_CLAMP_TO_BORDER: return HdWrapBlack;
        case GL_MIRRORED_REPEAT: return HdWrapMirror;
        //
        // For GlfImage legacy plugins that still use the GL_CLAMP
        // (obsoleted in OpenGL 3.0).
        //
        // Note that some graphics drivers produce results for GL_CLAMP
        // that match neither GL_CLAMP_TO_BORDER not GL_CLAMP_TO_EDGE.
        //
        // We pick GL_CLAMP_TO_EDGE here - breaking backwards compatibility.
        //
        case GL_CLAMP: return HdWrapClamp;
        default:
            TF_CODING_ERROR("Unsupported GL wrap mode 0x%04x", wrapMode);
        }
    }

    return HdWrapNoOpinion;
}

static
std::pair<HdWrap, HdWrap>
_GetWrapParameters(GlfUVTextureDataRefPtr const &uvTexture)
{
    if (!uvTexture) {
        return { HdWrapUseMetadata, HdWrapUseMetadata };
    }

    const GlfBaseTextureData::WrapInfo &wrapInfo = uvTexture->GetWrapInfo();

    return { _GetWrapParameter(wrapInfo.hasWrapModeS, wrapInfo.wrapModeS), 
             _GetWrapParameter(wrapInfo.hasWrapModeT, wrapInfo.wrapModeT) };
}

// Read from the HdStUvOrientationSubtextureIdentifier whether we need
// to flip the image.
//
// This is to support the legacy HwUvTexture_1 shader node which has the
// vertical orientation opposite to UsdUvTexture.
//
static
GlfImage::ImageOriginLocation
_GetImageOriginLocation(const HdStSubtextureIdentifier * const subId)
{
    using SubId = const HdStUvOrientationSubtextureIdentifier;
    
    if (SubId* const uvSubId = dynamic_cast<SubId*>(subId)) {
        if (uvSubId->GetFlipVertically()) {
            return GlfImage::OriginUpperLeft;
        }
    }
    return GlfImage::OriginLowerLeft;
}

HdStAssetUvTextureObject::HdStAssetUvTextureObject(
    const HdStTextureIdentifier &textureId,
    HdSt_TextureObjectRegistry * const textureObjectRegistry)
  : HdStUvTextureObject(textureId, textureObjectRegistry)
  , _wrapParameters{HdWrapUseMetadata, HdWrapUseMetadata}
{
}

HdStAssetUvTextureObject::~HdStAssetUvTextureObject()
{
    if (Hgi * hgi = _GetHgi()) {
        hgi->DestroyTexture(&_gpuTexture);
    }
}

void
HdStAssetUvTextureObject::_Load()
{
    TRACE_FUNCTION();

    GlfUVTextureDataRefPtr const textureData =
        GlfUVTextureData::New(
            GetTextureIdentifier().GetFilePath(),
            GetTargetMemory(),
            /* borders */ 0, 0, 0, 0);

    _cpuData = std::make_unique<HdSt_TextureObjectCpuData>(
        textureData,
        _GetDebugName(GetTextureIdentifier()),
        /* generateMips = */ true,
        _GetImageOriginLocation(
            GetTextureIdentifier().GetSubtextureIdentifier()));

    if (_cpuData->IsValid()) {
        if (_cpuData->GetTextureDesc().type != HgiTextureType2D) {
            TF_CODING_ERROR("Wrong texture type for uv");
        }
    }

    // _GetWrapParameters can only be called after the texture has
    // been loaded by HdSt_TextureObjectCpuData.
    _wrapParameters = _GetWrapParameters(textureData);
}

void
HdStAssetUvTextureObject::_Commit()
{
    TRACE_FUNCTION();

    Hgi * const hgi = _GetHgi();
    if (!hgi) {
        return;
    }

    // Free previously allocated texture
    hgi->DestroyTexture(&_gpuTexture);

    // Upload to GPU only if we have valid CPU data
    if (_cpuData && _cpuData->IsValid()) {
        const HgiTextureDesc &desc = _cpuData->GetTextureDesc();

        // Upload to GPU
        _gpuTexture = hgi->CreateTexture(desc);
        if (desc.mipLevels > 1) {
            HgiBlitCmdsUniquePtr const blitCmds = hgi->CreateBlitCmds();
            blitCmds->GenerateMipMaps(_gpuTexture);
            hgi->SubmitCmds(blitCmds.get());
        }
    }

    // Free CPU memory after transfer to GPU
    _cpuData.reset();
}

bool
HdStAssetUvTextureObject::IsValid() const
{
    return bool(_gpuTexture);
}

///////////////////////////////////////////////////////////////////////////////
// Field texture

#ifdef PXR_OPENVDB_SUPPORT_ENABLED

// Compute transform mapping GfRange3d to unit box [0,1]^3
static
GfMatrix4d
_ComputeSamplingTransform(const GfRange3d &range)
{
    const GfVec3d size(range.GetSize());

    const GfVec3d scale(1.0 / size[0], 1.0 / size[1], 1.0 / size[2]);

    return
        // First map range so that min becomes (0,0,0)
        GfMatrix4d(1.0).SetTranslateOnly(-range.GetMin()) *
        // Then scale to unit box
        GfMatrix4d(1.0).SetScale(scale);
}

// Compute transform mapping bounding box to unit box [0,1]^3
static
GfMatrix4d
_ComputeSamplingTransform(const GfBBox3d &bbox)
{
    return
        // First map so that bounding box goes to its GfRange3d
        bbox.GetInverseMatrix() *
        // Then scale to unit box [0,1]^3
        _ComputeSamplingTransform(bbox.GetRange());
}

#endif

HdStFieldTextureObject::HdStFieldTextureObject(
    const HdStTextureIdentifier &textureId,
    HdSt_TextureObjectRegistry * const textureObjectRegistry)
  : HdStTextureObject(textureId, textureObjectRegistry)
{
}

HdStFieldTextureObject::~HdStFieldTextureObject()
{
    if (Hgi * hgi = _GetHgi()) {
        hgi->DestroyTexture(&_gpuTexture);
    }
}

void
HdStFieldTextureObject::_Load()
{
    TRACE_FUNCTION();

    // Proper casting.
    HdStVdbSubtextureIdentifier const * vdbSubtextureId =
        dynamic_cast<const HdStVdbSubtextureIdentifier*>(
            GetTextureIdentifier().GetSubtextureIdentifier());

    if (!vdbSubtextureId) {
        TF_CODING_ERROR("Only supporting VDB files for now");
        return;
    }

#ifdef PXR_OPENVDB_SUPPORT_ENABLED
    GlfVdbTextureDataRefPtr const texData =
        GlfVdbTextureData::New(
            GetTextureIdentifier().GetFilePath(),
            vdbSubtextureId->GetGridName(),
            GetTargetMemory());

    _cpuData = std::make_unique<HdSt_TextureObjectCpuData>(
        texData,
        _GetDebugName(GetTextureIdentifier()));

    if (_cpuData->IsValid()) {
        if (_cpuData->GetTextureDesc().type != HgiTextureType3D) {
            TF_CODING_ERROR("Wrong texture type for field");
        }

        _bbox = texData->GetBoundingBox();
        _samplingTransform = _ComputeSamplingTransform(_bbox);
    } else {
        _bbox = GfBBox3d();
        _samplingTransform = GfMatrix4d(1.0);
    }

#endif
}

void
HdStFieldTextureObject::_Commit()
{
    TRACE_FUNCTION();

    Hgi * const hgi = _GetHgi();
    if (!hgi) {
        return;
    }
        
    // Free previously allocated texture
    hgi->DestroyTexture(&_gpuTexture);

    // Upload to GPU only if we have valid CPU data
    if (_cpuData && _cpuData->IsValid()) {
        _gpuTexture = hgi->CreateTexture(_cpuData->GetTextureDesc());
    }

    // Free CPU memory after transfer to GPU
    _cpuData.reset();
}

bool
HdStFieldTextureObject::IsValid() const
{
    return bool(_gpuTexture);
}

HdTextureType
HdStFieldTextureObject::GetTextureType() const
{
    return HdTextureType::Field;
}

///////////////////////////////////////////////////////////////////////////////
// Ptex texture

HdStPtexTextureObject::HdStPtexTextureObject(
    const HdStTextureIdentifier &textureId,
    HdSt_TextureObjectRegistry * const textureObjectRegistry)
  : HdStTextureObject(textureId, textureObjectRegistry)
  , _texelGLTextureName(0)
  , _layoutGLTextureName(0)
{
}

HdStPtexTextureObject::~HdStPtexTextureObject() = default;

void
HdStPtexTextureObject::_Load()
{
    // Glf is both loading the texture and creating the
    // GL resources, so not thread-safe. Everything is
    // postponed to the single-threaded Commit.
}

void
HdStPtexTextureObject::_Commit()
{
#ifdef PXR_PTEX_SUPPORT_ENABLED
    _gpuTexture = GlfPtexTexture::New(
        GetTextureIdentifier().GetFilePath());
    _gpuTexture->SetMemoryRequested(GetTargetMemory());

    _texelGLTextureName = _gpuTexture->GetGlTextureName();
    _layoutGLTextureName = _gpuTexture->GetLayoutTextureName();
#endif
}

bool
HdStPtexTextureObject::IsValid() const
{
    // Checking whether ptex texture is valid not supported yet.
    return true;
}

HdTextureType
HdStPtexTextureObject::GetTextureType() const
{
    return HdTextureType::Ptex;
}

///////////////////////////////////////////////////////////////////////////////
// Udim texture

static const char UDIM_PATTERN[] = "<UDIM>";
static const int UDIM_START_TILE = 1001;
static const int UDIM_END_TILE = 1100;

// Split a udim file path such as /someDir/myFile.<UDIM>.exr into a
// prefix (/someDir/myFile.) and suffix (.exr).
static
std::pair<std::string, std::string>
_SplitUdimPattern(const std::string &path)
{
    static const std::string pattern(UDIM_PATTERN);

    const std::string::size_type pos = path.find(pattern);

    if (pos != std::string::npos) {
        return { path.substr(0, pos), path.substr(pos + pattern.size()) };
    }
    
    return { std::string(), std::string() };
}

// Find all udim tiles for a given udim file path /someDir/myFile.<UDIM>.exr as
// pairs, e.g., (0, /someDir/myFile.1001.exr), ...
//
// The scene delegate is assumed to already have resolved the asset path with
// the <UDIM> pattern to a "file path" with the <UDIM> pattern as above.
// This function will replace <UDIM> by different integers and check whether
// the "file" exists using an ArGetResolver.
//
// Note that the ArGetResolver is still needed, for, e.g., usdz file
// where the path we get from the scene delegate is
// /someDir/myFile.usdz[myImage.<UDIM>.EXR] and we need to use the
// ArGetResolver to check whether, e.g., myImage.1001.EXR exists in
// the zip file /someDir/myFile.usdz by calling
// resolver.Resolve(/someDir/myFile.usdz[myImage.1001.EXR]).
// However, we don't need to bind, e.g., the usd stage's resolver context
// because that part of the resolution will be done by the scene delegate
// for us already.
//
static
std::vector<std::tuple<int, TfToken>>
_FindUdimTiles(const std::string &filePath)
{
    std::vector<std::tuple<int, TfToken>> result;

    // Get prefix and suffix from udim pattern.
    const std::pair<std::string, std::string>
        splitPath = _SplitUdimPattern(filePath);
    if (splitPath.first.empty() && splitPath.second.empty()) {
        TF_WARN("Expected udim pattern but got '%s'.",
                filePath.c_str());
        return result;
    }

    ArResolver& resolver = ArGetResolver();
    
    for (int i = UDIM_START_TILE; i < UDIM_END_TILE; i++) {
        // Add integer between prefix and suffix and see whether
        // the tile exists by consulting the resolver.
        const std::string resolvedPath =
            resolver.Resolve(
                splitPath.first + std::to_string(i) + splitPath.second);
        if (!resolvedPath.empty()) {
            // Record pair in result.
            result.emplace_back(i - UDIM_START_TILE, resolvedPath);
        }
    }

    return result;
}

HdStUdimTextureObject::HdStUdimTextureObject(
    const HdStTextureIdentifier &textureId,
    HdSt_TextureObjectRegistry * const textureObjectRegistry)
  : HdStTextureObject(textureId, textureObjectRegistry)
  , _texelGLTextureName(0)
  , _layoutGLTextureName(0)
{
}

HdStUdimTextureObject::~HdStUdimTextureObject() = default;

void
HdStUdimTextureObject::_Load()
{
    // Glf is both loading the tiles and creating the GL resources, so
    // not thread-safe.
    //
    // The only thing we can do here is determine the tiles.
    _tiles = _FindUdimTiles(GetTextureIdentifier().GetFilePath());
}

void
HdStUdimTextureObject::_Commit()
{
    // Load tiles.
    _gpuTexture = GlfUdimTexture::New(
        GetTextureIdentifier().GetFilePath(),
        GlfImage::OriginLowerLeft,
        std::move(_tiles));
    _gpuTexture->SetMemoryRequested(GetTargetMemory());

    _layoutGLTextureName = _gpuTexture->GetGlLayoutName();
    _texelGLTextureName = _gpuTexture->GetGlTextureName();
}

bool
HdStUdimTextureObject::IsValid() const
{
    // Checking whether ptex texture is valid not supported yet.
    return true;
}

HdTextureType
HdStUdimTextureObject::GetTextureType() const
{
    return HdTextureType::Udim;
}

PXR_NAMESPACE_CLOSE_SCOPE
