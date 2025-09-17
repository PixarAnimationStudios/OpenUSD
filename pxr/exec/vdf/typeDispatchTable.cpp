//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/typeDispatchTable.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

// Even though these are defaults, we define them out of line in order to 
// avoid tons of instantiations that would happen if these were inline.
// This is due to the heavy usage of the dispatch tables.

Vdf_TypeDispatchTableBase::Vdf_TypeDispatchTableBase() = default;
Vdf_TypeDispatchTableBase::~Vdf_TypeDispatchTableBase() = default;

bool
Vdf_TypeDispatchTableBase::IsTypeRegistered(const TfType t) const
{
    tbb::spin_rw_mutex::scoped_lock lock(_mutex, /* write = */ false);
    return _map.count(t);
}

bool
Vdf_TypeDispatchTableBase::_RegisterType(
    const std::type_info &ti, void *f)
{
    // Note: Registering a key twice doesn't hurt, because the function
    //       pointers are inserted into the same place in the map.
    //       These function pointers are not necessarily the same (e.g.
    //       for template instantiations in different modules).  If we
    //       would support dso unloading this would be a problem.

    const TfType t = TfType::Find(ti);
    if (TF_VERIFY(!t.IsUnknown(),
                  "Unknown TfType: %s", ArchGetDemangled(ti).c_str())) {
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, /* write = */ true);
        return _map.insert({ t, f }).second;
    }
    
    return false;
}

void *
Vdf_TypeDispatchTableBase::_FindOrFatalError(
    const TfType t) const
{
    tbb::spin_rw_mutex::scoped_lock lock(_mutex, /* write = */ false);
    _MapType::const_iterator i = _map.find(t);
    if (ARCH_UNLIKELY(i == _map.end())) {
        // Abort the program if not found.
        TF_FATAL_ERROR("Unsupported type: " + t.GetTypeName());
    }
    return i->second;
}

PXR_NAMESPACE_CLOSE_SCOPE
