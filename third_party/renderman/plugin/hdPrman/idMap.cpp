//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/idMap.h"
#include "pxr/base/arch/hash.h"
#include "pxr/base/tf/diagnostic.h"
#include <fstream>
#include "hdPrman/rixStrings.h"
#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

void
HdPrman_IdMap::Clear()
{
    _idMap.clear();
}

void
HdPrman_IdMap::RegisterId(
    IdDetails const& details,
    RtParamList *attrs)
{
    Key key = ArchHash64(details.name.c_str(), details.name.size());

    _idMap[key] = details;

    int id=-1, id2=-1;
    SplitKey(key, &id, &id2);

    attrs->SetString(
        RixStr.k_identifier_name,
        RtUString(details.name.c_str()));
    attrs->SetInteger(RixStr.k_identifier_id, id);
    attrs->SetInteger(RixStr.k_identifier_id2, id2);
}

bool
HdPrman_IdMap::GetDetails(Key key, HdPrman_IdMap::IdDetails *details) const
{
    const auto i = _idMap.find(key);
    if (i != _idMap.end()) {
        *details = i->second;
        return true;
    }
    return false;
}

void
HdPrman_IdMap::WriteIdMap(const std::string& filename)
{
    std::ofstream outFile(filename.c_str(), std::ios::binary);
    if (!outFile) {
        TF_WARN("Failed to create ID file '%s'", filename.c_str());
        return;
    }
    for (const auto& entry: _idMap) {
        const uint64_t id = entry.first;
        const uint64_t nameLen = entry.second.name.size() + 1; // 1 for NUL
        outFile
            .write(reinterpret_cast<const char*>(&id), sizeof(uint64_t))
            .write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen))
            .write(entry.second.name.c_str(), nameLen);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
