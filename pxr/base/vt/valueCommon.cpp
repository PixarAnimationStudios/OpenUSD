//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/dictionary.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

#include <tbb/spin_mutex.h>

#include <map>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <vector>

using std::map;
using std::string;
using std::type_info;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

void const *
Vt_FindOrCreateDefaultValue(std::type_info const &type,
                            Vt_DefaultValueHolder (*factory)())
{
    // This function returns a default value for \a type.  It stores a global
    // map from type name to value.  If we have an entry for the requested type
    // in the map already, return that.  Otherwise use \a factory to create a
    // new entry to store in the map, asserting that it produced a value of the
    // correct type.

    TfAutoMallocTag2 tag("Vt", "VtValue _FindOrCreateDefaultValue");
    
    typedef map<string, Vt_DefaultValueHolder> DefaultValuesMap;
    
    static DefaultValuesMap defaultValues;
    static tbb::spin_mutex defaultValuesMutex;

    string key = ArchGetDemangled(type);

    {
        // If there's already an entry for this type we can return it directly.
        tbb::spin_mutex::scoped_lock lock(defaultValuesMutex);
        DefaultValuesMap::iterator i = defaultValues.find(key);
        if (i != defaultValues.end())
            return i->second.GetPointer();
    }

    // We need to make a new entry.  Call the factory function while the mutex
    // is unlocked.  We do this because the factory is unknown code which could
    // plausibly call back into here, causing deadlock.  Assert that the factory
    // produced a value of the correct type.
    Vt_DefaultValueHolder newValue = factory();
    TF_AXIOM(TfSafeTypeCompare(newValue.GetType(), type));

    // We now lock the mutex and attempt to insert the new value.  This may fail
    // if another thread beat us to it while we were creating the new value and
    // weren't holding the lock.  If this happens, we leak the default value we
    // created that isn't used.
    tbb::spin_mutex::scoped_lock lock(defaultValuesMutex);
    DefaultValuesMap::iterator i =
        defaultValues.emplace(std::move(key), std::move(newValue)).first;
    return i->second.GetPointer();
}

#define _VT_IMPLEMENT_ZERO_VALUE_FACTORY(unused, elem)                   \
template <>                                                              \
Vt_DefaultValueHolder Vt_DefaultValueFactory<VT_TYPE(elem)>::Invoke()    \
{                                                                        \
    return Vt_DefaultValueHolder::Create(VtZero<VT_TYPE(elem)>());       \
}                                                                        \
template struct Vt_DefaultValueFactory<VT_TYPE(elem)>;

TF_PP_SEQ_FOR_EACH(_VT_IMPLEMENT_ZERO_VALUE_FACTORY,
                   ~,
                   VT_VEC_VALUE_TYPES
                   VT_MATRIX_VALUE_TYPES
                   VT_QUATERNION_VALUE_TYPES
                   VT_DUALQUATERNION_VALUE_TYPES)

PXR_NAMESPACE_CLOSE_SCOPE
