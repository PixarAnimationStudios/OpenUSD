//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/outputSpec.h"

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/staticData.h"

#include <tbb/spin_rw_mutex.h>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////

using Vdf_OutputSpecTypeInfoTable = TfHashMap<
    TfType, const Vdf_OutputSpecTypeInfo *, TfHash>;

static TfStaticData<Vdf_OutputSpecTypeInfoTable> _outputSpecTypeInfoTable;
static TfStaticData<tbb::spin_rw_mutex> _mutex;

VdfOutputSpec *
VdfOutputSpec::New(TfType type, const TfToken &name)
{
    const Vdf_OutputSpecTypeInfo *typeinfo = nullptr;
    const Vdf_OutputSpecTypeInfoTable &table = *_outputSpecTypeInfoTable;

    // Scope in order to lock only the required table access.
    {
        tbb::spin_rw_mutex::scoped_lock lock(*_mutex, /* write = */ false);
        Vdf_OutputSpecTypeInfoTable::const_iterator it = table.find(type);
        if (ARCH_UNLIKELY(it == table.end())) {
            // This seems harsh but it matches the behavior of the
            // VdfTypeDispatchTable-based runtime manufacturing used elsewhere.
            TF_FATAL_ERROR("Unknown output spec type");
        }
        typeinfo = it->second;
    }

    return new VdfOutputSpec(typeinfo, name);
}

VdfOutputSpec::~VdfOutputSpec()
{
}

std::string
VdfOutputSpec::GetTypeName() const
{
    return GetType().GetTypeName();
}

size_t
VdfOutputSpec::GetHash() const
{
    return TfHash::Combine(_name, GetType());
}

VdfVector *
VdfOutputSpec::AllocateCache() const
{
    TfAutoMallocTag2 tag("Vdf", "VdfOutputSpec::AllocateCache");
    return _typeinfo->allocateCache();
}

void
VdfOutputSpec::ResizeCache(VdfVector *vector, const VdfMask::Bits &bits) const
{
    return _typeinfo->resizeCache(vector, bits);
}

void
VdfOutputSpec::_RegisterType(const Vdf_OutputSpecTypeInfo *typeinfo)
{
    if (!TF_VERIFY(typeinfo && typeinfo->type)) {
        return;
    }

    tbb::spin_rw_mutex::scoped_lock lock(*_mutex, /* write = */ true);
    _outputSpecTypeInfoTable->insert({typeinfo->type, typeinfo});
}

PXR_NAMESPACE_CLOSE_SCOPE
