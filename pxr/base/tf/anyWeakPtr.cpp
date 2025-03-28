//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/anyWeakPtr.h"

using std::string;
using std::type_info;

PXR_NAMESPACE_OPEN_SCOPE

TfAnyWeakPtr::_EmptyHolder::~_EmptyHolder()
{
}

void
TfAnyWeakPtr::_EmptyHolder::Clone(_Data *target) const
{
    new (target) _EmptyHolder;
}

bool
TfAnyWeakPtr::_EmptyHolder::IsInvalid() const
{
    return false;
}

void const *
TfAnyWeakPtr::_EmptyHolder::GetUniqueIdentifier() const
{
    return 0;
}

TfWeakBase const *
TfAnyWeakPtr::_EmptyHolder::GetWeakBase() const
{
    return 0;
}

TfAnyWeakPtr::_EmptyHolder::operator bool() const
{
    return false;
}

bool
TfAnyWeakPtr::_EmptyHolder::_IsConst() const
{
    return true;
}

TfPyObjWrapper
TfAnyWeakPtr::_EmptyHolder::GetPythonObject() const
{
    return {};
}

const std::type_info &
TfAnyWeakPtr::_EmptyHolder::GetTypeInfo() const
{
    return typeid(void);
}

TfType const&
TfAnyWeakPtr::_EmptyHolder::GetType() const
{
    return TfType::GetUnknownType();
}

const void *
TfAnyWeakPtr::_EmptyHolder::_GetMostDerivedPtr() const
{
    return 0;
}

bool
TfAnyWeakPtr::_EmptyHolder::_IsPolymorphic() const
{
    return false;
}    



//! Return true *only* if this expiry checker is watching a weak pointer
// which has expired.
bool
TfAnyWeakPtr::IsInvalid() const
{
    return _Get()->IsInvalid();
}

void const *
TfAnyWeakPtr::GetUniqueIdentifier() const
{
    return _Get()->GetUniqueIdentifier();
}

TfWeakBase const *
TfAnyWeakPtr::GetWeakBase() const
{
    return _Get()->GetWeakBase();
}

TfAnyWeakPtr::operator bool() const
{
    return bool(*_Get());
}

bool
TfAnyWeakPtr::operator !() const
{
    return !bool(*this);

}

bool
TfAnyWeakPtr::operator ==(const TfAnyWeakPtr &rhs) const
{
    return GetUniqueIdentifier() == rhs.GetUniqueIdentifier();
}

bool
TfAnyWeakPtr::operator <(const TfAnyWeakPtr &rhs) const
{
    return GetUniqueIdentifier() < rhs.GetUniqueIdentifier();
}

const std::type_info &
TfAnyWeakPtr::GetTypeInfo() const
{
    return _Get()->GetTypeInfo();
}

TfType const&
TfAnyWeakPtr::GetType() const
{
    return _Get()->GetType();
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED
pxr_boost::python::api::object
TfAnyWeakPtr::_GetPythonObject() const
{
    TfPyLock pyLock;
    return _Get()->GetPythonObject().Get();
}
#endif // PXR_PYTHON_SUPPORT_ENABLED

TfAnyWeakPtr::_PointerHolderBase::~_PointerHolderBase() {
}

PXR_NAMESPACE_CLOSE_SCOPE
