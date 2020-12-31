//
// Copyright 2020 benmalartre
//
// Unlicensed
//

#include "pxr/imaging/plugin/LoFi/textureObject.h"

#include "pxr/imaging/plugin/LoFi/glfTextureCpuData.h"
#include "pxr/imaging/plugin/LoFi/assetUvTextureCpuData.h"
#include "pxr/imaging/plugin/LoFi/ptexTextureObject.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/imaging/plugin/LoFi/textureCpuData.h"
#include "pxr/imaging/plugin/LoFi/textureObjectRegistry.h"
#include "pxr/imaging/plugin/LoFi/subtextureIdentifier.h"
#include "pxr/imaging/plugin/LoFi/fieldSubtextureIdentifier.h"
#include "pxr/imaging/plugin/LoFi/textureIdentifier.h"
#include "pxr/imaging/plugin/LoFi/tokens.h"
#include "pxr/imaging/plugin/LoFi/udimTextureObject.h"

#include "pxr/imaging/glf/fieldTextureData.h"
#ifdef PXR_OPENVDB_SUPPORT_ENABLED
#include "pxr/imaging/glf/vdbTextureData.h"
#endif
#include "pxr/imaging/glf/field3DTextureDataBase.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/blitCmds.h"

#include "pxr/usd/ar/resolver.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
// LoFiTextureObject

LoFiTextureObject::LoFiTextureObject(
    const LoFiTextureIdentifier &textureId,
    LoFiTextureObjectRegistry * const textureObjectRegistry)
  : _textureObjectRegistry(textureObjectRegistry)
  , _textureId(textureId)
  , _targetMemory(0)
{
}

void
LoFiTextureObject::SetTargetMemory(const size_t targetMemory)
{
    if (_targetMemory == targetMemory) {
        return;
    }
    _targetMemory = targetMemory;
    _textureObjectRegistry->MarkTextureObjectDirty(shared_from_this());
}

LoFiResourceRegistry*
LoFiTextureObject::_GetResourceRegistry() const
{
    if (!TF_VERIFY(_textureObjectRegistry)) {
        return nullptr;
    }

    LoFiResourceRegistry* const registry =
        _textureObjectRegistry->GetResourceRegistry();
    TF_VERIFY(registry);

    return registry;
}

Hgi *
LoFiTextureObject::_GetHgi() const
{
    LoFiResourceRegistry* const registry = _GetResourceRegistry();
    if (!TF_VERIFY(registry)) {
        return nullptr;
    }

    Hgi * const hgi = registry->GetHgi();
    TF_VERIFY(hgi);

    return hgi;
}

void
LoFiTextureObject::_AdjustTotalTextureMemory(
    const int64_t memDiff)
{
    if (TF_VERIFY(_textureObjectRegistry)) {
        _textureObjectRegistry->AdjustTotalTextureMemory(memDiff);
    }
}

void
LoFiTextureObject::_AddToTotalTextureMemory(
    const HgiTextureHandle &texture)
{
    if (texture) {
        _AdjustTotalTextureMemory(texture->GetByteSizeOfResource());
    }
}

void
LoFiTextureObject::_SubtractFromTotalTextureMemory(
    const HgiTextureHandle &texture)
{
    if (texture) {
        _AdjustTotalTextureMemory(-texture->GetByteSizeOfResource());
    }
}

LoFiTextureObject::~LoFiTextureObject() = default;

///////////////////////////////////////////////////////////////////////////////
// Helpers

std::string
LoFiTextureObject::_GetDebugName(const LoFiTextureIdentifier &textureId) const
{
    const std::string &filePath = textureId.GetFilePath().GetString();
    const LoFiSubtextureIdentifier * const subId =
        textureId.GetSubtextureIdentifier();

    if (!subId) {
        return filePath;
    }

    if (const LoFiOpenVDBAssetSubtextureIdentifier * const vdbSubId =
            dynamic_cast<const LoFiOpenVDBAssetSubtextureIdentifier*>(subId)) {
        return
            filePath + " - "
            + vdbSubId->GetFieldName().GetString();
    }

    if (const LoFiField3DAssetSubtextureIdentifier * const f3dSubId =
            dynamic_cast<const LoFiField3DAssetSubtextureIdentifier*>(subId)) {
        return
            filePath + " - "
            + f3dSubId->GetFieldName().GetString() + " "
            + std::to_string(f3dSubId->GetFieldIndex()) + " "
            + f3dSubId->GetFieldPurpose().GetString();
    }

    if (const LoFiAssetUvSubtextureIdentifier * const assetUvSubId =
            dynamic_cast<const LoFiAssetUvSubtextureIdentifier*>(subId)) {
        return
            filePath
            + " - flipVertically="
            + std::to_string(int(assetUvSubId->GetFlipVertically()))
            + " - premultiplyAlpha="
            + std::to_string(int(assetUvSubId->GetPremultiplyAlpha()))
            + " - sourceColorSpace="
            + assetUvSubId->GetSourceColorSpace().GetString();
    }

    if (const LoFiPtexSubtextureIdentifier * const ptexSubId =
            dynamic_cast<const LoFiPtexSubtextureIdentifier*>(subId)) {
        return
            filePath
            + " - premultiplyAlpha="
            + std::to_string(int(ptexSubId->GetPremultiplyAlpha()));
    }

    if (const LoFiUdimSubtextureIdentifier * const udimSubId =
            dynamic_cast<const LoFiUdimSubtextureIdentifier*>(subId)) {
        return
            filePath +
            + " - premultiplyAlpha="
            + std::to_string(int(udimSubId->GetPremultiplyAlpha()))
            + " - sourceColorSpace="
            + udimSubId->GetSourceColorSpace().GetString();
    }

    return filePath + " - unknown subtexture identifier";
}

// Read from the LoFiSubtextureIdentifier whether we need
// to pre-multiply the texture by alpha
//
bool
LoFiTextureObject::_GetPremultiplyAlpha(
        const LoFiSubtextureIdentifier * const subId) const
{
    switch(GetTextureType()) {
    case HdTextureType::Uv:
        if (const LoFiAssetUvSubtextureIdentifier* const uvSubId =
            dynamic_cast<const LoFiAssetUvSubtextureIdentifier *>(subId)) {
            return uvSubId->GetPremultiplyAlpha();
        }
        return false;
    case HdTextureType::Ptex:
        if (const LoFiPtexSubtextureIdentifier* const ptexSubId =
            dynamic_cast<const LoFiPtexSubtextureIdentifier *>(subId)) {
        return ptexSubId->GetPremultiplyAlpha();
        }
        return false;
    case HdTextureType::Udim:
        if (const LoFiUdimSubtextureIdentifier* const udimSubId =
                dynamic_cast<const LoFiUdimSubtextureIdentifier *>(subId)) {
            return udimSubId->GetPremultiplyAlpha();
        }
        return false;
    default:
        return false;
    }
}

// Read from the LoFiSubtextureIdentifier its source color space
//
HioImage::SourceColorSpace
LoFiTextureObject::_GetSourceColorSpace(
        const LoFiSubtextureIdentifier * const subId) const
{
    TfToken sourceColorSpace;
    switch(GetTextureType()) {
    case HdTextureType::Uv:
        if (const LoFiAssetUvSubtextureIdentifier* const uvSubId =
            dynamic_cast<const LoFiAssetUvSubtextureIdentifier *>(subId)) {
            sourceColorSpace = uvSubId->GetSourceColorSpace();
        }
        break;
    case HdTextureType::Udim:
        if (const LoFiUdimSubtextureIdentifier* const udimSubId =
                dynamic_cast<const LoFiUdimSubtextureIdentifier *>(subId)) {
            sourceColorSpace = udimSubId->GetSourceColorSpace();
        }
        break;
    default:
        break;
    }

    if (sourceColorSpace == LoFiTokens->sRGB) {
        return HioImage::SRGB;
    }
    if (sourceColorSpace == LoFiTokens->raw) {
        return HioImage::Raw;
    }
    return HioImage::Auto;
}

///////////////////////////////////////////////////////////////////////////////
// Uv texture

LoFiUvTextureObject::LoFiUvTextureObject(
    const LoFiTextureIdentifier &textureId,
    LoFiTextureObjectRegistry * textureObjectRegistry)
  : LoFiTextureObject(textureId, textureObjectRegistry)
  , _wrapParameters{HdWrapNoOpinion, HdWrapNoOpinion}
{
}


HdTextureType
LoFiUvTextureObject::GetTextureType() const
{
    return HdTextureType::Uv;
}

LoFiUvTextureObject::~LoFiUvTextureObject()
{
    _DestroyTexture();
}

void
LoFiUvTextureObject::_SetWrapParameters(
    const std::pair<HdWrap, HdWrap> &wrapParameters)
{
    _wrapParameters = wrapParameters;
}

void
LoFiUvTextureObject::_SetCpuData(
    std::unique_ptr<LoFiTextureCpuData> &&cpuData)
{
    _cpuData = std::move(cpuData);
}

LoFiTextureCpuData *
LoFiUvTextureObject::_GetCpuData() const
{
    return _cpuData.get();
}

void
LoFiUvTextureObject::_CreateTexture(const HgiTextureDesc &desc)
{
    Hgi * const hgi = _GetHgi();
    if (!TF_VERIFY(hgi)) {
        return;
    }

    _DestroyTexture();

    _gpuTexture = hgi->CreateTexture(desc);
    _AddToTotalTextureMemory(_gpuTexture);
}

void
LoFiUvTextureObject::_GenerateMipmaps()
{
    LoFiResourceRegistry * const registry = _GetResourceRegistry();
    if (!TF_VERIFY(registry)) {
        return;
    }

    if (!_gpuTexture) {
        return;
    }

    HgiBlitCmds* const blitCmds = registry->GetGlobalBlitCmds();
    blitCmds->GenerateMipMaps(_gpuTexture);
}

void
LoFiUvTextureObject::_DestroyTexture()
{
    if (Hgi * hgi = _GetHgi()) {
        _SubtractFromTotalTextureMemory(_gpuTexture);
        hgi->DestroyTexture(&_gpuTexture);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Uv asset texture

// Read from the LoFiAssetUvSubtextureIdentifier whether we need
// to flip the image.
//
// This is to support the legacy HwUvTexture_1 shader node which has the
// vertical orientation opposite to UsdUvTexture.
//
static
HioImage::ImageOriginLocation
_GetImageOriginLocation(const LoFiSubtextureIdentifier * const subId)
{
    using SubId = const LoFiAssetUvSubtextureIdentifier;

    if (SubId* const uvSubId = dynamic_cast<SubId*>(subId)) {
        if (uvSubId->GetFlipVertically()) {
            return HioImage::OriginUpperLeft;
        }
    }
    return HioImage::OriginLowerLeft;
}

LoFiAssetUvTextureObject::LoFiAssetUvTextureObject(
    const LoFiTextureIdentifier &textureId,
    LoFiTextureObjectRegistry * const textureObjectRegistry)
  : LoFiUvTextureObject(textureId, textureObjectRegistry)
{
}

LoFiAssetUvTextureObject::~LoFiAssetUvTextureObject() = default;

void
LoFiAssetUvTextureObject::_Load()
{
    TRACE_FUNCTION();

    std::unique_ptr<LoFiAssetUvTextureCpuData> cpuData =
        std::make_unique<LoFiAssetUvTextureCpuData>(
            GetTextureIdentifier().GetFilePath(),
            GetTargetMemory(),
            _GetPremultiplyAlpha(
                GetTextureIdentifier().GetSubtextureIdentifier()),
            _GetImageOriginLocation(
                GetTextureIdentifier().GetSubtextureIdentifier()),
            LoFiTextureObject::_GetSourceColorSpace(
                GetTextureIdentifier().GetSubtextureIdentifier()));
    _SetWrapParameters(cpuData->GetWrapInfo());
    _SetCpuData(std::move(cpuData));
}

void
LoFiAssetUvTextureObject::_Commit()
{
    TRACE_FUNCTION();

    _DestroyTexture();

    if (LoFiTextureCpuData * const cpuData = _GetCpuData()) {
        if (cpuData->IsValid()) {
            // Upload to GPU
            _CreateTexture(cpuData->GetTextureDesc());
            if (cpuData->GetGenerateMipmaps()) {
                _GenerateMipmaps();
            }
        }
    }

    // Free CPU memory after transfer to GPU
    _SetCpuData(nullptr);
}

bool
LoFiAssetUvTextureObject::IsValid() const
{
    return bool(GetTexture());
}

///////////////////////////////////////////////////////////////////////////////
// Field texture

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

static
GlfFieldTextureDataRefPtr
_ComputeFieldTexData(
    const LoFiTextureIdentifier &textureId,
    const size_t targetMemory)
{
    const std::string &filePath = textureId.GetFilePath().GetString();
    const LoFiSubtextureIdentifier * const subId =
        textureId.GetSubtextureIdentifier();

#ifdef PXR_OPENVDB_SUPPORT_ENABLED
    if (const LoFiOpenVDBAssetSubtextureIdentifier * const vdbSubId =
            dynamic_cast<const LoFiOpenVDBAssetSubtextureIdentifier*>(subId)) {
        if (vdbSubId->GetFieldIndex() != 0) {
            TF_WARN("Support of field index when reading OpenVDB file not yet "
                    "implemented (file: %s, field name: %s, field index: %d",
                    filePath.c_str(),
                    vdbSubId->GetFieldName().GetText(),
                    vdbSubId->GetFieldIndex());
        }
        return GlfVdbTextureData::New(
            filePath, vdbSubId->GetFieldName(), targetMemory);
    }
#endif

    if (const LoFiField3DAssetSubtextureIdentifier * const f3dSubId =
            dynamic_cast<const LoFiField3DAssetSubtextureIdentifier*>(subId)) {
        GlfField3DTextureDataBaseRefPtr const texData =
            GlfField3DTextureDataBase::New(
                filePath,
                f3dSubId->GetFieldName(),
                f3dSubId->GetFieldIndex(),
                f3dSubId->GetFieldPurpose(),
                targetMemory);
        if (!texData) {
            TF_WARN("Could not find plugin to load Field3D file.");
        }
        return texData;
    }

    TF_CODING_ERROR("Unsupported field subtexture identifier");

    return TfNullPtr;
}


LoFiFieldTextureObject::LoFiFieldTextureObject(
    const LoFiTextureIdentifier &textureId,
    LoFiTextureObjectRegistry * const textureObjectRegistry)
  : LoFiTextureObject(textureId, textureObjectRegistry)
{
}

LoFiFieldTextureObject::~LoFiFieldTextureObject()
{
    if (Hgi * hgi = _GetHgi()) {
        _SubtractFromTotalTextureMemory(_gpuTexture);
        hgi->DestroyTexture(&_gpuTexture);
    }
}

void
LoFiFieldTextureObject::_Load()
{
    TRACE_FUNCTION();

    GlfFieldTextureDataRefPtr const texData = _ComputeFieldTexData(
        GetTextureIdentifier(),
        GetTargetMemory());

    if (!texData) {
        return;
    }

    texData->Read(
        /* degradeLevel = */ 0,
        /* generateMipmap = */ false);

    _cpuData = std::make_unique<LoFiGlfTextureCpuData>(
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

}

void
LoFiFieldTextureObject::_Commit()
{
    TRACE_FUNCTION();

    Hgi * const hgi = _GetHgi();
    if (!hgi) {
        return;
    }

    // Free previously allocated texture
    _SubtractFromTotalTextureMemory(_gpuTexture);
    hgi->DestroyTexture(&_gpuTexture);

    // Upload to GPU only if we have valid CPU data
    if (_cpuData && _cpuData->IsValid()) {
        _gpuTexture = hgi->CreateTexture(_cpuData->GetTextureDesc());
        _AddToTotalTextureMemory(_gpuTexture);
    }

    // Free CPU memory after transfer to GPU
    _cpuData.reset();
}

bool
LoFiFieldTextureObject::IsValid() const
{
    return bool(_gpuTexture);
}

HdTextureType
LoFiFieldTextureObject::GetTextureType() const
{
    return HdTextureType::Field;
}

PXR_NAMESPACE_CLOSE_SCOPE
