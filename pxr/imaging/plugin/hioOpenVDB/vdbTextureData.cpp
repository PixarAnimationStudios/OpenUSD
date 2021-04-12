//
// Copyright 2021 Pixar
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

#include "pxr/imaging/plugin/hioOpenVDB/vdbTextureData.h"
#include "pxr/imaging/plugin/hioOpenVDB/debugCodes.h"

#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/hio/fieldTextureData.h"

#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"

#include "openvdb/openvdb.h"
#include "openvdb/tools/Dense.h"
#include "openvdb/tools/GridTransformer.h"

PXR_NAMESPACE_OPEN_SCOPE

class HioOpenVDB_TextureDataFactory final
            : public HioFieldTextureDataFactoryBase
{
protected:
    HioFieldTextureDataSharedPtr _New(
        std::string const & filePath,
        std::string const & fieldName,
        int,
        std::string const &,
        size_t targetMemory) const override {
            return std::make_shared<HioOpenVDB_TextureData>(
                    filePath, fieldName, targetMemory);
        }
};

TF_REGISTRY_FUNCTION(TfType)
{
    using T = HioOpenVDB_TextureData;
    TfType t = TfType::Define<T, TfType::Bases<T::Base>>();
    t.SetFactory<HioOpenVDB_TextureDataFactory>();
}

HioOpenVDB_TextureData::HioOpenVDB_TextureData(
    std::string const &filePath,
    std::string const &gridName,
    const size_t targetMemory)
    : _filePath(filePath),
      _gridName(gridName),
      _targetMemory(targetMemory),
      _resizedWidth(0), _resizedHeight(0), _resizedDepth(1),
      _format(HioFormatUNorm8Vec3)
{
}

HioOpenVDB_TextureData::~HioOpenVDB_TextureData() = default;

const GfBBox3d &
HioOpenVDB_TextureData::GetBoundingBox() const
{
    return _boundingBox;
}

HioFormat
HioOpenVDB_TextureData::GetFormat() const
{
    return _format;
}

int
HioOpenVDB_TextureData::ResizedWidth() const
{
    return _resizedWidth;
}

int
HioOpenVDB_TextureData::ResizedHeight() const
{
    return _resizedHeight;
}

int
HioOpenVDB_TextureData::ResizedDepth() const
{
    return _resizedDepth;
}

bool
HioOpenVDB_TextureData::HasRawBuffer() const
{
    return bool(GetRawBuffer());
}

// A base class for a template to hold on to a OpenVDB dense grid.
//
// This would not be necessary if OpenVDB dense grids of different
// value types had a common base class and we could store a pointer
// to that base class.
//
// We can avoid a copy by using the abstract GetData and call the
// virtual destructor on the base class after the data have been
// uploaded to the GPU.
//
class HioOpenVDB_TextureData_DenseGridHolderBase
{
public:
    // Get the bounding box of the tree of the OpenVDB grid
    virtual const openvdb::CoordBBox & GetTreeBoundingBox() const = 0;

    // Get the raw data of the dense grid
    virtual const unsigned char * GetData() const = 0;

    virtual ~HioOpenVDB_TextureData_DenseGridHolderBase() = default;
};

namespace {

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

// Holds on to an OpenVDB dense grid
template<typename GridType>
class _DenseGridHolder final : public HioOpenVDB_TextureData_DenseGridHolderBase
{
public:
    using ValueType = typename GridType::ValueType;
    using DenseGrid = openvdb::tools::Dense<ValueType,
                                            openvdb::tools::LayoutXYZ>;
    using GridPtr = typename GridType::Ptr;

    // Create dense grid holder from grid and bounding box or return
    // null pointer for empty grid.
    // Callee owns result.
    static 
    _DenseGridHolder *New(const GridPtr &grid, const openvdb::CoordBBox &bbox)
    {
        TRACE_FUNCTION();

        if (bbox.empty()) {
            // Empty grid
            return nullptr;
        }
        // Allocate dense grid and copy grid to it.
        return new _DenseGridHolder(grid, bbox);
    }

    _DenseGridHolder(const GridPtr &grid, const openvdb::CoordBBox &bbox)
        // Allocate dense grid of given size
        : _denseGrid(bbox)
    {
        HF_MALLOC_TAG_FUNCTION();
        {
            TRACE_FUNCTION_SCOPE("HioOpenVDB_TextureData: Copy to dense");
            HF_MALLOC_TAG("Copy to dense");
            openvdb::tools::copyToDense(grid->tree(), _denseGrid);
        }
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

// A base class for a template to hold on to an OpenVDB grid.
//
// This is to dispatch to the templated openvdb::tools::resampleToMatch,
// dense grids, ...
//
class _GridHolderBase
{
public:
    // Get grid transform from OpenVDB grid.
    virtual GfMatrix4d GetGridTransform() const = 0;

    // Get metadata for corresponding OpenGL texture.
    virtual void GetMetadata(int *bytesPerPixel,
                             HioFormat *format) const = 0;

    // Create a new OpenVDB grid (of the right type) by resampling
    // the old grid. The new grid will have the given transform.
    virtual _GridHolderBase *GetResampled(const GfMatrix4d &newTransform) = 0;

    // Convert to dense grid.
    virtual HioOpenVDB_TextureData_DenseGridHolderBase *GetDense() const = 0;
    
    // Get bounding box of the tree in the grid.
    const openvdb::CoordBBox &GetTreeBoundingBox() const {
        return _treeBoundingBox;
    }

    // Dispatch OpenVDB grid pointer by type to construct corresponding
    // templated subclass of _GridHolderBase - also computes the bounding
    // box of the tree in the grid.
    static _GridHolderBase* New(const openvdb::GridBase::Ptr &grid);

protected:
    _GridHolderBase(const openvdb::GridBase::Ptr &grid)
      : _treeBoundingBox(_ComputeTreeBoundingBox(grid))
    { }

private:
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

    const openvdb::CoordBBox _treeBoundingBox;
};

template<typename GridType>
class _GridHolder final : public _GridHolderBase
{
public:
    using GridPtr = typename GridType::Ptr;
    using This = _GridHolder<GridType>;

    // Construct's _GridHolder if given OpenVDB grid pointer has the
    // correct type - also computed the bounding box of the tree in the grid.
    static _GridHolderBase* New(const openvdb::GridBase::Ptr &grid) {
        GridPtr const typedGrid = openvdb::gridPtrCast<GridType>(grid);
        if (!typedGrid) {
            return nullptr;
        }
        return new This(typedGrid);
    }

    _GridHolder(const GridPtr &grid)
        : _GridHolderBase(grid)
        , _grid(grid)
    {
    }

    GfMatrix4d GetGridTransform() const override {
        return _ExtractTransformFromGrid(_grid);
    }

    void GetMetadata(int *bytesPerPixel,
                     HioFormat *format) const override;
    
    _GridHolderBase *GetResampled(const GfMatrix4d &newTransform) override {
        TRACE_FUNCTION();

        GridPtr const result = GridType::create();

        result->setTransform(
            openvdb::math::Transform::createLinearTransform(
                openvdb::math::Mat4d(newTransform.data())));

        openvdb::tools::resampleToMatch<openvdb::tools::BoxSampler>(
            *_grid, *result);

        return New(result);
    }
    
    HioOpenVDB_TextureData_DenseGridHolderBase *GetDense() const override {
        return _DenseGridHolder<GridType>::New(_grid, GetTreeBoundingBox());
    }

private:
    const GridPtr _grid;
};

template<>
void
_GridHolder<openvdb::FloatGrid>::GetMetadata(int *bytesPerPixel,
                                             HioFormat *format) const
{
    *bytesPerPixel = sizeof(float);
    *format = HioFormatFloat32;
}

template<>
void
_GridHolder<openvdb::DoubleGrid>::GetMetadata(int *bytesPerPixel,
                                             HioFormat *format) const
{
    *bytesPerPixel = sizeof(double);
    *format = HioFormatDouble64;
}

template<>
void
_GridHolder<openvdb::Vec3fGrid>::GetMetadata(int *bytesPerPixel,
                                             HioFormat *format) const
{
    *bytesPerPixel = 3 * sizeof(float);
    *format = HioFormatFloat32Vec3;
}

template<>
void
_GridHolder<openvdb::Vec3dGrid>::GetMetadata(int *bytesPerPixel,
                                             HioFormat *format) const
{
    *bytesPerPixel = 3 * sizeof(double);
    *format = HioFormatDouble64Vec3;
}

_GridHolderBase *
_GridHolderBase::New(const openvdb::GridBase::Ptr &grid)
{
    if (!grid) {
        return nullptr;
    }

    if (_GridHolderBase * g = _GridHolder<openvdb::FloatGrid>::New(grid)) {
        TF_DEBUG(HIOOPENVDB_DEBUG_TEXTURE).Msg(
            "[VdbTextureData] Grid is holding floats\n");
        return g;
    }

    if (_GridHolderBase * g = _GridHolder<openvdb::DoubleGrid>::New(grid)) {
        TF_DEBUG(HIOOPENVDB_DEBUG_TEXTURE).Msg(
            "[VdbTextureData] Grid is holding doubles\n");
        return g;
    }

    if (_GridHolderBase * g = _GridHolder<openvdb::Vec3fGrid>::New(grid)) {
        TF_DEBUG(HIOOPENVDB_DEBUG_TEXTURE).Msg(
            "[VdbTextureData] Grid is holding float vectors\n");
        return g;
    }

    if (_GridHolderBase * g = _GridHolder<openvdb::Vec3dGrid>::New(grid)) {
        TF_DEBUG(HIOOPENVDB_DEBUG_TEXTURE).Msg(
            "[VdbTextureData] Grid is holding double vectors\n");
        return g;
    }
    
    TF_WARN("Unsupported OpenVDB grid type");
    return nullptr;
}

// Load the grid with given name from the OpenVDB file at given path
_GridHolderBase*
_LoadGrid(const std::string &filePath, std::string const &gridName)
{
    HF_MALLOC_TAG_FUNCTION();
    TRACE_FUNCTION();

    openvdb::initialize();
    openvdb::io::File f(filePath);
    
    {
        TRACE_FUNCTION_SCOPE("Opening VDB file");
        try {
            f.open();
        } catch (openvdb::IoError e) {
            TF_WARN("Could not open OpenVDB file: %s", e.what());
            return nullptr;
        } catch (openvdb::LookupError e) {
            // Occurs, e.g., when there is an unknown grid type in VDB file
            TF_WARN("Could not parse OpenVDB file: %s", e.what());
            return nullptr;
        }
    }
        
    if (!f.hasGrid(gridName)) {
        TF_WARN("OpenVDB file %s has no grid %s",
                filePath.c_str(), gridName.c_str());
        return nullptr;
    }
    
    openvdb::GridBase::Ptr result;

    {
        HF_MALLOC_TAG("readGrid");
        result = f.readGrid(gridName);
    }

    {
        TRACE_FUNCTION_SCOPE("Closing VDB file");
        // openvdb::io::File's d'tor is probably closing the file, but this
        // is not explicitly specified in the documentation.
        f.close();
    }

    return _GridHolderBase::New(result);
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

// We can compute the approximate distance of the new sampling points
// using the cube root of native to target memory - if it weren't for
// rounding and re-sampling issues.
//
// This function accounts for that so that if when we feed the resulting
// sampling point distance to OpenVDB's resampleToMatch, we should be
// under the target memory and not just near the target memory.
//
double
_ResamplingAdjustment(const int nativeLength, const double scale)
{
    // This is done in two steps:

    // First, we can use the approximate distance to compute how many
    // voxels the texture can have at most accross the direction we
    // consider here to not exceed the target memory.
    const int maxNumberOfSamples = floor(nativeLength / scale);

    // Second, before dividing the length of the interval containing all
    // original sampling points by the above number of samples, we account
    // for the fact that re-sampling might pick up ad additional sample
    // at each end.
    //
    // Example:
    //
    // Imagine you have samples at {-3, -2, -1, 0, 1, 2, 3} and pick a
    // distance of 1.3 for the new sampling points.
    //
    // You would expect 6 / 1.3 ~ 4.6 new sampling points.
    //
    // However, the value at 3.9 is not zero with linear interpolation
    // so the sampling points you need are at
    // {-3.9, -2.6, -1.3, 0, 1.3, 2.6, 3.9}, so actually 7 points in total.
    //
    return nativeLength / double(std::max(1, maxNumberOfSamples - 2));
}

} // end anonymous namespace

bool
HioOpenVDB_TextureData::Read()
{   
    TRACE_FUNCTION();

    TF_DEBUG(HIOOPENVDB_DEBUG_TEXTURE).Msg(
        "[VdbTextureData] Path: %s GridName: %s\n",
        _filePath.c_str(),
        _gridName.c_str());

    // Load grid from OpenVDB file
    std::unique_ptr<_GridHolderBase> gridHolder(
        _LoadGrid(_filePath, _gridName));

    if (!gridHolder) {
        // Runtime or coding errors already issued
        return false;
    }

    // Get grid transform 
    GfMatrix4d gridTransform = gridHolder->GetGridTransform();
    
    // Get _bytesPerPixel & _format
    int bytesPerPixel;
    gridHolder->GetMetadata(&bytesPerPixel,
                            &_format);

    // Get tree bounding box to compute native dimensions and size
    const openvdb::CoordBBox &nativeTreeBoundingBox =
        gridHolder->GetTreeBoundingBox();
    const openvdb::Coord nativeDim = nativeTreeBoundingBox.dim();
    const int nativeWidth  = nativeDim.x();
    const int nativeHeight = nativeDim.y();
    // Following convention to set depth to 1 for an empty texture.
    const int nativeDepth  = std::max(1, nativeDim.z());
    
    const size_t nativeSize = nativeTreeBoundingBox.volume() * bytesPerPixel;

    TF_DEBUG(HIOOPENVDB_DEBUG_TEXTURE).Msg(
        "[VdbTextureData] Native dimensions %d x %d x %d\n",
        nativeWidth, nativeHeight, nativeDepth);

    // Check whether native size is more than target memory if given
    if (nativeSize > _targetMemory && _targetMemory > 0) {
        TRACE_FUNCTION_SCOPE("Down-sampling");
        // We need to down-sample.

        // Compute the spacing of the points where we will
        // (re-)sample the volume.

        // As first approximation, use the cube-root.
        const double approxScale =
            cbrt(double(nativeSize) / double(_targetMemory));

        TF_DEBUG(HIOOPENVDB_DEBUG_TEXTURE).Msg(
            "[VdbTextureData] Approximate scaling factor %f\n", approxScale);

        // There will be additional samples near the boundary
        // of the original voluem, so scale down a bit more.
        const double scale =
            std::min({ _ResamplingAdjustment(nativeWidth,  approxScale),
                       _ResamplingAdjustment(nativeHeight, approxScale),
                       _ResamplingAdjustment(nativeDepth,  approxScale) });

        TF_DEBUG(HIOOPENVDB_DEBUG_TEXTURE).Msg(
            "[VdbTextureData] Scaling by factor %f\n", scale);
        
        // Apply voxel scaling to grid transform
        gridTransform =
            GfMatrix4d(GfVec4d(scale, scale, scale, 1.0)) * gridTransform;

        // And resample to match new grid transform
        gridHolder.reset(gridHolder->GetResampled(gridTransform));
    }

    // Convert grid to dense grid
    _denseGrid.reset(gridHolder->GetDense());

    if (!_denseGrid) {
        _resizedWidth = 0;
        _resizedHeight = 0;
        _resizedDepth = 1;

        // Not emitting warning as volume might be empty for
        // legitimate reason (for example during an animation).

        return false;
    }

    // Get the bounding box of dense grid and combine with above
    // grid transform to compute volume bounding box, dimensions and size.
    const openvdb::CoordBBox &treeBoundingBox =
        _denseGrid->GetTreeBoundingBox();

    _boundingBox.Set(_ToRange3d(treeBoundingBox),
                     gridTransform);

    const openvdb::Coord dim = treeBoundingBox.dim();
    _resizedWidth = dim.x();
    _resizedHeight = dim.y();
    _resizedDepth = dim.z();

    const size_t size = treeBoundingBox.volume() * bytesPerPixel;

    TF_DEBUG(HIOOPENVDB_DEBUG_TEXTURE).Msg(
        "[VdbTextureData] Resized dimensions %d x %d x %d "
        "(size: %zd, target: %zd)\n",
        _resizedWidth, _resizedHeight, _resizedDepth,
        size, _targetMemory);

    TF_DEBUG(HIOOPENVDB_DEBUG_TEXTURE).Msg(
        "[VdbTextureData] %s",
        (size <= _targetMemory || _targetMemory == 0) ?
            "Target memory was met." :
            "WARNING: the target memory was EXCEEDED");

    return true;
}

unsigned char const *
HioOpenVDB_TextureData::GetRawBuffer() const
{
    if (!_denseGrid) {
        return nullptr;
    }
    return _denseGrid->GetData();
}

PXR_NAMESPACE_CLOSE_SCOPE
