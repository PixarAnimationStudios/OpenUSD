//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ID_MAP_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_ID_MAP_H

#include "hdPrman/api.h"
#include "pxr/pxr.h"
#include <tbb/concurrent_unordered_map.h>
#include <cstdint>
#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

/// HdPrman_IdMap associates RenderMan id and id2 instance attribute
/// values with corresponding Hydra scene object identity details.
/// This is used to support picking objects from id/id2 AOV buffers.
///
/// For runtime interactive picking, HdPrmanFramebuffer uses the
/// idMap to translate id/id2 AOV values back to HdRenderIndex
/// primId and instanceId values.
///
/// For out-of-process picking against stored rendered images,
/// a separate idMap file can be serialized containing the same
/// information; see WriteIdMap().
///
class HdPrman_IdMap
{
public:
    // The u64 global ID for a geometry instance.
    // This is expected to be a stable ID that will be consistently
    // assigned based on scene path, and which does not depend
    // on the presence or absence of unrelated geometry.
    // In RenderMan, this is split as two u32 attributes:
    // identifier:id and identifier:id2.
    using Key = uint64_t;

    // Details associated with each Key.
    struct IdDetails {
        std::string name;    // Riley k_identifier_name value
        int primId = -1;     // HdRenderIndex primId
        int instanceId = -1; // HdRenderIndex instanceId
    };

    // Split a Key into id and id2.
    inline static void SplitKey(Key key, int *id, int *id2) {
        *id  = (key & 0xFFFFFFFF);
        *id2 = (key >> 32);
    }

    // Join id and id2 values into a Key.
    inline static Key KeyFromAttrs(int id, int id2) {
        return (uint64_t(unsigned(id2)) << 32) | uint64_t(unsigned(id));
    }

    // Assign identifier:id and identifier:id2 attribute values for
    // the given geometry instance, and store the association.
    // Sets *idAttr and *id2Attr.
    void RegisterId(
        IdDetails const& details,
        RtParamList *paramList);

    // Look up the details for the given ID.
    bool GetDetails(Key key, IdDetails *details) const;

    // Clear ID map.
    void Clear();

    // Write the path to id mapping to a file with the provided name.
    void WriteIdMap(const std::string& filename);

private:
    using _IdMap = tbb::concurrent_unordered_map<Key, IdDetails>;
    _IdMap _idMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
