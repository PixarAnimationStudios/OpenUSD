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
#include "pxr/base/tf/anyWeakPtr.h"

#include <ciso646>

using std::string;
using std::type_info;

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

boost::python::api::object
TfAnyWeakPtr::_EmptyHolder::GetPythonObject() const
{
    return boost::python::api::object();
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
    return not bool(*this);

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

boost::python::api::object
TfAnyWeakPtr::_GetPythonObject() const
{
    TfPyLock pyLock;
    return _Get()->GetPythonObject();
}

TfAnyWeakPtr::_PointerHolderBase::~_PointerHolderBase() {
}
