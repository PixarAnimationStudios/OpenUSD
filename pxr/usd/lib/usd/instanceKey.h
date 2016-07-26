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
#ifndef USD_INSTANCE_KEY_H
#define USD_INSTANCE_KEY_H

#include "pxr/usd/usd/clip.h"
#include "pxr/usd/usd/primData.h"

#include "pxr/usd/pcp/instanceKey.h"

#include <string>
#include <vector>

class PcpPrimIndex;

/// \class Usd_InstanceKey
/// Instancing key for prims. Instanceable prims that share the same
/// instance key are guaranteed to have the same opinions for name children
/// and properties and thus can share the same master.
class Usd_InstanceKey
{
public:
    Usd_InstanceKey();

    /// Create an instance key for the given instanceable prim index.
    explicit Usd_InstanceKey(const PcpPrimIndex& instance);

    /// Comparison operators.
    bool operator==(const Usd_InstanceKey& rhs) const;
    bool operator!=(const Usd_InstanceKey& rhs) const;

    /// Returns the hash value for this instance key.
    friend size_t hash_value(const Usd_InstanceKey& key);

    /// Returns string representation of this instance key
    /// for debugging purposes.
    std::string GetString() const;

private:
    PcpInstanceKey _pcpInstanceKey;
    std::vector<Usd_ResolvedClipInfo> _clipInfo;
};

#endif // USD_INSTANCE_KEY_H
