//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
