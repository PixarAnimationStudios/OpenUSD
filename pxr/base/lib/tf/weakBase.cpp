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
#include "pxr/pxr.h"

#include "pxr/base/tf/weakBase.h"

PXR_NAMESPACE_OPEN_SCOPE

// TF_TAGGED_ALLOCATION(Tf_Remnant, false);
// TF_FIXEDSIZE_ALLOCATION(Tf_Remnant, true);
// TF_INSTANTIATE_CLASS_ALLOCATOR(Tf_Remnant);


Tf_Remnant::~Tf_Remnant()
{
    if (ARCH_UNLIKELY(_notify)) {
        Tf_ExpiryNotifier::Invoke(this);
    }
}

void const*
TfWeakBase::GetUniqueIdentifier() const
{
    return _Register()->_GetUniqueIdentifier();
}

void
TfWeakBase::EnableNotification2() const
{
    _Register()->_notify2 = true;
}

void const *
Tf_Remnant::_GetUniqueIdentifier() const
{
    return this;
}

void
Tf_Remnant::EnableNotification() const
{
    _notify = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
