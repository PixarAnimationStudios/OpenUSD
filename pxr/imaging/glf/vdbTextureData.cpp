//
// Copyright 2019 Pixar
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

#include "pxr/imaging/glf/debugCodes.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/glf/utils.h"
#include "pxr/imaging/glf/vdbTextureData.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/trace/trace.h"

#include "openvdb/openvdb.h"
#include "openvdb/tools/Dense.h"

PXR_NAMESPACE_OPEN_SCOPE

GlfVdbTextureDataRefPtr
GlfVdbTextureData::New(
    std::string const &filePath, const size_t targetMemory)
{
    return TfCreateRefPtr(new GlfVdbTextureData(filePath, targetMemory));
}

GlfVdbTextureData::GlfVdbTextureData(
    const std::string &filePath,
    const size_t targetMemory)
    : _filePath(filePath),
      _targetMemory(targetMemory),
      _nativeWidth(0), _nativeHeight(0), _nativeDepth(1),
      _bytesPerPixel(0),
      _glInternalFormat(GL_RGB),
      _glFormat(GL_RGB),
      _glType(GL_UNSIGNED_BYTE),
      _size(0)
{
}

GlfVdbTextureData::~GlfVdbTextureData() = default;

int
GlfVdbTextureData::NumDimensions() const
{
    return 3;
}

GLenum
GlfVdbTextureData::GLInternalFormat() const {
    return _glInternalFormat;
};

GLenum
GlfVdbTextureData::GLFormat() const {
    return _glFormat;
};

GLenum
GlfVdbTextureData::GLType() const {
    return _glType;
};

size_t
GlfVdbTextureData::TargetMemory() const
{
    return _targetMemory;
}

GlfVdbTextureData::WrapInfo
GlfVdbTextureData::GetWrapInfo() const {
    return _wrapInfo;
}

int
GlfVdbTextureData::GetNumMipLevels() const {
    return 1;
}

size_t
GlfVdbTextureData::ComputeBytesUsed() const
{
    return _size;
}

size_t
GlfVdbTextureData::ComputeBytesUsedByMip(int mipLevel) const
{
    return _size;
}

int
GlfVdbTextureData::ResizedWidth(int mipLevel) const
{
    return _nativeWidth;
}

int
GlfVdbTextureData::ResizedHeight(int mipLevel) const
{
    return _nativeHeight;
}

int
GlfVdbTextureData::ResizedDepth(int mipLevel) const
{
    return _nativeDepth;
}

bool
GlfVdbTextureData::HasRawBuffer(int mipLevel) const
{
    return bool(GetRawBuffer(mipLevel));
}

// A base class for a template to hold on to a OpenVDB dense grid.
//
// This would not be necessary if OpenVDB dense grids of different
// value types had a common base class and we could store a pointer
// to that base class.
//
// We can avoid a copy by using the abstract GetData and call the
// virtual destructor on the base class after the data have been
// uploaded to the GPU by GlfBaseTexture::_CreateTexture.
//
class GlfVdbTextureData_DenseGridHolderBase
{
public:
    // Get the bounding box of the tree of the OpenVDB grid
    virtual const openvdb::CoordBBox & GetTreeBoundingBox() const = 0;

    // Get the raw data of the dense grid
    virtual const unsigned char * GetData() const = 0;

    virtual ~GlfVdbTextureData_DenseGridHolderBase() = default;

protected:
    // Compute the tree's bounding box of an OpenVDB grid
    static openvdb::CoordBBox
    _ComputeTreeBoundingBox(const openvdb::GridBase::Ptr &grid)
    {
        TRACE_FUNCTION();

        // There is a tradeoff between using 
        // evalLeafBoundingBox() (less CPU time) or
        // evalActiveVoxelBoundingBox() (less memory)
        // here.
        return grid->evalActiveVoxelBoundingBox();
    }
};


namespace {

// Holds on to an OpenVDB dense grid
template<typename GridType>
class _DenseGridHolder : public GlfVdbTextureData_DenseGridHolderBase
{
public:
    using ValueType = typename GridType::ValueType;
    using DenseGrid = openvdb::tools::Dense<ValueType,
                                            openvdb::tools::LayoutXYZ>;
    using GridPtr = typename GridType::Ptr;

    // Create dense grid holder or return null pointer for empty grid.
    // Callee owns result.
    static 
    _DenseGridHolder *New(const GridPtr &grid)
    {
        // Determine bounding box of grid
        const openvdb::CoordBBox bbox = _ComputeTreeBoundingBox(grid);
        if (bbox.empty()) {
            // Empty grid
            return nullptr;
        }
        // Allocate dense grid of that size and copy grid to it.
        return new _DenseGridHolder(grid, bbox);
    }

    _DenseGridHolder(const GridPtr &grid, const openvdb::CoordBBox &bbox)
        // Allocate dense grid of given size
        : _denseGrid(bbox)
    {
        TRACE_SCOPE("GlfVdbTextureData: Copy to dense");
        openvdb::tools::copyToDense(grid->tree(), _denseGrid);
    }

    const unsigned char * GetData() const override {
        return reinterpret_cast<const unsigned char*>(_denseGrid.data());
    }

    const openvdb::CoordBBox & GetTreeBoundingBox() const override {
        return _denseGrid.bbox();
    }

private:
    DenseGrid _denseGrid;
};

// Extracts the transform associated with an OpenVDB grid
GfMatrix4d
_ExtractTransformFromGrid(const openvdb::GridBase::Ptr &grid)
{
    // Get transform
    openvdb::math::Transform::ConstPtr const t = grid->constTransformPtr();
    if (!t) {
        return GfMatrix4d(1.0);
    }

    // Only support linear transforms so far.
    if (!t->isLinear()) {
        TF_WARN("OpenVDB grid has non-linear transform which is not supported");
        return GfMatrix4d(1.0);
    }

    // Get underlying map
    openvdb::math::MapBase::ConstPtr const b = t->baseMap();
    if (!b) {
        TF_WARN("Could not get map underlying transform of OpenVDB grid");
        return GfMatrix4d(1.0);
    }

    openvdb::math::AffineMap::ConstPtr const a = b->getAffineMap();
    if (!a) {
        TF_WARN("OpenVDB grid has non-affine map which is not supported");
        return GfMatrix4d(1.0);
    }

    const openvdb::math::Mat4d m = a->getMat4();
    return GfMatrix4d(reinterpret_cast<const double (*)[4]>(m.asPointer()));
}

// Load the first grid from an OpenVDB file
openvdb::GridBase::Ptr
_LoadGridFromFile(openvdb::io::File &f)
{
    TRACE_FUNCTION();

    if (f.beginName() == f.endName()) {
        const std::string &filename = f.filename();
        TF_WARN("OpenVDB file %s has no grid", filename.c_str());
        return nullptr;
    }

    const std::string &gridName = f.beginName().gridName();
    TF_DEBUG(GLF_DEBUG_VDB_TEXTURE).Msg(
        "[VdbTextureData] Loading first grid (name: '%s')\n",
        gridName.c_str());

    return f.readGrid(gridName);
}

// Load the first grid from the OpenVDB file at given path
openvdb::GridBase::Ptr
_LoadGrid(const std::string &filePath)
{
    TRACE_FUNCTION();

    openvdb::io::File f(filePath);
    
    try {
        f.open();
    } catch (openvdb::IoError e) {
        TF_WARN("Could not open OpenVDB file: %s", e.what());
        return nullptr;
    }
    
    openvdb::GridBase::Ptr const result = _LoadGridFromFile(f);

    f.close();

    return result;
}

GfVec3d
_ToVec3d(const openvdb::Coord &c)
{
    return GfVec3d(c.x(), c.y(), c.z());
}

GfRange3d
_ToRange3d(const openvdb::CoordBBox &b)
{
    return GfRange3d(_ToVec3d(b.min()), _ToVec3d(b.max()));
}

} // end anonymous namespace

bool
GlfVdbTextureData::Read(int degradeLevel, bool generateMipmap,
                        GlfImage::ImageOriginLocation originLocation)
{   
    TRACE_FUNCTION();

    TF_DEBUG(GLF_DEBUG_VDB_TEXTURE).Msg(
        "[VdbTextureData] Path: %s\n", _filePath.c_str());

    openvdb::initialize();
    
    openvdb::GridBase::Ptr const grid = _LoadGrid(_filePath);
    if (!grid) {
        return false;
    }

    if (openvdb::FloatGrid::Ptr const floatGrid =
            openvdb::gridPtrCast<openvdb::FloatGrid>(grid)) {

        TF_DEBUG(GLF_DEBUG_VDB_TEXTURE).Msg(
            "[VdbTextureData] Converting float grid to dense");
        TRACE_SCOPE("Converting to float dense grid");

        _denseGrid.reset(
            _DenseGridHolder<openvdb::FloatGrid>::New(floatGrid));
        _bytesPerPixel = sizeof(float);
        _glInternalFormat = GL_RED;
        _glFormat = GL_RED;
        _glType = GL_FLOAT;

    } else if (openvdb::DoubleGrid::Ptr const doubleGrid =
                   openvdb::gridPtrCast<openvdb::DoubleGrid>(grid)) {

        TF_DEBUG(GLF_DEBUG_VDB_TEXTURE).Msg(
            "[VdbTextureData] Converting double grid to dense");
        TRACE_SCOPE("Converting to double dense grid");

        _denseGrid.reset(
            _DenseGridHolder<openvdb::DoubleGrid>::New(doubleGrid));
        _bytesPerPixel = sizeof(double);
        _glInternalFormat = GL_RED;
        _glFormat = GL_RED;
        _glType = GL_DOUBLE;

    } else if (openvdb::Vec3fGrid::Ptr const vec3fGrid =
                   openvdb::gridPtrCast<openvdb::Vec3fGrid>(grid)) {

        TF_DEBUG(GLF_DEBUG_VDB_TEXTURE).Msg(
            "[VdbTextureData] Converting vec3f grid to dense");
        TRACE_SCOPE("Converting to vec3f dense grid");

        _denseGrid.reset(
            _DenseGridHolder<openvdb::Vec3fGrid>::New(vec3fGrid));
        _bytesPerPixel = 3 * sizeof(float);
        _glInternalFormat = GL_RGB;
        _glFormat = GL_RGB;
        _glType = GL_FLOAT;

    } else if (openvdb::Vec3dGrid::Ptr const vec3dGrid =
                   openvdb::gridPtrCast<openvdb::Vec3dGrid>(grid)) {

        TF_DEBUG(GLF_DEBUG_VDB_TEXTURE).Msg(
            "[VdbTextureData] Converting vec3d grid to dense");
        TRACE_SCOPE("Converting to vec3d dense grid");

        _denseGrid.reset(
            _DenseGridHolder<openvdb::Vec3dGrid>::New(vec3dGrid));
        _bytesPerPixel = 3 * sizeof(double);
        _glInternalFormat = GL_RGB;
        _glFormat = GL_RGB;
        _glType = GL_DOUBLE;

    } else {
        TF_WARN("Unsupported OpenVDB grid type");
        return false;
    }

    if (!_denseGrid) {
        _nativeWidth  = 0;
        _nativeHeight = 0;
        // Following convention from GlfBaseTexture to set
        // depth to 1 by default.
        _nativeDepth  = 1;
        _size = 0;

        // Not emitting warning as volume might be empty for
        // legitimate reason (for example during an animation).

        return false;
    }

    const openvdb::CoordBBox &treeBoundingBox =
        _denseGrid->GetTreeBoundingBox();

    _boundingBox.Set(_ToRange3d(treeBoundingBox),
                     _ExtractTransformFromGrid(grid));
    
    const openvdb::Coord dim = treeBoundingBox.dim();
    _nativeWidth  = dim.x();
    _nativeHeight = dim.y();
    _nativeDepth  = dim.z();

    TF_DEBUG(GLF_DEBUG_VDB_TEXTURE).Msg(
        "[VdbTextureData] Dimensions %d x %d x %d\n",
        _nativeWidth, _nativeHeight, _nativeDepth);

    _size = treeBoundingBox.volume() * _bytesPerPixel;

    return true;
}

unsigned char *
GlfVdbTextureData::GetRawBuffer(int mipLevel) const
{
    if (mipLevel > 0) {
        return nullptr;
    }

    if (!_denseGrid) {
        return nullptr;
    }
    return const_cast<unsigned char *>(_denseGrid->GetData());
}

PXR_NAMESPACE_CLOSE_SCOPE
