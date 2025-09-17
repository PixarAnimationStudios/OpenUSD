//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hioOpenVDB/utils.h"
#include "pxr/imaging/hioOpenVDB/vdbAssetInterface.h"
#include "pxr/base/trace/trace.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolvedPath.h"

#include "openvdb/io/Stream.h"

#include <istream>
#include <streambuf>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

/// Classes for converting a char buffer to a std::istream, similar to the
/// functionality of boost::iostreams.
class _CharBuf : public std::basic_streambuf<char>
{
public:
    _CharBuf(const char *p, size_t l)
    {
        setg((char*)p, (char*)p, (char*)p + l);
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override
    {
        return seekoff(pos - pos_type(off_type(0)), std::ios_base::beg, which);
    }

    pos_type seekoff(off_type off,
                    std::ios_base::seekdir dir,
                    std::ios_base::openmode which = std::ios_base::in) override
    {
        // Should use switch(dir) but that results in a compiler error due to
        // a missing case (_S_ios_seekdir_end) which isn't actually a case.
        if (dir == std::ios_base::cur) {
            gbump(off);
        } else if (dir == std::ios_base::end) {
            setg(eback(), egptr() + off, egptr());
        } else if (dir == std::ios_base::beg) {
            setg(eback(), eback() + off, egptr());
        }
        return gptr() - eback();
    }
};

class _CharStream : public std::istream
{
public:
    _CharStream(const char *p, size_t l)
      : std::istream(&_buffer),
        _buffer(p, l)
    {
        rdbuf(&_buffer);
    }

private:
    _CharBuf _buffer;
};

openvdb::GridPtrVecPtr
_ReadVDBGridsFromAssetBuffer(const std::shared_ptr<ArAsset>& asset)
{
    if (!asset) {
        return nullptr;
    }

    // AsAsset provides a char buffer and size, but openvdb::io::Stream
    // requres a std::istream. To bridge the gap, use the _CharStream
    // class above to wrap the char buffer from asset in a std::istream.
    std::shared_ptr<const char> bytes = asset->GetBuffer();
    _CharStream charStream(bytes.get(), asset->GetSize());

    openvdb::initialize();
    openvdb::io::Stream vdbStream(charStream);
    return vdbStream.getGrids();
}

} // end anonymous namespace


openvdb::GridBase::Ptr
HioOpenVDBGridFromAsset(const std::string& name, const std::string& assetPath)
{
    std::shared_ptr<ArAsset> asset =
        ArGetResolver().OpenAsset(ArResolvedPath(assetPath));

    // Try casting asset to a HioOpenVDBArAssetInterface, which provides
    // direct access to vdb grids, without writing them to a stream.
    if (HioOpenVDBArAssetInterface* const vdbAsset =
            dynamic_cast<HioOpenVDBArAssetInterface*>(asset.get())) {
        TRACE_FUNCTION_SCOPE("Reading VDB grids from "
                             "HioOpenVDBArAssetInterface.");
        return vdbAsset->GetGrid(name);

    } else {
        // As a fallback, attempt to read the vdb grids from asset's buffer.
        TRACE_FUNCTION_SCOPE("Reading VDB grids from ArAsset buffer.");
        if (openvdb::GridPtrVecPtr grids =
                _ReadVDBGridsFromAssetBuffer(asset)) {

            // Find the first grid that matches name in grids vector.
            for (const openvdb::GridBase::Ptr& grid : *grids) {
                if (grid->getName() == name) {
                    return grid;
                }
            }
        }
    }

    return nullptr;
}

openvdb::GridPtrVecPtr
HioOpenVDBGridsFromAsset(const std::string& assetPath)
{
    std::shared_ptr<ArAsset> asset =
        ArGetResolver().OpenAsset(ArResolvedPath(assetPath));

    // Try casting asset to a HioOpenVDBArAssetInterface, which provides
    // direct access to vdb grids, without writing them to a stream.
    if (HioOpenVDBArAssetInterface* const vdbAsset =
            dynamic_cast<HioOpenVDBArAssetInterface*>(asset.get())) {
        TRACE_FUNCTION_SCOPE("Reading VDB grids from "
                             "HioOpenVDBArAssetInterface.");
        return vdbAsset->GetGrids();

    } else {
        // As a fallback, attempt to read the vdb grids from asset's buffer.
        TRACE_FUNCTION_SCOPE("Reading VDB grids from ArAsset buffer");   
        return _ReadVDBGridsFromAssetBuffer(asset);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
