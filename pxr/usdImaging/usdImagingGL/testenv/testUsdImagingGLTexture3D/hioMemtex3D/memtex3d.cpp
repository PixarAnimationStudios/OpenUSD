//
// Copyright 2023 Pixar
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
#include "pxr/imaging/hio/fieldTextureData.h"
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

PXR_NAMESPACE_OPEN_SCOPE

// Hio class to open a .memtex3d file.
class HioMemtex3D : public HioFieldTextureData
{
public:
    using Base = HioFieldTextureData;

    HioMemtex3D();
    HioMemtex3D(std::string const& filePath, std::string const& fieldName, size_t targetMemory);

    ~HioMemtex3D() override;

    // HioFieldTextureData overrides
    const GfBBox3d& GetBoundingBox() const override;

    int ResizedWidth() const override;

    int ResizedHeight() const override;

    int ResizedDepth() const override;

    HioFormat GetFormat() const override;

    bool Read() override;

    bool HasRawBuffer() const override;

    unsigned char const* GetRawBuffer() const override;

private:
    std::string _GetFilenameExtension() const;

    std::string _filename;
    int _subimage;
    int _miplevel;
    HioImage::SourceColorSpace _sourceColorSpace;

    int _resizedWidth, _resizedHeight, _resizedDepth;
    std::vector<unsigned char> _data;
    std::string _name;

    HioFormat _format;

    GfBBox3d _boundingBox;
};

class UsdImagingMemTex3DFactory final
    : public HioFieldTextureDataFactoryBase
{
protected:
    HioFieldTextureDataSharedPtr _New(
        std::string const& filePath,
        std::string const& fieldName,
        int fieldIndex,
        std::string const& fieldPurpose,
        size_t targetMemory) const override
    {
        return std::make_shared<HioMemtex3D>(filePath, fieldName, targetMemory);
    }
};


TF_REGISTRY_FUNCTION(TfType)
{
    using T = HioMemtex3D;
    TfType t = TfType::Define<T, TfType::Bases<T::Base>>();
    t.SetFactory<UsdImagingMemTex3DFactory>();
}

HioMemtex3D::HioMemtex3D()
    : _subimage(0), _miplevel(0)
{
}

HioMemtex3D::HioMemtex3D(std::string const& filePath, std::string const& fieldName, size_t targetMemory)
{
}

/* virtual */
HioMemtex3D::~HioMemtex3D() = default;


const GfBBox3d&
HioMemtex3D::GetBoundingBox() const
{
    return _boundingBox;
}

HioFormat
HioMemtex3D::GetFormat() const
{
    return _format;
}

int
HioMemtex3D::ResizedWidth() const
{
    return _resizedWidth;
}

int
HioMemtex3D::ResizedHeight() const
{
    return _resizedHeight;
}

int
HioMemtex3D::ResizedDepth() const
{
    return _resizedDepth;
}

bool
HioMemtex3D::HasRawBuffer() const
{
    return bool(GetRawBuffer());
}

static void initBuffer(float* data, int x, int y, int z)
{
    for (int k = 0; k < z; k++)
    {
        for (int j = 0; j < y; j++)
        {
            for (int i = 0; i < x; i++)
            {
                data[0] = float(i) / float(x); // r
                data[1] = float(j) / float(y); // g
                data[2] = float(k) / float(z); // b
                data[3] = 1.0f; //a
                
                data += 4;
            }
        }
    }
}
bool HioMemtex3D::Read()
{
    _resizedWidth = 10;
    _resizedHeight = _resizedWidth;
    _resizedDepth = _resizedWidth;

    _data.reserve(_resizedWidth * _resizedHeight * _resizedDepth * 4 * 4);
    initBuffer(reinterpret_cast<float*>(_data.data()), _resizedWidth, _resizedHeight, _resizedDepth);

    _format = HioFormatFloat32Vec4;
    _boundingBox = GfBBox3d(GfRange3d(GfVec3d(0, 0, 0), GfVec3d(1, 1, 1)));

    return true;
}

unsigned char const* HioMemtex3D::GetRawBuffer() const
{
    return reinterpret_cast<const unsigned char*>(_data.data());
}


std::string 
HioMemtex3D::_GetFilenameExtension() const
{
    std::string fileExtension = ArGetResolver().GetExtension(_filename);
    return TfStringToLower(fileExtension);
}

PXR_NAMESPACE_CLOSE_SCOPE

