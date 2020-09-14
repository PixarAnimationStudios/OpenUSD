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
#ifndef PXR_USD_USD_INSTANCE_KEY_H
#define PXR_USD_USD_INSTANCE_KEY_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/clipSetDefinition.h"
#include "pxr/usd/usd/primData.h"
#include "pxr/usd/usd/stageLoadRules.h"
#include "pxr/usd/usd/stagePopulationMask.h"

#include "pxr/usd/pcp/instanceKey.h"

#include <string>
#include <vector>
#include <iosfwd>

PXR_NAMESPACE_OPEN_SCOPE

class PcpPrimIndex;

/// \class Usd_InstanceKey
///
/// Instancing key for prims. Instanceable prims that share the same
/// instance key are guaranteed to have the same opinions for name children
/// and properties and thus can share the same prototype.
///
class Usd_InstanceKey
{
public:
    Usd_InstanceKey();

    /// Create an instance key for the given instanceable prim index.
    explicit Usd_InstanceKey(const PcpPrimIndex& instance,
                             const UsdStagePopulationMask *popMask,
                             const UsdStageLoadRules &loadRules);

    /// Comparison operators.
    bool operator==(const Usd_InstanceKey& rhs) const;
    inline bool operator!=(const Usd_InstanceKey& rhs) const {
        return !(*this == rhs);
    }

private:
    friend size_t hash_value(const Usd_InstanceKey& key);

    friend std::ostream &
    operator<<(std::ostream &os, const Usd_InstanceKey &key);

    size_t _ComputeHash() const;

    PcpInstanceKey _pcpInstanceKey;
    std::vector<Usd_ClipSetDefinition> _clipDefs;
    UsdStagePopulationMask _mask;
    UsdStageLoadRules _loadRules;
    size_t _hash;
};

/// Return the hash code for \p key.
inline size_t
hash_value(const Usd_InstanceKey &key) {
    return key._hash;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_INSTANCE_KEY_H
