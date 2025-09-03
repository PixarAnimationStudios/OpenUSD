//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/vectorData.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/vectorAccessor.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/vt/array.h"

#include <ostream>
#include <string>
#include <typeindex>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

bool Vdf_VectorData::ShouldStoreCompressed(
    const VdfMask::Bits &bits, int elementSize)
{
    // Never compress any vector that has less than bigVectorSize elements or
    // is contiguously set (of the form 0*1+0*).
    const size_t bigVectorSize = 1000;
    if (bits.GetSize() < bigVectorSize || bits.AreContiguouslySet()) {
        return false;
    }

    // Don't bother compressing anything with small data sections.
    const size_t sectionSize = bits.GetLastSet() - bits.GetFirstSet() + 1;
    if (sectionSize < (bigVectorSize / 2)) {
        return false;
    }

    // If less than 12.5% of the data holding section (between first and last
    // set bits) is occupied, we consider this vector to have low occupation
    return bits.GetNumSet() < (sectionSize / 8);
}

// A function that streams out data.
//
template <class T>
static void
Vdf_VectorData_DebugStreamOut(
    Vdf_VectorData *vector,
    const VdfMask &mask,
    std::ostream *o)
{
    const Vdf_VectorAccessor<T> a(vector, vector->GetInfo());
    VdfMask::iterator it = mask.begin();
    for (; !it.IsAtEnd(); ++it) {
        const size_t idx = *it;
        *o << idx << ": "
           << a[idx]
           << "\n";
    }
}

// A map for doing type dispatch for debug printing.
//
typedef std::unordered_map<
    std::type_index,
    void (*)(Vdf_VectorData *, const VdfMask &, std::ostream *)>
        _DebugPrintDispatchTableType;

// Register a type for debug printing.
//
template <class T>
static void
Vdf_VectorData_RegisterDebugPrintType(_DebugPrintDispatchTableType *table)
{
    (*table)[std::type_index(typeid(T))] = Vdf_VectorData_DebugStreamOut<T>;
}

// We only support printing a small list of types that can be held in 
// Vdf.  This is where that list is defined and where it can be modified.
//
TF_MAKE_STATIC_DATA(_DebugPrintDispatchTableType, _debugPrintDispatchTable)
{
    _DebugPrintDispatchTableType *table = &(*_debugPrintDispatchTable);

    Vdf_VectorData_RegisterDebugPrintType<int>(table);
    Vdf_VectorData_RegisterDebugPrintType<double>(table);
    Vdf_VectorData_RegisterDebugPrintType<GfVec3d>(table);
    Vdf_VectorData_RegisterDebugPrintType<GfMatrix4d>(table);
    Vdf_VectorData_RegisterDebugPrintType<std::string>(table);
}

Vdf_VectorData::~Vdf_VectorData()
{
}

void
Vdf_VectorData::Expand(size_t first, size_t last)
{
    TF_VERIFY(false,
              "Unsupported attempt to expand storage of %s. "
              "Promotion to dense or sparse vector required.",
              ArchGetDemangled(typeid(*this)).c_str());
}

Vt_ArrayForeignDataSource*
Vdf_VectorData::GetSharedSource() const
{
    TF_VERIFY(false,
        "Unsupported attempt to get a shared source from "
        "non-shared vector %s. Must call Share() first.",
        ArchGetDemangled(typeid(*this)).c_str());
    return nullptr;
}

bool
Vdf_VectorData::IsSharable() const
{
    return false;
}

void
Vdf_VectorData::DebugPrint(
    const VdfMask &mask,
    std::ostream *o) const
{
    // Unfortunately, Vdf_VectorData::GetInfo & Vdf_VectorAccessor require a
    // non-const pointer even though we will not mutate any state via this
    // function.
    Vdf_VectorData *vector = const_cast<Vdf_VectorData *>(this);
    if (!vector->GetInfo().data) {
        // Bail out immediately if there is no data to print.
        return;
    }

    _DebugPrintDispatchTableType::const_iterator i =
        _debugPrintDispatchTable->find(vector->GetTypeInfo());

    if (i != _debugPrintDispatchTable->end()) {
        i->second(vector, mask, o);
    }
    else {
        *o << "(" << ArchGetDemangled(vector->GetTypeInfo()) << ")\n";
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
