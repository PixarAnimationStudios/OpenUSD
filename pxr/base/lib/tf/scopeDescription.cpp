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

#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"

#include "pxr/base/arch/threads.h"

#include <tbb/spin_mutex.h>

using std::vector;
using std::string;

PXR_NAMESPACE_OPEN_SCOPE

// Static description stack

static TfStaticData<vector<string> > _descriptionStack;
static TfStaticData<tbb::spin_mutex> _descriptionStackMutex;

static size_t _PushDescription(string const &d)
{
    tbb::spin_mutex::scoped_lock lock(*_descriptionStackMutex);
    _descriptionStack->push_back(d);
    return (_descriptionStack->size() - 1);
}

static void _PopDescription()
{
    tbb::spin_mutex::scoped_lock lock(*_descriptionStackMutex);
    _descriptionStack->pop_back();
}

static void _SetDescription(size_t const idx, string const& d)
{
    tbb::spin_mutex::scoped_lock lock(*_descriptionStackMutex);
    TF_AXIOM(idx < _descriptionStack->size());
    (*_descriptionStack)[idx] = d;
}

// TfScopeDescription

TfScopeDescription::TfScopeDescription(string const &description)
    : _stackIndex(InvalidIndex)
{
    if (ArchIsMainThread()) {
        _stackIndex = _PushDescription(description);
    }
}

TfScopeDescription::~TfScopeDescription()
{
    if (_stackIndex != InvalidIndex) {
        _PopDescription();
    }
}

void
TfScopeDescription::SetDescription(string const &description)
{
    if (_stackIndex != InvalidIndex) {
        _SetDescription(_stackIndex, description);
    }
}

vector<string>
TfGetCurrentScopeDescriptionStack()
{
    tbb::spin_mutex::scoped_lock lock(*_descriptionStackMutex);
    return *_descriptionStack;
}

PXR_NAMESPACE_CLOSE_SCOPE
