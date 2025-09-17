//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/maskRegistry.h"

#include "pxr/base/tf/staticData.h"

#include <array>
#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

TfStaticData<VdfMask, VdfMask::_AllOnes1Factory> VdfMask::_allOnes1;

// The fixed number of registries to distribute entries across, in order to
// prevent contention on the registry locks. We use the number of bits that have
// been discarded from the registry bucket index computation.
static constexpr size_t _NumRegistries =
    1 << Vdf_MaskRegistry::DiscardBucketBits;

using _MaskRegistryArray = std::array<Vdf_MaskRegistry, _NumRegistries>;
static TfStaticData<_MaskRegistryArray> _maskRegistryArray;

static Vdf_MaskRegistry &
_GetMaskRegistry(size_t maskHash)
{
    static constexpr size_t registryMask = _NumRegistries - 1;
    return (*_maskRegistryArray)[maskHash & registryMask];
}

// Extern function for use in testVdfMaskThreading.
VDF_API size_t
Vdf_MaskRegistry_GetSize()
{
    size_t totalSize = 0;
    _MaskRegistryArray &registries = *_maskRegistryArray;
    for (Vdf_MaskRegistry &reg : registries) {
        totalSize += reg.GetSize();
    }
    return totalSize;
}


VdfMask::_BitsImplRefPtr
VdfMask::_FindOrEmplace(VdfMask::Bits &&bits)
{
    const size_t hash = VdfMask::Bits::FastHash()(bits);
    // The mask registry manages the ref-count. Any _BitsImpl returned from
    // the registry will already have its ref-count incremented to account for
    // the reference that it just returned.
    return _BitsImplRefPtr(
        TfDelegatedCountDoNotIncrementTag,
        _GetMaskRegistry(hash).FindOrEmplace(std::move(bits), hash));
}

VdfMask::_BitsImplRefPtr
VdfMask::_FindOrInsert(const VdfMask::Bits &bits)
{
    const size_t hash = VdfMask::Bits::FastHash()(bits);
    // The mask registry manages the ref-count. Any _BitsImpl returned from
    // the registry will already have its ref-count incremented to account for
    // the reference that it just returned.
    return VdfMask::_BitsImplRefPtr(
        TfDelegatedCountDoNotIncrementTag,
        _GetMaskRegistry(hash).FindOrInsert(bits, hash));
}

void
VdfMask::_EraseBits(_BitsImpl *bits)
{
    const size_t hash = bits->GetHash();
    _GetMaskRegistry(hash).Erase(bits, hash);
}

// Output stream operator
std::ostream &
operator<<(std::ostream &os, const VdfMask &mask) {
    if (!mask._bits) {
        return os;
    }

    return os << mask._bits->Get();
}

PXR_NAMESPACE_CLOSE_SCOPE
