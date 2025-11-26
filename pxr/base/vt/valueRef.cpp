//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/valueRef.h"

#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/dictionary.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/numericCast.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include <tbb/spin_mutex.h>
#include <tbb/concurrent_unordered_map.h>

#include <map>
#include <ostream>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <vector>
#include <cmath>
#include <limits>

using std::map;
using std::string;
using std::type_info;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

static_assert(std::is_nothrow_move_constructible<VtValueRef>::value, "");
static_assert(std::is_nothrow_move_assignable<VtValueRef>::value, "");

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<VtValueRef>();
}

bool
VtValueRef::IsArrayValued() const
{
    return _info && _info->isArray;
}

bool
VtValueRef::IsArrayEditValued() const
{
    return GetElementTypeid() != typeid(void) && !IsArrayValued();
}

size_t
VtValueRef::_GetNumElements() const
{
    return _info ? _info->GetNumElements(_ptr) : 0;
}

TfType
VtValueRef::GetType() const
{
    if (IsEmpty()) {
        return TfType::Find<void>();
    }
    TfType t = TfType::FindByTypeid(_info->typeInfo);
    if (t.IsUnknown()) {
        TF_WARN("Returning unknown type for VtValueRef with unregistered "
                "C++ type %s", ArchGetDemangled(GetTypeid()).c_str());
    }
    return t;
}

std::string
VtValueRef::GetTypeName() const
{
    return ArchGetDemangled(GetTypeid());
}

VtValueRef::operator VtValue() const
{
    if (IsEmpty()) {
        return {};
    }
    return _info->GetVtValue(_ptr);
}

bool
VtValueRef::CanHash() const
{
    if (IsEmpty()) {
        return true;
    }
    return _info->isHashable;
}

size_t
VtValueRef::GetHash() const {
    if (IsEmpty()) {
        return 0;
    }
    size_t h = _info->Hash(_ptr);
    return h;
}

std::ostream &
operator<<(std::ostream &out, const VtValueRef &self) {
    return self.IsEmpty() ? out : self._info->StreamOut(self._ptr, out);
}

TfPyObjWrapper
VtValueRef::_GetPythonObject() const
{
    return _info ? _info->GetPyObj(_ptr) : TfPyObjWrapper();
}

void const *
VtValueRef::_FailGet(Vt_DefaultValueHolder (*factory)(),
                     std::type_info const &queryType) const
{
    // Issue a coding error detailing relevant types.
    if (IsEmpty()) {
        TF_CODING_ERROR("Attempted to get value of type '%s' from "
                        "empty VtValueRef.", ArchGetDemangled(queryType).c_str());
    } else {
        TF_CODING_ERROR("Attempted to get value of type '%s' from "
                        "VtValueRef holding '%s'",
                        ArchGetDemangled(queryType).c_str(),
                        ArchGetDemangled(GetTypeid()).c_str());
    }

    // Get a default value for query type, and use that.
    return Vt_FindOrCreateDefaultValue(queryType, factory);
}

template <class ValRef>
static std::string
_GetFailDescription(ValRef const &val)
{
    return val.IsEmpty()
        ? ("an empty " + ArchGetDemangled<ValRef>())
        : TfStringPrintf("a %s holding '%s'; making value empty",
                         ArchGetDemangled<ValRef>().c_str(),
                         ArchGetDemangled(val.GetTypeid()).c_str());
}

void
VtValueRef::_FailRemove(std::type_info const &t)
{
    TF_CODING_ERROR("Attempted to remove a value of type '%s' from %s",
                    ArchGetDemangled(t).c_str(),
                    _GetFailDescription(*this).c_str());
    *this = {};
}

std::ostream &
VtStreamOut(vector<VtValueRef> const &val, std::ostream &stream) {
    bool first = true;
    stream << '[';
    TF_FOR_ALL(i, val) {
        if (first)
            first = false;
        else
            stream << ", ";
        stream << *i;
    }
    stream << ']';
    return stream;
}


void
VtMutableValueRef::_FailAssign(std::type_info const &t)
{
    TF_CODING_ERROR("Attempted to assign a value of type '%s' to %s",
                    ArchGetDemangled(t).c_str(),
                    _GetFailDescription(*this).c_str());
    *this = {};
}

void
VtMutableValueRef::_FailSwap(std::type_info const &t)
{
    TF_CODING_ERROR("Attempted to swap a value of type '%s' with %s",
                    ArchGetDemangled(t).c_str(),
                    _GetFailDescription(*this).c_str());
    *this = {};
}

PXR_NAMESPACE_CLOSE_SCOPE

#ifdef PXR_PYTHON_SUPPORT_ENABLED

namespace PXR_BOOST_NAMESPACE::python::converter {

arg_rvalue_from_python<VtValueRef>::arg_rvalue_from_python(PyObject* obj)
    : base_type(obj)
{
}

VtValueRef &
arg_rvalue_from_python<VtValueRef>::operator()() {
    optValueRef = VtValueRef { base_type::operator()() };
    return *optValueRef;
}

} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_PYTHON_SUPPORT_ENABLED
