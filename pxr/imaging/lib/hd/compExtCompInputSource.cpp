//
// Copyright 2017 Pixar
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
#include "pxr/imaging/hd/compExtCompInputSource.h"
#include "pxr/imaging/hd/extCompCpuComputation.h"

PXR_NAMESPACE_OPEN_SCOPE

Hd_CompExtCompInputSource::Hd_CompExtCompInputSource(
                                 const TfToken &name,
                                 const HdExtCompCpuComputationSharedPtr &source,
                                 const TfToken &sourceOutputName)
 : Hd_ExtCompInputSource(name)
 , _source(source)
 , _sourceOutputIdx(HdExtCompCpuComputation::INVALID_OUTPUT_INDEX)
{
    _sourceOutputIdx = source->GetOutputIndex(sourceOutputName);
}

bool
Hd_CompExtCompInputSource::Resolve()
{
    bool sourceValid = _source->IsValid();
    if (sourceValid) {
        if (!_source->IsResolved()) {
            return false;
        }
    }

    if (!_TryLock()) return false;

    if (!sourceValid || _source->HasResolveError()) {
        _SetResolveError();
        return true;
    }

    _SetResolved();
    return true;
}

const VtValue &
Hd_CompExtCompInputSource::GetValue() const
{
    return _source->GetOutputByIndex(_sourceOutputIdx);
}


bool
Hd_CompExtCompInputSource::_CheckValid() const
{
    return (_source &&
           (_sourceOutputIdx != HdExtCompCpuComputation::INVALID_OUTPUT_INDEX));
}

PXR_NAMESPACE_CLOSE_SCOPE
