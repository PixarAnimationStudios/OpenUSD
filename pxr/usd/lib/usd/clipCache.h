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
#ifndef USD_CLIP_CACHE_H
#define USD_CLIP_CACHE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/clip.h"
#include "pxr/usd/sdf/pathTable.h"

#include <tbb/mutex.h>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


class PcpPrimIndex;

/// \class Usd_ClipCache
///
/// Private helper object for computing and caching clip information for 
/// a prim on a UsdStage.
///
class Usd_ClipCache
{
    Usd_ClipCache(Usd_ClipCache const &) = delete;
    Usd_ClipCache &operator=(Usd_ClipCache const &) = delete;
public:
    Usd_ClipCache();
    ~Usd_ClipCache();

    /// Populate the cache with clips for \p prim. Returns true if clips
    /// that may contribute opinions to attributes on \p prim are found,
    /// false otherwise.
    ///
    /// This function assumes that clips for ancestors of \p prim have 
    /// already been populated.
    bool PopulateClipsForPrim(
        const SdfPath& path, const PcpPrimIndex& primIndex);

    /// \struct Clips
    /// Structure containing a set of clips specified by a particular
    /// node in a prim index.
    struct Clips
    {
        Clips() : sourceLayerIndex(0) {}

        void Swap(Clips& rhs)
        {
            std::swap(sourceLayerStack, rhs.sourceLayerStack);
            std::swap(sourcePrimPath, rhs.sourcePrimPath);
            std::swap(sourceLayerIndex, rhs.sourceLayerIndex);
            std::swap(manifestClip, rhs.manifestClip);
            std::swap(valueClips, rhs.valueClips);
        }

        PcpLayerStackPtr sourceLayerStack;
        SdfPath sourcePrimPath;
        size_t sourceLayerIndex;
        Usd_ClipRefPtr manifestClip;
        Usd_ClipRefPtrVector valueClips;
    };

    /// Get all the layers that have been opened because we needed to extract
    /// data from their corresponding clips.  USD tries to be as lazy as 
    /// possible about opening clip layers to avoid unnecessary latency and
    /// memory bloat; however, once a layer is open, it will generally be
    /// kept open for the life of the stage.
    SdfLayerHandleSet GetUsedLayers() const;
    
    /// Get all clips that may contribute opinions to attributes on the
    /// prim at \p path, including clips that were authored on ancestral prims.
    ///
    /// The returned vector contains all clips that affect the prim at \p path
    /// in strength order. Each individual list of value clips will be ordered
    /// by start time.
    const std::vector<Clips>& 
    GetClipsForPrim(const SdfPath& path) const;

    /// \struct Lifeboat
    ///
    /// Structure for keeping invalidated clip data alive.
    /// \sa InvalidateClipsForPrim.
    ///
    struct Lifeboat
    {
        std::vector<Clips> _clips;
    };

    /// Invalidates the clip cache for prims at and below \p path. Any
    /// invalidated clips will be transferred into the \p lifeboat, instead
    /// of being disposed immediately. This potentially allows the underlying
    /// clip layer to be reused if the clip cache is repopulated while
    /// the lifeboat is still active.
    void InvalidateClipsForPrim(const SdfPath& path, Lifeboat* lifeboat);

private:
    const std::vector<Clips>& 
    _GetClipsForPrim_NoLock(const SdfPath& path) const;

    // Map from prim path to all clips that apply to that prim, including
    // ancestral clips. This map is sparse; only prims where clips are
    // authored will have entries.
    typedef SdfPathTable<std::vector<Clips> >_ClipTable;
    _ClipTable _table;
    mutable tbb::mutex _mutex;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_CLIP_CACHE_H
