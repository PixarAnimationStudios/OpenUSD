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
#include "pxr/usd/usd/instanceKey.h"

#include "pxr/usd/usd/resolver.h"

Usd_InstanceKey::Usd_InstanceKey()
{
}

Usd_InstanceKey::Usd_InstanceKey(const PcpPrimIndex& instance)
    : _pcpInstanceKey(instance)
{
    for (Usd_Resolver res(&instance); res.IsValid(); res.NextNode()) {
        Usd_ResolvedClipInfo clipInfo;
        Usd_ResolveClipInfo(res.GetNode(), &clipInfo);
        if (clipInfo.clipAssetPaths
            or clipInfo.clipManifestAssetPath
            or clipInfo.clipPrimPath
            or clipInfo.clipActive
            or clipInfo.clipTimes) {
            _clipInfo.push_back(clipInfo);
        }
    }
}

bool 
Usd_InstanceKey::operator==(const Usd_InstanceKey& rhs) const
{
    return _pcpInstanceKey == rhs._pcpInstanceKey and
        _clipInfo == rhs._clipInfo;
}

bool
Usd_InstanceKey::operator!=(const Usd_InstanceKey& rhs) const
{
    return not (*this == rhs);
}

size_t hash_value(const Usd_InstanceKey& key)
{
    size_t hash = hash_value(key._pcpInstanceKey);
    for (const Usd_ResolvedClipInfo& clipInfo: key._clipInfo) {
        boost::hash_combine(hash, clipInfo.GetHash());
    }

    return hash;
}
