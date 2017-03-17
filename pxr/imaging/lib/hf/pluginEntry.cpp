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
#include "pxr/imaging/hf/pluginEntry.h"
#include "pxr/imaging/hf/pluginBase.h"
#include "pxr/imaging/hf/pluginDesc.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


Hf_PluginEntry::Hf_PluginEntry(const TfType &type,
                               const std::string &displayName,
                               int   priority)
 : _type(type)
 , _displayName(displayName)
 , _priority(priority)
 , _instance(nullptr)
 , _refCount(0)
{
}

Hf_PluginEntry::~Hf_PluginEntry()
{
    // Memory leak detection - All instances should be released before
    // deleting the class
    if (!TF_VERIFY(_instance == nullptr)) {
        delete _instance;
    }
}

Hf_PluginEntry::Hf_PluginEntry(Hf_PluginEntry &&source)
 : _type(source._type)
 , _displayName(std::move(source._displayName))
 , _priority(source._priority)
 , _instance(source._instance)
 , _refCount(source._refCount)
{
}

Hf_PluginEntry &
Hf_PluginEntry::operator =(Hf_PluginEntry &&source)
{
    _type         = std::move(source._type);
    _displayName  = std::move(source._displayName);
    _priority     = std::move(source._priority);
    _instance     = std::move(source._instance);
    _refCount     = std::move(source._refCount);

    return *this;
}

TfToken
Hf_PluginEntry::GetId() const
{
    const std::string &typeName =_type.GetTypeName();
    return TfToken(typeName);
}

void
Hf_PluginEntry::GetDesc(HfPluginDesc *desc) const
{
    desc->id          = GetId();
    desc->displayName = _displayName;
    desc->priority    = _priority;
}

void
Hf_PluginEntry::IncRefCount()
{
    if (_refCount == 0) {
        _Factory *factory = _type.GetFactory<_Factory>();

        _instance = factory->New();
    }

    ++_refCount;
}

void
Hf_PluginEntry::DecRefCount()
{
    // something went wrong with ref counting
    if (!TF_VERIFY(_refCount > 0)) {
        _refCount = 0;
        return;
    }

    --_refCount;

    if (_refCount == 0) {
        delete _instance;
        _instance = nullptr;
    }
}

bool
Hf_PluginEntry::operator <(const Hf_PluginEntry &other) const
{
    // Policy is sort by priority then alphabetical order on
    // machine name.

    int aPriority = this->_priority;
    int bPriority = other._priority;

    if (aPriority > bPriority) {
        return true;
    } else if (aPriority < bPriority) {
        return false;
    } else {
        // Use TypeName string rather than machine name token
        // as we want to sort alphabetically not hash order.
        return this->_type.GetTypeName() < other._type.GetTypeName();
    }
}

void
Hf_PluginEntry::SetFactory(TfType &type,
                           _PluginFactoryFn &func)
{
    type.SetFactory(std::unique_ptr<_Factory>(new _Factory(func)));
}

PXR_NAMESPACE_CLOSE_SCOPE

